/**
********************************************************************************
*
* @file: adi_sharcfx_activations.cpp
*
* @brief: Contains optimized Relu, Tanh and Logistic functions
*
* @details: Contains the optimized Relu and Logistic activation functions for int8 input data and optimized Logictic and Tanh activation functions for int16 input data
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
* Function: adi_sharcfx_relu_int8
* @brief optimized implementation of Relu activation function
*
* @details optimized implementation of Relu activation function for int8 data
*
* Parameters:
* @param [in] pInput - input buffer
* @param [in] nSize - input height
* @param [in] nQuantizedMultiplier - multiplier, corresponds to TFLM quantization scheme
* @param [in] nQuantizedShift - shift, corresponds to TFLM quantization scheme
* @param [in] nInOffset - input offset, corresponds to TFLM quantization scheme
* @param [in] nOutOffset - output offset, corresponds to TFLM quantization scheme
* @param [in] output_activation_min - min value of activation function
* @param [in] output_activation_max - max value of activation function
*
* @param [out] pOutput - output buffer
*
* @return None
*
*******************************************************************************
*/ 
void adi_sharcfx_relu_int8(const int8_t* pInput,
                           int8_t* pOutput,
                           const uint32_t nSize,
                           uint32_t nQuantizedMultiplier,
                           int32_t nQuantizedShift,
                           int32_t nInOffset,
                           int32_t nOutOffset,
                           int32_t output_activation_min,
                           int32_t output_activation_max)
{
    const immediate Lane = 0;
    const immediate round_mode = 2;

    xb_vec2Mx16 vInOff = PDX_REP_2MX16((xb_vec2Mx16)nInOffset,Lane);
    xb_vec2Mx16 vOutOff = PDX_REP_2MX16((xb_vec2Mx16)nOutOffset,Lane);

    xb_vec2Mx16 vmin = PDX_REP_2MX16((xb_vec2Mx16)output_activation_min,Lane);//Replicates the lane of data specified, across all lanes of a vector register
    xb_vec2Mx16 vmax = PDX_REP_2MX16((xb_vec2Mx16)output_activation_max,Lane);//Replicates the lane of data specified, across all lanes of a vector register

    xb_vec2Mx8 *inp = (xb_vec2Mx8 *)pInput;
    valign ina=PDX_LA_2MX8_PP (inp);// define align vector

    valign outa = PDX_Z_ALIGN();
    xb_vec2Mx8 *outp = (xb_vec2Mx8 *)pOutput;

    xb_vec2Mx16 shift = PDX_REP_2MX16((xb_vec2Mx16)(nQuantizedShift+1),Lane);
    xb_vec2Mx16 multiplier = PDX_REP_2MX16((xb_vec2Mx16)((nQuantizedMultiplier + (1 << 15)) >> 16),Lane);

    xb_vec2Mx16 vin, result;
    xb_vec2Mx40 acc;

    int nElementsProcessed;
    for (nElementsProcessed = 0; (nSize - nElementsProcessed) >= (2*PDX_M);) {
        //load inputs
        PDX_LA16_2MX8_XP (vin, ina, inp, 2*PDX_M );
        vin-=vInOff;
        //reset acc
        acc=0;
        //multiply by multipler
        PDX_MULAW_2MX16(acc, multiplier, vin);
        //shift
        acc = PDX_SLS_2MX40(acc,shift);
        //saturate and save to 16bit
        result = PDX_PACKQSRV_2MX40(acc, round_mode);
        //add offset
        result +=vOutOff;
        //saturate
        result = PDX_MIN_2MX16(result,vmax);
        result = PDX_MAX_2MX16(result,vmin);
        //save output as 8-bit data
        PDX_SAV16_2MX8_XP(result, outa, outp,2*PDX_M);
        PDX_SAPOS_2MX8_FP(outa,outp);//flush
        nElementsProcessed += (2*PDX_M);
    }
    if((nSize - nElementsProcessed)>0){
        //load inputs
        PDX_LA16_2MX8_XP (vin, ina, inp, 0);
        vin-=vInOff;
        //reset acc
        acc=0;
        //multiply by multipler
        PDX_MULAW_2MX16(acc, multiplier, vin);
        //shift
        acc = PDX_SLS_2MX40(acc,shift);
        //saturate and save to 16bit
        result = PDX_PACKQSRV_2MX40(acc, round_mode);
        //add offset
        result +=vOutOff;
        //saturate
        result = PDX_MIN_2MX16(result,vmax);
        result = PDX_MAX_2MX16(result,vmin);
        //save output as 8-bit data
        PDX_SAV16_2MX8_XP(result, outa, outp,(nSize - nElementsProcessed));
        PDX_SAPOS_2MX8_FP(outa,outp);//flush
    }
}

/**
*******************************************************************************
* Function: adi_sharcfx_tanh_int16
* @brief optimized implementation of tanh activation function
*
* @details optimized implementation of tanh activation function for int16 data. The function returns the hyperbolic Tan of x. 16-bit fixed-point function
* accepts input in Q3.12 and form output in Q7.8 format.
*
* Parameters:
* @param [in] nInputMultiplier - multiplier, corresponds to TFLM quantization scheme
* @param [in] nInputLeftShift - shift, corresponds to TFLM quantization scheme
* @param [in] nLength - input size
* @param [in] pInputData - input buffer (Q3.12)
*
* @param [out] pOutputData - output buffer(Q7.8)
*
* @return None
*
*******************************************************************************
*/ 
void adi_sharcfx_tanh_int16(int32_t nInputMultiplier, 
                            int32_t nInputLeftShift, 
                            int32_t nLength,
                            const int16_t* pInputData, 
                            int16_t* pOutputData)
{
    int16_t* pInput_in_q3_12 = (int16_t*)nQFormatBuffer;
    xb_vec2Mx16 *inp = (xb_vec2Mx16 *)pInputData;
    xb_vec2Mx16 *          outp;
    outp=(      xb_vec2Mx16 *)pInput_in_q3_12;
    xb_vec2Mx16 vin;
    valign ina,outa; // define align vector
    ina=PDX_LA_2MX16_PP (inp); // prime, NOP if a[] is aligned
    outa = PDX_Z_ALIGN();
    //scaling input to fit into Q3.12.
    if (nInputMultiplier == 0)
    {
        for (int i = 0; i < nLength; i += (2*PDX_M))
        {
            xb_vec2Mx16 vTempInput ;
            PDX_LA_2MX16_XP (vin, ina, inp, (2*PDX_M)*2); // load aligned, extend
            vTempInput = PDX_SLS_2MX16(vin, nInputLeftShift);
            PDX_SAV_2MX16_XP(vTempInput,outa,outp, (2*PDX_M)*2);
            PDX_SAPOS_2MX16_FP( outa, outp );
        }
    }
    else
    {
        xb_vec2Mx40 vTempOut;
        int32_t nTempRound = nInputLeftShift > 0 ? (1<<(nInputLeftShift-1)) : 0;
        for (int i = 0; i < nLength; i += 2*PDX_M)
        {
            PDX_LA_2MX16_XP (vin, ina, inp, (2*PDX_M)*2); // load aligned, extend
            vTempOut = PDX_MULW_2MX16(vin,nInputMultiplier);
            vTempOut = PDX_ADD_2MX40(vTempOut,nTempRound);  //rounding multiplied scaled input with round data nTempRound for 16 bit data;
            vTempOut = PDX_SRA_2MX40(vTempOut,nInputLeftShift);
            PDX_SAV_2MX16_XP(PDX_PACKSIV_2MX40(vTempOut,0),outa,outp, (2*PDX_M)*2);
            PDX_SAPOS_2MX16_FP( outa, outp );
        }

    }

    vectanh_16b(pInput_in_q3_12, pOutputData, nLength );

    //scaling output to fit from Q7.8 to Q0.15
    xb_vec2Mx16 *out = (xb_vec2Mx16 *)pOutputData;
    outp=(      xb_vec2Mx16 *)pOutputData;
    ina=PDX_LA_2MX16_PP (out); // prime, NOP if a[] is aligned
    outa = PDX_Z_ALIGN();

    for(int i = 0; i < nLength; i += 2*PDX_M)
    {
        xb_vec2Mx16 vTempInput ;
        PDX_LA_2MX16_XP (vin, ina, out, (2*PDX_M)*2); // load aligned, extend

        vTempInput = PDX_SLSI_2MX16(vin, 7);

        PDX_SAV_2MX16_XP(vTempInput,outa,outp, (2*PDX_M)*2);
        PDX_SAPOS_2MX16_FP( outa, outp );
     }
}

/**
*******************************************************************************
* Function: adi_sharcfx_logistic_int8
* @brief optimized implementation of logistic/sigmoid activation function
*
* @details optimized implementation of tanh activation function for int8 data. The function returns the Logistic (1/(1+exp(-x))) of x. 8-bit
* fixed-point function accepts input in Q3.4 and form output in Q0.7 format.
*
* Parameters:
* @param [in] nInputZeroPoint - zero point, corresponds to TFLM quantization scheme
* @param [in] nInputMultiplier - multiplier, corresponds to TFLM quantization scheme
* @param [in] nInputLeftShift - shift, corresponds to TFLM quantization scheme
* @param [in] nInputSize - input size
* @param [in] pInputData - input buffer (Q3.4)
*
* @param [out] pOutputData - output buffer(Q0.7)
*
* @return None
*
*******************************************************************************
*/ 
void adi_sharcfx_logistic_int8(int32_t nInputZeroPoint, 
                               int32_t nInputMultiplier, 
                               int32_t nInputLeftShift, 
                               int32_t nInputSize, 
                               const int8_t* pInputData, 
                               int8_t* pOutputData)
{
    // Integer bits must be in sync with Prepare() function.
    static constexpr int32_t kOutputZeroPoint = -128;

    //scaling input to fit into Q3.4
    int8_t* pInput_in_q3_4 = (int8_t*)nQFormatBuffer;
    nInputLeftShift = 4-(27 - nInputLeftShift);
    nInputMultiplier = nInputMultiplier>>24;
    xb_vec4Mx8 vInZP = PDX_REP_4MX8((xb_vec4Mx8)nInputZeroPoint,0);
    xb_vec4Mx8 vTempShift = PDX_REP_4MX8((xb_vec4Mx8)nInputLeftShift,0);
    xb_vec4Mx8 vTempMult = PDX_REP_4MX8((xb_vec4Mx8)nInputMultiplier,0);
    xb_vec4Mx8 *inp = (xb_vec4Mx8 *)pInputData;
    xb_vec4Mx8 *          outp;
    outp=(      xb_vec4Mx8 *)pInput_in_q3_4;
    xb_vec4Mx8 vin;
    valign ina,outa; // define align vector
    ina=PDX_LA_4MX8_PP (inp); // prime, NOP if a[] is aligned
    outa = PDX_Z_ALIGN();
    for (int i = 0; i < nInputSize; i += 4*PDX_M)
    {
        xb_vec4Mx20 vTempOut;
        PDX_LA_4MX8_XP (vin, ina, inp, 4*PDX_M); // load aligned, extend;
        vin += vInZP;
        vTempOut = PDX_MULW_4MX8(vin, vTempMult);
        vTempOut = PDX_SLS_4MX20(vTempOut, vTempShift);
        vTempOut = PDX_ADD_4MX20(vTempOut,64);  //rounding multiplied scaled input with round data 1<<6 for 8 bit data;
        vTempOut = PDX_SRAI_4MX20(vTempOut,7);
        PDX_SAV_4MX8_XP(PDX_PACKSIV_4MX20(vTempOut,0),outa,outp, 4*PDX_M);
        PDX_SAPOS_4MX8_FP( outa, outp );
    }

    vecsigmoid_8b((int8_t *)pInput_in_q3_4, pOutputData, nInputSize );

    //scaling output to fit from Q0.7 to Q7.8
    int16_t* output_in_q7_8 = (int16_t*)nQFormatBuffer;
    xb_vec2Mx16 vOutZP = PDX_REP_2MX16((xb_vec2Mx16)kOutputZeroPoint,0);
    xb_vec2Mx8 *out = (xb_vec2Mx8 *)pOutputData;
    xb_vec2Mx16 vout;
    xb_vec2Mx8 *          outpSig;
    outpSig=(      xb_vec2Mx8 *)pOutputData;
    valign inaSig,outaSig; // define align vector
    inaSig=PDX_LA_2MX8_PP (out); // prime, NOP if a[] is aligned
    outaSig = PDX_Z_ALIGN();
    for (int i = 0; i < nInputSize; i += 2*PDX_M)
    {
        PDX_LA16_2MX8_XP (vout, inaSig, out, (2*PDX_M)*2); // load aligned, extend
        vout = PDX_SLLI_2MX16(vout, 1);
        vout = PDX_ADD_2MX16(vout, vOutZP);
        PDX_SAV16_2MX8_XP(vout,outaSig,outpSig, (2*PDX_M)*2);
        PDX_SAPOS_2MX8_FP( outaSig, outpSig );
    }
}

/**
*******************************************************************************
* Function: adi_sharcfx_logistic_int16
* @brief optimized implementation of logistic/sigmoid activation function
*
* @details optimized implementation of tanh activation function for int16 data. The function returns the sigmoid (1/(1+exp(-x))) of x. 16-bit fixed-point
* function accepts input in Q3.12 and form output in Q7.8 format.
*
* Parameters:
* @param [in] nInputMultiplier - multiplier, corresponds to TFLM quantization scheme
* @param [in] nInputLeftShift - shift, corresponds to TFLM quantization scheme
* @param [in] nInputSize - input size
* @param [in] pInputData - input buffer (Q3.12)
*
* @param [out] pOutputData - output buffer(Q7.8)
*
* @return None
*
*******************************************************************************
*/ 
void adi_sharcfx_logistic_int16(int32_t nInputMultiplier, 
                                int32_t nInputLeftShift, 
                                int32_t nInputSize, 
                                const int16_t* pInputData, 
                                int16_t* pOutputData)
{
    int16_t* pInput_in_q3_12 = (int16_t*)nQFormatBuffer;
    xb_vec2Mx16 *inp = (xb_vec2Mx16 *)pInputData;
    xb_vec2Mx16 *          outp;
    outp=(      xb_vec2Mx16 *)pInput_in_q3_12;
    xb_vec2Mx16 vin;
    valign ina,outa; // define align vector
    ina=PDX_LA_2MX16_PP (inp); // prime, NOP if a[] is aligned
    outa = PDX_Z_ALIGN();
    if (nInputMultiplier == 0)
    {
        pInput_in_q3_12 = (int16_t*)pInputData;
    }
    else
    {
        xb_vec2Mx40 vTempOut;
        int32_t nTempRound = nInputLeftShift > 0 ? (1<<(nInputLeftShift-1)) : 0;
        for (int i = 0; i < nInputSize; i += 2*PDX_M)
        {
            PDX_LA_2MX16_XP (vin, ina, inp, (2*PDX_M)*2); // load aligned, extend
            vTempOut = PDX_MULW_2MX16(vin,nInputMultiplier);
            vTempOut = PDX_ADD_2MX40(vTempOut,nTempRound);
            vTempOut = PDX_SRA_2MX40(vTempOut,nInputLeftShift);
            PDX_SAV_2MX16_XP(PDX_PACKSIV_2MX40(vTempOut,0),outa,outp, (2*PDX_M)*2);
            PDX_SAPOS_2MX16_FP( outa, outp );
        }
    }

    vecsigmoid_16b(pInput_in_q3_12, pOutputData , nInputSize);

    //scaling output to fit from Q7.8 to Q0.15
    xb_vec2Mx16 *out = (xb_vec2Mx16 *)pOutputData;
    outp=(      xb_vec2Mx16 *)pOutputData;
    ina=PDX_LA_2MX16_PP (out); // prime, NOP if a[] is aligned
    outa = PDX_Z_ALIGN();
    for (int i = 0; i < nInputSize; i += 2*PDX_M)
    {
        xb_vec2Mx16 vTempInput ;
        PDX_LA_2MX16_XP (vin, ina, out, (2*PDX_M)*2); // load aligned, extend
        vTempInput = PDX_SLSI_2MX16(vin, 7);
        PDX_SAV_2MX16_XP(vTempInput,outa,outp, (2*PDX_M)*2);
        PDX_SAPOS_2MX16_FP( outa, outp );
    }
}
