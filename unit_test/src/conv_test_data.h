/* Copyright 2021 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef CONV_TEST_DATA_H_
#define CONV_TEST_DATA_H_

#include "tensorflow/lite/c/common.h"

namespace tflite {
extern const int8_t kConvInput1x32x32x3[];
extern const int8_t kConvFilter8x3x3x3[];
extern const int32_t kConvBiasQuantized8[];
extern const int8_t kConvGoldenOutput1x16x16x8[];
extern int8_t output_data16x16x8[];

// Conv Test Case: Int8Input32x1Filter32x32
extern const float kConvInput32x1[];
extern const float kConvFilter32x32[];
extern const float kConvInput32x1Golden1x32[];
extern const float kConvInput1x32ZeroBias[];

// Kernel Conv Test Case: Int8Input1x19x19x25Filter14x5x5x25
extern int8_t input_quantized19x19x25[];
extern int8_t filter_quantized14x19x19x25[];
extern int8_t output_quantized8x8x14[];
extern int8_t golden_quantized8x8x14[];
extern int32_t bias_quantized19x19x25Inp[];
extern const float kConvInput1x19x19x25Golden1x8x8x14[];

// Kernel Conv Test Case: Int8Input1x16x16x32Filter12x3x3x32
extern int8_t input_quantized1x16x16x32[];
extern int8_t filter_quantized12x1x16x16x32[];
extern int8_t output_quantized1x14x14x12[];
extern int8_t golden_quantized1x14x14x12[];
extern int32_t bias_quantized16x16x32Inp[];
extern const float kConvInput1x16x16x32Golden1x14x14x12[];

// Kernel Conv Test Cases: Int16Input1x15x15x14Int8Filter7x3x3x14ActRelu
extern int16_t input_quantized1x15x15x14[];
extern int8_t filter_quantized7x3x3x14[];
extern int16_t golden_quantized1x7x7x7[];
extern int16_t output_data7x7x7[];
extern int32_t bias_quantized15x15x14Inp[];
extern const float kConvInt16Input1x15x15x14Golden1x7x7x7[];

// Kernel Conv Test Cases: Int16Input1x12x12x12Int8Filter6x5x5x12ActNone
extern int16_t input_quantized1x12x12x12[];
extern int8_t filter_quantized6x5x5x12[];
extern int16_t golden_quantized1x8x8x6[];
extern int16_t output_data8x8x6[];
extern int32_t bias_quantized12x12x12Inp[];
extern const float kConvInt16Input1x12x12x12Golden1x8x8x6[];

// Kernel Conv Test Cases: Int16Input1x15x15x10Int8Filter9x5x5x10ShouldMatchGolden
extern int16_t input_quantized15x15x10[];
extern int8_t filter_quantized9x5x5x10[];
extern int16_t golden_quantized6x6x9[];
extern int16_t output_data6x6x9[];
extern int32_t bias_quantized15x15x10Inp[];
extern const float kConvInt16Input1x15x15x10Golden1x6x6x9[];

// Kernel Conv Test Cases: Int16Input1x13x13x16Int8Filter8x3x3x16ActNone
extern const float Int16ConvCommonInput[];
extern const float kConvInt16Input1x13x13x16Golden1x11x11x8[];
extern int16_t input_quantized1x13x13x16[];
extern int8_t filter_quantized8x3x3x16[];
extern int16_t golden_quantized1x11x11x8[];
extern int16_t output_data11x11x8[];
extern int64_t bias_quantized13x13x16Inp[];

// Kernel Conv Test Cases: Int8Input1x20x20x32Filter16x5x5x32
extern int8_t input_quantized1x20x20x32[];
extern int8_t filter_quantized16x20x20x32[];
extern int8_t output_quantized1x16x16x16[];
extern int8_t golden_quantized1x16x16x16[];
extern const float Int8ConvCommonInput[];
extern const float Int8ConvCommonFilter[];
extern const float kConvInput1x20x20x32Golden1x16x16x16[];
extern const float kConvCommonZeroBias[];
extern int32_t bias_quantized20x20x32Inp[];

// Kernel Conv Test Case: Int8Input1x17x17x16Filter10x3x3x16ShouldMatchGolden
extern int8_t input_quantized1x17x17x16[];
extern int8_t filter_quantized10x1x17x17x16[];
extern int8_t output_quantized1x8x8x10[];
extern int8_t golden_quantized1x8x8x10[];
extern int32_t bias_quantized17x17x16[];
extern const float kConvInput1x17x17x16Golden1x8x8x10[];

// Kernel Conv Test Cases: Int8Filter1x3x3x1ShouldMatchGolden
extern const int8_t kConvInput1x4x4x1[];
extern const int8_t kConvInput1x5x5x1[];
extern const int8_t kConvFilter1x3x3x1[];
extern const int32_t kConvZeroBias[];
extern const int8_t kConvGoldenOutput4x4InputPaddingSame2x2[];
extern const int8_t kConvGoldenOutput5x5InputPaddingSame3x3[];

}  // namespace tflite

#endif  // CONV_TEST_DATA_H_
