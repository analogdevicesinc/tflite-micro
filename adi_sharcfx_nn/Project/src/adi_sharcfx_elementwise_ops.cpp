/**
********************************************************************************
*
* @file: adi_sharcfx_elementwise_ops.cpp
*
* @brief: contains optimized version of elementwise add and multiply
*
* @details: contains optimized version of elementwise add and multiply for int16 input data
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

/**
*******************************************************************************
* Function: adi_sharcfx_elementwise_mul_int16
* @brief vectorised elementwise multiplication for 16-bit integer input
*
* @details vectorised elementwise multiplication for 16-bit integer input
*
* Parameters:
* @param [in] pInput1 - input buffer 1 (16-bit)
* @param [in] pInput2 - input buffer 2 (16-bit)
* @param [in] nSize - input size
* @param [in] nQuantizedMultiplier - multiplier, corresponds to TFLM quantization scheme
* @param [in] nQuantizedShift - shift, corresponds to TFLM quantization scheme
* @param [in] nInOffset1 - input offset for input buffer 1
* @param [in] nInOffset2 - input offset for input buffer 2
* @param [in] nOutOffset - kernel width
* @param [in] output_activation_min - min value after activation function 
* @param [in] output_activation_max - max value after activation function
* 
* @param [out] pOutput - output data (16-bit)
*
* @return None
*
*
*******************************************************************************
*/ 
void adi_sharcfx_elementwise_mul_int16(const int16_t* pInput1,
                                       const int16_t* pInput2,
                                       int16_t* pOutput,
                                       int32_t nSize,
                                       uint32_t nQuantizedMultiplier,
                                       int32_t nQuantizedShift,
                                       int32_t nInOffset1,
                                       int32_t nInOffset2,
                                       int32_t nOutOffset,
                                       int32_t output_activation_min,
                                       int32_t output_activation_max)
{
    const immediate Lane = 0;
    const immediate round_mode = 2;

    xb_vec2Mx16 vInOff1 = PDX_REP_2MX16((xb_vec2Mx16)nInOffset1,Lane);
    xb_vec2Mx16 vInOff2 = PDX_REP_2MX16((xb_vec2Mx16)nInOffset2,Lane);
    xb_vec2Mx16 vOutOff = PDX_REP_2MX16((xb_vec2Mx16)nOutOffset,Lane);

    xb_vec2Mx16 vmin = PDX_REP_2MX16((xb_vec2Mx16)output_activation_min,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    xb_vec2Mx16 vmax = PDX_REP_2MX16((xb_vec2Mx16)output_activation_max,Lane);//Replicates the lane of data specified, across all lanes of a vector register

    xb_vec2Mx16 *inp1 = (xb_vec2Mx16 *)pInput1;
    xb_vec2Mx16 *inp2 = (xb_vec2Mx16 *)pInput2;
    valign ina1=PDX_LA_2MX16_PP (inp1);// define align vector
    valign ina2=PDX_LA_2MX16_PP (inp2);// define align vector

    valign outa = PDX_Z_ALIGN();
    xb_vec2Mx16 *outp = (xb_vec2Mx16 *)pOutput;

    xb_vec2Mx16 shift = PDX_REP_2MX16((xb_vec2Mx16)nQuantizedShift,Lane);
    xb_vec2Mx16 multiplier = PDX_REP_2MX16((xb_vec2Mx16)((nQuantizedMultiplier + (1 << 15)) >> 16),Lane);

    xb_vec2Mx16 vin1, vin2, product;
    xb_vec2Mx40 acc;

    int nElementsProcessed;
    for (nElementsProcessed = 0; (nSize - nElementsProcessed) >= (2*PDX_M);)
    {
        //reset acc
        acc=0;
        //load inputs
        PDX_LA_2MX16_XP (vin1, ina1, inp1, 2*PDX_M * sizeof(int16_t));
        PDX_LA_2MX16_XP (vin2, ina2, inp2, 2*PDX_M * sizeof(int16_t));

        //add offsets
        vin1+=vInOff1;
        vin2+=vInOff2;
        //multiply
        PDX_MULAQW_2MX16(acc, vin1, vin2);
        //pack to 16bit
        product = PDX_PACKQSRV_2MX40(acc, round_mode);
        //reset acc
        acc=0;
        //multiply by multipler
        PDX_MULAW_2MX16(acc, multiplier, product);
        //shift
        acc = PDX_SLS_2MX40(acc,shift);
        //saturate and save to 16bit
        product = PDX_PACKSIV_2MX40(acc, 0);
        //add offset
        product +=vOutOff;
        //saturate
        product = PDX_MIN_2MX16(product,vmax);
        product = PDX_MAX_2MX16(product,vmin);
        //save product and flush vector
        PDX_SAV_2MX16_XP(product,outa,outp, 2*PDX_M * sizeof(int16_t));
        PDX_SAPOS_2MX16_FP(outa,outp);//flush
        nElementsProcessed+=(2*PDX_M);
    }
    if((nSize - nElementsProcessed)>0)
    {
        //reset acc
        acc=0;
        //load inputs
        PDX_LA_2MX16_XP (vin1, ina1, inp1,0);
        PDX_LA_2MX16_XP (vin2, ina2, inp2,0);

        //add offsets
        vin1+=vInOff1;
        vin2+=vInOff2;
        //multiply
        PDX_MULAQW_2MX16(acc, vin1, vin2);
        //pack to 16bit
        product = PDX_PACKQSRV_2MX40(acc, round_mode);
        //reset acc
        acc=0;
        //multiply by multipler
        PDX_MULAW_2MX16(acc, multiplier, product);
        //shift
        acc = PDX_SLS_2MX40(acc,shift);
        //saturate and save to 16bit
        product = PDX_PACKSIV_2MX40(acc, 0);
        //add offset
        product +=vOutOff;
        //saturate
        product = PDX_MIN_2MX16(product,vmax);
        product = PDX_MAX_2MX16(product,vmin);
        //save product and flush vector
        PDX_SAV_2MX16_XP(product,outa,outp, (nSize - nElementsProcessed)* sizeof(int16_t));
        PDX_SAPOS_2MX16_FP(outa,outp);//flush
    }
}

/**
*******************************************************************************
* Function: adi_sharcfx_elementwise_add_int16
* @brief vectorised elementwise addition for 16-bit integer input
*
* @details vectorised elementwise addition for 16-bit integer input
*
* Parameters:
* @param [in] pInput1 - input buffer 1 (16-bit)
* @param [in] pInput2 - input buffer 2 (16-bit)
* @param [in] nBatches - number of batches
* @param [in] nInputLen - input size
* 
* @param [out] pOutput - output data (16-bit)
*
* @return None
*
*
*******************************************************************************
*/ 
void adi_sharcfx_elementwise_add_int16(const int16_t* pInput1,
                                       const int16_t* pInput2,
                                       int32_t nBatches,
                                       int32_t nInputLen,
                                       int16_t* pOutput)
{
    xb_vec2Mx40 acc = 0;
    xb_vec2Mx16 *inp1 = (xb_vec2Mx16 *)pInput1;
    xb_vec2Mx16 *inp2 = (xb_vec2Mx16 *)pInput2;
    valign ina1, ina2; // define align vector
    ina1=PDX_LA_2MX16_PP (inp1);
    ina2=PDX_LA_2MX16_PP (inp2);

    valign outa = PDX_Z_ALIGN();
    xb_vec2Mx16 *outp = (xb_vec2Mx16 *)pOutput;
//    valign outa = PDX_LA_2MX16_PP (outp);

    xb_vec2Mx16 vin1, vin2, sum;

    int nPixLeft = nInputLen;

    if(nInputLen%16)
    {
        for (int batch = 0; batch < nBatches; batch++) 
        {
            //reset input pointer for each batch
            inp1 = (xb_vec2Mx16 *)(pInput1 + batch*nInputLen);
            inp2 = (xb_vec2Mx16 *)(pInput2 + batch*nInputLen);
            for (nPixLeft = nInputLen; nPixLeft > 2*PDX_M; )
            {
                //load input
                PDX_LA_2MX16_XP (vin1, ina1, inp1, 2*PDX_M* sizeof(int16_t));
                PDX_LA_2MX16_XP (vin2, ina2, inp2, 2*PDX_M* sizeof(int16_t));
                //add with saturation
                sum = PDX_ADDS_2MX16(vin1, vin2);
                //save sum and flush vector
                PDX_SAV_2MX16_XP(sum,outa,outp, 2*PDX_M* sizeof(int16_t));
                PDX_SAPOS_2MX16_FP(outa,outp);//flush
                nPixLeft-= (2*PDX_M);
            }
            //load input
            PDX_LA_2MX16_XP (vin1, ina1, inp1, 2*PDX_M* sizeof(int16_t));
            PDX_LA_2MX16_XP (vin2, ina2, inp2, 2*PDX_M* sizeof(int16_t));
            //add with saturation
            sum = PDX_ADDS_2MX16(vin1, vin2);
            //save sum and flush vector
            PDX_SAV_2MX16_XP(sum,outa,outp, nPixLeft* sizeof(int16_t));
            PDX_SAPOS_2MX16_FP(outa,outp);//flush
        }
    }
    else
    {
        for (int batch = 0; batch < nBatches; batch++) 
        {
            //reset input pointer for each batch
            inp1 = (xb_vec2Mx16 *)(pInput1 + batch*nInputLen);
            inp2 = (xb_vec2Mx16 *)(pInput2 + batch*nInputLen);
            for (int i = 0; i < nInputLen; i+= (2*PDX_M))
            {
                //load input
                PDX_LA_2MX16_XP (vin1, ina1, inp1, 2*PDX_M* sizeof(int16_t));
                PDX_LA_2MX16_XP (vin2, ina2, inp2, 2*PDX_M* sizeof(int16_t));
                //add with saturation
                sum = PDX_ADDS_2MX16(vin1, vin2);
                //save sum and flush vector
                PDX_SAV_2MX16_XP(sum,outa,outp, 2*PDX_M* sizeof(int16_t));
                PDX_SAPOS_2MX16_FP(outa,outp);//flush
            }
        }
    }
}
