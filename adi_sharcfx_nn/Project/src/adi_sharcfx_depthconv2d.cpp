/**
********************************************************************************
*
* @file: adi_sharcfx_depthconv2d.cpp
*
* @brief: Contains optimized depthconv2d function
*
* @details: Contains optimized depthconv2d function
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

/*============= D A T A =============*/
//TODO: Make buffer dynamic
int8_t pTempLocal[TEMP_BUFFER_SIZE_L3]__attribute__((section(".L3.noload"), aligned(8)));

/*============= C O D E =============*/

/**
*******************************************************************************
* Function: adi_sharcfx_depthconv2d_int8
* @brief optimized depthconv2d function
*
* @details optimized depthconv2d function for 8-bit integer input. 2D depthwise-convolution in interleaved format using 16bit Eagle intrinsics. Also accounts for padding using a temp buffer
*
* Parameters:
* @param [in] pInputBuffer - input data
* @param [in] pWeightsBuffer - input weights buffer
* @param [in] pBiasBuffer - input bias buffer
* @param [in] nInputWidth - input width
* @param [in] nInputHeight - input height
* @param [in] nDepthMult - depth multiplier
* @param [in] nInChannels - input depth
* @param [in] nOutChannels - output depth
* @param [in] nKernelSizeWidth - kernel width
* @param [in] nKernelSizeHeight - kernel height
* @param [in] nTotalPaddingWidth - pad width
* @param [in] nTotalPaddingHeight - pad height
* @param [in] pQuantizedMultiplier - multiplier
* @param [in] pQuantizedShift - shift
* @param [in] pInZeroPoint - input zeropoint
* @param [in] pOutZeroPoint - output zeropoint
* @param [in] nStrideWidth - stride width
* @param [in] nStrideHeight - stride height
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
void adi_sharcfx_depthconv2d_int8(const int8_t *pInputBuffer,
                                     int8_t *pOutputBuffer,
                                     const int8_t *pWeightsBuffer,
                                     const int32_t *pBiasBuffer,
                                     int32_t nInputWidth,
                                     int32_t nInputHeight,
                                     int32_t nDepthMult,
                                     int32_t nInChannels,
                                     int32_t nOutChannels,
                                     int32_t nKernelSizeWidth,
                                     int32_t nKernelSizeHeight,
                                     int32_t nTotalPaddingWidth,
                                     int32_t nTotalPaddingHeight,
                                     int32_t *pQuantizedMultiplier,
                                     int32_t *pQuantizedShift,
                                     int32_t pInZeroPoint,
                                     int32_t pOutZeroPoint,
                                     int32_t nStrideWidth,
                                     int32_t nStrideHeight,
                                     int32_t nActMin,
                                     int32_t nActMax)
{
    //Peform an interleaved padding for rows and columns
    int i,nCol,nRow;
    int8_t *pPaddedBuffer;
	int8_t *pTempBufCopy;
	//chooses between the L1/L3 scratch buffer based on input/output dimensions.
	if(nOutChannels * nInputHeight * nInputWidth < TEMP_BUFFER_SIZE){
    	pTempBufCopy = (int8_t *)pTemp;
    }
    else
    {
    	pTempBufCopy = (int8_t *)pTempL3;
    }

	pPaddedBuffer = (int8_t *)pTempBufCopy;
    xb_vec4Mx8 vInZP = pInZeroPoint;

    //nDepthMult indicates whether the same set of input channels is used across all output weights
    //for eg 1 input channel with depth mult of 1 and 8 output channels uses same weights on 1 input channel
    //if depth mult is not 1, then num of input and output channels must match so diff weights on diff input channels in this code

    //this variable indicates how many channel padding has to be done in the interleaved format
    //if we are using depthmult = 1 it indicates we are operating on only 1 channel at a time so padding repeat = 1
    //else we have to do padding for each of the input/output channels

    int nNextPixelInc = 0;//same channel repeated for output channels
    int nPaddingChannels = 1;
    int nChannelIncrement = 1;
    int nChannelsProcesssedInLoop = MIN(2*PDX_M,nInChannels);

    //use different input channel for each output channel

    int8_t *pInpPtrTemp = pTempLocal;
    if(nInChannels == nOutChannels) {
    	//CASE depth multiplier = 1. Data does not need to be reformatted.
        nPaddingChannels = nInChannels;
        nNextPixelInc = MIN(2*PDX_M,nInChannels);
        nChannelIncrement = MIN(2*PDX_M,nInChannels);
        pInpPtrTemp = (int8_t*)pInputBuffer;
    }
    else
    {
    	//CASE depth multiplier != 1. Data does needs to be reformatted.
        //depth multiplier indicates whether the same set of input channels is used across all output weights
        nPaddingChannels = nInChannels*nDepthMult;
        nNextPixelInc = MIN(2*PDX_M,nInChannels*nDepthMult);
        nChannelIncrement = MIN(2*PDX_M,nInChannels*nDepthMult);
        nChannelsProcesssedInLoop = MIN(2*PDX_M,nInChannels*nDepthMult);

        //code to repeat input data to account for the required data format in cases where depth_mul param is not 1
        int8_t *pInpPtr = (int8_t *)pInputBuffer;
        pInpPtr = (int8_t *)pInputBuffer;
        for(int i=0; i< nInputHeight; i++){
        	for(int j=0; j< nInputWidth; j++){
    			for(int k=0; k< nInChannels; k++)
    			{
    					memset(pInpPtrTemp, *pInpPtr++, nDepthMult);
    					pInpPtrTemp += nDepthMult;
    			}
        	}
        }
        pInpPtrTemp = pTempLocal;
    }


    //This set of code handles odd padding cases
    //int nTotalPadding = nTotalPaddingWidth;
    int nInitialPaddingWidth = nTotalPaddingWidth/2;
    int nFinalPaddingWidth = (nTotalPaddingWidth + 1)/2 ;
    int nInitialPaddingHeight = nTotalPaddingHeight/2;
    int nFinalPaddingHeight = (nTotalPaddingHeight + 1)/2 ;

    // Padding for top for all initial padding rows
    for (i = 0; i < (nInputWidth + nTotalPaddingWidth)*nInitialPaddingHeight*nPaddingChannels; i++) {
        *pPaddedBuffer++ = -vInZP;
    }

    //num of bytes left in the column to process
    int nBytesLeft = (nInputWidth+nTotalPaddingWidth)*nPaddingChannels;

    //pad the remaining lines as well in an interleaved way 4*PDX_M at a time
    pInpPtrTemp -= nInitialPaddingWidth*nPaddingChannels;
    int nBytesNotPadded = 0;
    for (nCol = 0; nCol < (nInputWidth+nTotalPaddingWidth)*nPaddingChannels; nCol += 4*PDX_M) {

        xb_vec4Mx8 * inv = (xb_vec4Mx8 *)(pInpPtrTemp);  // to account for padding values
        //mask the first nInitialPadding * nPaddingRepeat number of cols
        vbool4M mask0 = PDX_MOVB_AU32 (0);

        //Initial padding
        if (nCol + 4*PDX_M < nInitialPaddingWidth*nPaddingChannels)  { //initial padding > 4*PDX_M
            //all 0s
            mask0 = PDX_MOVB_AU32(0);//all zero padding
        } else if (nCol < nInitialPaddingWidth*nPaddingChannels) {  // 0 < initial padding < 4*PDX_M
            //mix of 0s and valid pixels
            int nLeft = nCol + 4*PDX_M - nInitialPaddingWidth*nPaddingChannels; //num of non-zero elements
            uint32_t nShift = (((uint64_t)1)<<(nLeft)) - 1;
            nShift <<= 4*PDX_M - nLeft;//make lsb of mask = 0
            vbool4M m = PDX_MOVB_AU32 (nShift);
            mask0 |= m;
        } else {
            //no mask, all valid pixels
            mask0 |= PDX_MOVB_AU32(0xFFFFFFFF);
        }

        //1s denote where input needs to be read from buffer

        //Final padding
        nBytesNotPadded = (nCol + 4*PDX_M) - (nInputWidth + nInitialPaddingWidth)*nPaddingChannels - nBytesNotPadded;//check if we have finished with valid pixels
        if (nBytesNotPadded > 0) {
            int EndPadding = 4*PDX_M - nBytesNotPadded;//mask sets 1 to pixels not to be padded
            uint32_t nShift = (((uint64_t)1)<<(EndPadding)) - 1;
            vbool4M m = PDX_MOVB_AU32 (nShift);
            mask0 &= m;
        } else {
            nBytesNotPadded = 0;
        }

        //use the mask to copy the elements to temp buffer with padding
        int8_t *pOut = pPaddedBuffer;
        xb_vec4Mx8 in0;
        int nBytesWrite = MIN(nBytesLeft,4*PDX_M);

        //repeat the operation for all rows of the image
        for (nRow = 0; nRow < nInputHeight; nRow += 1) {
            valign ina = PDX_LA_4MX8_PP (inv);
            PDX_LA_4MX8_XP (in0, ina, inv, nInputWidth*nPaddingChannels);  // inc to next row
            in0 = PDX_MOV_4MX8_T (in0, -vInZP, mask0);  // Clear the first and/or last lane, if needed. set -zp so addition will zp will make it 0

            xb_vec4Mx8 * outv = (xb_vec4Mx8 *) pOut;
            valign outa = PDX_LA_4MX8_PP (outv);
            PDX_SAV_4MX8_XP (in0, outa, outv, nBytesWrite);//doing post increment bounded to 32 here
            PDX_SAPOS_4MX8_FP (outa, outv);
            pOut += (nInputWidth + nTotalPaddingWidth)*nPaddingChannels;
        }
        pInpPtrTemp += nBytesWrite;  // go to next set of columns
        pPaddedBuffer += nBytesWrite; // go to next column
        nBytesLeft -= nBytesWrite;
    }

	pPaddedBuffer = (int8_t *)(pTempBufCopy + (nInputWidth + nTotalPaddingWidth)*
                (nInputHeight + nInitialPaddingHeight)*nPaddingChannels);

    // Padding for bottom for all final padding rows
    for (i = 0; i < (nInputWidth + nTotalPaddingWidth)*nFinalPaddingHeight*nPaddingChannels; i++) {
        *pPaddedBuffer++ = -vInZP;
    }

    //Now run the depthwise convolution on the padded buffer
    xb_vec2Mx8 *inp;

    xb_vec2Mx40 acc;
    xb_vec2Mx16 vin,vwt;
    //calculate bias*wt
    xb_vec2Mx16 vInZP16 = pInZeroPoint;
    xb_vec2Mx16 vFilterZP = 0;
    int nBytesToWrite=0;
    xb_vecMx32 first8, last8;
    xb_vec2Mx40 outacc;
    xb_vecMx80 quant_acc;
    const immediate round_mode = ROUNDING_MODE;
    xb_vecMx32 mult_l,mult_h;
    xb_vecMx32 shift_l,shift_h;
    //xb_vec2Mx40 vbias;
    xb_vecMx32 vbias_l,vbias_h;
    xb_vecMx32 conv_out;
    xb_vecMx32 vmin = nActMin;
    xb_vecMx32 vmax = nActMax;
    xb_vecMx32 vOutZP = pOutZeroPoint;
    xb_vecMx8* __restrict outp  = (xb_vecMx8 *)pOutputBuffer;
    valign outa = PDX_LA_MX8_PP (outp); // prime, NOP if a[] is aligned
    int8_t *pIn;

    //Once padding is done, then run convolution
    //data is present in interleaved format - all channels for a pixel
    //we will do depthwise for all input channels at a time
    for(int nRow = 0;nRow + nKernelSizeHeight <= nInputHeight + nTotalPaddingHeight;nRow += nStrideHeight)
    {

        //align with row start

		 pIn = (int8_t *)(pTempBufCopy + (nInputWidth + nTotalPaddingWidth)*nRow*nPaddingChannels);  //pointer to move down the rows

        for(int nCol = 0;nCol + nKernelSizeWidth<= nInputWidth + nTotalPaddingWidth;nCol += nStrideWidth)
        {
            //perform operation for all output channels for 1 pixel

            //align with next col start
            int8_t *pInputIter = pIn;

            //perform on all inputs channels for 1 pixel first

            //if nDepthMult is 1 we have to run all the filters on same pixels
            //so we can only process 1 channel at a time
            //if this is not the case we can do upto 2*PDX_M input channel at a time
            int nRemainingChannels = nOutChannels;
            //read all inputs channels for a pixel upto 2*PDX_M
            for(int nChannel = 0;nChannel < nOutChannels;nChannel+=nChannelIncrement)
            {
                const int8_t *pCurrWts = pWeightsBuffer + nChannel; //Move to next filter for each output pixel
                int8_t *pInputCol = pInputIter;

                nBytesLeft = MIN(nChannelsProcesssedInLoop,nRemainingChannels);

                //read in mult factors and bias for the channels
                xb_vecMx32 *vMult = (xb_vecMx32 *)(pQuantizedMultiplier + nChannel);
                valign wMulta =  PDX_LA_MX32_PP(vMult);

                xb_vecMx32 *vShift = (xb_vecMx32 *)(pQuantizedShift + nChannel);
                valign wShifta =  PDX_LA_MX32_PP(vShift);

                xb_vecMx32 *vBias = (xb_vecMx32 *)(pBiasBuffer + nChannel);
                valign wBiasa =  PDX_LA_MX32_PP(vBias);

                //read 8 way 32 bit signed
                PDX_LA_MX32_XP (mult_l, wMulta, vMult, 4*PDX_M);
                PDX_LA_MX32_XP (mult_h, wMulta, vMult, 4*PDX_M);

                PDX_LA_MX32_XP (shift_l, wShifta, vShift, 4*PDX_M);
                PDX_LA_MX32_XP (shift_h, wShifta, vShift, 4*PDX_M);

                PDX_LA_MX32_XP (vbias_l, wBiasa, vBias, 4*PDX_M);
                PDX_LA_MX32_XP (vbias_h, wBiasa, vBias, 4*PDX_M);

                vbias_l = PDX_SLS_MX32(vbias_l,1);//*2 to match with acc
                vbias_h = PDX_SLS_MX32(vbias_h,1);//*2 to match with acc

                acc = 0;
                xb_vec2Mx8 *wtp = (xb_vec2Mx8 *)(pCurrWts); //Move to next filter for each output pixel
                valign wta = PDX_LA_2MX8_PP (wtp); //wt align vector

                //do depthwise on all these channels at once
                for(int nFilterHeight = 0;nFilterHeight < nKernelSizeHeight;nFilterHeight++)
                {
                    inp = (xb_vec2Mx8 *)(pInputCol);    //Reinitialise input pointer for each output pixel
                    valign ina = PDX_LA_2MX8_PP (inp); // align vector

                    for(int nFilterWidth = 0;nFilterWidth < nKernelSizeWidth;nFilterWidth++)
                    {
                        //reading 16 way 8 bit inputs and weights
                        PDX_LA16_2MX8_XP (vin, ina, inp, nPaddingChannels);//read 2*PDX_M number of channels for 1 pixel, skip to adjoining pixel
                        vin += vInZP16;        //Add input offset
                        ina = PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

                        PDX_LA16_2MX8_XP (vwt, wta, wtp, nOutChannels);//read 2*PDX_M number of channels for 1 pixel
                        vwt += vFilterZP;    //Add filter offset
                        wta = PDX_LA_2MX8_PP (wtp); // prime, NOP if a[] is aligned

                        //multiply and accumulate
                        PDX_MULAQW_2MX16(acc,vin,vwt);//acc contains upto 2*PDX_M channel results for pixel
                    }
                    //set to next row
                    pInputCol += (nInputWidth + nTotalPaddingWidth)*nPaddingChannels;
                    //pCurrWts += nKernelSizeWidth*nOutChannels;
                }

                //depth conv done now do scaling
                //perform scaling and addition

                //quantization compensation
                PDX_CVT32D_2MX40(last8, first8, acc);    //Converting 40bit results to 32bit to prevent loss of accuracy from packing of 40bit -> 16bit
                first8 += vbias_l;

                //first8
                quant_acc = mult_l * first8;    //Multiplying 2 32-bit vectors and storing result in 80bit vector
                //shift and round
                quant_acc = PDX_SLS_MX80(quant_acc,shift_l);//saturating left shift, right shift if negative
                //round and shift and saturate
                conv_out = PDX_PACKQSRV_MX80   (quant_acc, round_mode);    //pack 80bit result to 32bit with rounding and saturation.
                //add output zero point
                conv_out += vOutZP;
                //Saturate to 8 bit range output_activation_min to 127
                conv_out = PDX_MIN_MX32(conv_out,vmax);
                conv_out = PDX_MAX_MX32(conv_out,vmin);

                nBytesToWrite = nBytesLeft - PDX_M > 0 ? PDX_M : nBytesLeft;
                nBytesLeft-=PDX_M;

                PDX_SAV32_MX8_XP(conv_out, outa, outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                PDX_SAPOS_MX8_FP(outa,outp);//flush

                if(nBytesLeft>0){
                    //last8
                    last8 += vbias_h;

                    //Multiplying 2 32-bit vectors and storing result in 80bit vector
                    quant_acc = mult_h * last8;
                    //shift and round
                    quant_acc = PDX_SLS_MX80(quant_acc,shift_h);//saturating left shift, right shift if negative
                    //round and shift and saturate
                    conv_out = PDX_PACKQSRV_MX80   (quant_acc, round_mode);    //pack 80bit result to 32bit with rounding and saturation.
                    //add output zero point
                    conv_out += vOutZP;
                    //Saturate to 8 bit range output_activation_min to 127
                    conv_out = PDX_MIN_MX32(conv_out,vmax);
                    conv_out = PDX_MAX_MX32(conv_out,vmin);

                    nBytesToWrite = nBytesLeft - PDX_M > 0 ? PDX_M : nBytesLeft;
                    nBytesLeft-=PDX_M;
                    PDX_SAV32_MX8_XP(conv_out, outa, outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                    PDX_SAPOS_MX8_FP(outa,outp);//flush
                }
                nRemainingChannels -= nChannelIncrement;
                pInputIter += nNextPixelInc;//process next channel
            }
            pIn += nPaddingChannels*nStrideWidth;//go to next pixel
        }
    }
}

/**
*******************************************************************************
* Function: adi_sharcfx_depthconv2d_stride1_noninterleaved_int8
* @brief optimized depthconv2d function
*
* @details  2D depthwise-convolution with stride of 1, takes data in non-interleaved format using 16bit Eagle intrinsics. Also accounts for padding
*
* Parameters:
* @param [in] pInputBuffer - input data
* @param [in] pWeightsBuffer - input weights buffer
* @param [in] pBiasBuffer - input bias buffer
* @param [in] nInputWidth - input width
* @param [in] nDepthMult - depth multiplier
* @param [in] nInChannels - input depth
* @param [in] nOutChannels - output depth
* @param [in] nKernelSize - kernel size
* @param [in] nTotalPadding - padding length
* @param [in] pQuantizedMultiplier - multiplier
* @param [in] pQuantizedShift - shift
* @param [in] pInZeroPoint - input zeropoint
* @param [in] pOutZeroPoint - output zeropoint
* 
* @param [out] pOutputBuffer - output data
*
* @return None
*
*
*******************************************************************************
*/ 
void adi_sharcfx_depthconv2d_stride1_noninterleaved_int8(const int8_t *pInputBuffer,
                                                                         int8_t *pOutputBuffer,
                                                                         const int8_t *pWeightsBuffer,
                                                                         const int32_t *pBiasBuffer,
                                                                         int32_t nInputWidth,
                                                                         int32_t nDepthMult,
                                                                         int32_t nInChannels,
                                                                         int32_t nOutChannels,
                                                                         int8_t nKernelSize,
                                                                         int8_t nTotalPadding,
                                                                         uint32_t *pQuantizedMultiplier,
                                                                         int32_t *pQuantizedShift,
                                                                         int32_t pInZeroPoint,
                                                                         int32_t pOutZeroPoint)
{
    int8_t nStrideLen = 1;
    int8_t nOutputBufferWidth = (int8_t)(nInputWidth - nKernelSize + nTotalPadding)/nStrideLen + 1;

    const immediate Lane = 0;
    const immediate round_mode = ROUNDING_MODE;


    xb_vec4Mx8 vInZP = pInZeroPoint;//PDX_REP_4MX8((xb_vec4Mx8)pInZeroPoint,Lane);//Replicates the lane of data specified, across all lanes of a vector register

#ifdef USE_EXTRA_PRECISION
    xb_vecMx8* __restrict outp  = (xb_vecMx8 *)pOutputBuffer;
    xb_vecMx32  vOutZP = pOutZeroPoint;
    //load constants for range
    xb_vecMx32 vmin = ACT_MIN;
    xb_vecMx32 vmax = ACT_MAX;
    xb_vec2Mx40 vmax32bit = (xb_vec2Mx40)INT_32BIT_MAX;//Replicates the lane of data specified, across all lanes of a vector register
    xb_vec2Mx40 vmin32bit = (xb_vec2Mx40)INT_32BIT_MIN;//Replicates the lane of data specified, across all lanes of a vector register
    vbool2M greater_than32bit, lesser_than32bit;
    xb_vecMx32 first8, last8;
    valign outa = PDX_LA_MX8_PP (outp); // prime, NOP if a[] is aligned
#else
    xb_vec2Mx8* __restrict outp = (xb_vec2Mx8 *) pOutputBuffer;//outbuffer

    xb_vec2Mx16 vOutZP = pOutZeroPoint;//PDX_REP_2MX40((xb_vec2Mx16)pOutZeroPoint,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    //load constants for range
    xb_vec2Mx16 vmin = -128;//PDX_REP_2MX16((xb_vec2Mx16)-128,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    xb_vec2Mx16 vmax = 127;//PDX_REP_2MX16((xb_vec2Mx16)127,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    xb_vec2Mx16 conv_out, first16, last16;
    xb_vec4Mx20 vsum;
    xb_vec4Mx20 vmax16bit = (xb_vec4Mx20)0x07FFF;//PDX_REP_4MX20((xb_vec4Mx20)0x07FFF,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    xb_vec4Mx20 vmin16bit = (xb_vec4Mx20)0xF8000;//PDX_REP_4MX20((xb_vec4Mx20)0xF8000,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    vbool4M greater_than16bit, lesser_than16bit;
    valign outa = PDX_Z_ALIGN(); // initialize the alignment register vaWrite to zero
#endif


    int col = 0;
    xb_vec4Mx20 acc,tempacc1;
    xb_vec2Mx40 outacc;
    int nBytesToWrite;
    int nRemCols;
    xb_vec4Mx20 zeros = 0;

    for(int nChannels = 0;nChannels < nOutChannels;nChannels++)
    {
#ifdef USE_EXTRA_PRECISION
        xb_vecMx32 mult_acc_40 = pQuantizedMultiplier[nChannels];
        xb_vecMx80 outacc;

        //Load pQuantizedShift for channel
        xb_vecMx32 shift = pQuantizedShift[nChannels];
#else
        uint16_t ReducedShift = (uint16_t)((pQuantizedMultiplier[nChannels] + (1 << 15)) >> 16);//Qmult is  (val) << 31, reduce this by 16 here to fit in int16
        //balance Qmult shift remaining is 15 but we include the +1 shift for PDX_MULAQW_4MX8 too here
        //this shift of 16 is accounted for during PDX_PACKQSRV_2MX40
        xb_vec2Mx16 mult_acc = PDX_REP_2MX16(ReducedShift,Lane);            //Replicates the lane of data specified, across all lanes of a vector register

        const immediate nTotalShift = pQuantizedShift[nChannels];            //Load shift factor for quantization per channel
        xb_vec2Mx40 shift = PDX_REP_2MX40((xb_vec2Mx40)nTotalShift,Lane);    //Replicates the lane of data specified, across all lanes of a vector register

        //bias specific to output channel
        xb_vec2Mx40 vbias = PDX_REP_2MX40((xb_vec2Mx40)((int64_t)((int64_t)pBiasBuffer[nChannels]<<2)*(ReducedShift)),Lane);
        xb_vec2Mx40 outacc;
#endif

        //saturate bias within 20 bits
        //load bias*2 in nvec to match with vwt*vin*2
        //add zp * wt to bias
        int32_t zp_wt_sum;
        int32_t zp_wt_sum_first_pixel;
        int32_t zp_wt_sum_last_pixel;

        int32_t zp_wt_sum_first_row;
        int32_t zp_wt_sum_first_row_first_pixel;
        int32_t zp_wt_sum_first_row_last_pixel;

        int32_t zp_wt_sum_last_row;
        int32_t zp_wt_sum_last_row_first_pixel;
        int32_t zp_wt_sum_last_row_last_pixel;

        int16_t wt_sum = 0;
        int16_t wt_sum_first_pixel = 0;
        int16_t wt_sum_last_pixel = 0;
        int16_t wt_sum_first_row_first_pixel = 0;
        int16_t wt_sum_first_row = 0;
        int16_t wt_sum_first_row_last_pixel = 0;
        int16_t wt_sum_last_row_first_pixel = 0;
        int16_t wt_sum_last_row = 0;
        int16_t wt_sum_last_row_last_pixel = 0;

        //initialize weights for this channel
        //loading each of the (nKernelSize x nKernelSize) weights into it's own vector
        //each kernel weight is replicated in all the lanes to be multiplied in parallel
        int8_t nCurrWt;
        int8_t *filter_buff =  (int8_t *)(pWeightsBuffer + nChannels);

        nCurrWt = (int8_t)(*filter_buff);
        filter_buff += nInChannels;
        wt_sum += nCurrWt;
        wt_sum_last_pixel += nCurrWt;
        wt_sum_last_row += nCurrWt;
        wt_sum_last_row_last_pixel += nCurrWt;
        xb_vec4Mx8 w00 = PDX_REP_4MX8((xb_vec4Mx8)nCurrWt,Lane);//load current weight and replicate the same in all lanes of the vector

        nCurrWt = (int8_t)(*filter_buff);
        filter_buff += nInChannels;
        wt_sum += nCurrWt;
        wt_sum_first_pixel += nCurrWt;
        wt_sum_last_pixel += nCurrWt;
        wt_sum_last_row_first_pixel += nCurrWt;
        wt_sum_last_row += nCurrWt;
        wt_sum_last_row_last_pixel += nCurrWt;
        xb_vec4Mx8 w01 = PDX_REP_4MX8((xb_vec4Mx8)nCurrWt,Lane);//load current weight and replicate the same in all lanes of the vector

        nCurrWt = (int8_t)(*filter_buff);
        filter_buff += nInChannels;
        wt_sum += nCurrWt;
        wt_sum_first_pixel += nCurrWt;
        wt_sum_last_row_first_pixel += nCurrWt;
        wt_sum_last_row += nCurrWt;
        xb_vec4Mx8 w02 = PDX_REP_4MX8((xb_vec4Mx8)nCurrWt,Lane);//load current weight and replicate the same in all lanes of the vector

        //handle first col padding using weights w10,w20
        nCurrWt = (int8_t)(*filter_buff);
        filter_buff += nInChannels;
        wt_sum += nCurrWt;
        wt_sum_last_pixel += nCurrWt;
        wt_sum_first_row += nCurrWt;
        wt_sum_first_row_last_pixel += nCurrWt;
        wt_sum_last_row += nCurrWt;
        wt_sum_last_row_last_pixel += nCurrWt;
        xb_vec4Mx8 w10 = PDX_REP_4MX8((xb_vec4Mx8)nCurrWt,Lane);//load current weight and replicate the same in all lanes of the vector

        nCurrWt = (int8_t)(*filter_buff);
        filter_buff += nInChannels;
        wt_sum += nCurrWt;
        wt_sum_first_pixel += nCurrWt;
        wt_sum_last_pixel += nCurrWt;
        wt_sum_first_row_first_pixel += nCurrWt;
        wt_sum_first_row += nCurrWt;
        wt_sum_first_row_last_pixel += nCurrWt;
        wt_sum_last_row_first_pixel += nCurrWt;
        wt_sum_last_row += nCurrWt;
        wt_sum_last_row_last_pixel += nCurrWt;
        xb_vec4Mx8 w11 = PDX_REP_4MX8((xb_vec4Mx8)nCurrWt,Lane);//load current weight and replicate the same in all lanes of the vector

        nCurrWt = (int8_t)(*filter_buff);
        filter_buff += nInChannels;
        wt_sum += nCurrWt;
        wt_sum_first_pixel += nCurrWt;
        wt_sum_first_row_first_pixel += nCurrWt;
        wt_sum_first_row += nCurrWt;
        wt_sum_last_row_first_pixel += nCurrWt;
        wt_sum_last_row += nCurrWt;
        xb_vec4Mx8 w12 = PDX_REP_4MX8((xb_vec4Mx8)nCurrWt,Lane);//load current weight and replicate the same in all lanes of the vector

        nCurrWt = (int8_t)(*filter_buff);
        filter_buff += nInChannels;
        wt_sum += nCurrWt;
        wt_sum_last_pixel += nCurrWt;
        wt_sum_first_row += nCurrWt;
        wt_sum_first_row_last_pixel += nCurrWt;
        xb_vec4Mx8 w20 = PDX_REP_4MX8((xb_vec4Mx8)nCurrWt,Lane);//load current weight and replicate the same in all lanes of the vector

        nCurrWt = (int8_t)(*filter_buff);
        filter_buff += nInChannels;
        wt_sum += nCurrWt;
        wt_sum_first_pixel += nCurrWt;
        wt_sum_last_pixel += nCurrWt;
        wt_sum_first_row_first_pixel += nCurrWt;
        wt_sum_first_row += nCurrWt;
        wt_sum_first_row_last_pixel += nCurrWt;
        xb_vec4Mx8 w21 = PDX_REP_4MX8((xb_vec4Mx8)nCurrWt,Lane);//load current weight and replicate the same in all lanes of the vector

        nCurrWt = (int8_t)(*filter_buff);
        filter_buff += nInChannels;
        wt_sum += nCurrWt;
        wt_sum_first_pixel += nCurrWt;
        wt_sum_first_row_first_pixel += nCurrWt;
        wt_sum_first_row += nCurrWt;
        xb_vec4Mx8 w22 = PDX_REP_4MX8((xb_vec4Mx8)nCurrWt,Lane);//load current weight and replicate the same in all lanes of the vector


        //PDX_MULAQW_2MX16 will add a *2, so shift this sum also by 2
        zp_wt_sum = (pInZeroPoint) * wt_sum;//8 bit * 16 bit should fit within 32 bit

        zp_wt_sum_first_pixel = (pInZeroPoint) * wt_sum_first_pixel;//8 bit * 16 bit should fit within 32 bit
        zp_wt_sum_last_pixel = (pInZeroPoint) * wt_sum_last_pixel;//8 bit * 16 bit should fit within 32 bit

        zp_wt_sum_first_row = (pInZeroPoint) * wt_sum_first_row;//8 bit * 16 bit should fit within 32 bit
        zp_wt_sum_first_row_first_pixel = (pInZeroPoint) * wt_sum_first_row_first_pixel;//8 bit * 16 bit should fit within 32 bit
        zp_wt_sum_first_row_last_pixel = (pInZeroPoint) * wt_sum_first_row_last_pixel;//8 bit * 16 bit should fit within 32 bit

        zp_wt_sum_last_row = (pInZeroPoint) * wt_sum_last_row;//8 bit * 16 bit should fit within 32 bit
        zp_wt_sum_last_row_first_pixel = (pInZeroPoint) * wt_sum_last_row_first_pixel;//8 bit * 16 bit should fit within 32 bit
        zp_wt_sum_last_row_last_pixel = (pInZeroPoint) * wt_sum_last_row_last_pixel;//8 bit * 16 bit should fit within 32 bit

#ifdef USE_EXTRA_PRECISION
        zp_wt_sum += pBiasBuffer[nChannels];//bias specific to output channel
        zp_wt_sum <<= 1;

        zp_wt_sum_first_pixel += pBiasBuffer[nChannels];//bias specific to output channel
        zp_wt_sum_first_pixel <<= 1;
        zp_wt_sum_last_pixel += pBiasBuffer[nChannels];//bias specific to output channel
        zp_wt_sum_last_pixel <<= 1;

        zp_wt_sum_first_row += pBiasBuffer[nChannels];//bias specific to output channel
        zp_wt_sum_first_row <<= 1;

        zp_wt_sum_first_row_first_pixel += pBiasBuffer[nChannels];//bias specific to output channel
        zp_wt_sum_first_row_first_pixel <<= 1;

        zp_wt_sum_first_row_last_pixel += pBiasBuffer[nChannels];//bias specific to output channel
        zp_wt_sum_first_row_last_pixel <<= 1;

        zp_wt_sum_last_row += pBiasBuffer[nChannels];//bias specific to output channel
        zp_wt_sum_last_row <<= 1;

        zp_wt_sum_last_row_first_pixel += pBiasBuffer[nChannels];//bias specific to output channel
        zp_wt_sum_last_row_first_pixel <<= 1;

        zp_wt_sum_last_row_last_pixel += pBiasBuffer[nChannels];//bias specific to output channel
        zp_wt_sum_last_row_last_pixel <<= 1;

        xb_vecMx80 add_acc_80 = (xb_vecMx32)pQuantizedMultiplier[nChannels] * (xb_vecMx32)zp_wt_sum;
        xb_vecMx80 add_acc_80_first_pixel = (xb_vecMx32)pQuantizedMultiplier[nChannels] * (xb_vecMx32)zp_wt_sum_first_pixel;
        xb_vecMx80 add_acc_80_last_pixel = (xb_vecMx32)pQuantizedMultiplier[nChannels] * (xb_vecMx32)zp_wt_sum_last_pixel;

        xb_vecMx80 add_acc_80_first_row = (xb_vecMx32)pQuantizedMultiplier[nChannels] * (xb_vecMx32)zp_wt_sum_first_row;
        xb_vecMx80 add_acc_80_first_row_first_pixel = (xb_vecMx32)pQuantizedMultiplier[nChannels] * (xb_vecMx32)zp_wt_sum_first_row_first_pixel;
        xb_vecMx80 add_acc_80_first_row_last_pixel = (xb_vecMx32)pQuantizedMultiplier[nChannels] * (xb_vecMx32)zp_wt_sum_first_row_last_pixel;

        xb_vecMx80 add_acc_80_last_row = (xb_vecMx32)pQuantizedMultiplier[nChannels] * (xb_vecMx32)zp_wt_sum_last_row;
        xb_vecMx80 add_acc_80_last_row_first_pixel = (xb_vecMx32)pQuantizedMultiplier[nChannels] * (xb_vecMx32)zp_wt_sum_last_row_first_pixel;
        xb_vecMx80 add_acc_80_last_row_last_pixel = (xb_vecMx32)pQuantizedMultiplier[nChannels] * (xb_vecMx32)zp_wt_sum_last_row_last_pixel;
#else
        vsum = max((xb_vec4Mx20)(zp_wt_sum << 1), (xb_vec4Mx20)0xFFFFF);
        vsum = min(vsum, (xb_vec4Mx20)0x7FFFF);

        int64_t vconst_add = (int32_t)ReducedShift * (int32_t)pBiasBuffer[nChannels];
        vconst_add <<= 1;
        //saturate to fit within 40bits
        vconst_add = MAX(vconst_add,(int64_t)0xFFFFFF8000000000);
        vconst_add = MIN(vconst_add,(int64_t)0x7FFFFFFFFF);
        xb_vec2Mx40 add_acc = (int32_t)vconst_add;
#endif

        //create first row mask
        vbool4M mask0 = PDX_MOVB_AU32 (0xFFFFFFFE);                    //setting mask to account for initial padding
        vbool4M nomask = PDX_MOVB_AU32 (0xFFFFFFFF);                    //setting mask to account for initial padding

        //top row with first row padded, for 3x3 1 first col and 1 last col

        xb_vec4Mx8 * inp = (xb_vec4Mx8 *) (pInputBuffer - 1);  //pointer to move down the rows, -1 to simulate padding
        valign ina = PDX_LA_4MX8_PP (inp);

        xb_vec4Mx8 in00,in01,in02;//load input and increment input pointer to next location,w01,w02 not used
        xb_vec4Mx8 in10,in11,in12;
        xb_vec4Mx8 in20,in21,in22;

        xb_vec4Mx8 temp1, temp2;

        int nBytesLeft = nOutputBufferWidth;
        //First row we only process the bottom 2 rows assuming top row is padded

        for(int nCol = 0;nCol < nInputWidth;nCol += 4*PDX_M)
        {
            nRemCols = MIN(nBytesLeft - 1,4*PDX_M);
            uint32_t nShift = (((uint64_t)1)<<(nRemCols)) - 1;//If we have 16 elements remaining padding is needed on the last 1 hence nRemCols - 1
            vbool4M end_padding_mask = PDX_MOVB_AU32 (nShift);

            ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
            PDX_LA_4MX8_XP(in10, ina, inp, 1); //load input and increment input pointer to next location
            //in10 += vInZP;                         //add input zero point to each input vector

            ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
            PDX_LA_4MX8_XP (in11, ina, inp, 1); //load input and increment input pointer to next location
            //in11 += vInZP;                         //add input zero point to each input vector

            ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
            PDX_LA_4MX8_XP (in12, ina, inp, nInputWidth - 2);  //load input and increment input pointer to next row (here)
            //in12 += vInZP;                         //add input zero point to each input vector

            ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
            PDX_LA_4MX8_XP (in20, ina, inp, 1); //load input and increment input pointer to next location
            //in20 += vInZP;                         //add input zero point to each input vector

            ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
            PDX_LA_4MX8_XP (in21, ina, inp, 1); //load input and increment input pointer to next location
            //in21 += vInZP;                         //add input zero point to each input vector

            ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
            PDX_LA_4MX8_XP (in22, ina, inp, -(nInputWidth + 2) + 4*PDX_M);  //load input and increment input pointer back to beginning
            //in22 += vInZP;                         //add input zero point to each input vector

            //multiply and accumulate
            tempacc1 = w10 * in10;                    //Multiply  - 16way 16bit
            tempacc1 <<= 1;
            PDX_MULAQW_4MX8(tempacc1,w20,in20);                    //Multiply and Accumulate - 16way 16bit
            //if bool is TRUE, first op is selected or else second op is selected
            acc = PDX_MOV_4MX20_T(tempacc1,zeros,mask0);//simulate padding

            //acc += vsum;//Load bias into acc
            PDX_MULAQW_4MX8(acc,w11,in11);                    //Multiply and Accumulate - 16way 16bit
            PDX_MULAQW_4MX8(acc,w21,in21);                    //Multiply and Accumulate - 16way 16bit

            PDX_MULAQW_4MX8_T(acc,w12,in12,end_padding_mask);                    //Multiply and Accumulate - 16way 16bit
            PDX_MULAQW_4MX8_T(acc,w22,in22,end_padding_mask);                    //Multiply and Accumulate - 16way 16bit

#ifdef USE_EXTRA_PRECISION
            //create mask based on elements left
            //convert 32 8 bit to 32 32bit data
            //map higher bits in H to 3 and 4
            //map lower bits in L to 1 and 2
            vbool2M end_padding_mask34 = PDX_CVTBB2M_B_H(end_padding_mask);
            vboolM end_padding_mask4 = PDX_CVTBBM_B2M_H(end_padding_mask34);
            vboolM end_padding_mask3 = PDX_CVTBBM_B2M_L(end_padding_mask34);
            vbool2M end_padding_mask12 = PDX_CVTBB2M_B_L(end_padding_mask);
            vboolM end_padding_mask2 = PDX_CVTBBM_B2M_H(end_padding_mask12);
            vboolM end_padding_mask1 = PDX_CVTBBM_B2M_L(end_padding_mask12);

            //start padding mask
            vbool2M start_padding_mask34 = PDX_CVTBB2M_B_H(mask0);
            vboolM start_padding_mask4 = PDX_CVTBBM_B2M_H(start_padding_mask34);
            vboolM start_padding_mask3 = PDX_CVTBBM_B2M_L(start_padding_mask34);
            vbool2M start_padding_mask12 = PDX_CVTBB2M_B_L(mask0);
            vboolM start_padding_mask2 = PDX_CVTBBM_B2M_H(start_padding_mask12);
            vboolM start_padding_mask1 = PDX_CVTBBM_B2M_L(start_padding_mask12);


            //32 20 bit -> 8 80 bits
            //convert 32 20 bit acc to 8 32 bit acc.
            //convert L half of acc to 2 32 bit acc
            PDX_CVT32D_4MX20_L(last8,first8,acc);

            //keep 32 bit and do 80 bit multiplication

            //first8
            outacc = mult_acc_40 * first8;//8-way 32bit mac

            xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80_first_row,add_acc_80_first_row_first_pixel,start_padding_mask1);
            acc_proc = PDX_MOV_MX80_T(acc_proc,add_acc_80_first_row_last_pixel,end_padding_mask1);
            outacc += acc_proc;


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
            PDX_SAV32_MX8_XP(conv_out, outa,outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
            //to 8-way 8-bit signed, forward post-increment addressing mode
            PDX_SAPOS_MX8_FP(outa,outp);//flush

            //last8
            if(nBytesLeft>0){
                //first8
                outacc = mult_acc_40 * last8;//8-way 32bit mac

                xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80_first_row,add_acc_80_first_row_first_pixel,start_padding_mask2);
                acc_proc = PDX_MOV_MX80_T(acc_proc,add_acc_80_first_row_last_pixel,end_padding_mask2);
                outacc += acc_proc;


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
                PDX_SAV32_MX8_XP(conv_out, outa, outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 8-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_MX8_FP(outa,outp);//flush
            }


            if(nBytesLeft>0){
                PDX_CVT32D_4MX20_H(last8,first8,acc);

                //keep 32 bit and do 80 bit multiplication

                //first8
                outacc = mult_acc_40 * first8;//8-way 32bit mac

                xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80_first_row,add_acc_80_first_row_first_pixel,start_padding_mask3);
                acc_proc = PDX_MOV_MX80_T(acc_proc,add_acc_80_first_row_last_pixel,end_padding_mask3);
                outacc += acc_proc;

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
                nBytesLeft -= nBytesToWrite;
                PDX_SAV32_MX8_XP(conv_out, outa,outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 8-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_MX8_FP(outa,outp);//flush

                //last8
                if(nBytesLeft>0){
                    //first8
                    outacc = mult_acc_40 * last8;//8-way 32bit mac

                    //where start padding mask = 0, use add_acc_80_first_row_first_pixel
                    xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80_first_row,add_acc_80_first_row_first_pixel,start_padding_mask4);//simulate padding
                    //where end padding mask = 0 use add_acc_80_first_row_last_pixel
                    acc_proc = PDX_MOV_MX80_T(acc_proc,add_acc_80_first_row_last_pixel,end_padding_mask4);//simulate padding
                    outacc += acc_proc;

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
                    nBytesLeft -= nBytesToWrite;
                    PDX_SAV32_MX8_XP(conv_out, outa, outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                    //to 8-way 8-bit signed, forward post-increment addressing mode
                    PDX_SAPOS_MX8_FP(outa,outp);//flush
                }
            }
#else
            //account for 1st row padding
            //Quantization Compensation
            //Saturate each lane in acc before converting to 16bit
            //sets to true if first op greater than second op
            greater_than16bit =  PDX_GT_4MX20(acc, vmax16bit);//Get bool reg for registers greater than 16bit value
            //sets to true if first op less than second op
            lesser_than16bit =  PDX_LT_4MX20(acc, vmin16bit);//Get bool reg for registers greater than 16bit value
            //if bool is TRUE, first op is selected or else second op is selected
            acc = PDX_MOV_4MX20_T(vmax16bit, acc,greater_than16bit);
            acc = PDX_MOV_4MX20_T(vmin16bit, acc, lesser_than16bit);
            //Separate 32way 20bit data into 2 16way 16bit registers and perform quantization comp separately
            PDX_CVT16D_4MX20(last16, first16,acc);

            //first16
            outacc = mult_acc * first16;//PDX_MULMNW_2MX16(mult_acc,first16,0,0);
            outacc += add_acc;
            //shift and round
            outacc = PDX_SLS_2MX40(outacc,shift);//saturating left shift, right shift if negative
            //round and shift and saturate
            conv_out = PDX_PACKQSRV_2MX40  (outacc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
            //add output zero point
            conv_out += vOutZP;
            //Saturate to 8 bit range output_activation_min to 127
            conv_out = PDX_MIN_2MX16(conv_out,vmax);
            conv_out = PDX_MAX_2MX16(conv_out,vmin);
            nBytesToWrite = nBytesLeft - 2*PDX_M > 0 ? 2*PDX_M : nBytesLeft;
            PDX_SAV16_2MX8_XP(conv_out, outa, outp,nBytesToWrite);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
            //to 16-way 8-bit signed, forward post-increment addressing mode
            PDX_SAPOS_2MX8_FP(outa,outp);//flush
            nBytesLeft -= nBytesToWrite;

            //last16
            outacc = mult_acc * last16;//PDX_MULMNW_2MX16(mult_acc,last16,0,0);
            outacc += add_acc;
            //shift and round
            outacc = PDX_SLS_2MX40(outacc,shift);//saturating left shift, right shift if negative
            //round and shift and saturate
            conv_out = PDX_PACKQSRV_2MX40  (outacc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
            //add output zero point
            conv_out += vOutZP;
            //Saturate to 8 bit range output_activation_min to 127
            conv_out = PDX_MIN_2MX16(conv_out,vmax);
            conv_out = PDX_MAX_2MX16(conv_out,vmin);
            nBytesToWrite = nBytesLeft - 2*PDX_M > 0 ? 2*PDX_M : nBytesLeft;
            PDX_SAV16_2MX8_XP(conv_out, outa, outp,nBytesToWrite);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
            //to 16-way 8-bit signed, forward post-increment addressing mode
            PDX_SAPOS_2MX8_FP(outa,outp);//flush
            nBytesLeft -= nBytesToWrite;
#endif
            mask0 = nomask;
        }

        for(int nRow = 0;nRow < nInputWidth - 2;nRow++)
        {
            inp = (xb_vec4Mx8 *) (pInputBuffer + nRow*nInputWidth - 1);  //pointer to move down the rows, -1 for padding
            //Processing for remaining rows
            nBytesLeft = nOutputBufferWidth;
            mask0 = PDX_MOVB_AU32 (0xFFFFFFFE);                    //setting mask to account for initial padding
            for(int nCol = 0;nCol < nInputWidth;nCol += 4*PDX_M)
            {
                nRemCols = MIN(nBytesLeft - 1,4*PDX_M);
                uint32_t nShift = (((uint64_t)1)<<(nRemCols)) - 1;//if we have 16 el remaining, 15 dont need padding
                vbool4M end_padding_mask = PDX_MOVB_AU32 (nShift);

                ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
                PDX_LA_4MX8_XP(in00, ina, inp, 1); //load input and increment input pointer to next location
                //in00 += vInZP;                         //add input zero point to each input vector

                ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
                PDX_LA_4MX8_XP (in01, ina, inp, 1); //load input and increment input pointer to next location
                //in01 += vInZP;                         //add input zero point to each input vector

                ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
                PDX_LA_4MX8_XP (in02, ina, inp, nInputWidth - 2);  //load input and increment input pointer to next row (here)
                //in02 += vInZP;                         //add input zero point to each input vector

                ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
                PDX_LA_4MX8_XP(in10, ina, inp, 1); //load input and increment input pointer to next location
                //in10 += vInZP;                         //add input zero point to each input vector

                ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
                PDX_LA_4MX8_XP (in11, ina, inp, 1); //load input and increment input pointer to next location
                //in11 += vInZP;                         //add input zero point to each input vector

                ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
                PDX_LA_4MX8_XP (in12, ina, inp, nInputWidth - 2);  //load input and increment input pointer to next row (here)
                //in12 += vInZP;                         //add input zero point to each input vector

                ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
                PDX_LA_4MX8_XP (in20, ina, inp, 1); //load input and increment input pointer to next location
                //in20 += vInZP;                         //add input zero point to each input vector

                ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
                PDX_LA_4MX8_XP (in21, ina, inp, 1); //load input and increment input pointer to next location
                //in21 += vInZP;                         //add input zero point to each input vector

                ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
                PDX_LA_4MX8_XP (in22, ina, inp, -2*nInputWidth - 2 + 4*PDX_M);  //load input and increment input pointer back to beginning
                //in22 += vInZP;                         //add input zero point to each input vector

                //multiply and accumulate
                tempacc1 = w00 * in00;                    //Multiply  - 16way 16bit
                tempacc1 <<= 1;
                PDX_MULAQW_4MX8(tempacc1,w10,in10);                    //Multiply and Accumulate - 16way 16bit
                PDX_MULAQW_4MX8(tempacc1,w20,in20);                    //Multiply and Accumulate - 16way 16bit

                //if bool is TRUE, first op is selected or else second op is selected
                acc = PDX_MOV_4MX20_T(tempacc1,zeros,mask0);//simulate padding

                //acc = acc + vsum;
                PDX_MULAQW_4MX8(acc,w01,in01);                    //Multiply and Accumulate - 16way 16bit
                PDX_MULAQW_4MX8(acc,w11,in11);                    //Multiply and Accumulate - 16way 16bit
                PDX_MULAQW_4MX8(acc,w21,in21);                    //Multiply and Accumulate - 16way 16bit

                PDX_MULAQW_4MX8_T(acc,w02,in02,end_padding_mask);                    //Multiply and Accumulate - 16way 16bit
                PDX_MULAQW_4MX8_T(acc,w12,in12,end_padding_mask);                    //Multiply and Accumulate - 16way 16bit
                PDX_MULAQW_4MX8_T(acc,w22,in22,end_padding_mask);                    //Multiply and Accumulate - 16way 16bit
#ifdef USE_EXTRA_PRECISION
                //32 20 bit -> 8 80 bits
                //create mask based on elements left
                //convert 32 8 bit to 32 32bit data
                //map higher bits in H to 3 and 4
                //map lower bits in L to 1 and 2
                vbool2M end_padding_mask34 = PDX_CVTBB2M_B_H(end_padding_mask);
                vboolM end_padding_mask4 = PDX_CVTBBM_B2M_H(end_padding_mask34);
                vboolM end_padding_mask3 = PDX_CVTBBM_B2M_L(end_padding_mask34);
                vbool2M end_padding_mask12 = PDX_CVTBB2M_B_L(end_padding_mask);
                vboolM end_padding_mask2 = PDX_CVTBBM_B2M_H(end_padding_mask12);
                vboolM end_padding_mask1 = PDX_CVTBBM_B2M_L(end_padding_mask12);

                //start padding mask
                vbool2M start_padding_mask34 = PDX_CVTBB2M_B_H(mask0);
                vboolM start_padding_mask4 = PDX_CVTBBM_B2M_H(start_padding_mask34);
                vboolM start_padding_mask3 = PDX_CVTBBM_B2M_L(start_padding_mask34);
                vbool2M start_padding_mask12 = PDX_CVTBB2M_B_L(mask0);
                vboolM start_padding_mask2 = PDX_CVTBBM_B2M_H(start_padding_mask12);
                vboolM start_padding_mask1 = PDX_CVTBBM_B2M_L(start_padding_mask12);

                PDX_CVT32D_4MX20_L(last8,first8,acc);

                //keep 32 bit and do 80 bit multiplication

                //first8
                outacc = mult_acc_40 * first8;//8-way 32bit mac

                xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80,add_acc_80_first_pixel,start_padding_mask1);
                acc_proc = PDX_MOV_MX80_T(acc_proc,add_acc_80_last_pixel,end_padding_mask1);
                outacc += acc_proc;

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
                nBytesLeft -= nBytesToWrite;
                PDX_SAV32_MX8_XP(conv_out, outa,outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 8-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_MX8_FP(outa,outp);//flush

                //last8
                if(nBytesLeft>0){
                    //first8
                    outacc = mult_acc_40 * last8;//8-way 32bit mac
                    xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80,add_acc_80_first_pixel,start_padding_mask2);
                    acc_proc = PDX_MOV_MX80_T(acc_proc,add_acc_80_last_pixel,end_padding_mask2);
                    outacc += acc_proc;

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
                    nBytesLeft -= nBytesToWrite;
                    PDX_SAV32_MX8_XP(conv_out, outa, outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                    //to 8-way 8-bit signed, forward post-increment addressing mode
                    PDX_SAPOS_MX8_FP(outa,outp);//flush
                }


                if(nBytesLeft>0){
                    PDX_CVT32D_4MX20_H(last8,first8,acc);

                    //keep 32 bit and do 80 bit multiplication

                    //first8
                    outacc = mult_acc_40 * first8;//8-way 32bit mac

                    xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80,add_acc_80_first_pixel,start_padding_mask3);
                    acc_proc = PDX_MOV_MX80_T(acc_proc,add_acc_80_last_pixel,end_padding_mask3);
                    outacc += acc_proc;


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
                    nBytesLeft -= nBytesToWrite;
                    PDX_SAV32_MX8_XP(conv_out, outa,outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                    //to 8-way 8-bit signed, forward post-increment addressing mode
                    PDX_SAPOS_MX8_FP(outa,outp);//flush

                    //last8
                    if(nBytesLeft>0){
                        //first8

                        xb_vecMx80 outacc1;
                        outacc1 = mult_acc_40 * last8;//8-way 32bit mac

                        xb_vecMx80 acc_proc_in = PDX_MOV_MX80_T(add_acc_80,add_acc_80_first_pixel,start_padding_mask4);
                        acc_proc_in = PDX_MOV_MX80_T(acc_proc_in,add_acc_80_last_pixel,end_padding_mask4);
                        outacc1 += acc_proc_in;

                        //shift and round
                        outacc1 = PDX_SLS_MX80(outacc1,shift);//saturating left shift, right shift if negative
                        //round and shift and saturate
                        //convert from 8 80bit -> 8 8bit

                        xb_vecMx32 conv_out1 = PDX_PACKQSRV_MX80(outacc1, round_mode);//shift by 32 bit 80->32

                        //add output zero point
                        conv_out1 += vOutZP;
                        //Saturate to 8 bit range output_activation_min to 127
                        conv_out1 = PDX_MIN_MX32(conv_out1,vmax);
                        conv_out1 = PDX_MAX_MX32(conv_out1,vmin);
                        nBytesToWrite = nBytesLeft - PDX_M > 0 ? PDX_M : nBytesLeft;
                        nBytesLeft -= nBytesToWrite;
                        PDX_SAV32_MX8_XP(conv_out1, outa, outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                        //to 8-way 8-bit signed, forward post-increment addressing mode
                        PDX_SAPOS_MX8_FP(outa,outp);//flush
                    }
                }
#else
                //account for 1st row padding
                //Quantization Compensation
                //Saturate each lane in acc before converting to 16bit
                //sets to true if first op greater than second op
                greater_than16bit =  PDX_GT_4MX20(acc, vmax16bit);//Get bool reg for registers greater than 16bit value
                //sets to true if first op less than second op
                lesser_than16bit =  PDX_LT_4MX20(acc, vmin16bit);//Get bool reg for registers greater than 16bit value
                //if bool is TRUE, first op is selected or else second op is selected
                acc = PDX_MOV_4MX20_T(vmax16bit, acc,greater_than16bit);
                acc = PDX_MOV_4MX20_T(vmin16bit, acc, lesser_than16bit);
                //Separate 32way 20bit data into 2 16way 16bit registers and perform quantization comp separately
                PDX_CVT16D_4MX20(last16, first16, acc);

                //first16
                outacc = mult_acc * first16;//PDX_MULMNW_2MX16(mult_acc,first16,0,0);
                outacc += add_acc;
                //shift and round
                outacc = PDX_SLS_2MX40(outacc,shift);//saturating left shift, right shift if negative
                //round and shift and saturate
                conv_out = PDX_PACKQSRV_2MX40  (outacc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
                //add output zero point
                conv_out += vOutZP;
                //Saturate to 8 bit range output_activation_min to 127
                conv_out = PDX_MIN_2MX16(conv_out,vmax);
                conv_out = PDX_MAX_2MX16(conv_out,vmin);
                nBytesToWrite = nBytesLeft - 2*PDX_M > 0 ? 2*PDX_M : nBytesLeft;
                PDX_SAV16_2MX8_XP(conv_out, outa, outp,nBytesToWrite);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 16-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_2MX8_FP(outa,outp);//flush
                nBytesLeft -= nBytesToWrite;

                //last16
                outacc = mult_acc * last16;//PDX_MULMNW_2MX16(mult_acc,last16,0,0);
                outacc += add_acc;
                //shift and round
                outacc = PDX_SLS_2MX40(outacc,shift);//saturating left shift, right shift if negative
                //round and shift and saturate
                conv_out = PDX_PACKQSRV_2MX40  (outacc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
                //add output zero point
                conv_out += vOutZP;
                //Saturate to 8 bit range output_activation_min to 127
                conv_out = PDX_MIN_2MX16(conv_out,vmax);
                conv_out = PDX_MAX_2MX16(conv_out,vmin);
                nBytesToWrite = nBytesLeft - 2*PDX_M > 0 ? 2*PDX_M : nBytesLeft;
                PDX_SAV16_2MX8_XP(conv_out, outa, outp,nBytesToWrite);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 16-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_2MX8_FP(outa,outp);//flush
                nBytesLeft -= nBytesToWrite;
#endif
                mask0 = nomask;
            }
        }

        //Process for last row
        inp = (xb_vec4Mx8 *) (pInputBuffer + (nInputWidth - 2)*nInputWidth - 1);  //pointer to move down the rows -1 for padding
        nBytesLeft = nOutputBufferWidth;
        mask0 = PDX_MOVB_AU32 (0xFFFFFFFE);                    //setting mask to account for initial padding
        //First row we only process the top 2 rows assuming bottom row is padded
        for(int nCol = 0;nCol < nInputWidth;nCol += 4*PDX_M)
        {
            nRemCols = MIN(nBytesLeft - 1,4*PDX_M);
            uint32_t nShift = (((uint64_t)1)<<(nRemCols)) - 1;//only last element to be padded
            vbool4M end_padding_mask = PDX_MOVB_AU32 (nShift);

            valign ina = PDX_LA_4MX8_PP (inp);
            PDX_LA_4MX8_XP(in00, ina, inp, 1); //load input and increment input pointer to next location
            //in00 += vInZP;                         //add input zero point to each input vector

            ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
            PDX_LA_4MX8_XP (in01, ina, inp, 1); //load input and increment input pointer to next location
            //in01 += vInZP;                         //add input zero point to each input vector

            ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
            PDX_LA_4MX8_XP (in02, ina, inp, nInputWidth - 2);  //load input and increment input pointer to next row (here)
            //in02 += vInZP;                         //add input zero point to each input vector

            ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
            PDX_LA_4MX8_XP (in10, ina, inp, 1); //load input and increment input pointer to next location
            //in10 += vInZP;                         //add input zero point to each input vector

            ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
            PDX_LA_4MX8_XP (in11, ina, inp, 1); //load input and increment input pointer to next location
            //in11 += vInZP;                         //add input zero point to each input vector

            ina = PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
            PDX_LA_4MX8_XP (in12, ina, inp, -(nInputWidth + 2) + 4*PDX_M);  //load input and increment input pointer back to beginning
            //in12 += vInZP;                         //add input zero point to each input vector

            //multiply and accumulate
            tempacc1 = w00 * in00;
            tempacc1 <<= 1;
            PDX_MULAQW_4MX8(tempacc1,w10,in10);                    //Multiply and Accumulate - 16way 16bit

            //if bool is TRUE, first op is selected or else second op is selected
            acc = PDX_MOV_4MX20_T(tempacc1,zeros,mask0);//simulate padding

            //acc += vsum;                                    //load bias                //Load bias into acc
            PDX_MULAQW_4MX8(acc,w01,in01);                    //Multiply and Accumulate - 16way 16bit
            PDX_MULAQW_4MX8(acc,w11,in11);                    //Multiply and Accumulate - 16way 16bit

            PDX_MULAQW_4MX8_T(acc,w02,in02,end_padding_mask);                    //Multiply and Accumulate - 16way 16bit
            PDX_MULAQW_4MX8_T(acc,w12,in12,end_padding_mask);                    //Multiply and Accumulate - 16way 16bit
#ifdef USE_EXTRA_PRECISION
            //32 20 bit -> 8 80 bits
            //create mask based on elements left
            //convert 32 8 bit to 32 32bit data
            //map higher bits in H to 3 and 4
            //map lower bits in L to 1 and 2
            vbool2M end_padding_mask34 = PDX_CVTBB2M_B_H(end_padding_mask);
            vboolM end_padding_mask4 = PDX_CVTBBM_B2M_H(end_padding_mask34);
            vboolM end_padding_mask3 = PDX_CVTBBM_B2M_L(end_padding_mask34);
            vbool2M end_padding_mask12 = PDX_CVTBB2M_B_L(end_padding_mask);
            vboolM end_padding_mask2 = PDX_CVTBBM_B2M_H(end_padding_mask12);
            vboolM end_padding_mask1 = PDX_CVTBBM_B2M_L(end_padding_mask12);

            //start padding mask
            vbool2M start_padding_mask34 = PDX_CVTBB2M_B_H(mask0);
            vboolM start_padding_mask4 = PDX_CVTBBM_B2M_H(start_padding_mask34);
            vboolM start_padding_mask3 = PDX_CVTBBM_B2M_L(start_padding_mask34);
            vbool2M start_padding_mask12 = PDX_CVTBB2M_B_L(mask0);
            vboolM start_padding_mask2 = PDX_CVTBBM_B2M_H(start_padding_mask12);
            vboolM start_padding_mask1 = PDX_CVTBBM_B2M_L(start_padding_mask12);

            PDX_CVT32D_4MX20_L(last8,first8,acc);

            //keep 32 bit and do 80 bit multiplication

            //first8
            outacc = mult_acc_40 * first8;//8-way 32bit mac
            xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80_last_row,add_acc_80_last_row_first_pixel,start_padding_mask1);
            acc_proc = PDX_MOV_MX80_T(acc_proc,add_acc_80_last_row_last_pixel,end_padding_mask1);
            outacc += acc_proc;


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
            nBytesLeft -= nBytesToWrite;
            PDX_SAV32_MX8_XP(conv_out, outa,outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
            //to 8-way 8-bit signed, forward post-increment addressing mode
            PDX_SAPOS_MX8_FP(outa,outp);//flush

            //last8
            if(nBytesLeft>0){
                //first8
                outacc = mult_acc_40 * last8;//8-way 32bit mac
                //where start padding mask = 0, use add_acc_80_first_row_first_pixel
                xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80_last_row,add_acc_80_last_row_first_pixel,start_padding_mask2);//simulate padding
                //where end padding mask = 0 use add_acc_80_first_row_last_pixel
                acc_proc = PDX_MOV_MX80_T(acc_proc,add_acc_80_last_row_last_pixel,end_padding_mask2);//simulate padding
                outacc += acc_proc;

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
                nBytesLeft -= nBytesToWrite;
                PDX_SAV32_MX8_XP(conv_out, outa, outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 8-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_MX8_FP(outa,outp);//flush
            }


            if(nBytesLeft>0){
                PDX_CVT32D_4MX20_H(last8,first8,acc);

                //keep 32 bit and do 80 bit multiplication

                //first8
                outacc = mult_acc_40 * first8;//8-way 32bit mac
                xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80_last_row,add_acc_80_last_row_first_pixel,start_padding_mask3);
                acc_proc = PDX_MOV_MX80_T(acc_proc,add_acc_80_last_row_last_pixel,end_padding_mask3);
                outacc += acc_proc;

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
                nBytesLeft -= nBytesToWrite;
                PDX_SAV32_MX8_XP(conv_out, outa,outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 8-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_MX8_FP(outa,outp);//flush

                //last8
                if(nBytesLeft>0){
                    //first8
                    outacc = mult_acc_40 * last8;//8-way 32bit mac
                    xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80_last_row,add_acc_80_last_row_first_pixel,start_padding_mask4);
                    acc_proc = PDX_MOV_MX80_T(acc_proc,add_acc_80_last_row_last_pixel,end_padding_mask4);
                    outacc += acc_proc;

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
                    nBytesLeft -= nBytesToWrite;
                    PDX_SAV32_MX8_XP(conv_out, outa, outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                    //to 8-way 8-bit signed, forward post-increment addressing mode
                    PDX_SAPOS_MX8_FP(outa,outp);//flush
                }
            }
#else
            //account for 1st row padding
            //Quantization Compensation
            //Saturate each lane in acc before converting to 16bit
            //sets to true if first op greater than second op
            greater_than16bit =  PDX_GT_4MX20(acc, vmax16bit);//Get bool reg for registers greater than 16bit value
            //sets to true if first op less than second op
            lesser_than16bit =  PDX_LT_4MX20(acc, vmin16bit);//Get bool reg for registers greater than 16bit value
            //if bool is TRUE, first op is selected or else second op is selected
            acc = PDX_MOV_4MX20_T(vmax16bit, acc,greater_than16bit);
            acc = PDX_MOV_4MX20_T(vmin16bit, acc, lesser_than16bit);
            //Separate 32way 20bit data into 2 16way 16bit registers and perform quantization comp separately
            PDX_CVT16D_4MX20(last16, first16, acc);

            //first16
            outacc = mult_acc * first16;//PDX_MULMNW_2MX16(mult_acc,first16,0,0);
            outacc += add_acc;
            //shift and round
            outacc = PDX_SLS_2MX40(outacc,shift);//saturating left shift, right shift if negative
            //round and shift and saturate
            conv_out = PDX_PACKQSRV_2MX40  (outacc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
            //add output zero point
            conv_out += vOutZP;
            //Saturate to 8 bit range output_activation_min to 127
            conv_out = PDX_MIN_2MX16(conv_out,vmax);
            conv_out = PDX_MAX_2MX16(conv_out,vmin);
            nBytesToWrite = nBytesLeft - 2*PDX_M > 0 ? 2*PDX_M : nBytesLeft;
            PDX_SAV16_2MX8_XP(conv_out, outa, outp,nBytesToWrite);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
            //to 16-way 8-bit signed, forward post-increment addressing mode
            PDX_SAPOS_2MX8_FP(outa,outp);//flush
            nBytesLeft -= nBytesToWrite;

            //last16
            outacc = mult_acc * last16;//PDX_MULMNW_2MX16(mult_acc,last16,0,0);
            outacc += add_acc;
            //shift and round
            outacc = PDX_SLS_2MX40(outacc,shift);//saturating left shift, right shift if negative
            //round and shift and saturate
            conv_out = PDX_PACKQSRV_2MX40  (outacc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
            //add output zero point
            conv_out += vOutZP;
            //Saturate to 8 bit range output_activation_min to 127
            conv_out = PDX_MIN_2MX16(conv_out,vmax);
            conv_out = PDX_MAX_2MX16(conv_out,vmin);
            nBytesToWrite = nBytesLeft - 2*PDX_M > 0 ? 2*PDX_M : nBytesLeft;
            PDX_SAV16_2MX8_XP(conv_out, outa, outp,nBytesToWrite);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
            //to 16-way 8-bit signed, forward post-increment addressing mode
            PDX_SAPOS_2MX8_FP(outa,outp);//flush
            nBytesLeft -= nBytesToWrite;
#endif
            mask0 = nomask;
        }
        pInputBuffer += (nDepthMult==1)*(nInputWidth*nInputWidth);
    }
}

/**
*******************************************************************************
* Function: adi_sharcfx_depthconv2d_stride2_noninterleaved_int8
* @brief optimized depthconv2d function
*
* @details  2D depthwise-convolution with stride of 2, takes data in non-interleaved format using 16bit Eagle intrinsics. Also accounts for padding
*
* Parameters:
* @param [in] pInputBuffer - input data
* @param [in] pWeightsBuffer - input weights buffer
* @param [in] pBiasBuffer - input bias buffer
* @param [in] nInputWidth - input width
* @param [in] nDepthMult - depth multiplier
* @param [in] nInChannels - input depth
* @param [in] nOutChannels - output depth
* @param [in] nKernelSize - kernel size
* @param [in] nTotalPadding - padding length
* @param [in] pQuantizedMultiplier - multiplier
* @param [in] pQuantizedShift - shift
* @param [in] pInZeroPoint - input zeropoint
* @param [in] pOutZeroPoint - output zeropoint
* 
* @param [out] pOutputBuffer - output data
*
* @return None
*
*
*******************************************************************************
*/ 
//FIXME: Change the depthconv to work with interleaved data.
void adi_sharcfx_depthconv2d_stride2_noninterleaved_int8(const int8_t   *pInputBuffer,
                                                                       int8_t         *pOutputBuffer,
                                                                       const int8_t   *pWeightsBuffer,
                                                                       const int32_t  *pBiasBuffer,
                                                                       int32_t        nInputWidth,
                                                                       int32_t        nDepthMult,
                                                                       int32_t        nInChannels,
                                                                       int32_t        nOutChannels,
                                                                       int8_t         nKernelSize,
                                                                       int8_t         nTotalPadding,
                                                                       uint32_t       *pQuantizedMultiplier,
                                                                       int32_t        *pQuantizedShift,
                                                                       int32_t        pInZeroPoint,
                                                                       int32_t        pOutZeroPoint)
{
    int8_t nStrideLen = 2;
    int8_t nOutputBufferWidth = (int8_t)(nInputWidth - nKernelSize + nTotalPadding)/nStrideLen + 1;

    const immediate Lane = 0;
    const immediate round_mode = ROUNDING_MODE;



#ifdef USE_EXTRA_PRECISION
    xb_vecMx8* __restrict outp  = (xb_vecMx8 *)pOutputBuffer;
    xb_vecMx32  vOutZP = pOutZeroPoint;
    //load constants for range
    xb_vecMx32 vmin = ACT_MIN;
    xb_vecMx32 vmax = ACT_MAX;
    xb_vec2Mx40 vmax32bit = (xb_vec2Mx40)INT_32BIT_MAX;//Replicates the lane of data specified, across all lanes of a vector register
    xb_vec2Mx40 vmin32bit = (xb_vec2Mx40)INT_32BIT_MIN;//Replicates the lane of data specified, across all lanes of a vector register
    vbool2M greater_than32bit, lesser_than32bit;
    xb_vecMx32 first8, last8;
    valign outa = PDX_LA_MX8_PP(outp); // initialize the alignment register vaWrite to zero
#else
    valign outa = PDX_Z_ALIGN(); // initialize the alignment register vaWrite to zero
    //outbuffer
    xb_vec2Mx8 * outp = (xb_vec2Mx8 *) pOutputBuffer;

    //zeropts and range are global across all channels
    xb_vec2Mx16 vOutZP = PDX_REP_2MX40((xb_vec2Mx16)pOutZeroPoint,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    //load constants for range
    xb_vec2Mx16 vmin = PDX_REP_2MX16((xb_vec2Mx16)-128,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    xb_vec2Mx16 vmax = PDX_REP_2MX16((xb_vec2Mx16)127,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    xb_vec2Mx16 conv_out, first16, last16;
    xb_vec4Mx20 vsum;
    xb_vec4Mx20 vmax16bit = PDX_REP_4MX20((xb_vec4Mx20)0x7FFF,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    xb_vec4Mx20 vmin16bit = PDX_REP_4MX20((xb_vec4Mx20)0xFFFF8000,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    vbool4M greater_than16bit, lesser_than16bit;
#endif

    xb_vec4Mx8 vInZP = pInZeroPoint;//PDX_REP_4MX8((xb_vec4Mx8)pInZeroPoint,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    for(int nChannels = 0;nChannels < nOutChannels;nChannels++)
    {
#ifdef USE_EXTRA_PRECISION
        xb_vecMx32 mult_acc_40 = pQuantizedMultiplier[nChannels];
        xb_vecMx80 outacc;

        //Load pQuantizedShift for channel
        xb_vecMx32 shift = pQuantizedShift[nChannels];
#else
        uint16_t ReducedShift = (uint16_t)((pQuantizedMultiplier[nChannels] + (1 << 15)) >> 16);//Qmult is  (val) << 31, reduce this by 16 here to fit in int16
        //balance Qmult shift remaining is 15 but we include the +1 shift for PDX_MULAQW_2MX16 too here
        //this shift of 16 is accounted for during PDX_PACKQSRV_2MX40
        xb_vec2Mx16 mult_acc = PDX_REP_2MX16(ReducedShift,Lane);//Replicates the lane of data specified, across all lanes of a vector register

        //Load pQuantizedShift for channel
        const immediate nTotalShift = pQuantizedShift[nChannels]-1;
        xb_vec2Mx40 shift = PDX_REP_2MX40((xb_vec2Mx40)nTotalShift,Lane);//Replicates the lane of data specified, across all lanes of a vector register

        //bias specific to output channel
        xb_vec2Mx40 vbias = PDX_REP_2MX40((xb_vec2Mx40)((int64_t)((int64_t)pBiasBuffer[nChannels]<<2)*(ReducedShift)),Lane);
#endif

        //saturate bias within 20 bits
        //load bias*2 in nvec to match with vwt*vin*2
        //add zp * wt to bias
        int32_t zp_wt_sum;
        int32_t zp_wt_sum_last_pixel;
        int32_t zp_wt_sum_last_row;
        int32_t zp_wt_sum_last_row_last_pixel;

        int16_t wt_sum = 0;
        int16_t wt_sum_last_pixel = 0;

        int16_t wt_sum_last_row = 0;
        int16_t wt_sum_last_row_last_pixel = 0;

        //initialize weights for this channel
        //loading each of the (nKernelSize x nKernelSize) weights into it's own vector
        //each kernel weight is replicated in all the lanes to be multiplied in parallel
        int8_t nCurrWt;
        int8_t *filter_buff =  (int8_t *)pWeightsBuffer + nChannels;

        nCurrWt = (int8_t)(*filter_buff);
        wt_sum += nCurrWt;
        wt_sum_last_row += nCurrWt;
        wt_sum_last_pixel += nCurrWt;
        wt_sum_last_row_last_pixel += nCurrWt;
        filter_buff+=nOutChannels;

        xb_vec4Mx8 w00 = PDX_REP_4MX8((xb_vec4Mx8)nCurrWt,Lane);//load current weight and replicate the same in all lanes of the vector
        nCurrWt = (int8_t)(*filter_buff);
        wt_sum += nCurrWt;
        wt_sum_last_row += nCurrWt;
        wt_sum_last_pixel += nCurrWt;
        wt_sum_last_row_last_pixel += nCurrWt;
        filter_buff+=nOutChannels;

        xb_vec4Mx8 w01 = PDX_REP_4MX8((xb_vec4Mx8)nCurrWt,Lane);//load current weight and replicate the same in all lanes of the vector
        nCurrWt = (int8_t)(*filter_buff);
        wt_sum += nCurrWt;
        wt_sum_last_row += nCurrWt;
        filter_buff+=nOutChannels;
        xb_vec4Mx8 w02 = PDX_REP_4MX8((xb_vec4Mx8)nCurrWt,Lane);//load current weight and replicate the same in all lanes of the vector

        //handle first col padding using weights w10,w20
        nCurrWt = (int8_t)(*filter_buff);
        wt_sum += nCurrWt;
        wt_sum_last_row += nCurrWt;
        wt_sum_last_pixel += nCurrWt;
        wt_sum_last_row_last_pixel += nCurrWt;
        filter_buff+=nOutChannels;

        xb_vec4Mx8 w10 = PDX_REP_4MX8((xb_vec4Mx8)nCurrWt,Lane);//load current weight and replicate the same in all lanes of the vector
        nCurrWt = (int8_t)(*filter_buff);
        wt_sum += nCurrWt;
        wt_sum_last_row += nCurrWt;
        wt_sum_last_pixel += nCurrWt;
        wt_sum_last_row_last_pixel += nCurrWt;
        filter_buff+=nOutChannels;

        xb_vec4Mx8 w11 = PDX_REP_4MX8((xb_vec4Mx8)nCurrWt,Lane);//load current weight and replicate the same in all lanes of the vector
        nCurrWt = (int8_t)(*filter_buff);
        wt_sum += nCurrWt;
        wt_sum_last_row += nCurrWt;
        filter_buff+=nOutChannels;
        xb_vec4Mx8 w12 = PDX_REP_4MX8((xb_vec4Mx8)nCurrWt,Lane);//load current weight and replicate the same in all lanes of the vector

        nCurrWt = (int8_t)(*filter_buff);
        wt_sum += nCurrWt;
        wt_sum_last_pixel += nCurrWt;
        filter_buff+=nOutChannels;

        xb_vec4Mx8 w20 = PDX_REP_4MX8((xb_vec4Mx8)nCurrWt,Lane);//load current weight and replicate the same in all lanes of the vector
        nCurrWt = (int8_t)(*filter_buff);
        wt_sum += nCurrWt;
        wt_sum_last_pixel += nCurrWt;
        filter_buff+=nOutChannels;

        xb_vec4Mx8 w21 = PDX_REP_4MX8((xb_vec4Mx8)nCurrWt,Lane);//load current weight and replicate the same in all lanes of the vector
        nCurrWt = (int8_t)(*filter_buff);
        wt_sum += nCurrWt;
        filter_buff+=nOutChannels;
        xb_vec4Mx8 w22 = PDX_REP_4MX8((xb_vec4Mx8)nCurrWt,Lane);//load current weight and replicate the same in all lanes of the vector

        //PDX_MULAQW_2MX16 will add a *2, so shift this sum also by 2
        zp_wt_sum = (pInZeroPoint) * wt_sum;//8 bit * 16 bit should fit within 32 bit
        zp_wt_sum_last_pixel = (pInZeroPoint) * wt_sum_last_pixel;//8 bit * 16 bit should fit within 32 bit
        zp_wt_sum_last_row = (pInZeroPoint) * wt_sum_last_row;//8 bit * 16 bit should fit within 32 bit
        zp_wt_sum_last_row_last_pixel = (pInZeroPoint) * wt_sum_last_row_last_pixel;//8 bit * 16 bit should fit within 32 bit

#ifdef USE_EXTRA_PRECISION
        zp_wt_sum += pBiasBuffer[nChannels];//bias specific to output channel
        zp_wt_sum_last_pixel += pBiasBuffer[nChannels];//bias specific to output channel
        zp_wt_sum_last_row += pBiasBuffer[nChannels];//bias specific to output channel
        zp_wt_sum_last_row_last_pixel += pBiasBuffer[nChannels];//bias specific to output channel

        zp_wt_sum <<= 1;
        zp_wt_sum_last_pixel <<= 1;
        zp_wt_sum_last_row <<= 1;
        zp_wt_sum_last_row_last_pixel <<= 1;

        xb_vecMx80 add_acc_80 = (xb_vecMx32)pQuantizedMultiplier[nChannels] * (xb_vecMx32)zp_wt_sum;
        xb_vecMx80 add_acc_80_last_pixel = (xb_vecMx32)pQuantizedMultiplier[nChannels] * (xb_vecMx32)zp_wt_sum_last_pixel;
        xb_vecMx80 add_acc_80_last_row = (xb_vecMx32)pQuantizedMultiplier[nChannels] * (xb_vecMx32)zp_wt_sum_last_row;
        xb_vecMx80 add_acc_80_last_row_last_pixel = (xb_vecMx32)pQuantizedMultiplier[nChannels] * (xb_vecMx32)zp_wt_sum_last_row_last_pixel;
#else
        vsum = max((xb_vec4Mx20)(zp_wt_sum << 1), (xb_vec4Mx20)0xFFFFF);
        vsum = min(vsum, (xb_vec4Mx20)0x7FFFF);

        xb_vec2Mx40 outacc;
#endif

        //create first row mask
        vbool4M mask0 = PDX_MOVB_AU32 (0xFFFE);
        vbool4M mask2 = PDX_MOVB_AU32 (0x7FFF);

        xb_vec4Mx8 masked_w00 = PDX_MOV_V_T (w00, 0, mask0);  // Clear the first and/or last lane, if needed
        xb_vec4Mx8 masked_w02 = PDX_MOV_V_T (w02, 0, mask2);  // Clear the first and/or last lane, if needed

        //set first weight w10 to 0
        xb_vec4Mx8 masked_w10 = PDX_MOV_V_T (w10, 0, mask0);  // Clear the first and/or last lane, if needed
        xb_vec4Mx8 masked_w12 = PDX_MOV_V_T (w12, 0, mask2);  // Clear the first and/or last lane, if needed

        //set first weight w20 to 0
        xb_vec4Mx8 masked_w20 = PDX_MOV_V_T (w20, 0, mask0);  // Clear the first and/or last lane, if needed
        xb_vec4Mx8 masked_w22 = PDX_MOV_V_T (w22, 0, mask2);  // Clear the first and/or last lane, if needed

        //top row with first row padded, for 3x3 1 first col and 1 last col

        xb_vec4Mx8 * inp = (xb_vec4Mx8 *) pInputBuffer;  // create a pointer to move down the rows
        valign ina = PDX_LA_4MX8_PP (inp);


        xb_vec4Mx8 in00,in01,in02;
        xb_vec4Mx8 in10,in11,in12;
        xb_vec4Mx8 in20,in21,in22;
        xb_vec4Mx8 temp1, temp2;
        xb_vec4Mx20 tempacc;
        vbool4M temp_mask;
        vbool2M save_mask_low;

        int nBytesLeft = nOutputBufferWidth;
        int nBytesToWrite = 0;

        //---------------------------------------------All rows except last----------------------------------------------
        //for first to penultimate rows in one channel - no init padding
        for(int j = 0;j < nOutputBufferWidth - 1;j++)
        {
            inp = (xb_vec4Mx8 *) (pInputBuffer + (j * nStrideLen * nInputWidth));
            //first to penultimate pixels in row - no padding
            for(nBytesLeft = nOutputBufferWidth;nBytesLeft > 4*PDX_M;)
            {
                PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next element
                in00 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

                PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next row
                in01 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

                PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, nInputWidth - 2);  // read two vectors and go to next row
                in02 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

                PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next row
                in10 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

                PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next row
                in11 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

                PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, nInputWidth - 2);  // read two vectors and go to next row
                in12 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

                PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next row
                in20 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

                PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next row
                in21 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

                PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, - (nInputWidth * 2) - 2 + 4*PDX_M *2);  // go back to beginning + 64
                in22 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

#ifdef USE_EXTRA_PRECISION
                tempacc = 0;
#else
                tempacc = PDX_REP_4MX20(vsum,Lane);//load bias
#endif
                //multiply and accumulate
                PDX_MULAQW_4MX8(tempacc,w00,in00);
                PDX_MULAQW_4MX8(tempacc,w01,in01);
                PDX_MULAQW_4MX8(tempacc,w02,in02);
                PDX_MULAQW_4MX8(tempacc,w10,in10);
                PDX_MULAQW_4MX8(tempacc,w11,in11);
                PDX_MULAQW_4MX8(tempacc,w12,in12);
                PDX_MULAQW_4MX8(tempacc,w20,in20);
                PDX_MULAQW_4MX8(tempacc,w21,in21);
                PDX_MULAQW_4MX8(tempacc,w22,in22);

#ifdef USE_EXTRA_PRECISION
                //32 20 bit -> 8 80 bits
                //convert 32 20 bit acc to 8 32 bit acc.
                //convert L half of acc to 2 32 bit acc

                PDX_CVT32D_4MX20_L(last8,first8,tempacc);

                //keep 32 bit and do 80 bit multiplication

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
                PDX_SAV32_MX8_XP(conv_out, outa,outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 8-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_MX8_FP(outa,outp);//flush

                //last8
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
                    PDX_SAV32_MX8_XP(conv_out, outa, outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                    //to 8-way 8-bit signed, forward post-increment addressing mode
                    PDX_SAPOS_MX8_FP(outa,outp);//flush
                }


                if(nBytesLeft>0){
                    PDX_CVT32D_4MX20_H(last8,first8,tempacc);

                    //keep 32 bit and do 80 bit multiplication

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
                    nBytesLeft -= nBytesToWrite;
                    PDX_SAV32_MX8_XP(conv_out, outa,outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                    //to 8-way 8-bit signed, forward post-increment addressing mode
                    PDX_SAPOS_MX8_FP(outa,outp);//flush

                    //last8
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
                        nBytesLeft -= nBytesToWrite;
                        PDX_SAV32_MX8_XP(conv_out, outa, outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                        //to 8-way 8-bit signed, forward post-increment addressing mode
                        PDX_SAPOS_MX8_FP(outa,outp);//flush
                    }
                }
#else
                //Quantization Compensation
                //Saturate each lane in acc before converting to 16bit
                //sets to true if first op greater than second op
                greater_than16bit =  PDX_GT_4MX20(tempacc, vmax16bit);//Get bool reg for registers greater than 16bit value
                //sets to true if first op less than second op
                lesser_than16bit =  PDX_LT_4MX20(tempacc, vmin16bit);//Get bool reg for registers greater than 16bit value
                //if bool is TRUE, first op is selected or else second op is selected
                tempacc = PDX_MOV_4MX20_T(vmax16bit, tempacc,greater_than16bit);
                tempacc = PDX_MOV_4MX20_T(vmin16bit, tempacc, lesser_than16bit);
                //Separate 32way 20bit data into 2 16way 16bit registers and perform quantization comp separately
                PDX_CVT16D_4MX20(last16, first16, tempacc);

                //first16
                outacc=0;
                PDX_MULAQW_2MX16(outacc,mult_acc,first16);
                outacc+=vbias;
                //shift and round
                outacc = PDX_SLS_2MX40(outacc,shift);//saturating left shift, right shift if negative
                //round and shift and saturate
                conv_out = PDX_PACKQSRV_2MX40  (outacc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
                //add output zero point
                conv_out += vOutZP;
                //Saturate to 8 bit range output_activation_min to 127
                conv_out = PDX_MIN_2MX16(conv_out,vmax);
                conv_out = PDX_MAX_2MX16(conv_out,vmin);
                nBytesToWrite = nBytesLeft - 2*PDX_M > 0 ? 2*PDX_M : nBytesLeft;
                nBytesLeft-=2*PDX_M;
                PDX_SAV16_2MX8_XP(conv_out, outa, outp,nBytesToWrite);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 16-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_2MX8_FP(outa,outp);//flush

                //last16
                outacc=0;
                PDX_MULAQW_2MX16(outacc,mult_acc,last16);
                outacc+=vbias;
                //shift and round
                outacc = PDX_SLS_2MX40(outacc,shift);//saturating left shift, right shift if negative
                //round and shift and saturate
                conv_out = PDX_PACKQSRV_2MX40  (outacc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
                //add output zero point
                conv_out += vOutZP;
                //Saturate to 8 bit range output_activation_min to 127
                conv_out = PDX_MIN_2MX16(conv_out,vmax);
                conv_out = PDX_MAX_2MX16(conv_out,vmin);
                nBytesToWrite = nBytesLeft - 2*PDX_M > 0 ? 2*PDX_M : nBytesLeft;
                nBytesLeft-=2*PDX_M;
                PDX_SAV16_2MX8_XP(conv_out, outa, outp,nBytesToWrite);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 16-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_2MX8_FP(outa,outp);//flush
#endif
            }

            //last pixel in row - column padding present

            vbool4M padding_mask = PDX_MOVB_AU32 ((0x1<<(nBytesLeft-1)) - 1);
            xb_vec4Mx8 masked_w02 = PDX_MOV_V_T (w02, 0, padding_mask);  // Clear the first and/or last lane, if needed
            xb_vec4Mx8 masked_w12 = PDX_MOV_V_T (w12, 0, padding_mask);  // Clear the first and/or last lane, if needed
            xb_vec4Mx8 masked_w22 = PDX_MOV_V_T (w22, 0, padding_mask);  // Clear the first and/or last lane, if needed


            PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next element
            in00 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

            PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next row
            in01 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

            PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, nInputWidth - 2);  // read two vectors and go to next row
            in02 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

            PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next row
            in10 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

            PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next row
            in11 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

            PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, nInputWidth - 2);  // read two vectors and go to next row
            in12 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

            PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next row
            in20 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

            PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next row
            in21 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

            PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 0);  // go back to beginning + 64
            in22 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

#ifdef USE_EXTRA_PRECISION
            tempacc = 0;
#else
            tempacc = PDX_REP_4MX20(vsum,Lane);//load bias
#endif

            //multiply and accumulate
            PDX_MULAQW_4MX8(tempacc,w00,in00);
            PDX_MULAQW_4MX8(tempacc,w01,in01);
            PDX_MULAQW_4MX8(tempacc,masked_w02,in02);
            PDX_MULAQW_4MX8(tempacc,w10,in10);
            PDX_MULAQW_4MX8(tempacc,w11,in11);
            PDX_MULAQW_4MX8(tempacc,masked_w12,in12);
            PDX_MULAQW_4MX8(tempacc,w20,in20);
            PDX_MULAQW_4MX8(tempacc,w21,in21);
            PDX_MULAQW_4MX8(tempacc,masked_w22,in22);

#ifdef USE_EXTRA_PRECISION
            //32 20 bit -> 8 80 bits
            //convert 32 20 bit acc to 8 32 bit acc.
            //convert L half of acc to 2 32 bit acc

            PDX_CVT32D_4MX20_L(last8,first8,tempacc);

            //keep 32 bit and do 80 bit multiplication

            //first8
            outacc = mult_acc_40 * first8;//8-way 32bit mac

            //if bool is TRUE, first op is selected or else second op is selected
            //create mask based on elements left
            int nRemCols = MIN(nBytesLeft - 1,4*PDX_M);//1 last pixel is padded
            uint32_t nShift = (((uint64_t)1)<<(nRemCols)) - 1;//only last element to be padded
            vbool4M cal_padding_mask = PDX_MOVB_AU32(nShift);
            //create mask based on elements left
            //convert 32 8 bit to 32 32bit data
            //map higher bits in H to 3 and 4
            //map lower bits in L to 1 and 2
            vbool2M end_padding_mask34 = PDX_CVTBB2M_B_H(cal_padding_mask);
            vboolM end_padding_mask4 = PDX_CVTBBM_B2M_H(end_padding_mask34);
            vboolM end_padding_mask3 = PDX_CVTBBM_B2M_L(end_padding_mask34);
            vbool2M end_padding_mask12 = PDX_CVTBB2M_B_L(cal_padding_mask);
            vboolM end_padding_mask2 = PDX_CVTBBM_B2M_H(end_padding_mask12);
            vboolM end_padding_mask1 = PDX_CVTBBM_B2M_L(end_padding_mask12);

            xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80,add_acc_80_last_pixel,end_padding_mask1);//simulate padding
            outacc += acc_proc;

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
            PDX_SAV32_MX8_XP(conv_out, outa,outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
            //to 8-way 8-bit signed, forward post-increment addressing mode
            PDX_SAPOS_MX8_FP(outa,outp);//flush

            //last8
            if(nBytesLeft>0){
                //first8
                outacc = mult_acc_40 * last8;//8-way 32bit mac

                xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80,add_acc_80_last_pixel,end_padding_mask2);//simulate padding
                outacc += acc_proc;

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
                PDX_SAV32_MX8_XP(conv_out, outa, outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 8-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_MX8_FP(outa,outp);//flush
            }


            if(nBytesLeft>0){
                PDX_CVT32D_4MX20_H(last8,first8,tempacc);

                //keep 32 bit and do 80 bit multiplication

                //first8
                outacc = mult_acc_40 * first8;//8-way 32bit mac

                xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80,add_acc_80_last_pixel,end_padding_mask3);//simulate padding
                outacc += acc_proc;


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
                nBytesLeft -= nBytesToWrite;
                PDX_SAV32_MX8_XP(conv_out, outa,outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 8-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_MX8_FP(outa,outp);//flush

                //last8
                if(nBytesLeft>0){
                    //first8
                    outacc = mult_acc_40 * last8;//8-way 32bit mac

                    xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80,add_acc_80_last_pixel,end_padding_mask4);//simulate padding
                    outacc += acc_proc;


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
                    nBytesLeft -= nBytesToWrite;
                    PDX_SAV32_MX8_XP(conv_out, outa, outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                    //to 8-way 8-bit signed, forward post-increment addressing mode
                    PDX_SAPOS_MX8_FP(outa,outp);//flush
                }
            }
#else
            //Quantization Compensation
            //Saturate each lane in acc before converting to 16bit
            //sets to true if first op greater than second op
            greater_than16bit =  PDX_GT_4MX20(tempacc, vmax16bit);//Get bool reg for registers greater than 16bit value
            //sets to true if first op less than second op
            lesser_than16bit =  PDX_LT_4MX20(tempacc, vmin16bit);//Get bool reg for registers greater than 16bit value
            //if bool is TRUE, first op is selected or else second op is selected
            tempacc = PDX_MOV_4MX20_T(vmax16bit, tempacc,greater_than16bit);
            tempacc = PDX_MOV_4MX20_T(vmin16bit, tempacc, lesser_than16bit);
            //Separate 32way 20bit data into 2 16way 16bit registers and perform quantization comp separately
            PDX_CVT16D_4MX20(last16, first16, tempacc);

            //first16
            outacc=0;
            PDX_MULAQW_2MX16(outacc,mult_acc,first16);
            outacc+=vbias;
            //shift and round
            outacc = PDX_SLS_2MX40(outacc,shift);//saturating left shift, right shift if negative
            //round and shift and saturate
            conv_out = PDX_PACKQSRV_2MX40  (outacc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
            //add output zero point
            conv_out += vOutZP;
            //Saturate to 8 bit range output_activation_min to 127
            conv_out = PDX_MIN_2MX16(conv_out,vmax);
            conv_out = PDX_MAX_2MX16(conv_out,vmin);
            nBytesToWrite = nBytesLeft - 2*PDX_M > 0 ? 2*PDX_M : nBytesLeft;
            nBytesLeft-=2*PDX_M;
            temp_mask = PDX_MOVB_AU32((0b1<<(nBytesToWrite)) - 1);
            save_mask_low = PDX_CVTBB2M_B4M_L(temp_mask);
            PDX_SAV16_2MX8_XP_T(conv_out, outa, outp,nBytesToWrite, temp_mask, save_mask_low);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
            //to 16-way 8-bit signed, forward post-increment addressing mode
            PDX_SAPOS_2MX8_FP(outa,outp);//flush

            //last16
            if(nBytesLeft>0){
                outacc=0;
                PDX_MULAQW_2MX16(outacc,mult_acc,last16);
                outacc+=vbias;
                //shift and round
                outacc = PDX_SLS_2MX40(outacc,shift);//saturating left shift, right shift if negative
                //round and shift and saturate
                conv_out = PDX_PACKQSRV_2MX40  (outacc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
                //add output zero point
                conv_out += vOutZP;
                //Saturate to 8 bit range output_activation_min to 127
                conv_out = PDX_MIN_2MX16(conv_out,vmax);
                conv_out = PDX_MAX_2MX16(conv_out,vmin);
                nBytesToWrite = nBytesLeft - 2*PDX_M > 0 ? 2*PDX_M : nBytesLeft;
                nBytesLeft-=2*PDX_M;
                temp_mask = PDX_MOVB_AU32((0b1<<(nBytesToWrite)) - 1);
                save_mask_low = PDX_CVTBB2M_B4M_L(temp_mask);
                PDX_SAV16_2MX8_XP_T(conv_out, outa, outp,nBytesToWrite, temp_mask, save_mask_low);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 16-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_2MX8_FP(outa,outp);//flush
            }
#endif
        }
        //----------------------------------------------LAST ROW---------------------------------------------
        //Last padded row done separately
        //w20,w21,w22 not used
        nBytesLeft = nOutputBufferWidth;
        inp =(xb_vec4Mx8 *) ((pInputBuffer + (nInputWidth-2) * nInputWidth));
        ina = PDX_LA_4MX8_PP (inp);
        //ensure we do last padded pixel separately
        for(nBytesLeft = nOutputBufferWidth;nBytesLeft > 4*PDX_M;)
        {
            PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next element
            in00 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

            PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next row
            in01 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

            PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, nInputWidth - 2);  // read two vectors and go to next row
            in02 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

            PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next row
            in10 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

            PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next row
            in11 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

            PDX_LA_4MX8D_XP (temp1, temp2, ina, inp,  - (nInputWidth) - 2 + 4*PDX_M *2);  // read two vectors and go to next row
            in12 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

#ifdef USE_EXTRA_PRECISION
            tempacc = 0;
#else
            tempacc = PDX_REP_4MX20(vsum,Lane);//load bias
#endif

            //multiply and accumulate
            PDX_MULAQW_4MX8(tempacc,w00,in00);
            PDX_MULAQW_4MX8(tempacc,w01,in01);
            PDX_MULAQW_4MX8(tempacc,w02,in02);
            PDX_MULAQW_4MX8(tempacc,w10,in10);
            PDX_MULAQW_4MX8(tempacc,w11,in11);
            PDX_MULAQW_4MX8(tempacc,w12,in12);

#ifdef USE_EXTRA_PRECISION
            //32 20 bit -> 8 80 bits
            //convert 32 20 bit acc to 8 32 bit acc.
            //convert L half of acc to 2 32 bit acc

            PDX_CVT32D_4MX20_L(last8,first8,tempacc);

            //keep 32 bit and do 80 bit multiplication

            //first8
            outacc = mult_acc_40 * first8;//8-way 32bit mac
            outacc += add_acc_80_last_row;

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
            PDX_SAV32_MX8_XP(conv_out, outa,outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
            //to 8-way 8-bit signed, forward post-increment addressing mode
            PDX_SAPOS_MX8_FP(outa,outp);//flush

            //last8
            if(nBytesLeft>0){
                //first8
                outacc = mult_acc_40 * last8;//8-way 32bit mac
                outacc += add_acc_80_last_row;

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
                PDX_SAV32_MX8_XP(conv_out, outa, outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 8-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_MX8_FP(outa,outp);//flush
            }


            if(nBytesLeft>0){
                PDX_CVT32D_4MX20_H(last8,first8,tempacc);

                //keep 32 bit and do 80 bit multiplication

                //first8
                outacc = mult_acc_40 * first8;//8-way 32bit mac
                outacc += add_acc_80_last_row;

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
                nBytesLeft -= nBytesToWrite;
                PDX_SAV32_MX8_XP(conv_out, outa,outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 8-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_MX8_FP(outa,outp);//flush

                //last8
                if(nBytesLeft>0){
                    //first8
                    outacc = mult_acc_40 * last8;//8-way 32bit mac
                    outacc += add_acc_80_last_row;

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
                    nBytesLeft -= nBytesToWrite;
                    PDX_SAV32_MX8_XP(conv_out, outa, outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                    //to 8-way 8-bit signed, forward post-increment addressing mode
                    PDX_SAPOS_MX8_FP(outa,outp);//flush
                }
            }
#else
            //Quantization Compensation
            //Saturate each lane in acc before converting to 16bit
            //sets to true if first op greater than second op
            greater_than16bit =  PDX_GT_4MX20(tempacc, vmax16bit);//Get bool reg for registers greater than 16bit value
            //sets to true if first op less than second op
            lesser_than16bit =  PDX_LT_4MX20(tempacc, vmin16bit);//Get bool reg for registers greater than 16bit value
            //if bool is TRUE, first op is selected or else second op is selected
            tempacc = PDX_MOV_4MX20_T(vmax16bit, tempacc,greater_than16bit);
            tempacc = PDX_MOV_4MX20_T(vmin16bit, tempacc, lesser_than16bit);
            //Separate 32way 20bit data into 2 16way 16bit registers and perform quantization comp separately
            PDX_CVT16D_4MX20(last16, first16, tempacc);

            //first16
            outacc=0;
            PDX_MULAQW_2MX16(outacc,mult_acc,first16);
            outacc+=vbias;
            //shift and round
            outacc = PDX_SLS_2MX40(outacc,shift);//saturating left shift, right shift if negative
            //round and shift and saturate
            conv_out = PDX_PACKQSRV_2MX40  (outacc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
            //add output zero point
            conv_out += vOutZP;
            //Saturate to 8 bit range output_activation_min to 127
            conv_out = PDX_MIN_2MX16(conv_out,vmax);
            conv_out = PDX_MAX_2MX16(conv_out,vmin);
            nBytesToWrite = nBytesLeft - 2*PDX_M > 0 ? 2*PDX_M : nBytesLeft;
            nBytesLeft-=2*PDX_M;
            PDX_SAV16_2MX8_XP(conv_out, outa, outp,nBytesToWrite);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
            //to 16-way 8-bit signed, forward post-increment addressing mode
            PDX_SAPOS_2MX8_FP(outa,outp);//flush

            //last16
            outacc=0;
            PDX_MULAQW_2MX16(outacc,mult_acc,last16);
            outacc+=vbias;
            //shift and round
            outacc = PDX_SLS_2MX40(outacc,shift);//saturating left shift, right shift if negative
            //round and shift and saturate
            conv_out = PDX_PACKQSRV_2MX40  (outacc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
            //add output zero point
            conv_out += vOutZP;
            //Saturate to 8 bit range output_activation_min to 127
            conv_out = PDX_MIN_2MX16(conv_out,vmax);
            conv_out = PDX_MAX_2MX16(conv_out,vmin);
            nBytesToWrite = nBytesLeft - 2*PDX_M > 0 ? 2*PDX_M : nBytesLeft;
            nBytesLeft-=2*PDX_M;
            PDX_SAV16_2MX8_XP(conv_out, outa, outp,nBytesToWrite);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
            //to 16-way 8-bit signed, forward post-increment addressing mode
            PDX_SAPOS_2MX8_FP(outa,outp);//flush
#endif
        }

        //last row - with last pixel

        vbool4M padding_mask = PDX_MOVB_AU32 ((0x1<<(nBytesLeft-1)) - 1);
        masked_w02 = PDX_MOV_V_T (w02, 0, padding_mask);  // Clear the first and/or last lane, if needed
        masked_w12 = PDX_MOV_V_T (w12, 0, padding_mask);  // Clear the first and/or last lane, if needed
        //masked_w22 = PDX_MOV_V_T (w22, 0, padding_mask);

        PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next element
        in00 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

        PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next row
        in01 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

        PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, nInputWidth - 2);  // read two vectors and go to next row
        in02 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

        PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next row
        in10 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

        PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 1);  // read two vectors and go to next row
        in11 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

        PDX_LA_4MX8D_XP (temp1, temp2, ina, inp, 0);  // read two vectors and go to next row
        in12 = PDX_SELI_4MX8 (temp1, temp2, PDX_SELI_8B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes

#ifdef USE_EXTRA_PRECISION
        tempacc = 0;
#else
        tempacc = PDX_REP_4MX20(vsum,Lane);//load bias
#endif


        //multiply and accumulate
        PDX_MULAQW_4MX8(tempacc,w00,in00);
        PDX_MULAQW_4MX8(tempacc,w01,in01);
        PDX_MULAQW_4MX8(tempacc,masked_w02,in02);
        PDX_MULAQW_4MX8(tempacc,w10,in10);
        PDX_MULAQW_4MX8(tempacc,w11,in11);
        PDX_MULAQW_4MX8(tempacc,masked_w12,in12);

#ifdef USE_EXTRA_PRECISION
        //32 20 bit -> 8 80 bits
        //convert 32 20 bit acc to 8 32 bit acc.
        //convert L half of acc to 2 32 bit acc

        PDX_CVT32D_4MX20_L(last8,first8,tempacc);

        //keep 32 bit and do 80 bit multiplication

        //first8
        outacc = mult_acc_40 * first8;//8-way 32bit mac

        //if bool is TRUE, first op is selected or else second op is selected
        //create mask based on elements left
        int nRemCols = MIN(nBytesLeft - 1,4*PDX_M);//1 last pixel is padded
        uint32_t nShift = (((uint64_t)1)<<(nRemCols)) - 1;//only last element to be padded
        vbool4M cal_padding_mask = PDX_MOVB_AU32(nShift);
        //create mask based on elements left
        //convert 32 8 bit to 32 32bit data
        //map higher bits in H to 3 and 4
        //map lower bits in L to 1 and 2
        vbool2M end_padding_mask34 = PDX_CVTBB2M_B_H(cal_padding_mask);
        vboolM end_padding_mask4 = PDX_CVTBBM_B2M_H(end_padding_mask34);
        vboolM end_padding_mask3 = PDX_CVTBBM_B2M_L(end_padding_mask34);
        vbool2M end_padding_mask12 = PDX_CVTBB2M_B_L(cal_padding_mask);
        vboolM end_padding_mask2 = PDX_CVTBBM_B2M_H(end_padding_mask12);
        vboolM end_padding_mask1 = PDX_CVTBBM_B2M_L(end_padding_mask12);

        xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80_last_row,add_acc_80_last_row_last_pixel,end_padding_mask1);//simulate padding
        outacc += acc_proc;

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
        PDX_SAV32_MX8_XP(conv_out, outa,outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
        //to 8-way 8-bit signed, forward post-increment addressing mode
        PDX_SAPOS_MX8_FP(outa,outp);//flush

        //last8
        if(nBytesLeft>0){
            //first8
            outacc = mult_acc_40 * last8;//8-way 32bit mac

            xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80_last_row,add_acc_80_last_row_last_pixel,end_padding_mask2);//simulate padding
            outacc += acc_proc;

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
            PDX_SAV32_MX8_XP(conv_out, outa, outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
            //to 8-way 8-bit signed, forward post-increment addressing mode
            PDX_SAPOS_MX8_FP(outa,outp);//flush
        }


        if(nBytesLeft>0){
            PDX_CVT32D_4MX20_H(last8,first8,tempacc);

            //keep 32 bit and do 80 bit multiplication

            //first8
            outacc = mult_acc_40 * first8;//8-way 32bit mac

            xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80_last_row,add_acc_80_last_row_last_pixel,end_padding_mask3);//simulate padding
            outacc += acc_proc;

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
            nBytesLeft -= nBytesToWrite;
            PDX_SAV32_MX8_XP(conv_out, outa,outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
            //to 8-way 8-bit signed, forward post-increment addressing mode
            PDX_SAPOS_MX8_FP(outa,outp);//flush

            //last8
            if(nBytesLeft>0){
                //first8
                outacc = mult_acc_40 * last8;//8-way 32bit mac
                xb_vecMx80 acc_proc = PDX_MOV_MX80_T(add_acc_80_last_row,add_acc_80_last_row_last_pixel,end_padding_mask4);//simulate padding
                outacc += acc_proc;

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
                nBytesLeft -= nBytesToWrite;
                PDX_SAV32_MX8_XP(conv_out, outa, outp, nBytesToWrite);//8-way 8-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 8-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_MX8_FP(outa,outp);//flush
            }
        }
#else
        //Quantization Compensation
        //Saturate each lane in acc before converting to 16bit
        //sets to true if first op greater than second op
        greater_than16bit =  PDX_GT_4MX20(tempacc, vmax16bit);//Get bool reg for registers greater than 16bit value
        //sets to true if first op less than second op
        lesser_than16bit =  PDX_LT_4MX20(tempacc, vmin16bit);//Get bool reg for registers greater than 16bit value
        //if bool is TRUE, first op is selected or else second op is selected
        tempacc = PDX_MOV_4MX20_T(vmax16bit, tempacc,greater_than16bit);
        tempacc = PDX_MOV_4MX20_T(vmin16bit, tempacc, lesser_than16bit);
        //Separate 32way 20bit data into 2 16way 16bit registers and perform quantization comp separately
        PDX_CVT16D_4MX20(last16, first16, tempacc);

        //first16
        outacc=0;
        PDX_MULAQW_2MX16(outacc,mult_acc,first16);
        outacc+=vbias;
        //shift and round
        outacc = PDX_SLS_2MX40(outacc,shift);//saturating left shift, right shift if negative
        //round and shift and saturate
        conv_out = PDX_PACKQSRV_2MX40  (outacc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
        //add output zero point
        conv_out += vOutZP;
        //Saturate to 8 bit range output_activation_min to 127
        conv_out = PDX_MIN_2MX16(conv_out,vmax);
        conv_out = PDX_MAX_2MX16(conv_out,vmin);
        nBytesToWrite = nBytesLeft - 2*PDX_M > 0 ? 2*PDX_M : nBytesLeft;
        nBytesLeft-=2*PDX_M;
        temp_mask = PDX_MOVB_AU32((0b1<<(nBytesToWrite)) - 1);
        save_mask_low = PDX_CVTBB2M_B4M_L(temp_mask);
        PDX_SAV16_2MX8_XP_T(conv_out, outa, outp,nBytesToWrite, temp_mask, save_mask_low);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
        //to 16-way 8-bit signed, forward post-increment addressing mode
        PDX_SAPOS_2MX8_FP(outa,outp);//flush

        //last16
        if(nBytesLeft>0){
            outacc=0;
            PDX_MULAQW_2MX16(outacc,mult_acc,last16);
            outacc+=vbias;
            //shift and round
            outacc = PDX_SLS_2MX40(outacc,shift);//saturating left shift, right shift if negative
            //round and shift and saturate
            conv_out = PDX_PACKQSRV_2MX40  (outacc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
            //add output zero point
            conv_out += vOutZP;
            //Saturate to 8 bit range output_activation_min to 127
            conv_out = PDX_MIN_2MX16(conv_out,vmax);
            conv_out = PDX_MAX_2MX16(conv_out,vmin);
            nBytesToWrite = nBytesLeft - 2*PDX_M > 0 ? 2*PDX_M : nBytesLeft;
            nBytesLeft-=2*PDX_M;
            temp_mask = PDX_MOVB_AU32((0b1<<(nBytesToWrite)) - 1);
            save_mask_low = PDX_CVTBB2M_B4M_L(temp_mask);
            PDX_SAV16_2MX8_XP_T(conv_out, outa, outp,nBytesToWrite, temp_mask, save_mask_low);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
            //to 16-way 8-bit signed, forward post-increment addressing mode
            PDX_SAPOS_2MX8_FP(outa,outp);//flush
        }
#endif
        pInputBuffer += (nDepthMult==1)*(nInputWidth*nInputWidth);
    }
}


//FIXME: Change the depthconv to work with interleaved data.
/**
*******************************************************************************
* Function: adi_sharcfx_depthconv2d_stride2_kernel8x10_noninterleaved_int8
* @brief optimized depthconv2d function
*
* @details optimized depthconv2d function for 8-bit integer input. 2D depthwise-convolution in interleaved format using 16bit Eagle intrinsics. Also accounts for padding using a temp buffer
* Assumes input is coming in non-interleaved data format and targets only kernel size of (8,10) and stride of 2
*
* Parameters:
* @param [in] pInputBuffer - input data
* @param [in] pWeightsBuffer - input weights buffer
* @param [in] pBiasBuffer - input bias buffer
* @param [in] nInputWidth - input width
* @param [in] nInputLength - input height
* @param [in] nInChannels - input depth
* @param [in] nOutputWidth - output width
* @param [in] nOutputLength - output height
* @param [in] nOutChannels - output depth
* @param [in] nKernelWidth - kernel width
* @param [in] nKernelLength - kernel height
* @param [in] nPaddingWidth - pad width
* @param [in] nPaddingLength - pad height
* @param [in] pQuantizedMultiplier - multiplier
* @param [in] pQuantizedShift - shift
* @param [in] pInZeroPoint - input zeropoint
* @param [in] pOutZeroPoint - output zeropoint
* @param [in] output_activation_min - min value after activation function
* @param [in] output_activation_max - max value after activation function
* 
* @param [out] pOutputBuffer - output data
*
* @return None
*
*
*******************************************************************************
*/ 
void adi_sharcfx_depthconv2d_stride2_kernel8x10_noninterleaved_int8 (const int8_t *pInputBuffer,
                                                const int8_t *pWeightsBuffer,
                                                const int32_t *pBiasBuffer,
                                                int8_t *pOutputBuffer,
                                                int32_t nInputWidth,
                                                int32_t nInputLength,
                                                int32_t nInChannels,
                                                int32_t nOutputWidth,
                                                int32_t nOutputLength,
                                                int32_t nOutChannels,
                                                int8_t nKernelWidth,
                                                int8_t nKernelLength,
                                                int8_t nPaddingWidth,
                                                int8_t nPaddingLength,
                                                uint32_t *pQuantizedMultiplier,
                                                int32_t *pQuantizedShift,
                                                int32_t pInZeroPoint,
                                                int32_t pOutZeroPoint,
                                                int32_t output_activation_min,
                                                int32_t output_activation_max)
{
    int8_t nStrideLen = 2;

    const immediate Lane = 0;
    const immediate round_mode = 2;

    valign outa = PDX_Z_ALIGN(); // initialize the alignment register vaWrite to zero

    //outbuffer
    xb_vec2Mx8 * outp = (xb_vec2Mx8 *) pOutputBuffer;

    //zeropts and range are global across all channels
    xb_vec2Mx16 vInZP = PDX_REP_2MX16((xb_vec2Mx16)pInZeroPoint,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    xb_vecMx32 vOutZP = PDX_REP_MX32((xb_vecMx32)pOutZeroPoint,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    //load constants for range
    xb_vec2Mx16 vmin = PDX_REP_2MX16((xb_vec2Mx16)output_activation_min,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    xb_vec2Mx16 vmax = PDX_REP_2MX16((xb_vec2Mx16)output_activation_max,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    //load constants for 16bit range
    xb_vec2Mx40 vmax16bit = PDX_REP_2MX40((xb_vec2Mx40)0x7FFF,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    xb_vec2Mx40 vmin16bit = PDX_REP_2MX40((xb_vec2Mx40)0xFFFFFFFFFFFF8000,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    vbool2M greater_than16bit, lesser_than16bit;

    //variables
    xb_vec2Mx16 temp1, temp2;
    xb_vec2Mx16 vInput, vWeights;
    vbool4M temp_mask;
    vbool2M fffe, fffc, ffff;//Masks for row padding
    //FFFF does not mask any lane.
    //FFFE masks only the first lane.
    //FFFC masks the first 2 lanes.
    /*INITIAL PADDING
     * Function has been designed to store one convolution result in one lane of the accumulator.
     * When there is padding to be accounted for, the idea here is to mask the weights of the
     * corresponding input that is supposed to be padded so that it does not contribute to the
     * conv result.
     * Here, padding width(row) is 3 and stride is 2.
     * The first input pixel(weight) will not contribute to the first 2 convolutions, hence the FFFC mask.
     * The second and third pixel(weight) will not contribute to the first convolution, hence the FFFE mask.
     * All the remaining pixels will be a part of the conv result and hence will not be masked.
    */
    //FINAL PADDING is calculated dynamically based on the remaining pixels.

    vbool2M padding_mask;
    xb_vec2Mx40 save_reg;
    xb_vec2Mx16 temp_reg;
    xb_vecMx32 conv_out;
    int nBytesToWrite=0;
    xb_vecMx32 first16, last16;
    xb_vec2Mx40 outacc;
    xb_vecMx80 quant_acc;

    temp_mask = PDX_MOVB_AU32(0xFFFCFFFE);
    PDX_CVTBB2M_B4M(fffc, fffe, temp_mask);
    temp_mask = PDX_MOVB_AU32(0xFFFFFFFF);
    PDX_CVTBB2M_B4M(ffff, ffff, temp_mask);

    for(int nChannels = 0;nChannels < nOutChannels;nChannels++)
    {
        xb_vec2Mx8 * inp = (xb_vec2Mx8 *) (pInputBuffer-nPaddingWidth);//account for col padding
        valign ina = PDX_LA_2MX8_PP (inp);

        int8_t *filter_buff =  (int8_t *)pWeightsBuffer + nPaddingLength*nKernelWidth*nOutChannels + nChannels;//skip padded rows
        int8_t nCurrWt;

        xb_vec2Mx40 vBias = PDX_REP_2MX40((xb_vec2Mx40)pBiasBuffer[nChannels],Lane);

        xb_vecMx32 mult_acc = PDX_REP_MX32(pQuantizedMultiplier[nChannels],Lane);

        const immediate nTotalShift = pQuantizedShift[nChannels];            //Load shift factor for quantization per channel
        xb_vecMx32  shift = PDX_REP_MX32((xb_vecMx32 )(nTotalShift-1),Lane);    //Replicates the lane of data specified, across all lanes of a vector register

        //First row
        //Loops are designed wrt output pixels
        int nBytesLeft = nOutputWidth;
        for(int nCol = 0; nCol<nOutputWidth; nCol+=2*PDX_M)
        {
            outacc=0;
            for(int j=nPaddingLength; j<nKernelLength; j++)//Start from after padding
            {
                for(int k=0; k<nKernelWidth; k++)
                {
                    //load inputs and add input offset
                    PDX_LA16D_4MX8_XP (temp1, temp2, ina, inp, 1 );  //find intrinsic that does not update base addr
                    vInput = PDX_SELI_2MX16 (temp1, temp2, PDX_SELI_16B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes - stride 2
                    vInput += vInZP;

                    //Padding
                    if(nBytesLeft<2*PDX_M){//Final Padding
                        if(k==nKernelWidth-1){temp_mask = PDX_MOVB_AU32((0b1<<(nBytesLeft-2)) - 1);padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);}                            //Masking 2 lanes
                        else if(k==nKernelWidth-2 || k==nKernelWidth-3){temp_mask = PDX_MOVB_AU32((0b1<<(nBytesLeft-1)) - 1);padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);}    //Masking 1 lane
                        else padding_mask = ffff;                                                                                                                            //No masking
                    }
                    else{//Initial Padding
                        if(k==0) padding_mask = fffc;                //Masking 2 lanes
                        else if(k==1 || k==2) padding_mask = fffe;    //Masking 1 lane
                        else padding_mask = ffff;                    //No Masking
                    }
                    //Load weights
                    vWeights=0;
                    nCurrWt = *filter_buff;
                    filter_buff+=nOutChannels;
                    PDX_REP_2MX16_T(vWeights, (xb_vec2Mx16)nCurrWt,Lane, padding_mask);//load current weight and replicate the same in all lanes of the vector

                    //multiply and accumulate
                    PDX_MULAQW_2MX16(outacc,vInput,vWeights);

                }
//                inp += (-nKernelWidth + nInputWidth);//move to next row of inputs
                inp = (xb_vec2Mx8 *) (pInputBuffer-nPaddingWidth + ((j-3)*nInputWidth)+ (nBytesLeft<2*PDX_M)*2*PDX_M*nStrideLen);
                ina = PDX_LA_2MX8_PP (inp);
            }
            //quantization compensation
            outacc+=vBias;//add bias
            PDX_CVT32D_2MX40(last16, first16, outacc);    //Converting 40bit results to 32bit to prevent loss of accuracy from packing of 40bit -> 16bit

            //first16
            quant_acc=0;
            PDX_MULAQW_MX32(quant_acc,mult_acc,first16);    //Multiplying 2 32-bit vectors and storing result in 80bit vector
            //shift and round
            quant_acc = PDX_SLS_MX80(quant_acc,shift);//saturating left shift, right shift if negative
            //round and shift and saturate
            conv_out = PDX_PACKQSRV_MX80   (quant_acc, round_mode);    //pack 80bit result to 32bit with rounding and saturation.
            //add output zero point
            conv_out += vOutZP;
            //Following instrunctions are needed as there is no direct way to typecast 32bit data to 8bit
            save_reg = PDX_CVT40_MX32_L(conv_out);                    //convert to 40bit
            //
            temp_reg =PDX_PACKIV_2MX40  (save_reg, 0);                //pack 40bit data into 16bit vector
            //Saturate to 8 bit range output_activation_min to 127
            temp_reg = PDX_MIN_2MX16(temp_reg,vmax);                //8bit saturation check
            temp_reg = PDX_MAX_2MX16(temp_reg,vmin);                //8bit saturation check
            nBytesToWrite = nBytesLeft - PDX_M > 0 ? PDX_M : nBytesLeft;
            nBytesLeft-=PDX_M;
            temp_mask = PDX_MOVB_AU32((0b1<<(nBytesToWrite)) - 1);
            padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);
            PDX_SAV16_2MX8_XP_T(temp_reg, outa, outp,nBytesToWrite, temp_mask, padding_mask);//Store 16-bit data as 8-bit
            PDX_SAPOS_2MX8_FP(outa,outp);//flush

            if(nBytesLeft>0){
                //last16
                quant_acc=0;
                PDX_MULAQW_MX32(quant_acc,mult_acc,last16);
                //shift and round
                quant_acc = PDX_SLS_MX80(quant_acc,shift);//saturating left shift, right shift if negative
                //round and shift and saturate
                conv_out = PDX_PACKQSRV_MX80   (quant_acc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
                //add output zero point
                conv_out += vOutZP;
                //
                save_reg = PDX_CVT40_MX32_L(conv_out);
                //
                temp_reg =PDX_PACKIV_2MX40  (save_reg, 0);
                //Saturate to 8 bit range output_activation_min to 127
                temp_reg = PDX_MIN_2MX16(temp_reg,vmax);
                temp_reg = PDX_MAX_2MX16(temp_reg,vmin);
                nBytesToWrite = nBytesLeft - PDX_M > 0 ? PDX_M : nBytesLeft;
                nBytesLeft-=PDX_M;
                temp_mask = PDX_MOVB_AU32((0b1<<(nBytesToWrite)) - 1);
                padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);
                PDX_SAV16_2MX8_XP_T(temp_reg, outa, outp,nBytesToWrite, temp_mask, padding_mask);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 16-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_2MX8_FP(outa,outp);//flush
            }
            inp = (xb_vec2Mx8 *) (pInputBuffer-nPaddingWidth+2*PDX_M*nStrideLen);
            ina = PDX_LA_2MX8_PP (inp);
            filter_buff =  (int8_t *)pWeightsBuffer + nPaddingLength*nKernelWidth*nOutChannels + nChannels;//skip padded rows
        }
        //Second row--rest input buffer
        nBytesLeft = nOutputWidth;
        inp = (xb_vec2Mx8 *) (pInputBuffer-nPaddingWidth);//account for col padding
        ina = PDX_LA_2MX8_PP (inp);
        filter_buff =  (int8_t *)(pWeightsBuffer + (nPaddingLength-nStrideLen)*nKernelWidth*nOutChannels)+ nChannels;
        for(int nCol = 0; nCol<nOutputWidth; nCol+=2*PDX_M)
        {
            outacc=0;
            for(int j=nPaddingLength-nStrideLen; j<nKernelLength; j++)//Start from after padding
            {
                for(int k=0; k<nKernelWidth; k++)
                {
                    //load inputs and add input offset
                    PDX_LA16D_4MX8_XP (temp1, temp2, ina, inp, 1 );  //find intrinsic that does not update base addr
                    vInput = PDX_SELI_2MX16 (temp1, temp2, PDX_SELI_16B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes - stride 2
                    vInput += vInZP;

                    //Padding
                    if(nBytesLeft<2*PDX_M){
                        if(k==nKernelWidth-1){temp_mask = PDX_MOVB_AU32((0b1<<(nBytesLeft-2)) - 1);padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);}
                        else if(k==nKernelWidth-2 || k==nKernelWidth-3){temp_mask = PDX_MOVB_AU32((0b1<<(nBytesLeft-1)) - 1);padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);}
                        else padding_mask = ffff;
                    }
                    else{
                        if(k==0) padding_mask = fffc;
                        else if(k==1 || k==2) padding_mask = fffe;
                        else padding_mask = ffff;
                    }
                    //Load weights
                    vWeights=0;
                    nCurrWt = *filter_buff;
                    filter_buff+=nOutChannels;
                    PDX_REP_2MX16_T(vWeights, (xb_vec2Mx16)nCurrWt,Lane, padding_mask);//load current weight and replicate the same in all lanes of the vector

                    //multiply and accumulate
                    PDX_MULAQW_2MX16(outacc,vInput,vWeights);

                }
//                inp += (-nKernelWidth + nInputWidth);//move to next row of inputs
                inp = (xb_vec2Mx8 *) (pInputBuffer-nPaddingWidth + ((j-1)*nInputWidth)+ (nBytesLeft<2*PDX_M)*2*PDX_M*nStrideLen);
                ina = PDX_LA_2MX8_PP (inp);
            }
            //quantization compensation
            outacc+=vBias;//add bias
            PDX_CVT32D_2MX40(last16, first16, outacc);

            //first16
            quant_acc=0;
            PDX_MULAQW_MX32(quant_acc,mult_acc,first16);
            //shift and round
            quant_acc = PDX_SLS_MX80(quant_acc,shift);//saturating left shift, right shift if negative
            //round and shift and saturate
            conv_out = PDX_PACKQSRV_MX80   (quant_acc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
            //add output zero point
            conv_out += vOutZP;
            //
            save_reg = PDX_CVT40_MX32_L(conv_out);
            //
            temp_reg =PDX_PACKIV_2MX40  (save_reg, 0);
            //Saturate to 8 bit range output_activation_min to 127
            temp_reg = PDX_MIN_2MX16(temp_reg,vmax);
            temp_reg = PDX_MAX_2MX16(temp_reg,vmin);
            nBytesToWrite = nBytesLeft - PDX_M > 0 ? PDX_M : nBytesLeft;
            nBytesLeft-=PDX_M;
            temp_mask = PDX_MOVB_AU32((0b1<<(nBytesToWrite)) - 1);
            padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);
            PDX_SAV16_2MX8_XP_T(temp_reg, outa, outp,nBytesToWrite, temp_mask, padding_mask);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
            //to 16-way 8-bit signed, forward post-increment addressing mode
            PDX_SAPOS_2MX8_FP(outa,outp);//flush

            if(nBytesLeft>0){
                //last16
                quant_acc=0;
                PDX_MULAQW_MX32(quant_acc,mult_acc,last16);
                //shift and round
                quant_acc = PDX_SLS_MX80(quant_acc,shift);//saturating left shift, right shift if negative
                //round and shift and saturate
                conv_out = PDX_PACKQSRV_MX80   (quant_acc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
                //add output zero point
                conv_out += vOutZP;
                //
                save_reg = PDX_CVT40_MX32_L(conv_out);
                //
                temp_reg =PDX_PACKIV_2MX40  (save_reg, 0);
                //Saturate to 8 bit range output_activation_min to 127
                temp_reg = PDX_MIN_2MX16(temp_reg,vmax);
                temp_reg = PDX_MAX_2MX16(temp_reg,vmin);
                nBytesToWrite = nBytesLeft - PDX_M > 0 ? PDX_M : nBytesLeft;
                nBytesLeft-=PDX_M;
                temp_mask = PDX_MOVB_AU32((0b1<<(nBytesToWrite)) - 1);
                padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);
                PDX_SAV16_2MX8_XP_T(temp_reg, outa, outp,nBytesToWrite, temp_mask, padding_mask);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 16-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_2MX8_FP(outa,outp);//flush
            }
            inp = (xb_vec2Mx8 *) (pInputBuffer-nPaddingWidth+2*PDX_M*nStrideLen);
            ina = PDX_LA_2MX8_PP (inp);
            filter_buff =  (int8_t *)(pWeightsBuffer + (nPaddingLength-nStrideLen)*nKernelWidth*nOutChannels)+ nChannels;
        }

        //Middle rows
        for(int nOutRow = 2; nOutRow<nOutputLength-3; nOutRow++)
        {
            nBytesLeft = nOutputWidth;
            inp = (xb_vec2Mx8 *) (pInputBuffer-nPaddingWidth + (nOutRow-2)*nInputWidth*nStrideLen);//account for col padding
            ina = PDX_LA_2MX8_PP (inp);
            filter_buff =  (int8_t *)pWeightsBuffer + nChannels;
            for(int nCol = 0; nCol<nOutputWidth; nCol+=2*PDX_M)
            {
                outacc=0;
                for(int j=0; j<nKernelLength; j++)//Start from after padding
                {
                    for(int k=0; k<nKernelWidth; k++)
                    {
                        //load inputs and add input offset
                        PDX_LA16D_4MX8_XP (temp1, temp2, ina, inp, 1 );  //find intrinsic that does not update base addr
                        vInput = PDX_SELI_2MX16 (temp1, temp2, PDX_SELI_16B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes - stride 2
                        vInput += vInZP;

                        //Padding
                        if(nBytesLeft<2*PDX_M){
                            if(k==nKernelWidth-1){temp_mask = PDX_MOVB_AU32((0b1<<(nBytesLeft-2)) - 1);padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);}
                            else if(k==nKernelWidth-2 || k==nKernelWidth-3){temp_mask = PDX_MOVB_AU32((0b1<<(nBytesLeft-1)) - 1);padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);}
                            else padding_mask = ffff;
                        }
                        else{
                            if(k==0) padding_mask = fffc;
                            else if(k==1 || k==2) padding_mask = fffe;
                            else padding_mask = ffff;
                        }
                        //Load weights
                        vWeights=0;
                        nCurrWt = *filter_buff;
                        filter_buff+=nOutChannels;
                        PDX_REP_2MX16_T(vWeights, (xb_vec2Mx16)nCurrWt,Lane, padding_mask);//load current weight and replicate the same in all lanes of the vector

                        //multiply and accumulate
                        PDX_MULAQW_2MX16(outacc,vInput,vWeights);

                    }
    //                inp += (-nKernelWidth + nInputWidth);//move to next row of inputs
                    inp = (xb_vec2Mx8 *) (pInputBuffer-nPaddingWidth + (nOutRow-2)*nInputWidth*nStrideLen + ((j+1)*nInputWidth)+ (nBytesLeft<2*PDX_M)*2*PDX_M*nStrideLen);
                    ina = PDX_LA_2MX8_PP (inp);
                }
                //quantization compensation
                outacc+=vBias;//add bias
                PDX_CVT32D_2MX40(last16, first16, outacc);

                //first16
                quant_acc=0;
                PDX_MULAQW_MX32(quant_acc,mult_acc,first16);
                //shift and round
                quant_acc = PDX_SLS_MX80(quant_acc,shift);//saturating left shift, right shift if negative
                //round and shift and saturate
                conv_out = PDX_PACKQSRV_MX80   (quant_acc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
                //add output zero point
                conv_out += vOutZP;
                //
                save_reg = PDX_CVT40_MX32_L(conv_out);
                //
                temp_reg =PDX_PACKIV_2MX40  (save_reg, 0);
                //Saturate to 8 bit range output_activation_min to 127
                temp_reg = PDX_MIN_2MX16(temp_reg,vmax);
                temp_reg = PDX_MAX_2MX16(temp_reg,vmin);
                nBytesToWrite = nBytesLeft - PDX_M > 0 ? PDX_M : nBytesLeft;
                nBytesLeft-=PDX_M;
                temp_mask = PDX_MOVB_AU32((0b1<<(nBytesToWrite)) - 1);
                padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);
                PDX_SAV16_2MX8_XP_T(temp_reg, outa, outp,nBytesToWrite, temp_mask, padding_mask);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 16-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_2MX8_FP(outa,outp);//flush


                //last16
                if(nBytesLeft>0){
                    quant_acc=0;
                    PDX_MULAQW_MX32(quant_acc,mult_acc,last16);
                    //shift and round
                    quant_acc = PDX_SLS_MX80(quant_acc,shift);//saturating left shift, right shift if negative
                    //round and shift and saturate
                    conv_out = PDX_PACKQSRV_MX80   (quant_acc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
                    //add output zero point
                    conv_out += vOutZP;
                    //
                    save_reg = PDX_CVT40_MX32_L(conv_out);
                    //
                    temp_reg =PDX_PACKIV_2MX40  (save_reg, 0);
                    //Saturate to 8 bit range output_activation_min to 127
                    temp_reg = PDX_MIN_2MX16(temp_reg,vmax);
                    temp_reg = PDX_MAX_2MX16(temp_reg,vmin);
                    nBytesToWrite = nBytesLeft - PDX_M > 0 ? PDX_M : nBytesLeft;
                    nBytesLeft-=PDX_M;
                    temp_mask = PDX_MOVB_AU32((0b1<<(nBytesToWrite)) - 1);
                    padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);
                    PDX_SAV16_2MX8_XP_T(temp_reg, outa, outp,nBytesToWrite, temp_mask, padding_mask);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
                    //to 16-way 8-bit signed, forward post-increment addressing mode
                    PDX_SAPOS_2MX8_FP(outa,outp);//flush
                }
                inp = (xb_vec2Mx8 *) (pInputBuffer-nPaddingWidth + (nOutRow-2)*nInputWidth*nStrideLen + 2*PDX_M*nStrideLen);//account for col padding
                ina = PDX_LA_2MX8_PP (inp);
                filter_buff =  (int8_t *)pWeightsBuffer + nChannels;
            }
        }

        //Second to last row - one row padded at the end
        nBytesLeft = nOutputWidth;
        inp = (xb_vec2Mx8 *) (pInputBuffer-nPaddingWidth + (nOutputLength-3-2)*nInputWidth*nStrideLen);//account for col padding
        ina = PDX_LA_2MX8_PP (inp);
        filter_buff =  (int8_t *)pWeightsBuffer + nChannels;
        for(int nCol = 0; nCol<nOutputWidth; nCol+=2*PDX_M)
        {
            outacc=0;
            for(int j=0; j<nKernelLength-1; j++)//Start from after padding
            {
                for(int k=0; k<nKernelWidth; k++)
                {
                    //load inputs and add input offset
                    PDX_LA16D_4MX8_XP (temp1, temp2, ina, inp, 1 );  //find intrinsic that does not update base addr
                    vInput = PDX_SELI_2MX16 (temp1, temp2, PDX_SELI_16B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes - stride 2
                    vInput += vInZP;

                    //Padding
                    if(nBytesLeft<2*PDX_M){
                        if(k==nKernelWidth-1){temp_mask = PDX_MOVB_AU32((0b1<<(nBytesLeft-2)) - 1);padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);}
                        else if(k==nKernelWidth-2 || k==nKernelWidth-3){temp_mask = PDX_MOVB_AU32((0b1<<(nBytesLeft-1)) - 1);padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);}
                        else padding_mask = ffff;
                    }
                    else{
                        if(k==0) padding_mask = fffc;
                        else if(k==1 || k==2) padding_mask = fffe;
                        else padding_mask = ffff;
                    }
                    //Load weights
                    vWeights=0;
                    nCurrWt = *filter_buff;
                    filter_buff+=nOutChannels;
                    PDX_REP_2MX16_T(vWeights, (xb_vec2Mx16)nCurrWt,Lane, padding_mask);//load current weight and replicate the same in all lanes of the vector

                    //multiply and accumulate
                    PDX_MULAQW_2MX16(outacc,vInput,vWeights);

                }
//                inp += (-nKernelWidth + nInputWidth);//move to next row of inputs
                inp = (xb_vec2Mx8 *) (pInputBuffer-nPaddingWidth + (nOutputLength-3-2)*nInputWidth*nStrideLen + ((j+1)*nInputWidth)+ (nBytesLeft<2*PDX_M)*2*PDX_M*nStrideLen);
                ina = PDX_LA_2MX8_PP (inp);
            }
            //quantization compensation
            outacc+=vBias;//add bias
            PDX_CVT32D_2MX40(last16, first16, outacc);

            //first16
            quant_acc=0;
            PDX_MULAQW_MX32(quant_acc,mult_acc,first16);
            //shift and round
            quant_acc = PDX_SLS_MX80(quant_acc,shift);//saturating left shift, right shift if negative
            //round and shift and saturate
            conv_out = PDX_PACKQSRV_MX80   (quant_acc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
            //add output zero point
            conv_out += vOutZP;
            //
            save_reg = PDX_CVT40_MX32_L(conv_out);
            //
            temp_reg =PDX_PACKIV_2MX40  (save_reg, 0);
            //Saturate to 8 bit range output_activation_min to 127
            temp_reg = PDX_MIN_2MX16(temp_reg,vmax);
            temp_reg = PDX_MAX_2MX16(temp_reg,vmin);
            nBytesToWrite = nBytesLeft - PDX_M > 0 ? PDX_M : nBytesLeft;
            nBytesLeft-=PDX_M;
            temp_mask = PDX_MOVB_AU32((0b1<<(nBytesToWrite)) - 1);
            padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);
            PDX_SAV16_2MX8_XP_T(temp_reg, outa, outp,nBytesToWrite, temp_mask, padding_mask);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
            //to 16-way 8-bit signed, forward post-increment addressing mode
            PDX_SAPOS_2MX8_FP(outa,outp);//flush


            //last16
            if(nBytesLeft>0){
                quant_acc=0;
                PDX_MULAQW_MX32(quant_acc,mult_acc,last16);
                //shift and round
                quant_acc = PDX_SLS_MX80(quant_acc,shift);//saturating left shift, right shift if negative
                //round and shift and saturate
                conv_out = PDX_PACKQSRV_MX80   (quant_acc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
                //add output zero point
                conv_out += vOutZP;
                //
                save_reg = PDX_CVT40_MX32_L(conv_out);
                //
                temp_reg =PDX_PACKIV_2MX40  (save_reg, 0);
                //Saturate to 8 bit range output_activation_min to 127
                temp_reg = PDX_MIN_2MX16(temp_reg,vmax);
                temp_reg = PDX_MAX_2MX16(temp_reg,vmin);
                nBytesToWrite = nBytesLeft - PDX_M > 0 ? PDX_M : nBytesLeft;
                nBytesLeft-=PDX_M;
                temp_mask = PDX_MOVB_AU32((0b1<<(nBytesToWrite)) - 1);
                padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);
                PDX_SAV16_2MX8_XP_T(temp_reg, outa, outp,nBytesToWrite, temp_mask, padding_mask);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 16-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_2MX8_FP(outa,outp);//flush
            }
            inp = (xb_vec2Mx8 *) (pInputBuffer-nPaddingWidth + (nOutputLength-3-2)*nInputWidth*nStrideLen + 2*PDX_M*nStrideLen);//account for col padding
            ina = PDX_LA_2MX8_PP (inp);
            filter_buff =  (int8_t *)pWeightsBuffer + nChannels;
        }

        //Penultimate row
        nBytesLeft = nOutputWidth;
        inp = (xb_vec2Mx8 *) (pInputBuffer-nPaddingWidth + (nOutputLength-4)*nInputWidth*nStrideLen);//account for col padding
        ina = PDX_LA_2MX8_PP (inp);
        filter_buff =  (int8_t *)pWeightsBuffer + nChannels;
        for(int nCol = 0; nCol<nOutputWidth; nCol+=2*PDX_M)
        {
            outacc=0;
            for(int j=0; j<nKernelLength-3; j++)//Start from after padding
            {
                for(int k=0; k<nKernelWidth; k++)
                {
                    //load inputs and add input offset
                    PDX_LA16D_4MX8_XP (temp1, temp2, ina, inp, 1 );  //find intrinsic that does not update base addr
                    vInput = PDX_SELI_2MX16 (temp1, temp2, PDX_SELI_16B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes - stride 2
                    vInput += vInZP;

                    //Padding
                    if(nBytesLeft<2*PDX_M){
                        if(k==nKernelWidth-1){temp_mask = PDX_MOVB_AU32((0b1<<(nBytesLeft-2)) - 1);padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);}
                        else if(k==nKernelWidth-2 || k==nKernelWidth-3){temp_mask = PDX_MOVB_AU32((0b1<<(nBytesLeft-1)) - 1);padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);}
                        else padding_mask = ffff;
                    }
                    else{
                        if(k==0) padding_mask = fffc;
                        else if(k==1 || k==2) padding_mask = fffe;
                        else padding_mask = ffff;
                    }
                    //Load weights
                    vWeights=0;
                    nCurrWt = *filter_buff;
                    filter_buff+=nOutChannels;
                    PDX_REP_2MX16_T(vWeights, (xb_vec2Mx16)nCurrWt,Lane, padding_mask);//load current weight and replicate the same in all lanes of the vector

                    //multiply and accumulate
                    PDX_MULAQW_2MX16(outacc,vInput,vWeights);

                }
//                inp += (-nKernelWidth + nInputWidth);//move to next row of inputs
                inp = (xb_vec2Mx8 *) (pInputBuffer-nPaddingWidth + (nOutputLength-4)*nInputWidth*nStrideLen + ((j+1)*nInputWidth)+ (nBytesLeft<2*PDX_M)*2*PDX_M*nStrideLen);
                ina = PDX_LA_2MX8_PP (inp);
            }
            //quantization compensation
            outacc+=vBias;//add bias
            PDX_CVT32D_2MX40(last16, first16, outacc);

            //first16
            quant_acc=0;
            PDX_MULAQW_MX32(quant_acc,mult_acc,first16);
            //shift and round
            quant_acc = PDX_SLS_MX80(quant_acc,shift);//saturating left shift, right shift if negative
            //round and shift and saturate
            conv_out = PDX_PACKQSRV_MX80   (quant_acc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
            //add output zero point
            conv_out += vOutZP;
            //
            save_reg = PDX_CVT40_MX32_L(conv_out);
            //
            temp_reg =PDX_PACKIV_2MX40  (save_reg, 0);
            //Saturate to 8 bit range output_activation_min to 127
            temp_reg = PDX_MIN_2MX16(temp_reg,vmax);
            temp_reg = PDX_MAX_2MX16(temp_reg,vmin);
            nBytesToWrite = nBytesLeft - PDX_M > 0 ? PDX_M : nBytesLeft;
            nBytesLeft-=PDX_M;
            temp_mask = PDX_MOVB_AU32((0b1<<(nBytesToWrite)) - 1);
            padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);
            PDX_SAV16_2MX8_XP_T(temp_reg, outa, outp,nBytesToWrite, temp_mask, padding_mask);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
            //to 16-way 8-bit signed, forward post-increment addressing mode
            PDX_SAPOS_2MX8_FP(outa,outp);//flush

            if(nBytesLeft>0){
                //last16
                quant_acc=0;
                PDX_MULAQW_MX32(quant_acc,mult_acc,last16);
                //shift and round
                quant_acc = PDX_SLS_MX80(quant_acc,shift);//saturating left shift, right shift if negative
                //round and shift and saturate
                conv_out = PDX_PACKQSRV_MX80   (quant_acc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
                //add output zero point
                conv_out += vOutZP;
                //
                save_reg = PDX_CVT40_MX32_L(conv_out);
                //
                temp_reg =PDX_PACKIV_2MX40  (save_reg, 0);
                //Saturate to 8 bit range output_activation_min to 127
                temp_reg = PDX_MIN_2MX16(temp_reg,vmax);
                temp_reg = PDX_MAX_2MX16(temp_reg,vmin);
                nBytesToWrite = nBytesLeft - PDX_M > 0 ? PDX_M : nBytesLeft;
                nBytesLeft-=PDX_M;
                temp_mask = PDX_MOVB_AU32((0b1<<(nBytesToWrite)) - 1);
                padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);
                PDX_SAV16_2MX8_XP_T(temp_reg, outa, outp,nBytesToWrite, temp_mask, padding_mask);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 16-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_2MX8_FP(outa,outp);//flush
            }
            inp = (xb_vec2Mx8 *) (pInputBuffer-nPaddingWidth + (nOutputLength-4)*nInputWidth*nStrideLen + 2*PDX_M*nStrideLen);//account for col padding
            ina = PDX_LA_2MX8_PP (inp);
            filter_buff =  (int8_t *)pWeightsBuffer + nChannels;
        }
        //Last row
        nBytesLeft = nOutputWidth;
        inp = (xb_vec2Mx8 *) (pInputBuffer-nPaddingWidth + (nOutputLength-3)*nInputWidth*nStrideLen);//account for col padding
        ina = PDX_LA_2MX8_PP (inp);
        filter_buff =  (int8_t *)pWeightsBuffer + nChannels;
        for(int nCol = 0; nCol<nOutputWidth; nCol+=2*PDX_M)
        {
            outacc=0;
            for(int j=0; j<nKernelLength-5; j++)//Start from after padding
            {
                for(int k=0; k<nKernelWidth; k++)
                {
                    //load inputs and add input offset
                    PDX_LA16D_4MX8_XP (temp1, temp2, ina, inp, 1 );  //find intrinsic that does not update base addr
                    vInput = PDX_SELI_2MX16 (temp1, temp2, PDX_SELI_16B_EXTRACT_1_OF_2_OFF_0);// Get all the even bytes - stride 2
                    vInput += vInZP;

                    //Padding
                    if(nBytesLeft<2*PDX_M){
                        if(k==nKernelWidth-1){temp_mask = PDX_MOVB_AU32((0b1<<(nBytesLeft-2)) - 1);padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);}
                        else if(k==nKernelWidth-2 || k==nKernelWidth-3){temp_mask = PDX_MOVB_AU32((0b1<<(nBytesLeft-1)) - 1);padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);}
                        else padding_mask = ffff;
                    }
                    else{
                        if(k==0) padding_mask = fffc;
                        else if(k==1 || k==2) padding_mask = fffe;
                        else padding_mask = ffff;
                    }
                    //Load weights
                    vWeights=0;
                    nCurrWt = *filter_buff;
                    filter_buff+=nOutChannels;
                    PDX_REP_2MX16_T(vWeights, (xb_vec2Mx16)nCurrWt,Lane, padding_mask);//load current weight and replicate the same in all lanes of the vector

                    //multiply and accumulate
                    PDX_MULAQW_2MX16(outacc,vInput,vWeights);

                }
//                inp += (-nKernelWidth + nInputWidth);//move to next row of inputs
                inp = (xb_vec2Mx8 *) (pInputBuffer-nPaddingWidth + (nOutputLength-3)*nInputWidth*nStrideLen + ((j+1)*nInputWidth)+ (nBytesLeft<2*PDX_M)*2*PDX_M*nStrideLen);
                ina = PDX_LA_2MX8_PP (inp);
            }
            //quantization compensation
            outacc+=vBias;//add bias
            PDX_CVT32D_2MX40(last16, first16, outacc);

            //first16
            quant_acc=0;
            PDX_MULAQW_MX32(quant_acc,mult_acc,first16);
            //shift and round
            quant_acc = PDX_SLS_MX80(quant_acc,shift);//saturating left shift, right shift if negative
            //round and shift and saturate
            conv_out = PDX_PACKQSRV_MX80   (quant_acc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
            //add output zero point
            conv_out += vOutZP;
            //
            save_reg = PDX_CVT40_MX32_L(conv_out);
            //
            temp_reg =PDX_PACKIV_2MX40  (save_reg, 0);
            //Saturate to 8 bit range output_activation_min to 127
            temp_reg = PDX_MIN_2MX16(temp_reg,vmax);
            temp_reg = PDX_MAX_2MX16(temp_reg,vmin);
            nBytesToWrite = nBytesLeft - PDX_M > 0 ? PDX_M : nBytesLeft;
            nBytesLeft-=PDX_M;
            temp_mask = PDX_MOVB_AU32((0b1<<(nBytesToWrite)) - 1);
            padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);
            PDX_SAV16_2MX8_XP_T(temp_reg, outa, outp,nBytesToWrite, temp_mask, padding_mask);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
            //to 16-way 8-bit signed, forward post-increment addressing mode
            PDX_SAPOS_2MX8_FP(outa,outp);//flush

            if(nBytesLeft>0){
                //last16
                quant_acc=0;
                PDX_MULAQW_MX32(quant_acc,mult_acc,last16);
                //shift and round
                quant_acc = PDX_SLS_MX80(quant_acc,shift);//saturating left shift, right shift if negative
                //round and shift and saturate
                conv_out = PDX_PACKQSRV_MX80   (quant_acc, round_mode);//shifts by 16 bits and symmetric rounding with saturation
                //add output zero point
                conv_out += vOutZP;
                //
                save_reg = PDX_CVT40_MX32_L(conv_out);
                //
                temp_reg =PDX_PACKIV_2MX40  (save_reg, 0);
                //Saturate to 8 bit range output_activation_min to 127
                temp_reg = PDX_MIN_2MX16(temp_reg,vmax);
                temp_reg = PDX_MAX_2MX16(temp_reg,vmin);
                nBytesToWrite = nBytesLeft - PDX_M > 0 ? PDX_M : nBytesLeft;
                nBytesLeft-=PDX_M;
                temp_mask = PDX_MOVB_AU32((0b1<<(nBytesToWrite)) - 1);
                padding_mask = PDX_CVTBB2M_B4M_L(temp_mask);
                PDX_SAV16_2MX8_XP_T(temp_reg, outa, outp,nBytesToWrite, temp_mask, padding_mask);//16-way 16-bit signed Aligning vector register variable-length store intrinsic, converting
                //to 16-way 8-bit signed, forward post-increment addressing mode
                PDX_SAPOS_2MX8_FP(outa,outp);//flush
            }
            inp = (xb_vec2Mx8 *) (pInputBuffer-nPaddingWidth + (nOutputLength-3)*nInputWidth*nStrideLen+ 2*PDX_M*nStrideLen);//account for col padding
            ina = PDX_LA_2MX8_PP (inp);
            filter_buff =  (int8_t *)pWeightsBuffer + nChannels;
        }
    }
}
