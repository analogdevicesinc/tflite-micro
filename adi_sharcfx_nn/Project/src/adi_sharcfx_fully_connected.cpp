/**
********************************************************************************
*
* @file: adi_sharcfx_fully_connected.cpp
*
* @brief: contains optimized version of fully connected layer
*
* @details: contains optimized version of fully connected layer or 8bit and 16bit integer input
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

/*============= C O D E =============*/


void adi_sharcfx_fully_connected_int16(const int16_t* pInputBuffer,
                                       const int8_t* pWeightsBuffer,
                                       const int32_t* pBiasBuffer,
                                       int16_t* pOutputBuffer,
                                       int32_t nFilterDepth,
                                       int32_t nOutsize,
                                       int32_t nBatches,
                                       uint32_t nQuantizedMultiplier,
                                       int32_t nQuantizedShift,
                                       int32_t nInputOffset,
                                       int32_t nFilterOffset,
                                       int32_t nOutputOffset,
                                       int32_t output_activation_min,
                                       int32_t output_activation_max)
{
    int16_t* __restrict outp = (int16_t*) pOutputBuffer;

    const immediate Lane=0;
    //Defining the input and filter offsets
    xb_vec2Mx16 vInZP = PDX_REP_2MX16((xb_vec2Mx16)nInputOffset,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    xb_vec2Mx16 vFilterZP = PDX_REP_2MX16((xb_vec2Mx16)nFilterOffset,Lane);//Replicates the lane of data specified, across all lanes of a vector register

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

    if(nFilterDepth % 16)
    {
        int32_t nFilterDepth_16mul = nFilterDepth - (nFilterDepth % 16);
        for (int b = 0; b < nBatches; b++)
        {
            //No of filter is equals to the nOutsize
            for (int32_t nChannelCnt = 0; nChannelCnt < nOutsize; nChannelCnt++)
            {
                acc = 0;//reset accumulator

                xb_vec2Mx16 *inp = (xb_vec2Mx16 *)(pInputBuffer + b*nFilterDepth);    //Reinitialise input pointer for each output pixel
                valign ina; // define align vector
                ina=PDX_LA_2MX16_PP (inp); // prime, NOP if a[] is aligned

                xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + nFilterDepth*nChannelCnt); //Move to next filter for each output pixel
                valign wta;    // define align vector
                wta=PDX_LA_2MX8_PP (wtp);

                //for all the input pixels
                for (nPixProcessed= 0; nFilterDepth - nPixProcessed > 2*PDX_M; nPixProcessed += (2*PDX_M))
                {
                    PDX_LA_2MX16_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
                    PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
                    vin+=vInZP;        //Add input offset
                    vwt+=vFilterZP;    //Add filter offset

                    PDX_MULAQW_2MX16(acc,vwt,vin);
                }

                PDX_LA_2MX16_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
                PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
                vin+=vInZP;        //Add input offset
                vwt+=vFilterZP;    //Add filter offset

                temp_mask = PDX_MOVB_AU32((0b1<<(nFilterDepth % 16)) - 1);
                acc_mask = PDX_CVTBB2M_B4M_L(temp_mask);
                //multiply and accumulate
                PDX_MULAQW_2MX16_T(acc,vwt,vin,acc_mask);

                sat_sum = PDX_RADD_2MX40(acc);                                    //PDX_RADD_2MX40: Adds across all 16lanes of acc and returns sum
                //adding bias<<1 to compensate for sign bit during multiplication
                //doubling index for bias buffer to account for 32bit bias instead of 64bit
                if(pBiasBuffer)
                {
                    sat_sum += (xb_int40)(pBiasBuffer[nChannelCnt*2]<<1);
                }
                temp = (xb_int32)((int32_t)((int64_t)PDX_CVT64_40(sat_sum)));    //convert 40bit var into 32bit to perform multiplication

                product = PDX_MULW_32(temp, (uint32_t)nQuantizedMultiplier);    //multiply with the quantization multiplier; product(80bit) = temp(32bit) * nQuantizedMultiplier(32bit)
                product =  PDX_SLA_80(product, (xb_int32)nQuantizedShift);        //shift result by quantization multiplier
                temp = PDX_PACKQSRV_80(product,2);                                //packs 80bit product into 32bit var with saturation and rounding
                temp+= (xb_int32)nOutputOffset;                                    //add output offset
                temp = MIN(temp, (xb_int32)output_activation_max);
                temp = MAX(temp, (xb_int32)output_activation_min);//saturation check to store result
                *outp++ =(int16_t)((int32_t)temp);                                //store result as 16-bit data
            }
        }
    }
    else{
        for (int b = 0; b < nBatches; b++)
        {
            //No of filter is equals to the nOutsize
            for (int32_t nChannelCnt = 0; nChannelCnt < nOutsize; nChannelCnt++)
            {
                acc = 0;//reset accumulator

                xb_vec2Mx16 * inp = (xb_vec2Mx16 *)(pInputBuffer + b*nFilterDepth);    //Reinitialise input pointer for each output pixel
                valign ina; // define align vector
                ina=PDX_LA_2MX16_PP (inp); // prime, NOP if a[] is aligned

                xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + nFilterDepth*nChannelCnt*2); //Move to next filter for each output pixel
                valign wta;    // define align vector
                wta=PDX_LA_2MX8_PP (wtp);

                //for all the input pixels
                for (int32_t nFilterCnt = 0; nFilterCnt < nFilterDepth; nFilterCnt += (2*PDX_M))
                {
                    PDX_LA_2MX16_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
                    PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M);
                    vin+=vInZP;        //Add input offset
                    vwt+=vFilterZP;    //Add filter offset

                    PDX_MULAQW_2MX16(acc,vwt,vin);
                }
                sat_sum = PDX_RADD_2MX40(acc);                                    //PDX_RADD_2MX40: Adds across all 16lanes of acc and returns sum
                //adding bias<<1 to compensate for sign bit during multiplication
                //doubling index for bias buffer to account for 32bit bias instead of 64bit
                if(pBiasBuffer)
                {
                    sat_sum += (xb_int40)(pBiasBuffer[nChannelCnt*2]<<1);
                }
                temp = (xb_int32)((int32_t)((int64_t)PDX_CVT64_40(sat_sum)));    //convert 40bit var into 32bit to perform multiplication

                product = PDX_MULW_32(temp, (uint32_t)nQuantizedMultiplier);    //multiply with the quantization multiplier; product(80bit) = temp(32bit) * nQuantizedMultiplier(32bit)
                product =  PDX_SLA_80(product, (xb_int32)nQuantizedShift);        //shift result by quantization multiplier
                temp = PDX_PACKQSRV_80(product,2);                                //packs 80bit product into 32bit var with saturation and rounding
                temp+= (xb_int32)nOutputOffset;                                    //add output offset
                temp = MIN(temp, (xb_int32)output_activation_max);
                temp = MAX(temp, (xb_int32)output_activation_min);				//saturation check to store result
                *outp++ =(int16_t)((int32_t)temp);                                //store result as 16-bit data
            }
        }
    }
}

void adi_sharcfx_fully_connected_int8(const int8_t* pInputBuffer,
                                      const int8_t* pWeightsBuffer,
                                      const int32_t* pBiasBuffer,
                                      int8_t* pOutputBuffer,
                                      int32_t nFilterDepth,
                                      int32_t nOutsize,
                                      int32_t nBatches,
                                      uint32_t nQuantizedMultiplier,
                                      int32_t nQuantizedShift,
                                      int32_t nInputOffset,
                                      int32_t nFilterOffset,
                                      int32_t nOutputOffset,
                                      int32_t output_activation_min,
                                      int32_t output_activation_max)
{
    int8_t* __restrict outp = (int8_t*) pOutputBuffer;

    const immediate Lane=0;
    //Defining the input and filter offsets
    xb_vec2Mx16 vInZP = PDX_REP_2MX16((xb_vec2Mx16)nInputOffset,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    xb_vec2Mx16 vFilterZP = PDX_REP_2MX16((xb_vec2Mx16)nFilterOffset,Lane);//Replicates the lane of data specified, across all lanes of a vector register

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
    if(nFilterDepth % 16){
        int32_t nFilterDepth_16mul = nFilterDepth - (nFilterDepth % 16);
        for (int b = 0; b < nBatches; b++){
            //No of filter is equals to the nOutsize
            for (int32_t nChannelCnt = 0; nChannelCnt < nOutsize; nChannelCnt++)
            {
                acc = 0;//reset accumulator

                inp = (xb_vec2Mx8 *)(pInputBuffer + b*nFilterDepth);    //Reinitialise input pointer for each output pixel
                valign ina; // define align vector
                ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

                xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + nFilterDepth*nChannelCnt); //Move to next filter for each output pixel
                valign wta;    // define align vector
                wta=PDX_LA_2MX8_PP (wtp);

                //for all the input pixels
                for (nPixProcessed= 0; nPixProcessed < nFilterDepth_16mul; nPixProcessed += (2*PDX_M))
                {
                    PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
                    PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
                    vin+=vInZP;        //Add input offset
                    vwt+=vFilterZP;    //Add filter offset

                    PDX_MULAQW_2MX16(acc,vwt,vin);

                }

                PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
                PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
                vin+=vInZP;        //Add input offset
                vwt+=vFilterZP;    //Add filter offset

                temp_mask = PDX_MOVB_AU32((0b1<<((nFilterDepth % 16))) - 1);
                acc_mask = PDX_CVTBB2M_B4M_L(temp_mask);
                //multiply and accumulate
                PDX_MULAQW_2MX16_T(acc,vwt,vin,acc_mask);

                sat_sum = PDX_RADD_2MX40(acc);                                    //PDX_RADD_2MX40: Adds across all 16lanes of acc and returns sum
                if(pBiasBuffer)
                {
                    //adding bias>>1 to compensate for sign bit during multiplication
                    sat_sum += (xb_int40)(pBiasBuffer[nChannelCnt*2]<<1);
                }
                temp = (xb_int32)((int32_t)((int64_t)PDX_CVT64_40(sat_sum)));    //convert 40bit var into 32bit to perform multiplication

                product = PDX_MULW_32(temp, (uint32_t)nQuantizedMultiplier);    //multiply with the quantization multiplier; product(80bit) = temp(32bit) * nQuantizedMultiplier(32bit)
                product =  PDX_SLA_80(product, (xb_int32)nQuantizedShift);        //shift result by quantization multiplier
                temp = PDX_PACKQSRV_80(product,2);                                //packs 80bit product into 32bit var with saturation and rounding
                temp+= (xb_int32)nOutputOffset;                                    //add output offset
                temp = MIN(temp, (xb_int32)output_activation_max);
                temp = MAX(temp, (xb_int32)output_activation_min);//8bit saturation check to store result
                *outp++ =(int8_t)((int32_t)temp);                                //store result as 8-bit data
            }
        }
    }
    else{
        for (int b = 0; b < nBatches; b++){
            //No of filter is equals to the nOutsize
            for (int32_t nChannelCnt = 0; nChannelCnt < nOutsize; nChannelCnt++)
            {
                acc = 0;//reset accumulator

                inp = (xb_vec2Mx8 *)(pInputBuffer + b*nFilterDepth);    //Reinitialise input pointer for each output pixel
                valign ina; // define align vector
                ina=PDX_LA_2MX8_PP (inp); // prime, NOP if a[] is aligned

                xb_vec2Mx8 *wtp = (xb_vec2Mx8 *) (pWeightsBuffer + nFilterDepth*nChannelCnt); //Move to next filter for each output pixel
                valign wta;    // define align vector
                wta=PDX_LA_2MX8_PP (wtp);

                //for all the input pixels
                for (int32_t nFilterCnt = 0; nFilterCnt < nFilterDepth; nFilterCnt += (2*PDX_M))
                {
                    PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M); // load aligned, extend
                    PDX_LA16_2MX8_XP (vwt, wta, wtp, 2*PDX_M); // load aligned, extend
                    vin+=vInZP;        //Add input offset
                    vwt+=vFilterZP;    //Add filter offset

                    PDX_MULAQW_2MX16(acc,vwt,vin);

                }
                sat_sum = PDX_RADD_2MX40(acc);                                    //PDX_RADD_2MX40: Adds across all 16lanes of acc and returns sum
                if(pBiasBuffer)
                {
                    //adding bias>>1 to compensate for sign bit during multiplication
                    sat_sum += (xb_int40)(pBiasBuffer[nChannelCnt*2]<<1);
                }
                temp = (xb_int32)((int32_t)((int64_t)PDX_CVT64_40(sat_sum)));    //convert 40bit var into 32bit to perform multiplication

                product = PDX_MULW_32(temp, (uint32_t)nQuantizedMultiplier);    //multiply with the quantization multiplier; product(80bit) = temp(32bit) * nQuantizedMultiplier(32bit)
                product =  PDX_SLA_80(product, (xb_int32)nQuantizedShift);        //shift result by quantization multiplier
                temp = PDX_PACKQSRV_80(product,2);                                //packs 80bit product into 32bit var with saturation and rounding
                temp+= (xb_int32)nOutputOffset;                                    //add output offset
                temp = MIN(temp, (xb_int32)output_activation_max);
                temp = MAX(temp, (xb_int32)output_activation_min);//8bit saturation check to store result
                *outp++ =(int8_t)((int32_t)temp);                                //store result as 8-bit data
            }
        }
    }
}
