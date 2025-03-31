/**
********************************************************************************
*
* @file: adi_sharcfx_conv2d.cpp
*
* @brief: Contains optimized conv2d functions
*
* @details: Contains optimized conv2d functions for various configurations targetting different usecases
*
*******************************************************************************
 Copyright(c) 2024 Analog Devices, Inc. All Rights Reserved. This software is
 proprietary & confidential to Analog Devices, Inc. and its licensors. By using
 this software you agree to the terms of the associated Analog Devices License
 Agreement.
*******************************************************************************
*/

/*============= I N C L U D E S =============*/
#include "adi_sharcfx_nn.h"

/*============= F U N C T I O N P R O T O T Y P E S =============*/
inline int8_t quantize_and_store(xb_vec2Mx40 acc,
                                 int32_t pBiasBuffer,
                                 int32_t nQuantizedMultiplierValue,
                                 int32_t nQuantizedShiftValue,
                                 int32_t nFil,
                                 int32_t pOutZeroPoint);

/*============= C O D E =============*/

/**
*******************************************************************************
* Function: adi_sharcfx_conv2d_kernel1x1_int8
* @brief optimized conv2d function
*
* @details optimized conv2d function for 8-bit integer input. 1x1 2D convolution in interleaved format using 16bit Eagle intrinsics
*
* Parameters:
* @param [in] pInputBuffer - input data
* @param [in] pWeightsBuffer - input weights buffer
* @param [in] pBiasBuffer - input bias buffer
* @param [in] nBatches - batch size
* @param [in] nInChannels - input depth
* @param [in] nOutChannels - output depth
* @param [in] nSize - # of kernels
* @param [in] pQuantizedMultiplier - multiplier
* @param [in] pQuantizedShift - shift
* @param [in] nInputOffset - input zeropoint
* @param [in] nOutputOffset - output zeropoint
* 
* @param [out] pOutputBuffer - output data
*
* @return None
*
*
*******************************************************************************
*/ 
void adi_sharcfx_conv2d_kernel1x1_int8(const int8_t* pInputBuffer,
                                    const int8_t* pWeightsBuffer,
                                    const int32_t* pBiasBuffer,
                                    int8_t* pOutputBuffer,
                                    int32_t nBatches,
                                    int32_t nInChannels,
                                    int32_t nOutChannels,
                                    int32_t nSize,
                                    int32_t *nQuantizedMultiplier,
                                    int32_t *nQuantizedShift,
                                    int32_t nInZeroPoint,
                                    int32_t nOutZeroPoint)
{
	int8_t* __restrict outp = (int8_t*) pOutputBuffer;

	const immediate Lane=0;
	//Defining the input and filter offsets
	xb_vec2Mx16 vInZP = PDX_REP_2MX16((xb_vec2Mx16)nInZeroPoint,Lane);//Replicates the lane of data specified, across all lanes of a vector register

	xb_vec2Mx16 vin,vwt;
	xb_int32 temp;
	xb_int40 sat_sum;
	xb_int80 product;

	vbool4M temp_mask;
	vbool2M acc_mask, ffff;

	temp_mask = PDX_MOVB_AU32(0xFFFFFFFF);
	PDX_CVTBB2M_B4M(ffff, ffff, temp_mask);

	int32_t nPixProcessed=0;

	xb_vec2Mx40 acc = 0;
	xb_vec2Mx8 *inp = (xb_vec2Mx8 *)pInputBuffer;
	int32_t nFilterDepth = nInChannels;
	int32_t nFilterRem = nFilterDepth % 16;//2*PDX_M

	if(nFilterRem != 0)
	{
		int32_t nFilterDepth_16mul = nFilterDepth - nFilterRem;
		temp_mask = PDX_MOVB_AU32((0b1<<(nFilterRem)) - 1);
		acc_mask = PDX_CVTBB2M_B4M_L(temp_mask);

		for (int b = 0; b < nBatches; b++)
		{
			//process upto 2*PDX_M channels at at time for conv
			for(int32_t nCol=0; nCol < nSize; nCol++)//per pixel, do upto 2*PDX_M channel of input channel at at time
			{
				//do all output channels for a single pixel
				for (int32_t nChannelCnt = 0; nChannelCnt < nOutChannels; nChannelCnt++)
				{
					acc = 0;//reset accumulator

					//reset input
					inp = (xb_vec2Mx8 *)(pInputBuffer + nCol*nInChannels);	//Reinitialise input pointer for each output pixel
					valign ina; // define align vector
					ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

					xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + nChannelCnt*nInChannels); //Move to next filter for each output pixel
					valign wta;	// define align vector
					wta=PDX_LA_2MX8_PP (wtp);

					//for all the input pixels
					for (nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
					{
						PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
						PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
						vin+=vInZP;		//Add input offset

						PDX_MULAQW_2MX16(acc,vwt,vin);

					}

					PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
					PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
					vin+=vInZP;		//Add input offset

					//multiply and accumulate
					PDX_MULAQW_2MX16_T(acc,vwt,vin,acc_mask);

					sat_sum = PDX_RADD_2MX40(acc);									//PDX_RADD_2MX40: Adds across all 16lanes of acc and returns sum
					//adding bias<<1 to compensate for sign bit during multiplication
					//doubling index for bias buffer to account for 32bit bias instead of 64bit
					if(pBiasBuffer)	
					{
						sat_sum += (xb_int40)(pBiasBuffer[nChannelCnt]<<1);
					}
//					sat_sum = (MAX(MIN(sat_sum, (xb_int40)INT_32BIT_MAX), (xb_int40)INT_32BIT_MIN));//32bit saturation check to store result
					sat_sum = MIN(sat_sum, (xb_int40)INT_32BIT_MAX);
					sat_sum = MAX(sat_sum, (xb_int40)INT_32BIT_MIN);
					temp = (xb_int32)((int32_t)((int64_t)PDX_CVT64_40(sat_sum)));	//convert 40bit var into 32bit to perform multiplication

					product = PDX_MULW_32(temp, (uint32_t)nQuantizedMultiplier[nChannelCnt]);	//multiply with the quantization multiplier; product(80bit) = temp(32bit) * nQuantizedMultiplier(32bit)
					product =  PDX_SLA_80(product, (xb_int32)nQuantizedShift[nChannelCnt]);		//shift result by quantization multiplier
					temp = PDX_PACKQSRV_80(product,2);								//packs 80bit product into 32bit var with saturation and rounding
					temp+= (xb_int32)nOutZeroPoint;									//add output offset
					temp = MIN(temp, (xb_int32)INT_8BIT_MAX);
					temp = MAX(temp, (xb_int32)INT_8BIT_MIN);//8bit saturation check to store result
					*outp++ =(int8_t)((int32_t)temp);								//store result as 8-bit data
				}
			}
		}
	} else 
	{
		for (int b = 0; b < nBatches; b++)
		{
			//No of filter is equals to the nOutsize
			//process upto 2*PDX_M channels at at time for conv
			for(int32_t nCol=0; nCol < nSize; nCol++)//per pixel, do upto 2*PDX_M channel of input channel at at time
			{
				//do all output channels for a single pixel
				for (int32_t nChannelCnt = 0; nChannelCnt < nOutChannels; nChannelCnt++)
				{
					acc = 0;//reset accumulator

					//reset input
					inp = (xb_vec2Mx8 *)(pInputBuffer + nCol*nInChannels);	//Reinitialise input pointer for each output pixel
					valign ina; // define align vector
					ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

					xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + nChannelCnt*nInChannels); //Move to next filter for each output pixel
					valign wta;	// define align vector
					wta=PDX_LA_2MX8_PP (wtp);

					//for all the input pixels
					for (nPixProcessed= 0; nPixProcessed < nInChannels; nPixProcessed += (2*PDX_M))
					{
						PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
						PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
						vin+=vInZP;		//Add input offset

						PDX_MULAQW_2MX16(acc,vwt,vin);

					}

					sat_sum = PDX_RADD_2MX40(acc);									//PDX_RADD_2MX40: Adds across all 16lanes of acc and returns sum
					//adding bias<<1 to compensate for sign bit during multiplication
					//doubling index for bias buffer to account for 32bit bias instead of 64bit
					if(pBiasBuffer)	
					{
						sat_sum += (xb_int40)(pBiasBuffer[nChannelCnt]<<1);
					}
//					sat_sum = (MAX(MIN(sat_sum, (xb_int40)INT_32BIT_MAX), (xb_int40)INT_32BIT_MIN));//32bit saturation check to store result
					sat_sum = MIN(sat_sum, (xb_int40)INT_32BIT_MAX);
					sat_sum = MAX(sat_sum, (xb_int40)INT_32BIT_MIN);
					temp = (xb_int32)((int32_t)((int64_t)PDX_CVT64_40(sat_sum)));	//convert 40bit var into 32bit to perform multiplicatio

					product = PDX_MULW_32(temp, (uint32_t)nQuantizedMultiplier[nChannelCnt]);	//multiply with the quantization multiplier; product(80bit) = temp(32bit) * nQuantizedMultiplier(32bit)
					product =  PDX_SLA_80(product, (xb_int32)nQuantizedShift[nChannelCnt]);		//shift result by quantization multiplier
					temp = PDX_PACKQSRV_80(product,2);								//packs 80bit product into 32bit var with saturation and rounding
					temp+= (xb_int32)nOutZeroPoint;									//add output offset
					temp = MIN(temp, (xb_int32)INT_8BIT_MAX);
					temp = MAX(temp, (xb_int32)INT_8BIT_MIN);//8bit saturation check to store result
					*outp++ =(int8_t)((int32_t)temp);								//store result as 8-bit data
				}
			}
		}
	}
	return;
}


/**
*******************************************************************************
* Function: adi_sharcfx_conv2d_kernel1x1_noninterleaved_int16
* @brief optimized conv2d function
*
* @details 1x1 2D convolution in non-interleaved format using 16bit Eagle intrinsics
*
* Parameters:
* @param [in] pInputBuffer - input data
* @param [in] pWeightsBuffer - input weights buffer
* @param [in] pBiasBuffer - input bias buffer
* @param [in] nInChannels - input depth
* @param [in] nOutChannels - output depth
* @param [in] nWidth - input width
* @param [in] pQuantizedMultiplier - multiplier
* @param [in] pQuantizedShift - shift
* @param [in] nInputOffset - input zeropoint
* @param [in] nOutputOffset - output zeropoint
*
* @param [out] pOutputBuffer - output data
*
* @return None
*
*
*******************************************************************************
*/
void adi_sharcfx_conv2d_kernel1x1_noninterleaved_int16(const int8_t  *pInputBuffer,
                              int8_t        *pOutputBuffer,
                              const int8_t  *pWeightsBuffer,
                              const int32_t *pBiasBuffer,
                              int32_t       nOutWidth,
                              int32_t       nInChannels,
                              int32_t       nOutChannels,
                              uint32_t      *pQuantizedMultiplier,
                              int32_t       *pQuantizedShift,
                              int32_t       pInZeroPoint,
                              int32_t       pOutZeroPoint)
{
	//TODO:Separate out width and height
	const int8_t *pInputBuffer_copy = pInputBuffer;
	const int nSize = (nOutWidth * nOutWidth);
	const immediate Lane = 0;
	const immediate round_mode = ROUNDING_MODE;

	int nBytesToWrite = 0;
	xb_vec2Mx16 vwt, vin;
	xb_vec2Mx40 vsum;//Accumulator

	valign ax, az;//align input and output

#ifdef USE_EXTRA_PRECISION
	xb_vecMx8* __restrict pz = (xb_vecMx8 *)pOutputBuffer;
	xb_vecMx32  vOutZP = pOutZeroPoint;
	//load constants for range
	xb_vecMx32 vmin = ACT_MIN;
	xb_vecMx32 vmax = ACT_MAX;
	xb_vec2Mx40 vmax32bit = (xb_vec2Mx40)INT_32BIT_MAX;//Replicates the lane of data specified, across all lanes of a vector register
	xb_vec2Mx40 vmin32bit = (xb_vec2Mx40)INT_32BIT_MIN;//Replicates the lane of data specified, across all lanes of a vector register
	vbool2M greater_than32bit, lesser_than32bit;
	xb_vecMx32 first8, last8;

#else
	xb_vec2Mx8* __restrict pz = (xb_vec2Mx8 *)pOutputBuffer;
	xb_vec2Mx16  vOutZP = pOutZeroPoint;
	xb_vec2Mx16 vmin = -128;
	xb_vec2Mx16 vmax = 127;
#endif


	az = PDX_Z_ALIGN(); // initialize the alignment register to zero

	//zeropts and range are global across all channels
	xb_vec2Mx16 vInZP = pInZeroPoint;


	for(int32_t nChannel = 0; nChannel < nOutChannels; nChannel++)//per out channel
	{
		pInputBuffer = pInputBuffer_copy;//reset back to beginning of buffer for next channel

#ifdef USE_EXTRA_PRECISION
		xb_vecMx32 mult_acc_40 = pQuantizedMultiplier[nChannel];
		xb_vecMx80 outacc;

		//Load pQuantizedShift for channel
		xb_vecMx32 shift = pQuantizedShift[nChannel];
#else
		//Quant multiplier and shift per output channel, we know this will fit in int16
		uint16_t ReducedShift = (uint16_t)((pQuantizedMultiplier[nChannel] + (1 << 15)) >> 16);//Qmult is  (val) << 31, reduce this by 16 here to fit in int16
		//balance Qmult shift remaining is 15 but we include the +1 shift for PDX_MULAQW_2MX16 too here
		//this shift of 16 is accounted for during PDX_PACKQSRV_2MX40
		xb_vec2Mx16 mult_acc = ReducedShift;

		//Load pQuantizedShift for channel
		xb_vec2Mx40 shift = pQuantizedShift[nChannel];
#endif

		//add zp * wt to bias
		int32_t zp_wt_sum;
		int16_t wt_sum = 0;
		const int8_t *pCurrWeights = pWeightsBuffer;//back to beginning
		for(int32_t j=0; j < nInChannels; j++)
		{
			//weights are 8 bit so we have enough headroom
			int16_t nCurrWt = (int16_t)(*pCurrWeights++);
			wt_sum += nCurrWt;
		}

		//PDX_MULAQW_2MX16 will add a *2, so shift this sum also by 2
		zp_wt_sum = (pInZeroPoint) * wt_sum;//8 bit * 16 bit should fit within 32 bit
		zp_wt_sum += pBiasBuffer[nChannel];//bias specific to output channel
		zp_wt_sum <<= 1;

#ifdef USE_EXTRA_PRECISION
		xb_vecMx80 add_acc_80 = (xb_vecMx32)pQuantizedMultiplier[nChannel] * (xb_vecMx32)zp_wt_sum;
#else
		int64_t vconst_add = (int32_t)ReducedShift * (int32_t)zp_wt_sum;
		//saturate to fit within 40bits
		vconst_add = MAX(vconst_add,(int64_t)0xFFFFFF8000000000);
		vconst_add = MIN(vconst_add,(int64_t)0x7FFFFFFFFF);
		xb_vec2Mx40 add_acc = vconst_add;//PDX_REP_2MX40((xb_vec2Mx40)vconst_add,Lane);
#endif

		int nBytesLeft = nSize;
		for(int32_t nCol=0; nCol < nSize; nCol += 2*PDX_M)//per pixel, do 16(2*PDX_M) at a time
		{

			xb_vec2Mx40 acc = 0;

			const int8_t *pCurrWeights = pWeightsBuffer;//back to beginning
			const xb_vec2Mx8 *px = (const xb_vec2Mx8 *)pInputBuffer;

			//devNote-Out data despite being in 16 bit is effectively 8 bit
			//so we have a headroom of 40 - 8+8 = 24 bit headroom
			//we can do 2^23 macs before this will overflow so we can be assured
			//that even if acc doesnt saturate here we should be fine
			for(int32_t j=0; j < nInChannels; j++)
			{
				PDX_LSR16_8_IP(vwt,pCurrWeights,1);

				ax = PDX_LA_2MX8_PP (px); // prime, NOP if a[] is aligned
				//Load 8 bit input sign extended to 16 bit
				PDX_LA16_2MX8_XP(vin, ax, px, nSize); // load aligned, extend, increment to next channel


				//multiply and accumulate to 40 bit register
				//this will effectively do acc += vwt*vin*2;
				PDX_MULAQW_2MX16(acc,vwt,vin);
			}
#ifdef USE_EXTRA_PRECISION
			//16 40 bit -> 8 80 bits

			//keep 32 bit and do 80 bit multiplication
			//copy from acc to narrow register vector

			//Saturate each lane in acc before converting to 32bit
			//sets to true if first op greater than second op
			greater_than32bit =  PDX_GT_2MX40(acc, vmax32bit);//Get bool reg for registers greater than 32bit value
			//sets to true if first op less than second op
			lesser_than32bit =  PDX_LT_2MX40(acc, vmin32bit);//Get bool reg for registers greater than 32bit value

			//if bool is TRUE, first op is selected or else second op is selected
			acc = PDX_MOV_2MX40_T(vmax32bit, acc, greater_than32bit);
			acc = PDX_MOV_2MX40_T(vmin32bit, acc, lesser_than32bit);

			//there is no saturation in this conversion, hence to avoid conversion the 32 bit overflow check.
			//16-way 40-bit wide vector signed Integer convert operation, converting to 16-way (pair) 32-bit
			PDX_CVT32D_2MX40(last8, first8, acc);

			//first8
			outacc = mult_acc_40 * first8;//8-way 32bit mac
			outacc += add_acc_80;

			//shift and round
			outacc = PDX_SLS_MX80(outacc,shift);//saturating left shift, right shift if negative
			//round and shift and saturate
			//convert from 8 80bit -> 8 8bit

			xb_vecMx32 conv_out = PDX_PACKQSRV_MX80(outacc, round_mode);//shift by 32 bit 80->32

			//add output zero point
			conv_out += vOutZP;
			//Saturate to 8 bit range output_activation_min to 127
			conv_out = PDX_MIN_MX32(conv_out,vmax);
			conv_out = PDX_MAX_MX32(conv_out,vmin);
			nBytesToWrite = nBytesLeft - PDX_M > 0 ? PDX_M : nBytesLeft;
			nBytesLeft -= PDX_M;
			PDX_SAV32_MX8_XP(conv_out, az, pz, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
			//to 8-way 8-bit signed, forward post-increment addressing mode
			PDX_SAPOS_MX8_FP(az,pz);//flush

			//last16
			if(nBytesLeft>0){
				//first8
				outacc = mult_acc_40 * last8;//8-way 32bit mac
				outacc += add_acc_80;

				//shift and round
				outacc = PDX_SLS_MX80(outacc,shift);//saturating left shift, right shift if negative
				//round and shift and saturate
				//convert from 8 80bit -> 8 8bit

				xb_vecMx32 conv_out = PDX_PACKQSRV_MX80(outacc, round_mode);//shift by 32 bit 80->32

				//add output zero point
				conv_out += vOutZP;
				//Saturate to 8 bit range output_activation_min to 127
				conv_out = PDX_MIN_MX32(conv_out,vmax);
				conv_out = PDX_MAX_MX32(conv_out,vmin);
				nBytesToWrite = nBytesLeft - PDX_M > 0 ? PDX_M : nBytesLeft;
				nBytesLeft -= PDX_M;
				PDX_SAV32_MX8_XP(conv_out, az, pz, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
				//to 8-way 8-bit signed, forward post-increment addressing mode
				PDX_SAPOS_MX8_FP(az,pz);//flush
			}
			pInputBuffer += 2*PDX_M;//move onto next set of 16 pixels

#else
			//16 40 bit -> 16 bit and then again 40 bits mac

			//convert this to 16 bit for 16 bit multiplication with pQuantizedMultiplier
			//if this is beyond range of 16bit it will saturate
			//if we shift this by 16 here per storing, there is a chance of this becoming zero for low values

			xb_vec2Mx16 conv_out = PDX_PACKSIV_2MX40  (acc, 0);

			//multiply with 16 bit ReducedShift in 40 bit register
			acc = mult_acc * conv_out;
			acc += add_acc;//add (bias + sum(wt)*zp) * reduced shift * 2
			acc = PDX_SLS_2MX40(acc,shift);//saturating left shift, right shift if negative

			//round and shift and saturate
			conv_out = PDX_PACKQSRV_2MX40  (acc, round_mode);//shifts by 16 bits and symmetric rounding with saturation

			//add output zero point
			conv_out += vOutZP;

			//Saturate to 8 bit range -128 to 127
			conv_out = PDX_MIN_2MX16(conv_out,vmax);
			conv_out = PDX_MAX_2MX16(conv_out,vmin);

			nBytesToWrite = nBytesLeft - 2*PDX_M > 0 ? 2*PDX_M : nBytesLeft;

		    PDX_SAV16_2MX8_XP(conv_out, az, pz,nBytesToWrite);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
		    //to 16-way 8-bit signed, forward post-increment addressing mode

		    PDX_SAPOS_2MX8_FP(az,pz);//flush

			nBytesLeft -= 2*PDX_M;
			pInputBuffer += 2*PDX_M;//move onto next set of 16 pixels
#endif
		}
		pWeightsBuffer += nInChannels;//interleaved, move to next channel
	}
	return;
}


/**
*******************************************************************************
* Function: adi_sharcfx_conv2d_kernel3x3_stride2_same_pad_int8
* @brief optimized conv2d function
*
* @details optimized conv2d function for 8-bit integer input. 3x3 2D convolution in interleaved format using 16bit Eagle intrinsics and stride of 2x2
*
* Parameters:
* @param [in] pInputBuffer - input data
* @param [in] pWeightsBuffer - input weights buffer
* @param [in] pBiasBuffer - input bias buffer
* @param [in] nInChannels - input depth
* @param [in] nOutChannels - output depth
* @param [in] nWidth - input width
* @param [in] nHeight - input height
* @param [in] nFilters - # of kernels
* @param [in] pQuantizedMultiplier - multiplier
* @param [in] pQuantizedShift - shift
* @param [in] nInputOffset - input zeropoint
* @param [in] nOutputOffset - output zeropoint
* 
* @param [out] pOutputBuffer - output data
*
* @return None
*
*
*******************************************************************************
*/ 
void adi_sharcfx_conv2d_kernel3x3_stride2_valid_pad_int8(const int8_t* pInputBuffer,
                                     const int8_t* pWeightsBuffer,
                                     const int32_t* pBiasBuffer,
                                     int8_t* pOutputBuffer,
                                     int32_t nInChannels,
                                     int32_t nOutChannels,
                                     int32_t nWidth,
                                     int32_t nHeight,
                                     int32_t nFilters,
                                     int32_t *nQuantizedMultiplier,
                                     int32_t *nQuantizedShift,
                                     int32_t pInZeroPoint,
                                     int32_t pOutZeroPoint)
{

	int8_t* __restrict outp = (int8_t*) pOutputBuffer;

	const immediate Lane=0;
	//Defining the input and filter offsets
	xb_vec2Mx16 vInZP = PDX_REP_2MX16((xb_vec2Mx16)pInZeroPoint,Lane);//Replicates the lane of data specified, across all lanes of a vector register

	xb_vec2Mx16 vin,vwt;
	xb_int32 temp;
	xb_int40 sat_sum;
	xb_int80 product;

	vbool4M temp_mask;
	vbool2M acc_mask, ffff;

	xb_vec2Mx40 acc = 0;
	xb_vec2Mx8 *inp = (xb_vec2Mx8 *)pInputBuffer;
	int32_t nFilterDepth = nInChannels;
	int32_t nFilterRem = nFilterDepth % 16;//2*PDX_M
	int32_t nFilterDepth_16mul = nFilterDepth - nFilterRem;

	//if channels are multiple of 16
	if(nFilterRem == 0)
	{
		int32_t nRow = 0,nCol =0;
		//for 0th row and 0th column
		for(nRow = 1 ;nRow < nHeight-1; nRow+=STRIDE_2)
		{
			for(nCol = 1; nCol < nWidth-1; nCol+=STRIDE_2)
			{
				for(int32_t nFil =0; nFil<nFilters; nFil++)
				{
					acc = 0;//reset accumulator

					for(int32_t nKernelHeight = -1; nKernelHeight<= 1 ; nKernelHeight++)
					{
						int32_t nCurrentRow = (nRow + nKernelHeight);
						for(int32_t nKernelWidth = -1; nKernelWidth<= 1 ; nKernelWidth++)
						{
							int32_t nCurrentCol = (nCol + nKernelWidth);

							inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
							valign ina; // define align vector
							ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

							xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
							valign wta;	// define align vector
							wta=PDX_LA_2MX8_PP (wtp);

							//for all the input pixels
							for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
							{

								PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
								PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
								vin+=vInZP;		//Add input offset

								PDX_MULAQW_2MX16(acc,vwt,vin);
							}
						}
					}
					*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
				}
			}
		}
	}
	else
	{
		int32_t nRow = 0,nCol =0;
		//for 0th row and 0th column

		nFilterDepth_16mul = nFilterDepth - nFilterRem;
		temp_mask = PDX_MOVB_AU32((0b1<<(nFilterRem)) - 1);
		acc_mask = PDX_CVTBB2M_B4M_L(temp_mask);  // mask to accumulate values of remaining channels

		for(nRow = 1 ;nRow < nHeight-1; nRow+=STRIDE_2)
		{
			for(nCol = 1; nCol < nWidth-1; nCol+=STRIDE_2)
			{
				for(int32_t nFil =0; nFil<nFilters; nFil++)
				{
					acc = 0;//reset accumulator

					for(int32_t nKernelHeight = -1; nKernelHeight<= 1 ; nKernelHeight++)
					{
						int32_t nCurrentRow = (nRow + nKernelHeight);
						for(int32_t nKernelWidth = -1; nKernelWidth<= 1 ; nKernelWidth++)
						{
							int32_t nCurrentCol = (nCol + nKernelWidth);

							inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
							valign ina; // define align vector
							ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

							xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
							valign wta;	// define align vector
							wta=PDX_LA_2MX8_PP (wtp);

							//for all the input pixels
							for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
							{

								PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
								PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
								vin+=vInZP;		//Add input offset

								PDX_MULAQW_2MX16(acc,vwt,vin);
							}
							//for remaining channels
							PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
							PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
							vin+=vInZP;		//Add input offset

							//multiply and accumulate
							//for ex if input channels are 35, inputs from 1st 32 channels are handled in above for loop.
							// for rest 3 channels the mask will mask out rest 13 lanes and consider only 1st 3 lane for multiply and accumulate.
							PDX_MULAQW_2MX16_T(acc,vwt,vin,acc_mask);
						}
					}
					*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
				}
			}
		}
	}
}

/**
*******************************************************************************
* Function: adi_sharcfx_conv2d_kernel3x3_stride1_same_pad_int8
* @brief optimized conv2d function
*
* @details optimized conv2d function for 8-bit integer input. 3x3 2D convolution in interleaved format using 16bit Eagle intrinsics and stride of 1x1. Assumes same padding
*
* Parameters:
* @param [in] pInputBuffer - input data
* @param [in] pWeightsBuffer - input weights buffer
* @param [in] pBiasBuffer - input bias buffer
* @param [in] nInChannels - input depth
* @param [in] nOutChannels - output depth
* @param [in] nWidth - input width
* @param [in] nHeight - input height
* @param [in] nFilters - # of kernels
* @param [in] pQuantizedMultiplier - multiplier
* @param [in] pQuantizedShift - shift
* @param [in] nInputOffset - input zeropoint
* @param [in] nOutputOffset - output zeropoint
* 
* @param [out] pOutputBuffer - output data
*
* @return None
*
*
*******************************************************************************
*/ 
void adi_sharcfx_conv2d_kernel3x3_stride1_same_pad_int8(const int8_t* pInputBuffer,
                                     const int8_t* pWeightsBuffer,
                                     const int32_t* pBiasBuffer,
                                     int8_t* pOutputBuffer,
                                     int32_t nInChannels,
                                     int32_t nOutChannels,
                                     int32_t nWidth,
                                     int32_t nHeight,
                                     int32_t nFilters,
                                     int32_t *nQuantizedMultiplier,
                                     int32_t *nQuantizedShift,
                                     int32_t pInZeroPoint,
                                     int32_t pOutZeroPoint)
{

	int8_t* __restrict outp = (int8_t*) pOutputBuffer;

	const immediate Lane=0;
	//Defining the input and filter offsets
	xb_vec2Mx16 vInZP = PDX_REP_2MX16((xb_vec2Mx16)pInZeroPoint,Lane);//Replicates the lane of data specified, across all lanes of a vector register

	xb_vec2Mx16 vin,vwt;
	xb_int32 temp;
	xb_int40 sat_sum;
	xb_int80 product;

	vbool4M temp_mask;
	vbool2M acc_mask, ffff;

	xb_vec2Mx40 acc = 0;
	xb_vec2Mx8 *inp = (xb_vec2Mx8 *)pInputBuffer;
	int32_t nFilterDepth = nInChannels;
	int32_t nFilterRem = nFilterDepth % 16;//2*PDX_M
	int32_t nFilterDepth_16mul = nFilterDepth - nFilterRem;

	int32_t nChannelCnt = 0;

	/*
	 * The code is split into 7 sections as shown below:
	 *
	 * +---+---------------------------------+---+
	 * | 1 |               2                 | 3 |
	 * +---+---------------------------------+---+
	 * |   x                                 x   |
	 * |   x   xxxxxxxxxxxxxxxxxxxxxxxxxxx   x   |
	 * |   x                                 x   |
	 * |   x    xxxxxxxxxxxxxxxxxxxxxxxxxx   x   |
	 * |   x                4                x   |
	 * |   x   xxxxxxxxxxxxxxxxxxxxxxxxxxx   x   |
	 * |   x                                 x   |
	 * |   x                                 x   |
	 * +---+---------------------------------+---+
	 * | 5 |                6                | 7 |
	 * +---+---------------------------------+---+
	 *
	 *  1st pixel and last pixel of the top row and bottom
	 *  row is handled separately.
	 *  All the middle elements of the top row and the bottom
	 *  row are handled separately
	 *  For middle part of the input frame, inside one for loop
	 *  1st column and the last column is handled separately.
	 */

	//if channels are multiple of 16
	if(nFilterRem == 0)
	{
		int32_t nRow = 0,nCol =0;
		//for 0th row and 0th column
		int32_t pixel=0;

		for(int32_t nFil =0; nFil<nFilters; nFil++)
		{
			acc = 0;//reset accumulator

			//for nCurrentRow = 0 & nCurrentCol = 0
			//   mask for 3x3 filter
			//    000
			//    011
			//    011
			// when the loop is running 1st column or top row of the filter kernel we have to
			// reject the calculations done and save rest of the values in accumulator
			//Hence neglecting nKernelHeight = -1 and nKernelWidth = -1
			for(int32_t nKernelHeight = 0; nKernelHeight<= 1 ; nKernelHeight++)
			{
				int32_t nCurrentRow = (nRow + nKernelHeight);
				for(int32_t nKernelWidth = 0; nKernelWidth<= 1 ; nKernelWidth++)
				{
					int32_t nCurrentCol = (nCol + nKernelWidth);

					inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
					valign ina; // define align vector
					ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

					xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
					valign wta;	// define align vector
					wta=PDX_LA_2MX8_PP (wtp);

					//for all the input pixels
					for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
					{

						PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
						PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
						vin+=vInZP;		//Add input offset

						PDX_MULAQW_2MX16(acc,vwt,vin);
						nChannelCnt++;
					}
				}
			}
			*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
		}

		//rest of the elements of top row execpt the last element
		nRow = 0;
		for(nCol = 1; nCol < nWidth-1; nCol++)
		{

			for(int32_t nFil =0; nFil<nFilters; nFil++)
			{
				acc = 0;//reset accumulator

				//for nCurrentRow = 0 & nCurrentCol = 1 to n-2 (2nd column to last but one column)
				//   mask for 3x3 filter
				//    000
				//    111
				//    111
				// when the loop is running top row of the filter kernel we have to
				// reject the calculations done and save rest of the values in accumulator
				// hence neglecting nKernelHeight = -1
				for(int32_t nKernelHeight = 0; nKernelHeight<= 1 ; nKernelHeight++)
				{
					int32_t nCurrentRow = (nRow + nKernelHeight);
					for(int32_t nKernelWidth = -1; nKernelWidth<= 1 ; nKernelWidth++)
					{
						int32_t nCurrentCol = (nCol + nKernelWidth);

						inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
						valign ina; // define align vector
						ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

						xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
						valign wta;	// define align vector
						wta=PDX_LA_2MX8_PP (wtp);

						//for all the input pixels
						for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
						{
							PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
							PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
							vin+=vInZP;		//Add input offset

							PDX_MULAQW_2MX16(acc,vwt,vin);
						}
					}
				}
				*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);

			}
		}

		//last column of top row
		nRow = 0;
		nCol=nWidth-1;
		for(int32_t nFil =0; nFil<nFilters; nFil++)
		{
			acc = 0;//reset accumulator

			//for nCurrentRow = 0 & nCurrentCol = nWidth-1 (last column)
			//   mask for 3x3 filter
			//    000
			//    110
			//    110
			// when the loop is running to 3rd column or top row of the filter kernel we have to
			// reject the calculations done and save rest of the values in accumulator
			// hence neglect nKernelHeight = -1 and nKernelWidth = 1
			for(int32_t nKernelHeight = 0; nKernelHeight<= 1 ; nKernelHeight++)
			{
				int32_t nCurrentRow = (nRow + nKernelHeight);

				for(int32_t nKernelWidth = -1; nKernelWidth<=0 ; nKernelWidth++)
				{
					int32_t nCurrentCol = nCol + nKernelWidth;

					inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
					valign ina; // define align vector
					ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

					xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
					valign wta;	// define align vector
					wta=PDX_LA_2MX8_PP (wtp);

					//for all the input pixels
					for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
					{
						PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
						PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
						vin+=vInZP;		//Add input offset

						PDX_MULAQW_2MX16(acc,vwt,vin);
					}
				}
			}

			*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
		}

		//From Row =1 to row = nHeight-2 for all columns

		for(nRow = 1 ;nRow < nHeight-1; nRow++)
		{
			//for col  = 0
			nCol = 0;
			for(int32_t nFil =0; nFil<nFilters; nFil++)
			{
				acc = 0;//reset accumulator

				//for nCurrentRow = 0 & nCurrentCol = 0 to n-1
				// when nCurrentCol == 0
				//   mask for 3x3 filter
				//    011
				//    011
				//    011
				// when the loop is running to 1st column of the filter kernel we have to
				// reject the calculations done and save rest of the values in accumulator
				// hence reject nKernelWidth= -1
				for(int32_t nKernelHeight = -1; nKernelHeight<= 1 ; nKernelHeight++)
				{
					int32_t nCurrentRow = (nRow + nKernelHeight);

					for(int32_t nKernelWidth = 0; nKernelWidth<= 1 ; nKernelWidth++)
					{
						int32_t nCurrentCol = (nCol + nKernelWidth);

						inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
						valign ina; // define align vector
						ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

						xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
						valign wta;	// define align vector
						wta=PDX_LA_2MX8_PP (wtp);

						//for all the input pixels
						for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
						{
							PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
							PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
							vin+=vInZP;		//Add input offset

							PDX_MULAQW_2MX16(acc,vwt,vin);
						}
					}
				}

				*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
			}

			for(nCol = 1;nCol < nWidth-1; nCol++)
			{
				for(int32_t nFil =0; nFil<nFilters; nFil++)
				{
					acc = 0;//reset accumulator
					//for middle part (inputs without involving border rows or columns)
					//we take all the pixels of the kernel into account for computation.
					for(int32_t nKernelHeight = -1; nKernelHeight<= 1 ; nKernelHeight++)
					{
						int32_t nCurrentRow = (nRow + nKernelHeight);
						for(int32_t nKernelWidth = -1; nKernelWidth<= 1 ; nKernelWidth++)
						{
							int32_t nCurrentCol = (nCol + nKernelWidth);

							inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
							valign ina; // define align vector
							ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

							xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
							valign wta;	// define align vector
							wta=PDX_LA_2MX8_PP (wtp);

							//for all the input pixels
							for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
							{
								PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
								PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
								vin+=vInZP;		//Add input offset

								PDX_MULAQW_2MX16(acc,vwt,vin);
							}
						}
					}
					*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
				}
			}
			nCol = nWidth-1;
			for(int32_t nFil =0; nFil<nFilters; nFil++)
			{
				acc = 0;//reset accumulator

				//for nCurrentRow = 0 & nCurrentCol = 0 to n-1
				// when nCurrentCol == nWidth-1
				//   mask for 3x3 filter
				//    110
				//    110
				//    110
				// when the loop is running to 3rd column of the filter kernel we have to
				// reject the calculations done and save rest of the values in accumulator
				// hence neglect nKernelWidth = 1
				for(int32_t nKernelHeight = -1; nKernelHeight<= 1 ; nKernelHeight++)
				{
					int32_t nCurrentRow = (nRow + nKernelHeight);
					for(int32_t nKernelWidth = -1; nKernelWidth<= 0 ; nKernelWidth++)
					{
						int32_t nCurrentCol = (nCol + nKernelWidth);

						inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
						valign ina; // define align vector
						ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

						xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
						valign wta;	// define align vector
						wta=PDX_LA_2MX8_PP (wtp);

						//for all the input pixels
						for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
						{
							PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
							PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
							vin+=vInZP;		//Add input offset

							PDX_MULAQW_2MX16(acc,vwt,vin);
						}
					}
				}

				*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
			}
		}


		//nRow = nHeight -1 and nCol = 0
		nRow = nHeight -1 ;
		nCol =0;
		//for (nHeight -1)th row and 0th column
		for(int32_t nFil =0; nFil<nFilters; nFil++)
		{
			acc = 0;//reset accumulator

			//for nCurrentRow = nHeight -1 & nCurrentCol = 0
			//   mask for 3x3 filter
			//    011
			//    011
			//    000
			// when the loop is running 1st column or 3rd row of the filter kernel we have to
			// reject the calculations done and save rest of the values in accumulator
			//hence neglect nKernelHeight= 1 and nKernelWidth = -1
			for(int32_t nKernelHeight = -1; nKernelHeight<= 0 ; nKernelHeight++)
			{
				int32_t nCurrentRow = (nRow + nKernelHeight);
				for(int32_t nKernelWidth = 0; nKernelWidth<= 1 ; nKernelWidth++)
				{
					int32_t nCurrentCol = (nCol + nKernelWidth);


					inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
					valign ina; // define align vector
					ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

					xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
					valign wta;	// define align vector
					wta=PDX_LA_2MX8_PP (wtp);

					//for all the input pixels
					for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
					{

						PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
						PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
						vin+=vInZP;		//Add input offset

						PDX_MULAQW_2MX16(acc,vwt,vin);
					}
				}
			}

			*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
		}

		//rest of the elements of last row execpt the last element
		nRow = nHeight -1;
		for(nCol = 1; nCol < nWidth-1; nCol++)
		{
			for(int32_t nFil =0; nFil<nFilters; nFil++)
			{
				acc = 0;//reset accumulator

				//for last row
				//   mask for 3x3 filter
				//    111
				//    111
				//    000
				// when the loop is running top row of the filter kernel we have to
				// reject the calculations done and save rest of the values in accumulator
				// hence neglect nKernelHeight = 1

				for(int32_t nKernelHeight = -1; nKernelHeight<= 0 ; nKernelHeight++)
				{
					int32_t nCurrentRow =(nRow + nKernelHeight);
					for(int32_t nKernelWidth = -1; nKernelWidth<= 1 ; nKernelWidth++)
					{
						int32_t nCurrentCol = nCol + nKernelWidth;

						inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
						valign ina; // define align vector
						ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

						xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
						valign wta;	// define align vector
						wta=PDX_LA_2MX8_PP (wtp);

						//for all the input pixels
						for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
						{
							PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
							PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
							vin+=vInZP;		//Add input offset

							PDX_MULAQW_2MX16(acc,vwt,vin);
						}
					}
				}
			*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
			}
		}

		//last column of last row
		nRow = nHeight -1;
		nCol=nWidth-1;
		for(int32_t nFil =0; nFil<nFilters; nFil++)
		{
			acc = 0;//reset accumulator
			// for last column of last row
			//   mask for 3x3 filter
			//    110
			//    110
			//    000
			// when the loop is running to 3rd column and last row of the filter kernel we have to
			// reject the calculations done and save rest of the values in accumulator
			//hence we reject nKernelHeight = 1 and nKernelWidth = 1

			for(int32_t nKernelHeight = -1; nKernelHeight<= 0 ; nKernelHeight++)
			{
				int32_t nCurrentRow = (nRow + nKernelHeight);
				for(int32_t nKernelWidth = -1; nKernelWidth<= 0 ; nKernelWidth++)
				{
					int32_t nCurrentCol = nCol + nKernelWidth;

					inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
					valign ina; // define align vector
					ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

					xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
					valign wta;	// define align vector
					wta=PDX_LA_2MX8_PP (wtp);

					//for all the input pixels
					for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
					{
						PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
						PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
						vin+=vInZP;		//Add input offset

						PDX_MULAQW_2MX16(acc,vwt,vin);
					}
				}
			}

			*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
		}
	}
	//to handle cases when the input channels are not multiple of 16. We handle input channels in chunks of 16 and
	//perform load and accumulate operation once for the remaining channels with appropriate mask
	else
	{
		int32_t nRow = 0,nCol =0;
		//for 0th row and 0th column

		nFilterDepth_16mul = nFilterDepth - nFilterRem;
		temp_mask = PDX_MOVB_AU32((0b1<<(nFilterRem)) - 1);
		acc_mask = PDX_CVTBB2M_B4M_L(temp_mask);  // mask to accumulate values of remaining channels

		for(int32_t nFil =0; nFil<nFilters; nFil++)
		{
			acc = 0;//reset accumulator

			//for nCurrentRow = 0 & nCurrentCol = 0
			//   mask for 3x3 filter
			//    000
			//    011
			//    011
			// when the loop is running 1st column or top row of the filter kernel we have to
			// reject the calculations done and save rest of the values in accumulator
			//Hence neglecting nKernelHeight = -1 and nKernelWidth = -1
			for(int32_t nKernelHeight = 0; nKernelHeight<= 1 ; nKernelHeight++)
			{
				int32_t nCurrentRow = (nRow + nKernelHeight);
				for(int32_t nKernelWidth = 0; nKernelWidth<= 1 ; nKernelWidth++)
				{
					int32_t nCurrentCol = (nCol + nKernelWidth);

					inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
					valign ina; // define align vector
					ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

					xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
					valign wta;	// define align vector
					wta=PDX_LA_2MX8_PP (wtp);

					//for all the input pixels
					for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
					{

						PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
						PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
						vin+=vInZP;		//Add input offset

						PDX_MULAQW_2MX16(acc,vwt,vin);
					}

					//for remaining channels
					PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
					PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
					vin+=vInZP;		//Add input offset

					//multiply and accumulate
					//for ex if input channels are 35, inputs from 1st 32 channels are handled in above for loop.
					// for rest 3 channels the mask will mask out rest 13 lanes and consider only 1st 3 lane for multiply and accumulate.
					PDX_MULAQW_2MX16_T(acc,vwt,vin,acc_mask);

				}
			}

			*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
		}

		//rest of the elements of top row execpt the last element
		nRow = 0;
		for(nCol = 1; nCol < nWidth-1; nCol++)
		{

			for(int32_t nFil =0; nFil<nFilters; nFil++)
			{
				acc = 0;//reset accumulator

				//for nCurrentRow = 0 & nCurrentCol = 1 to n-2 (2nd column to last but one column)
				//   mask for 3x3 filter
				//    000
				//    111
				//    111
				// when the loop is running top row of the filter kernel we have to
				// reject the calculations done and save rest of the values in accumulator
				// hence neglecting nKernelHeight = -1
				for(int32_t nKernelHeight = 0; nKernelHeight<= 1 ; nKernelHeight++)
				{
					int32_t nCurrentRow = (nRow + nKernelHeight);
					for(int32_t nKernelWidth = -1; nKernelWidth<= 1 ; nKernelWidth++)
					{
						int32_t nCurrentCol = (nCol + nKernelWidth);

						inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
						valign ina; // define align vector
						ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

						xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
						valign wta;	// define align vector
						wta=PDX_LA_2MX8_PP (wtp);

						//for all the input pixels
						for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
						{
							PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
							PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
							vin+=vInZP;		//Add input offset

							PDX_MULAQW_2MX16(acc,vwt,vin);
						}
						//for remaining channels
						PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
						PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
						vin+=vInZP;		//Add input offset

						//multiply and accumulate
						PDX_MULAQW_2MX16_T(acc,vwt,vin,acc_mask);
					}
				}

				*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
			}
		}

		//last column of top row
		nRow = 0;
		nCol=nWidth-1;
		for(int32_t nFil =0; nFil<nFilters; nFil++)
		{
			acc = 0;//reset accumulator

			//for nCurrentRow = 0 & nCurrentCol = nWidth -1 (last column)
			//   mask for 3x3 filter
			//    000
			//    110
			//    110
			// when the loop is running to 3rd column or top row of the filter kernel we have to
			// reject the calculations done and save rest of the values in accumulator
			//hence reject the nKernelHeight= -1 and nKernelWidth =1
			for(int32_t nKernelHeight = 0; nKernelHeight<= 1 ; nKernelHeight++)
			{
				int32_t nCurrentRow = (nRow + nKernelHeight);
				for(int32_t nKernelWidth = -1; nKernelWidth<= 0 ; nKernelWidth++)
				{
					int32_t nCurrentCol =nCol + nKernelWidth;

					inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
					valign ina; // define align vector
					ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

					xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
					valign wta;	// define align vector
					wta=PDX_LA_2MX8_PP (wtp);

					//for all the input pixels
					for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
					{
						PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
						PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
						vin+=vInZP;		//Add input offset

						PDX_MULAQW_2MX16(acc,vwt,vin);
					}
					//for remaining channels
					PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
					PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
					vin+=vInZP;		//Add input offset

					//multiply and accumulate
					PDX_MULAQW_2MX16_T(acc,vwt,vin,acc_mask);
				}
			}

			*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
		}

		//From Row =1 to row = nHeight-2 for all columns
		for(nRow = 1 ;nRow < nHeight-1; nRow++)
		{
			//for col  = 0
			nCol = 0;
			for(int32_t nFil =0; nFil<nFilters; nFil++)
			{
				acc = 0;//reset accumulator
				// when nCurrentCol == 0
				//   mask for 3x3 filter
				//    011
				//    011
				//    011
				// when the loop is running to 1st column of the filter kernel we have to
				// reject the calculations done and save rest of the values in accumulator
				//hence reject nKernelWidth = -1

				for(int32_t nKernelHeight = -1; nKernelHeight<= 1 ; nKernelHeight++)
				{
					int32_t nCurrentRow = (nRow + nKernelHeight);
					for(int32_t nKernelWidth = 0; nKernelWidth<= 1 ; nKernelWidth++)
					{
						int32_t nCurrentCol = (nCol + nKernelWidth);

						inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
						valign ina; // define align vector
						ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

						xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
						valign wta;	// define align vector
						wta=PDX_LA_2MX8_PP (wtp);

						//for all the input pixels
						for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
						{
							PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
							PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
							vin+=vInZP;		//Add input offset

							PDX_MULAQW_2MX16(acc,vwt,vin);
						}
						//for remaining channels
						PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
						PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
						vin+=vInZP;		//Add input offset

						//multiply and accumulate
						PDX_MULAQW_2MX16_T(acc,vwt,vin,acc_mask);
					}
				}

				*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
			}

			for(nCol = 1;nCol < nWidth-1; nCol++)
			{
				for(int32_t nFil =0; nFil<nFilters; nFil++)
				{
					acc = 0;//reset accumulator
					//for middle part (inputs without involving border rows or columns)
					//we take all the pixels of the kernel into account for computation.
					for(int32_t nKernelHeight = -1; nKernelHeight<= 1 ; nKernelHeight++)
					{
						int32_t nCurrentRow = (nRow + nKernelHeight);
						for(int32_t nKernelWidth = -1; nKernelWidth<= 1 ; nKernelWidth++)
						{
							int32_t nCurrentCol = (nCol + nKernelWidth);

							inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
							valign ina; // define align vector
							ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

							xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
							valign wta;	// define align vector
							wta=PDX_LA_2MX8_PP (wtp);

							//for all the input pixels
							for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
							{
								PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
								PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
								vin+=vInZP;		//Add input offset

								PDX_MULAQW_2MX16(acc,vwt,vin);
							}
							//for remaining channels
							PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
							PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
							vin+=vInZP;		//Add input offset

							//multiply and accumulate
							PDX_MULAQW_2MX16_T(acc,vwt,vin,acc_mask);
						}
					}

					*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
				}
			}
			nCol = nWidth-1;
			for(int32_t nFil =0; nFil<nFilters; nFil++)
			{
				acc = 0;//reset accumulator

				// when nCurrentCol == nWidth-1
				//   mask for 3x3 filter
				//    110
				//    110
				//    110
				// when the loop is running to 3rd column of the filter kernel we have to
				// reject the calculations done and save rest of the values in accumulator
				//hence reject nKernelWidth = 1
				for(int32_t nKernelHeight = -1; nKernelHeight<= 1 ; nKernelHeight++)
				{
					int32_t nCurrentRow = (nRow + nKernelHeight);
					for(int32_t nKernelWidth = -1; nKernelWidth<= 0 ; nKernelWidth++)
					{
						int32_t nCurrentCol = (nCol + nKernelWidth);

						inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
						valign ina; // define align vector
						ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

						xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
						valign wta;	// define align vector
						wta=PDX_LA_2MX8_PP (wtp);

						//for all the input pixels
						for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
						{
							PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
							PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
							vin+=vInZP;		//Add input offset

							PDX_MULAQW_2MX16(acc,vwt,vin);
						}
						//for remaining channels
						PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
						PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
						vin+=vInZP;		//Add input offset

						//multiply and accumulate
						PDX_MULAQW_2MX16_T(acc,vwt,vin,acc_mask);
					}
				}

				*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
			}
		}

		//nRow = nHeight -1 and nCol = 0
		nRow = nHeight -1 ;
		nCol =0;
		//for 0th row and 0th column
		for(int32_t nFil =0; nFil<nFilters; nFil++)
		{
			acc = 0;//reset accumulator

			//for nCurrentRow = nHeight -1 & nCurrentCol = 0
			//   mask for 3x3 filter
			//    011
			//    011
			//    000
			// when the loop is running 1st column or 3rd row of the filter kernel we have to
			// reject the calculations done and save rest of the values in accumulator
			//hence neglect nKernelHeight = 1 and nKernelWidth = -1
			for(int32_t nKernelHeight = -1; nKernelHeight<= 0 ; nKernelHeight++)
			{
				int32_t nCurrentRow = (nRow + nKernelHeight);
				for(int32_t nKernelWidth = 0; nKernelWidth<= 1 ; nKernelWidth++)
				{
					int32_t nCurrentCol = (nCol + nKernelWidth);


					inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
					valign ina; // define align vector
					ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

					xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
					valign wta;	// define align vector
					wta=PDX_LA_2MX8_PP (wtp);

					//for all the input pixels
					for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
					{

						PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
						PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
						vin+=vInZP;		//Add input offset

						PDX_MULAQW_2MX16(acc,vwt,vin);
					}
					//for remaining channels
					PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
					PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
					vin+=vInZP;		//Add input offset

					//multiply and accumulate
					PDX_MULAQW_2MX16_T(acc,vwt,vin,acc_mask);
				}
			}

			*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
		}

		//rest of the elements of last row execpt the last element
		nRow = nHeight -1;
		for(nCol = 1; nCol < nWidth-1; nCol++)
		{
			for(int32_t nFil =0; nFil<nFilters; nFil++)
			{
				acc = 0;//reset accumulator

				//for nCurrentRow = 0 & nCurrentCol = 1 to n-2 (2nd column to last but one column)
				//   mask for 3x3 filter
				//    111
				//    111
				//    000
				// when the loop is running 3rd row of the filter kernel we have to
				// reject the calculations done and save rest of the values in accumulator
				//hence neglect nKernelHeight = 1
				for(int32_t nKernelHeight = -1; nKernelHeight<= 0 ; nKernelHeight++)
				{
					int32_t nCurrentRow = (nRow + nKernelHeight);
					for(int32_t nKernelWidth = -1; nKernelWidth<= 1 ; nKernelWidth++)
					{
						int32_t nCurrentCol = nCol + nKernelWidth;

						inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
						valign ina; // define align vector
						ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

						xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
						valign wta;	// define align vector
						wta=PDX_LA_2MX8_PP (wtp);

						//for all the input pixels
						for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
						{
							PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
							PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
							vin+=vInZP;		//Add input offset

							PDX_MULAQW_2MX16(acc,vwt,vin);
						}
						//for remaining channels
						PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
						PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
						vin+=vInZP;		//Add input offset

						//multiply and accumulate
						PDX_MULAQW_2MX16_T(acc,vwt,vin,acc_mask);
					}
				}

				*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
			}
		}

		//last column of last row
		nRow = nHeight -1;
		nCol=nWidth-1;
		for(int32_t nFil =0; nFil<nFilters; nFil++)
		{
			acc = 0;//reset accumulator

			//for nCurrentRow = 0 & nCurrentCol = nWidth (last column)
			//   mask for 3x3 filter
			//    110
			//    110
			//    000
			// when the loop is running to 3rd column or last row of the filter kernel we have to
			// reject the calculations done and save rest of the values in accumulator
			//hence neglect nKernelHeight= 1 and nKernelWidth =1

			for(int32_t nKernelHeight = -1; nKernelHeight<= 0 ; nKernelHeight++)
			{
				int32_t nCurrentRow = (nRow + nKernelHeight);
				for(int32_t nKernelWidth = -1; nKernelWidth<= 0 ; nKernelWidth++)
				{
					int32_t nCurrentCol =  nCol + nKernelWidth;

					inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
					valign ina; // define align vector
					ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

					xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
					valign wta;	// define align vector
					wta=PDX_LA_2MX8_PP (wtp);

					//for all the input pixels
					for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
					{
						PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
						PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
						vin+=vInZP;		//Add input offset

						PDX_MULAQW_2MX16(acc,vwt,vin);
					}
					//for remaining channels
					PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
					PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
					vin+=vInZP;		//Add input offset

					//multiply and accumulate
					PDX_MULAQW_2MX16_T(acc,vwt,vin,acc_mask);
				}
			}

			*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
		}
	}
}



/**
*******************************************************************************
* Function: adi_sharcfx_conv2d_kernel3x3_stride1_valid_pad_int8
* @brief optimized conv2d function
*
* @details optimized conv2d function for 8-bit integer input. 3x3 2D convolution in interleaved format using 16bit Eagle intrinsics and stride of 1x1. Assumes valid padding
*
* Parameters:
* @param [in] pInputBuffer - input data
* @param [in] pWeightsBuffer - input weights buffer
* @param [in] pBiasBuffer - input bias buffer
* @param [in] nInChannels - input depth
* @param [in] nOutChannels - output depth
* @param [in] nWidth - input width
* @param [in] nHeight - input height
* @param [in] nFilters - # of kernels
* @param [in] pQuantizedMultiplier - multiplier
* @param [in] pQuantizedShift - shift
* @param [in] nInputOffset - input zeropoint
* @param [in] nOutputOffset - output zeropoint
* 
* @param [out] pOutputBuffer - output data
*
* @return None
*
*
*******************************************************************************
*/ 
void adi_sharcfx_conv2d_kernel3x3_stride1_valid_pad_int8 (const int8_t* pInputBuffer,
                                                    const int8_t* pWeightsBuffer,
                                                    const int32_t* pBiasBuffer,
                                                    int8_t* pOutputBuffer,
                                                    int32_t nInChannels,
                                                    int32_t nOutChannels,
                                                    int32_t nWidth,
                                                    int32_t nHeight,
                                                    int32_t nFilters,
                                                    int32_t *nQuantizedMultiplier,
                                                    int32_t *nQuantizedShift,
                                                    int32_t pInZeroPoint,
                                                    int32_t pOutZeroPoint)
{

	int8_t* __restrict outp = (int8_t*) pOutputBuffer;

	const immediate Lane=0;
	//Defining the input and filter offsets
	xb_vec2Mx16 vInZP = PDX_REP_2MX16((xb_vec2Mx16)pInZeroPoint,Lane);//Replicates the lane of data specified, across all lanes of a vector register

	xb_vec2Mx16 vin,vwt;
	xb_int32 temp;
	xb_int40 sat_sum;
	xb_int80 product;

	vbool4M temp_mask;
	vbool2M acc_mask, ffff;

	xb_vec2Mx40 acc = 0;
	xb_vec2Mx8 *inp = (xb_vec2Mx8 *)pInputBuffer;
	int32_t nFilterDepth = nInChannels;
	int32_t nFilterRem = nFilterDepth % 16;//2*PDX_M
	int32_t nFilterDepth_16mul = nFilterDepth - nFilterRem;

	int32_t nChannelCnt = 0;
	valign ina, wta; // define align vector
	//if channels are multiple of 16
	if(nFilterRem == 0)
	{
		int32_t nRow,nCol;
		int8_t* pInt = (int8_t*)pInputBuffer;
		for(nRow = 0 ;nRow < nHeight-2; nRow++)
		{
			for(nCol = 0;nCol < nWidth-2; nCol++)
			{
				int8_t* pCurrInt = pInt;
				int8_t* pCurrWt = (int8_t*)pWeightsBuffer;
				for(int32_t nFil = 0; nFil < nFilters; nFil++)
				{
					int8_t* pProcPixel = pCurrInt;//reset
					acc = 0;//reset accumulator
					for(int32_t nKernelHeight = 0; nKernelHeight < 3 ; nKernelHeight++)
					{
						for(int32_t nKernelWidth = 0; nKernelWidth < 3 ; nKernelWidth++)
						{
							//inp = (xb_vec2Mx8 *)(pInputBuffer + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);	//Reinitialise input pointer for each output pixel
							inp = (xb_vec2Mx8 *)(pProcPixel);
							ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

							//xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight + 1)*INT_3x3_FILTER_WIDTH*nInChannels + (nKernelWidth+1)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels); //Move to next filter for each output pixel
							xb_vec2Mx8 *wtp = (xb_vec2Mx8 *)pCurrWt;
							wta=PDX_LA_2MX8_PP (wtp);

							//for all the input pixels
							for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
							{
								PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
								PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
								vin+=vInZP;		//Add input offset

								PDX_MULAQW_2MX16(acc,vwt,vin);
							}
							//jump to next pixel
							pProcPixel += nInChannels;
							pCurrWt += nInChannels;
						}
						//jump to next row
						pProcPixel += (nWidth*nInChannels) - 3*nInChannels;
					}
					*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
				}
				pInt += nInChannels;//move to next pixel since stride is 1
			}
			pInt += 2*nInChannels;//move to next pixel since stride is 1
		}
	}
	//to handle cases when the input channels are not multiple of 16. We handle input channels in chunks of 16 and
	//perform load and accumulate operation once for the remaining channels with appropriate mask
	else
	{
		nFilterDepth_16mul = nFilterDepth - nFilterRem;
		temp_mask = PDX_MOVB_AU32((0b1<<(nFilterRem)) - 1);
		acc_mask = PDX_CVTBB2M_B4M_L(temp_mask);  // mask to accumulate values of remaining channels

		int32_t nRow,nCol;
		int8_t* pInt = (int8_t*)pInputBuffer;
		for(nRow = 0 ;nRow < nHeight-2; nRow++)
		{
			for(nCol = 0;nCol < nWidth-2; nCol++)
			{
				int8_t* pCurrInt = pInt;
				int8_t* pCurrWt = (int8_t*)pWeightsBuffer;
				for(int32_t nFil = 0; nFil < nFilters; nFil++)
				{
					int8_t* pProcPixel = pCurrInt;//reset
					acc = 0;//reset accumulator
					for(int32_t nKernelHeight = 0; nKernelHeight < 3 ; nKernelHeight++)
					{
						for(int32_t nKernelWidth = 0; nKernelWidth < 3 ; nKernelWidth++)
						{
							//inp = (xb_vec2Mx8 *)(pCurrInt + nCurrentRow*nWidth*nInChannels + nCurrentCol*nInChannels);
							inp = (xb_vec2Mx8 *)(pProcPixel);
							//Reinitialise input pointer for each output pixel
							valign ina; // define align vector
							ina = PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

							//xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + (nKernelHeight)*INT_3x3_FILTER_WIDTH*nInChannels +	(nKernelWidth)*nInChannels + nFil*INT_3X3_FILTER_WIDTH_x_HEIGHT*nInChannels);
							xb_vec2Mx8 *wtp = (xb_vec2Mx8 *)pCurrWt;
							//Move to next filter for each output pixel
							valign wta;	// define align vector
							wta=PDX_LA_2MX8_PP (wtp);

							//for all the input pixels
							for (int32_t nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
							{
								PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
								PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
								vin+=vInZP;		//Add input offset

								PDX_MULAQW_2MX16(acc,vwt,vin);
							}
							//for remaining channels
							PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
							PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
							vin+=vInZP;		//Add input offset

							//multiply and accumulate
							PDX_MULAQW_2MX16_T(acc,vwt,vin,acc_mask);

							//jump to next pixel
							pProcPixel += nInChannels;
							pCurrWt += nInChannels;
						}
						//jump to next row
						pProcPixel += (nWidth*nInChannels) - 3*nInChannels;
					}
					*outp++ = quantize_and_store(acc,pBiasBuffer[nFil],nQuantizedMultiplier[nFil],nQuantizedShift[nFil],nFil,pOutZeroPoint);
				}
				pInt += nInChannels;//move to next pixel since stride is 1
			}
			pInt += 2*nInChannels;//move to next pixel since stride is 1
		}
	}
}

/*UTILITY FUCNTION*/
//TODO: Add comment on function of utility function
void transform_weights(
		int8_t* pOldBuffer,
		int8_t* pNewBuffer,
		int32_t nKernelH,
		int32_t nKernelW,
		int32_t nKernelCh,
		int32_t nNumKernels)
{
	int32_t temp =nKernelCh*nKernelW*nKernelH;
	for(int32_t i=0; i<nKernelH;i++){
		int32_t i_temp = i*nKernelCh*nKernelW;
		for(int32_t j=0; j<nKernelW;j++){
			int32_t j_temp = j*nKernelCh;
			for(int32_t k=0; k<nKernelCh;k++){
				for(int32_t l=0; l<nNumKernels;l++){

					*pNewBuffer++ = *(pOldBuffer + l*temp+
												   k +
												   j_temp+
												   i_temp);
				}
			}
		}
	}
}


/*UTILITY FUNCTION*/
//TODO: Add comment on function of utility function
inline int8_t quantize_and_store(
				xb_vec2Mx40 acc,
				int32_t pBiasBuffer,
				int32_t nQuantizedMultiplierValue,
				int32_t nQuantizedShiftValue,
				int32_t nFil,
				int32_t pOutZeroPoint
				)
{
	xb_int32 temp;
	xb_int40 sat_sum;
	xb_int80 product;
	sat_sum = PDX_RADD_2MX40(acc);	//PDX_RADD_2MX40: Adds across all 16lanes of acc and returns sum
	if(pBiasBuffer)	{
		sat_sum += ((xb_int40)pBiasBuffer)<<1;
	}
//	sat_sum = (MAX(MIN(sat_sum, (xb_int40)INT_32BIT_MAX), (xb_int40)INT_32BIT_MIN));//32bit saturation check to store result
	sat_sum = MIN(sat_sum, (xb_int40)INT_32BIT_MAX);
	sat_sum = MAX(sat_sum, (xb_int40)INT_32BIT_MIN);
	temp = (xb_int32)((int32_t)((int64_t)PDX_CVT64_40(sat_sum)));	//convert 40bit var into 32bit to perform multiplication

	product = PDX_MULW_32(temp, (xb_int32)nQuantizedMultiplierValue);	//multiply with the quantization multiplier; product(80bit) = temp(32bit) * nQuantizedMultiplier(32bit)
	product =  PDX_SLA_80(product, (xb_int32)nQuantizedShiftValue);		//shift result by quantization multiplier

	temp = PDX_PACKQSRV_80(product,ROUNDING_MODE_2);				//packs 80bit product into 32bit var with saturation and rounding
	temp+= (xb_int32)pOutZeroPoint;									//add output offset
	temp = MIN(temp, (xb_int32)INT_8BIT_MAX);
	temp = MAX(temp, (xb_int32)INT_8BIT_MIN);//8bit saturation check to store result

	return (int8_t)((int32_t)temp);
}


/*UTILITY FUCNTION*/
//Pad image with zeros as per the number or height and width

void pad_image_intrinsic (
    int8_t* pInputBuffer,
    int8_t* pOutputBuffer,
	int32_t nInHeight,
	int32_t nInWidth,
	int32_t nInChannels,
	int32_t nZeroPoint,
	int32_t nPadHeight,
	int32_t nPadWidth)
{
    int8_t* pIn_buffer = pInputBuffer;
    int8_t* pPadded_buffer = pOutputBuffer;

    int32_t nPadWidth_L = nPadWidth >> 1;  // Left Padding
    int32_t nPadWidth_R = nPadWidth - nPadWidth_L;
    int32_t nPadHeight_T = nPadHeight >> 1;  // Top padding
    int32_t nPadHeight_B = nPadHeight - nPadHeight_T;

    int32_t newHeight = nInHeight + nPadHeight;
    int32_t newWidth = nInWidth + nPadWidth;

    int32_t row, col;

    xb_vec2Mx16 v_load;
	xb_vec2Mx16 *inp = (xb_vec2Mx16 *)pIn_buffer;
	xb_vec2Mx16 *store_ptr = (xb_vec2Mx16 *)pPadded_buffer;
	int32_t load_loop_count = (nInWidth*nInChannels)/(4*PDX_M);
	int32_t load_remaining = (nInWidth*nInChannels)%(4*PDX_M);
	valign load_align, store_align; // define align vector
	store_align= PDX_Z_ALIGN(); // prime, NOP if a[] is aligned


    // Fill top padding
    for (row = 0; row < nPadHeight_T; row++) {
            memset(pPadded_buffer, -nZeroPoint, sizeof(int8_t) * nInChannels * newWidth);
            pPadded_buffer += nInChannels * newWidth;
    }

    // Fill middle rows
    for (row = 0; row < nInHeight; row++) {
        // Fill left padding
        for (col = 0; col < nPadWidth_L; col++) {
            memset(pPadded_buffer, -nZeroPoint, sizeof(int8_t) * nInChannels);
            pPadded_buffer += nInChannels;
        }

        //Load store
        inp = (xb_vec2Mx16 *)(pIn_buffer);

		load_align= PDX_LA_2MX16_PP (inp); // prime, NOP if a[] is aligned
		store_ptr = (xb_vec2Mx16 *)(pPadded_buffer);

        for(int load_counter = 0; load_counter < load_loop_count; load_counter++){
			PDX_LAV_2MX16_XP (v_load, load_align, inp, 4*PDX_M);
			PDX_SA_2MX16_IP(v_load, store_align, store_ptr);
        }

        pPadded_buffer += load_loop_count*4*PDX_M;
		pIn_buffer += load_loop_count*4*PDX_M;

		PDX_SAPOS_2MX16_FP(store_align, store_ptr);
		for(int rem = 0; rem < load_remaining; rem++){
			*(pPadded_buffer++) =*(pIn_buffer++);
		}

        // Fill right padding
        for (col = 0; col < nPadWidth_R; col++) {
            memset(pPadded_buffer, -nZeroPoint, sizeof(int8_t) * nInChannels);
            pPadded_buffer += nInChannels;
        }
    }

    // Fill bottom padding
    for (row = 0; row < nPadHeight_B; row++) {
    			memset(pPadded_buffer, -nZeroPoint, sizeof(int8_t) * nInChannels * newWidth);
    	        pPadded_buffer += nInChannels * newWidth;
    }
}

void pad_image_optimized (
    int8_t* pInputBuffer,
    int8_t* pOutputBuffer,
	int32_t nInHeight,
	int32_t nInWidth,
	int32_t nInChannels,
	int32_t nZeroPoint,
	int32_t nPadHeight,
	int32_t nPadWidth)
{
    int8_t* pIn_buffer = pInputBuffer;
    int8_t* pPadded_buffer = pOutputBuffer;

    int32_t nPadWidth_L = nPadWidth >> 1;  // Left Padding
    int32_t nPadWidth_R = nPadWidth - nPadWidth_L;
    int32_t nPadHeight_T = nPadHeight >> 1;  // Top padding
    int32_t nPadHeight_B = nPadHeight - nPadHeight_T;

    int32_t newHeight = nInHeight + nPadHeight;
    int32_t newWidth = nInWidth + nPadWidth;

    int32_t row, col;

    // Fill top padding
    for (row = 0; row < nPadHeight_T; row++) {
            memset(pPadded_buffer, -nZeroPoint, sizeof(int8_t) * nInChannels * newWidth);
            pPadded_buffer += nInChannels * newWidth;
    }

    // Fill middle rows
    for (row = 0; row < nInHeight; row++) {
        // Fill left padding
        for (col = 0; col < nPadWidth_L; col++) {
            memset(pPadded_buffer, -nZeroPoint, sizeof(int8_t) * nInChannels);
            pPadded_buffer += nInChannels;
        }

        // Copy input buffer
		//we can try loop unrolling
        for(int i=0; i<nInChannels * nInWidth; i++){
				*(pPadded_buffer++) =*(pIn_buffer++);
			}

        // Fill right padding
        for (col = 0; col < nPadWidth_R; col++) {
            memset(pPadded_buffer, -nZeroPoint, sizeof(int8_t) * nInChannels);
            pPadded_buffer += nInChannels;
        }
    }

    // Fill bottom padding
    for (row = 0; row < nPadHeight_B; row++) {
    			memset(pPadded_buffer, -nZeroPoint, sizeof(int8_t) * nInChannels * newWidth);
    	        pPadded_buffer += nInChannels * newWidth;
    }
}

void pad_image (
		int8_t* pInputBuffer,
		int8_t* pOutputBuffer,
		int32_t nInHeight,
		int32_t nInWidth,
		int32_t nInChannels,
		int32_t nZeroPoint,
		int32_t nPadHeight,
		int32_t nPadWidth)
{
	int8_t* pIn_buffer = (int8_t*) pInputBuffer;
	int8_t* pPadded_buffer = (int8_t*) pOutputBuffer;

	int32_t nPadWidth_L, nPadWidth_R, nPadHeight_T, nPadHeight_B;
	nPadWidth_L =(int32_t) (nPadWidth>>1);				//Left Padding
	nPadWidth_R = nPadWidth - nPadWidth_L;

	nPadHeight_T =(int32_t) (nPadHeight>>1);			//Top padding
	nPadHeight_B = nPadHeight - nPadHeight_T;

	int32_t newHeight = nInHeight+nPadHeight;
	int32_t newWidth = nInWidth+nPadWidth;

	for(int32_t nRow = 0; nRow< newHeight; nRow++){
		for(int32_t nCol = 0; nCol< newWidth; nCol++){
			if (nCol<nPadWidth_L){//Left padding if present
				memset(pPadded_buffer, -nZeroPoint, sizeof(int8_t)*nInChannels);
				pPadded_buffer+=nInChannels;
			}
			if (nCol>=nPadWidth_L && nCol<nInWidth+nPadWidth_L){
				if (nRow<nPadHeight_T){//Top padding if present
					memset(pPadded_buffer, -nZeroPoint, sizeof(int8_t)*nInChannels);
					pPadded_buffer+=nInChannels;
				}
				if (nRow>=nInHeight+nPadHeight_T){//Bottom padding if present
					memset(pPadded_buffer, -nZeroPoint, sizeof(int8_t)*nInChannels);
					pPadded_buffer+=nInChannels;
				}
				if (nRow>=nPadHeight_T && nRow<nInHeight+nPadHeight_T){//No padding
					memcpy(pPadded_buffer, pIn_buffer, sizeof(int8_t)*nInChannels);
					pPadded_buffer+=nInChannels;
					pIn_buffer +=nInChannels;
				}
			}
			if (nCol>=(nInWidth+nPadWidth_L)){//Check for right padding
				memset(pPadded_buffer, -nZeroPoint, sizeof(int8_t)*nInChannels);
				pPadded_buffer+=nInChannels;
			}
		}
	}
}

/*UTILITY FUCNTION*/
//Copy the input output kernel number of times for convolution
void get_padded_input(
		int8_t* pOldBuffer,
		int8_t* pNewBuffer,
		int32_t nKernelH,
		int32_t nKernelW,
		int32_t nKernelCh,
		int32_t nNumKernels,
		int32_t nInputH,
		int32_t nInputW,
		int32_t nInputCh)
{
	for(int32_t i=0; i<nKernelH;i++){
		int32_t i_temp = i*nInputCh*nInputW;
		for(int32_t j=0; j<nKernelW;j++){
			int32_t j_temp = j*nInputCh;
			for(int32_t k=0; k<nKernelCh;k++){
				memset(pNewBuffer, *(pOldBuffer + k +
												   j_temp+
												   i_temp),
									sizeof(int8_t)*nNumKernels);
				pNewBuffer+=nNumKernels;
			}
		}
	}
}

void get_padded_input_intrinsic(
    int8_t* pOldBuffer,
    int8_t* pNewBuffer,
    int32_t nKernelH,
    int32_t nKernelW,
    int32_t nKernelCh,
    int32_t nNumKernels,
    int32_t nInputH,
    int32_t nInputW,
    int32_t nInputCh)
{
	int8_t mult_32 = nNumKernels/(4*PDX_M);
	int8_t rem_32 =  nNumKernels%(4*PDX_M);
	xb_vec4Mx8 a;
	xb_vec4Mx8 *store_ptr = (xb_vec4Mx8 *)pNewBuffer;
	valign store_align; // define align vector
	store_align= PDX_Z_ALIGN(); // prime, NOP if a[] is aligned

    for (int32_t i = 0; i < nKernelH; i++) {
        int32_t i_offset = i * nInputCh * nInputW;
        for (int32_t j = 0; j < nKernelW; j++) {
            int32_t j_offset = j * nInputCh;
            int8_t* pOldRow = pOldBuffer + i_offset + j_offset;

            for (int32_t k = 0; k < nKernelCh; k++) {
                int8_t value = *(pOldRow + k);
                int8_t* value_ptr = &value;

                for(int i = 0; i < mult_32; i++){
                	PDX_LSR_8_XP(a, value_ptr, 0);
                	PDX_SA_4MX8_IP(a, store_align, store_ptr);
                }
                pNewBuffer += mult_32*4*PDX_M;

                for(int i = 0; i < rem_32; i++){
                	*(pNewBuffer++) = value;
                }
            }
        }
    }
}
void get_padded_input_byte(
    int8_t* pOldBuffer,
    int8_t* pNewBuffer,
    int32_t nKernelH,
    int32_t nKernelW,
    int32_t nKernelCh,
    int32_t nNumKernels,
    int32_t nInputH,
    int32_t nInputW,
    int32_t nInputCh)
{
    for (int32_t i = 0; i < nKernelH; i++) {
        int32_t i_offset = i * nInputCh * nInputW;
        for (int32_t j = 0; j < nKernelW; j++) {
            int32_t j_offset = j * nInputCh;
            int8_t* pOldRow = pOldBuffer + i_offset + j_offset;

            for (int32_t k = 0; k < nKernelCh; k++) {
                int8_t value = *(pOldRow + k);
                for (int32_t n = 0; n < nNumKernels; n++) {
                    *(pNewBuffer++) = value;
                }
            }
        }
    }
}

/**
*******************************************************************************
* Function: adi_sharcfx_conv2d_dilation1x1_int8
* @brief optimized conv2d function
*
* @details optimized conv2d function for 8-bit integer input. 2D convolution in interleaved format using 16bit Eagle intrinsics. Assumes dialation 1x1
*
* Parameters:
* @param [in] pInputBuffer - input data
* @param [in] pWeightsBuffer - input weights buffer
* @param [in] pBiasBuffer - input bias buffer
* @param [in] nBatches - batch size
* @param [in] nInChannels - input depth
* @param [in] nOutChannels - output depth
* @param [in] nKernelHeight - kernel height
* @param [in] nKernelWidth - kernel width
* @param [in] stride_height - stride height
* @param [in] stride_width - stride width
* @param [in] nPadHeight - padding height
* @param [in] nPadWidth - padding width
* @param [in] nOutHeight - output height
* @param [in] nOutWidth - output width
* @param [in] pQuantizedMultiplier - multiplier
* @param [in] pQuantizedShift - shift
* @param [in] pInZeroPoint - input zeropoint
* @param [in] pOutZeroPoint - output zeropoint
* @param [in] nFilterZeroPoint - filter zeropoint
* @param [in] nActMin - min value after activation function
* @param [in] nActMax - max value after activation function
* 
* @param [out] pOutputBuffer - output data
*
* @return None
*
*
*******************************************************************************
*/ 
void adi_sharcfx_conv2d_dilation1x1_int8(
		const int8_t* pInputBuffer,
		const int8_t* pWeightsBuffer,
		const int32_t* pBiasBuffer,
		int8_t* pOutputBuffer,
		int32_t nBatches,
		int32_t nInChannels,
		int32_t nOutChannels,
		int32_t nKernelHeight,
		int32_t nKernelWidth,
		int32_t nNumKernels,
		int32_t nInputWidth,
		int32_t nInputHeight,
		int32_t stride_height,
		int32_t stride_width,
		int32_t nPadHeight,
		int32_t nPadWidth,
		int32_t nOutHeight,
		int32_t nOutWidth,
		int32_t *pQuantizedMultiplier,
		int32_t *pQuantizedShift,
		int32_t pInZeroPoint,
		int32_t pOutZeroPoint,
		int32_t nFilterZeroPoint,
		int32_t nActMin,
		int32_t nActMax)
{
	xb_vecMx8* __restrict outp  = (xb_vecMx8 *)pOutputBuffer;
	valign outa = PDX_LA_MX8_PP (outp); // prime, NOP if a[] is aligned
	const immediate Lane=0;

	xb_vec2Mx16 vin,vwt;
	xb_int32 temp;
	xb_int40 sat_sum;
	xb_int80 product;

	vbool4M temp_mask;
	vbool2M acc_mask, ffff;

	xb_vec2Mx40 acc = 0;

	int32_t nTotalPadWidth, nTotalPadHeight;
	nTotalPadWidth = (nOutWidth-1)*stride_width + nKernelWidth - nInputWidth;
	nTotalPadHeight = (nOutHeight-1)*stride_height + nKernelHeight - nInputHeight;

	uint32_t nWeightBufSize =nNumKernels*nKernelHeight*nKernelWidth*nInChannels;
	uint32_t nPaddedInputSize =nInChannels*(nInputWidth+nTotalPadWidth)*(nInputHeight+nTotalPadHeight);
	int32_t nIndexBuffer = 128;


	transform_weights((int8_t*) pWeightsBuffer,(int8_t*)&pTemp[nPaddedInputSize+nIndexBuffer], nKernelHeight,nKernelWidth,nInChannels, nNumKernels);


	xb_vec2Mx8 *wtp;// = (xb_vec2Mx8 *)pWeightsBuffer;
	xb_vec2Mx8 *inp;// = (xb_vec2Mx8 *)pInputBuffer;
	xb_vecMx32 first8, last8;
	xb_vec2Mx40 outacc;
	xb_vecMx80 quant_acc, quant_acc2;
	const immediate round_mode = ROUNDING_MODE;
	xb_vecMx32 mult_l,mult_h;
	xb_vecMx32 shift_l,shift_h;
	//xb_vec2Mx40 vbias;
	xb_vecMx32 vbias_l,vbias_h;
	xb_vecMx32 conv_out, conv_out2;
	xb_vecMx32 vmin = nActMin;
	xb_vecMx32 vmax = nActMax;
	xb_vec2Mx16 vInZP = pInZeroPoint;
	xb_vecMx32 vOutZP = pOutZeroPoint;
	xb_vec2Mx16 vFilterZP =nFilterZeroPoint;

	int32_t nFilterDepth = nInChannels;

	int32_t nOutputHeight = nOutHeight; //Stride 1
	int32_t nOutputWidth = nOutWidth;

	int32_t nOutChannelsMod16 = ((int)(nOutChannels / 16))*16;//2*PDX_M
	int32_t nPixLeft = nOutChannels;
	int32_t nPixToWrite = 0;

	for (int32_t nBatch = 0; nBatch < nBatches; ++nBatch) 
	{
		//Get padded image
		pad_image_intrinsic((int8_t*) (pInputBuffer+nBatch*nInputHeight*nInputWidth*nInChannels),(int8_t*)pTemp, nInputHeight, nInputWidth, nInChannels,pInZeroPoint, nTotalPadHeight,nTotalPadWidth);

		for (int32_t out_y = 0; out_y < nOutputHeight; ++out_y) 
		{
			//update inp and wt
			for (int32_t out_x = 0; out_x < nOutputWidth; ++out_x) 
			{
				nPixLeft = nOutChannels;
				//Extract the corresponding input buffer
                get_padded_input_byte((int8_t*) (pTemp+out_y*stride_height*nInChannels*(nInputWidth+nTotalPadWidth)+stride_width*out_x*nInChannels),(int8_t*)&pTemp[nPaddedInputSize+nIndexBuffer+nWeightBufSize], nKernelHeight,nKernelWidth,nInChannels, nNumKernels, nInputHeight+nTotalPadHeight,nInputWidth+nTotalPadWidth,nInChannels);

				for (int32_t nOutChannel = 0; nOutChannel < nOutChannelsMod16; nOutChannel+=2*PDX_M) 
				{
					inp = (xb_vec2Mx8*)(&pTemp[nPaddedInputSize+nIndexBuffer+nWeightBufSize] + nOutChannel);
					valign ina; // define align vector
					ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

					wtp = (xb_vec2Mx8*)(&pTemp[nPaddedInputSize+nIndexBuffer]+ nOutChannel);
					valign wta;	// define align vector
					wta=PDX_LA_2MX8_PP (wtp);

					//read in mult factors and bias for the channels
					xb_vecMx32 *vMult = (xb_vecMx32 *)(pQuantizedMultiplier + nOutChannel);
					valign wMulta =  PDX_LA_MX32_PP(vMult);

					xb_vecMx32 *vShift = (xb_vecMx32 *)(pQuantizedShift + nOutChannel);
					valign wShifta =  PDX_LA_MX32_PP(vShift);

					xb_vecMx32 *vBias = (xb_vecMx32 *)(pBiasBuffer + nOutChannel);
					valign wBiasa =  PDX_LA_MX32_PP(vBias);

					//read 8 way 32 bit signed
					PDX_LA_MX32_XP (mult_l, wMulta, vMult, 4*PDX_M);
					PDX_LA_MX32_XP (mult_h, wMulta, vMult, 0);

					PDX_LA_MX32_XP (shift_l, wShifta, vShift, 4*PDX_M);
					PDX_LA_MX32_XP (shift_h, wShifta, vShift, 0);

					PDX_LA_MX32_XP (vbias_l, wBiasa, vBias, 4*PDX_M);
					PDX_LA_MX32_XP (vbias_h, wBiasa, vBias, 0);

					vbias_l = PDX_SLS_MX32(vbias_l,1);//*2 to match with acc
					vbias_h = PDX_SLS_MX32(vbias_h,1);//*2 to match with acc

					acc = 0;// Reset acc
					//Have input buffer in required format. Perform the conv/matmul op and then store  min(2*PDX_M, RemainingOutChannels) pixels at a time
					//Use same input buffer for all filters to get one row of output pixels.
					for(int32_t nKerCh =0; nKerCh<nInChannels*nKernelHeight*nKernelWidth;nKerCh++)
					{
						//READ IP
						PDX_LA16_2MX8_XP (vin, ina, inp, nNumKernels);//read 2*PDX_M number of channels for 1 pixel, skip to adjoining pixel
						vin += vInZP;		//Add input offset
						ina = PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned
						//READ WT
						PDX_LA16_2MX8_XP (vwt, wta, wtp, nNumKernels);//read 2*PDX_M number of channels for 1 pixel
//								vwt += vFilterZP;	//Add filter offset
						wta = PDX_LA_2MX8_PP (wtp); // prime, NOP if a[] is aligned
						//MAC
						PDX_MULAQW_2MX16(acc,vin,vwt);//acc contains upto 2*PDX_M channel results for pixel

					}
					//Quantize and store
					//quantization compensation

					PDX_CVT32D_2MX40(last8, first8, acc);	//Converting 40bit results to 32bit to prevent loss of accuracy from packing of 40bit -> 16bit
					if (pBiasBuffer){
						first8 += vbias_l;
						last8 += vbias_h;
					}

					//first8
					quant_acc = mult_l * first8;	//Multiplying 2 32-bit vectors and storing result in 80bit vector
					//last8
					quant_acc2 = mult_h * last8;	//Multiplying 2 32-bit vectors and storing result in 80bit vector
					//shift and round
					quant_acc = PDX_SLS_MX80(quant_acc,shift_l);//saturating left shift, right shift if negative
					//shift and round
					quant_acc2 = PDX_SLS_MX80(quant_acc2,shift_h);//saturating left shift, right shift if negative
					//round and shift and saturate
					conv_out = PDX_PACKQSRV_MX80   (quant_acc, round_mode);	//pack 80bit result to 32bit with rounding and saturation.
					//round and shift and saturate
					conv_out2 = PDX_PACKQSRV_MX80   (quant_acc2, round_mode);	//pack 80bit result to 32bit with rounding and saturation.
					//add output zero point
					conv_out += vOutZP;
					//add output zero point
					conv_out2 += vOutZP;
					//Saturate to 8 bit range output_activation_min to 127
					conv_out = PDX_MIN_MX32(conv_out,vmax);
					conv_out2 = PDX_MAX_MX32(conv_out2,vmin);
					conv_out = PDX_MAX_MX32(conv_out,vmin);
					conv_out2 = PDX_MIN_MX32(conv_out2,vmax);

					nPixLeft -= 2*PDX_M;
					PDX_SAV32_MX8_XP(conv_out, outa, outp, PDX_M);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
					PDX_SAPOS_MX8_FP(outa,outp);//flush
					PDX_SAV32_MX8_XP(conv_out2, outa, outp, PDX_M);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
					PDX_SAPOS_MX8_FP(outa,outp);//flush

				}
				//Handle non-multiple of 16 pixels
				if(nPixLeft>0)
				{
					inp = (xb_vec2Mx8*)(&pTemp[nPaddedInputSize+nIndexBuffer+nWeightBufSize] + nOutChannelsMod16);
					valign ina; // define align vector
					ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

					wtp = (xb_vec2Mx8*)(&pTemp[nPaddedInputSize+nIndexBuffer]+ nOutChannelsMod16);
					valign wta;	// define align vector
					wta=PDX_LA_2MX8_PP (wtp);

					//read in mult factors and bias for the channels
					xb_vecMx32 *vMult = (xb_vecMx32 *)(pQuantizedMultiplier + nOutChannelsMod16);
					valign wMulta =  PDX_LA_MX32_PP(vMult);

					xb_vecMx32 *vShift = (xb_vecMx32 *)(pQuantizedShift + nOutChannelsMod16);
					valign wShifta =  PDX_LA_MX32_PP(vShift);

					xb_vecMx32 *vBias = (xb_vecMx32 *)(pBiasBuffer + nOutChannelsMod16);
					valign wBiasa =  PDX_LA_MX32_PP(vBias);

					//read 8 way 32 bit signed
					PDX_LA_MX32_XP (mult_l, wMulta, vMult, 4*PDX_M);
					PDX_LA_MX32_XP (mult_h, wMulta, vMult, 0);

					PDX_LA_MX32_XP (shift_l, wShifta, vShift, 4*PDX_M);
					PDX_LA_MX32_XP (shift_h, wShifta, vShift, 0);

					PDX_LA_MX32_XP (vbias_l, wBiasa, vBias, 4*PDX_M);
					PDX_LA_MX32_XP (vbias_h, wBiasa, vBias, 0);

					vbias_l = PDX_SLS_MX32(vbias_l,1);//*2 to match with acc
					vbias_h = PDX_SLS_MX32(vbias_h,1);//*2 to match with acc

					acc = 0;// Reset acc
					//Have input buffer in required format. Perform the conv/matmul op and then store  min(2*PDX_M, RemainingOutChannels) pixels at a time
					//Use same input buffer for all filters to get one row of output pixels.

					for(int32_t nKerCh =0; nKerCh<nInChannels*nKernelHeight*nKernelWidth;nKerCh++)
					{
						//READ IP
						PDX_LA16_2MX8_XP (vin, ina, inp, nNumKernels);//read 2*PDX_M number of channels for 1 pixel, skip to adjoining pixel
						vin += vInZP;		//Add input offset
						ina = PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned
						//READ WT
						PDX_LA16_2MX8_XP (vwt, wta, wtp, nNumKernels);//read 2*PDX_M number of channels for 1 pixel
	//								vwt += vFilterZP;	//Add filter offset
						wta = PDX_LA_2MX8_PP (wtp); // prime, NOP if a[] is aligned
						//MAC
						PDX_MULAQW_2MX16(acc,vin,vwt);//acc contains upto 2*PDX_M channel results for pixel

					}
					//Quantize and store
					//quantization compensation

					PDX_CVT32D_2MX40(last8, first8, acc);	//Converting 40bit results to 32bit to prevent loss of accuracy from packing of 40bit -> 16bit
					if (pBiasBuffer)
					{
						first8 += vbias_l;
					}
					//first8
					quant_acc = mult_l * first8;	//Multiplying 2 32-bit vectors and storing result in 80bit vector
					//shift and round
					quant_acc = PDX_SLS_MX80(quant_acc,shift_l);//saturating left shift, right shift if negative
					//round and shift and saturate
					conv_out = PDX_PACKQSRV_MX80   (quant_acc, round_mode);	//pack 80bit result to 32bit with rounding and saturation.
					//add output zero point
					conv_out += vOutZP;
					//Saturate to 8 bit range output_activation_min to 127
					conv_out = PDX_MIN_MX32(conv_out,vmax);
					conv_out = PDX_MAX_MX32(conv_out,vmin);
					nPixToWrite = MIN(nPixLeft,PDX_M);
					PDX_SAV32_MX8_XP(conv_out, outa, outp, nPixToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
					PDX_SAPOS_MX8_FP(outa,outp);//flush
					nPixLeft -= nPixToWrite;

					if (nPixLeft>0)
					{
					if (pBiasBuffer)
					{
							last8 += vbias_h;
					}
						//last8
						quant_acc = mult_h * last8;	//Multiplying 2 32-bit vectors and storing result in 80bit vector
						//shift and round
						quant_acc = PDX_SLS_MX80(quant_acc,shift_h);//saturating left shift, right shift if negative
						//round and shift and saturate
						conv_out = PDX_PACKQSRV_MX80   (quant_acc, round_mode);	//pack 80bit result to 32bit with rounding and saturation.
						//add output zero point
						conv_out += vOutZP;
						//Saturate to 8 bit range output_activation_min to 127
						conv_out = PDX_MIN_MX32(conv_out,vmax);
						conv_out = PDX_MAX_MX32(conv_out,vmin);
						nPixToWrite = MIN(nPixLeft,PDX_M);
						PDX_SAV32_MX8_XP(conv_out, outa, outp, nPixToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
						PDX_SAPOS_MX8_FP(outa,outp);//flush
						nPixLeft -= nPixToWrite;
					}
				}
			}
		}
	}
}
