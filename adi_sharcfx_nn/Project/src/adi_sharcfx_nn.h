/**
********************************************************************************
*
* @file: adi_sharcfx_nn.h
*
* @brief: header file for adi NN library
*
* @details: primary header file for adi NN library
*
*******************************************************************************
 Copyright(c) 2024 Analog Devices, Inc. All Rights Reserved. This software is
 proprietary & confidential to Analog Devices, Inc. and its licensors. By using
 this software you agree to the terms of the associated Analog Devices License
 Agreement.
*******************************************************************************
*/

#ifndef __ADI_SHARCFX_NN_H__
#define __ADI_SHARCFX_NN_H__


/*============= I N C L U D E S =============*/
#include "adi_sharcfx_common.h"
#include "debug.h"

/*============= D E F I N E S =============*/

/*Defines to enable and disable optimizations*/
#define USE_OPTIMIZED_DEPTHCONV
#define USE_OPTIMISED_3X3_CONV
#define USE_OPTIMIZED_1x1_CONV
//#define USE_NON_INTERLEAVED_DEPTHCONV
//#define USE_OPTIMIZED_FC
//#define USE_OPTIMIZED_LSTM
//#define USE_NON_INTERLEAVED_1x1_CONV
//#define USE_OPTIMIZED_AVGPOOL
//#define USE_OPTIMIZED_TANH_INT16
//#define USE_OPTIMIZED_LOGISTIC_INT8
//#define USE_OPTIMIZED_LOGISTIC_INT16
//#define OPT_LOGISTIC_INT16_MULTIPLIER
//#define OPT_TANH_INT16_MULTIPLIER

/* Enable or disable profiling */
//#define DISPLAY_CYCLE_COUNTS

#ifdef DISPLAY_CYCLE_COUNTS
#define __PRE_FX_COMPATIBILITY
#define DO_CYCLE_COUNTS
#include <cycle_count.h>
#endif
/*============= D A T A =============*/

/*============= F U N C T I O N P R O T O T Y P E S =============*/
void adi_sharcfx_maxpool_int8(const int32_t input_y,
                              const int32_t input_x,
                              const int32_t output_y,
                              const int32_t output_x,
                              const int32_t stride_y,
                              const int32_t stride_x,
                              const int32_t kernel_y,
                              const int32_t kernel_x,
                              const int32_t pad_y,
                              const int32_t pad_x,
                              const int32_t act_min,
                              const int32_t act_max,
                              const int32_t ch_src,
                              const int8_t  *src,
                              int8_t        *dst);

void adi_sharcfx_depthconv2d_stride2_noninterleaved_int8(const int8_t *pInputBuffer,
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
                                                         int32_t pOutZeroPoint);

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
                                                         int32_t pOutZeroPoint);

void adi_sharcfx_depthconv2d_stride2_kernel8x10_noninterleaved_int8(const int8_t *pInputBuffer,
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
                                                                    int32_t output_activation_max);

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
                                  int32_t nActMax);

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
                                      int32_t output_activation_max);

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
                                       int32_t output_activation_max);

void adi_sharcfx_tanh_int16(int32_t nInputMultiplier, 
                            int32_t nInputLeftShift, 
                            int32_t nLength,
                            const int16_t* pInputData, 
                            int16_t* pOutputData);

void adi_sharcfx_logistic_int8 (int32_t nInputZeroPoint, 
                                int32_t nInputMultiplier, 
                                int32_t nInputLeftShift, 
                                int32_t nInputSize, 
                                const int8_t* pInputData, 
                                int8_t* pOutputData);

void adi_sharcfx_logistic_int16(int32_t nInputMultiplier, 
                                int32_t nInputLeftShift, 
                                int32_t nInputSize, 
                                const int16_t* pInputData, 
                                int16_t* pOutputData);

void adi_sharcfx_elementwise_add_int16(const int16_t* pInput1,
                                       const int16_t* pInput2,
                                       int32_t nBatches,
                                       int32_t nInputLen,
                                       int16_t* pOutput);

void adi_sharcfx_elementwise_mul_int16(const int16_t* pInput1,
                                       const int16_t* pInput2,
                                       int16_t* pOutput,
                                       int32_t nSize,
                                       uint32_t pQuantizedMultiplier,
                                       int32_t pQuantizedShift,
                                       int32_t pInOffset1,
                                       int32_t pInOffset2,
                                       int32_t pOutOffset,
                                       int32_t output_activation_min,
                                       int32_t output_activation_max);

void adi_sharcfx_relu_int8(const int8_t* pInput,
                           int8_t* pOutput,
                           const uint32_t nSize,
                           uint32_t pQuantizedMultiplier,
                           int32_t pQuantizedShift,
                           int32_t pInOffset,
                           int32_t pOutOffset,
                           int32_t output_activation_min,
                           int32_t output_activation_max);

void adi_sharcfx_conv2d_dilation1x1_int8(const int8_t* pInputBuffer,
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
                                         int32_t nActMax);

void adi_sharcfx_conv2d_kernel3x3_stride1_valid_pad_int8(const int8_t* pInputBuffer,
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
                                                         int32_t pOutZeroPoint);

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
                                                        int32_t pOutZeroPoint);

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
                                                        int32_t pOutZeroPoint);

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
                                       int32_t nInputOffset,
                                       int32_t nOutputOffset);

void adi_sharcfx_conv2d_kernel1x1_noninterleaved_int16(const int16_t *pInputBuffer,
                                                       int16_t *pOutputBuffer,
                                                       const int8_t *pWeightsBuffer,
                                                       const int32_t *pBiasBuffer,
                                                       int32_t nWidth,
                                                       int32_t nInChannels,
                                                       int32_t nOutChannels,
                                                       uint32_t *pQuantizedMultiplier,
                                                       int32_t *pQuantizedShift,
                                                       int32_t pInZeroPoint,
                                                       int32_t pOutZeroPoint);

void adi_sharcfx_conv2d_kernel1x1_noninterleaved_int8(const int8_t *pInputBuffer,
                                                      int8_t *pOutputBuffer,
                                                      const int8_t *pWeightsBuffer,
                                                      const int32_t *pBiasBuffer,
                                                      int32_t nWidth,
                                                      int32_t nInChannels,
                                                      int32_t nOutChannels,
                                                      uint32_t *pQuantizedMultiplier,
                                                      int32_t *pQuantizedShift,
                                                      int8_t pInZeroPoint,
                                                      uint32_t pOutZeroPoint);
                             
#endif /* __ADI_SHARCFX_NN_H__ */
