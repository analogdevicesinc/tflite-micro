/* Copyright 2023 The TensorFlow Authors. All Rights Reserved.

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

#include "conv_test.h"

#include "tensorflow/lite/c/builtin_op_data.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/kernels/kernel_runner.h"
#include "conv_test_data.h"
#include "tensorflow/lite/micro/micro_utils.h"
#include "tensorflow/lite/micro/test_helpers.h"
#include "micro_test.h"
#include "unit_test.h"

namespace tflite {
namespace testing {
namespace {
// Common inputs and outputs.
constexpr int kInputElements = 16;
static int kInputShape[] = {4, 2, 2, 4, 1};
static const float kInputData[kInputElements] = {1, 1, 1, 1, 2, 2, 2, 2,
                                                 1, 2, 3, 4, 1, 2, 3, 4};

constexpr int kFilterElements = 12;
static int kFilterShape[] = {4, 3, 2, 2, 1};
static const float kFilterData[kFilterElements] = {1,  2, 3,  4,  -1, 1,
                                                   -1, 1, -1, -1, 1,  1};

constexpr int kBiasElements = 3;
static int kBiasShape[] = {1, 3};
static const float kBiasData[kBiasElements] = {1, 2, 3};

constexpr int kOutputElements = 12;
static int kOutputShape[] = {4, 2, 1, 2, 3};
static const float kGoldenData[kOutputElements] = {18, 2, 5, 18, 2, 5,
                                                   17, 4, 3, 37, 4, 3};

static TfLiteConvParams common_conv_params = {
    kTfLitePaddingValid,  // padding
    2,                    // stride_width
    2,                    // stride_height
    kTfLiteActNone,       // activation
    1,                    // dilation_width_factor
    1,                    // dilation_height_factor
    kTfLiteNoType         // quantized_bias_type
};

}  // namespace
}  // namespace testing
}  // namespace tflite

using namespace micro_test;

int convolutionTest() {
  micro_test::tests_passed = 0;
  micro_test::tests_failed = 0;
  tflite::InitializeTest();

TF_LITE_MICRO_TEST(SimpleTestQuantized4bitPerChannel) {
  const int output_dims_count = 12;
  int8_t output_data[output_dims_count];

  const float input_scale = 0.5f;
  const float output_scale = 1.0f;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  int8_t input_quantized[tflite::testing::kInputElements];
  int8_t filter_quantized[tflite::testing::kFilterElements];
  int32_t bias_quantized[tflite::testing::kBiasElements];
  int8_t golden_quantized[tflite::testing::kOutputElements];
  int zero_points[tflite::testing::kBiasElements + 1];
  float scales[tflite::testing::kBiasElements + 1];

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk,
      tflite::testing::TestConvQuantizedPerChannel(
          tflite::testing::kInputShape, tflite::testing::kInputData,
          input_quantized, input_scale, input_zero_point,
          tflite::testing::kFilterShape, tflite::testing::kFilterData,
          filter_quantized, tflite::testing::kBiasShape,
          tflite::testing::kBiasData, bias_quantized, scales, zero_points,
          tflite::testing::kOutputShape, tflite::testing::kGoldenData,
          golden_quantized, output_scale, output_zero_point,
          &tflite::testing::common_conv_params, tflite::Register_CONV_2D(),
          output_data, kTfLiteInt4));
}

TF_LITE_MICRO_TEST(SimpleTestQuantizedPerChannel) {
  const int output_dims_count = 12;
  int8_t output_data[output_dims_count];

  const float input_scale = 0.5f;
  const float output_scale = 1.0f;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  int8_t input_quantized[tflite::testing::kInputElements];
  int8_t filter_quantized[tflite::testing::kFilterElements];
  int32_t bias_quantized[tflite::testing::kBiasElements];
  int8_t golden_quantized[tflite::testing::kOutputElements];
  int zero_points[tflite::testing::kBiasElements + 1];
  float scales[tflite::testing::kBiasElements + 1];

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk,
      tflite::testing::TestConvQuantizedPerChannel(
          tflite::testing::kInputShape, tflite::testing::kInputData,
          input_quantized, input_scale, input_zero_point,
          tflite::testing::kFilterShape, tflite::testing::kFilterData,
          filter_quantized, tflite::testing::kBiasShape,
          tflite::testing::kBiasData, bias_quantized, scales, zero_points,
          tflite::testing::kOutputShape, tflite::testing::kGoldenData,
          golden_quantized, output_scale, output_zero_point,
          &tflite::testing::common_conv_params, tflite::Register_CONV_2D(),
          output_data));
}

TF_LITE_MICRO_TEST(SimpleTestFloat) {
  float output_data[tflite::testing::kOutputElements];

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk,
      tflite::testing::TestConvFloat(
          tflite::testing::kInputShape, tflite::testing::kInputData,
          tflite::testing::kFilterShape, tflite::testing::kFilterData,
          tflite::testing::kBiasShape, tflite::testing::kBiasData,
          tflite::testing::kOutputShape, tflite::testing::kGoldenData,
          &tflite::testing::common_conv_params, tflite::Register_CONV_2D(),
          output_data));
}

TF_LITE_MICRO_TEST(InputAndFilterSameWidthHeight) {
  const int output_dims_count = 2;
  float output_data[output_dims_count];

  int kFilterShape[] = {4, 1, 2, 4, 1};
  const float filter_values[] = {1, 2, 3, 4, -1, -1, 1, 1};
  int kBiasShape[] = {1, 1};
  const float bias_values[] = {0};
  int kOutputShape[] = {4, 2, 1, 1, 1};
  const float expected_output[] = {10, 34};

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk,
      tflite::testing::TestConvFloat(
          tflite::testing::kInputShape, tflite::testing::kInputData,
          kFilterShape, filter_values, kBiasShape, bias_values, kOutputShape,
          expected_output, &tflite::testing::common_conv_params,
          tflite::Register_CONV_2D(), output_data));
}

TF_LITE_MICRO_TEST(InputOutputDifferentTypeIsError) {
  using tflite::testing::CreateQuantizedTensor;
  using tflite::testing::CreateTensor;
  using tflite::testing::IntArrayFromInts;

  TfLiteIntArray* input_dims = IntArrayFromInts(tflite::testing::kInputShape);
  TfLiteIntArray* filter_dims = IntArrayFromInts(tflite::testing::kFilterShape);
  TfLiteIntArray* bias_dims = IntArrayFromInts(tflite::testing::kBiasShape);
  TfLiteIntArray* output_dims = IntArrayFromInts(tflite::testing::kOutputShape);
  const int output_dims_count = tflite::ElementCount(*output_dims);
  constexpr int inputs_size = 3;
  constexpr int outputs_size = 1;
  constexpr int tensors_size = inputs_size + outputs_size;

  int8_t output_data[tflite::testing::kOutputElements];
  TfLiteTensor tensors[tensors_size] = {
      CreateTensor(tflite::testing::kInputData, input_dims),
      CreateTensor(tflite::testing::kFilterData, filter_dims),
      CreateTensor(tflite::testing::kBiasData, bias_dims),
      CreateQuantizedTensor(output_data, output_dims, /*scale=*/0.0f,
                            /*zero_point=*/0),
  };
  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteError,
      tflite::testing::InvokeConv(tensors, tensors_size, output_dims_count,
                                  &tflite::testing::common_conv_params,
                                  tflite::Register_CONV_2D(), output_data));
}

TF_LITE_MICRO_TEST(HybridModeIsError) {
  using tflite::testing::CreateQuantizedTensor;
  using tflite::testing::CreateTensor;
  using tflite::testing::IntArrayFromInts;

  TfLiteIntArray* input_dims = IntArrayFromInts(tflite::testing::kInputShape);
  TfLiteIntArray* filter_dims = IntArrayFromInts(tflite::testing::kFilterShape);
  TfLiteIntArray* bias_dims = IntArrayFromInts(tflite::testing::kBiasShape);
  TfLiteIntArray* output_dims = IntArrayFromInts(tflite::testing::kOutputShape);
  const int output_dims_count = tflite::ElementCount(*output_dims);
  constexpr int inputs_size = 3;
  constexpr int outputs_size = 1;
  constexpr int tensors_size = inputs_size + outputs_size;

  int8_t filter_data[tflite::testing::kFilterElements] = {};
  float output_data[tflite::testing::kOutputElements];
  TfLiteTensor tensors[tensors_size] = {
      CreateTensor(tflite::testing::kInputData, input_dims),
      CreateQuantizedTensor(filter_data, filter_dims,
                            /*scale=*/0.0f,
                            /*zero_point=*/0),
      CreateTensor(tflite::testing::kBiasData, bias_dims),
      CreateTensor(output_data, output_dims),
  };
  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteError,
      tflite::testing::InvokeConv(tensors, tensors_size, output_dims_count,
                                  &tflite::testing::common_conv_params,
                                  tflite::Register_CONV_2D(), output_data));
}

TF_LITE_MICRO_TEST(SimpleTestQuantized16x8PerChannel64bBias) {
  const int output_dims_count = 12;
  int16_t output_data[output_dims_count];

  const float input_scale = 0.5f;
  const float output_scale = 1.0f;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  int16_t input_quantized[tflite::testing::kInputElements];
  int8_t filter_quantized[tflite::testing::kFilterElements];
  std::int64_t bias_quantized[tflite::testing::kBiasElements];
  int16_t golden_quantized[tflite::testing::kOutputElements];
  int zero_points[tflite::testing::kBiasElements + 1];
  float scales[tflite::testing::kBiasElements + 1];

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk,
      tflite::testing::TestConvQuantizedPerChannel(
          tflite::testing::kInputShape, tflite::testing::kInputData,
          input_quantized, input_scale, input_zero_point,
          tflite::testing::kFilterShape, tflite::testing::kFilterData,
          filter_quantized, tflite::testing::kBiasShape,
          tflite::testing::kBiasData, bias_quantized, scales, zero_points,
          tflite::testing::kOutputShape, tflite::testing::kGoldenData,
          golden_quantized, output_scale, output_zero_point,
          &tflite::testing::common_conv_params, tflite::Register_CONV_2D(),
          output_data));
}

TF_LITE_MICRO_TEST(SimpleTestQuantized16x8PerChannel32bBias) {
  const int output_dims_count = 12;
  int16_t output_data[output_dims_count];

  const float input_scale = 0.5f;
  const float output_scale = 1.0f;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  int16_t input_quantized[tflite::testing::kInputElements];
  int8_t filter_quantized[tflite::testing::kFilterElements];
  int32_t bias_quantized[tflite::testing::kBiasElements];
  int16_t golden_quantized[tflite::testing::kOutputElements];
  int zero_points[tflite::testing::kBiasElements + 1];
  float scales[tflite::testing::kBiasElements + 1];

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk,
      tflite::testing::TestConvQuantizedPerChannel(
          tflite::testing::kInputShape, tflite::testing::kInputData,
          input_quantized, input_scale, input_zero_point,
          tflite::testing::kFilterShape, tflite::testing::kFilterData,
          filter_quantized, tflite::testing::kBiasShape,
          tflite::testing::kBiasData, bias_quantized, scales, zero_points,
          tflite::testing::kOutputShape, tflite::testing::kGoldenData,
          golden_quantized, output_scale, output_zero_point,
          &tflite::testing::common_conv_params, tflite::Register_CONV_2D(),
          output_data));
}

TF_LITE_MICRO_TEST(SimpleTestDilatedQuantizedPerChannel) {
  const int output_dims_count = 24;
  int8_t output_data[output_dims_count];

  const float input_scale = 0.5f;
  const float output_scale = 1.0f;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  const int input_elements = 48;
  int input_shape[] = {4, 2, 4, 6, 1};
  const float input_data[] = {
      // b = 0
      1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4,
      // b = 1
      1, 2, 3, 4, 5, 6, 2, 6, 2, 4, 4, 2, 3, 2, 6, 5, 1, 4, 1, 2, 1, 4, 6, 3};
  const int output_elements = 24;
  int output_shape[] = {4, 2, 2, 2, 3};
  const float golden_data[] = {25, 2, 7, 25, 2, 7, 10, 2, -3, 10, 2, -3,
                               39, 7, 6, 50, 3, 4, 14, 4, -5, 15, 0, -7};

  int8_t input_quantized[input_elements];
  int8_t filter_quantized[tflite::testing::kFilterElements];
  int32_t bias_quantized[tflite::testing::kBiasElements];
  int8_t golden_quantized[output_elements];
  int zero_points[tflite::testing::kBiasElements + 1];
  float scales[tflite::testing::kBiasElements + 1];

  TfLiteConvParams conv_params{tflite::testing::common_conv_params};
  conv_params.dilation_width_factor = 3;
  conv_params.dilation_height_factor = 2;

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk,
      tflite::testing::TestConvQuantizedPerChannel(
          input_shape, input_data, input_quantized, input_scale,
          input_zero_point, tflite::testing::kFilterShape,
          tflite::testing::kFilterData, filter_quantized,
          tflite::testing::kBiasShape, tflite::testing::kBiasData,
          bias_quantized, scales, zero_points, output_shape, golden_data,
          golden_quantized, output_scale, output_zero_point, &conv_params,
          tflite::Register_CONV_2D(), output_data));
}

TF_LITE_MICRO_TEST(SimpleTestQuantizedPerChannelRelu6) {
  const int output_dims_count = 12;
  int8_t output_data[output_dims_count];

  const float bias_values[] = {1, 2, -3};
  const float golden_data[] = {6, 2, 0, 6, 2, 0, 6, 4, 0, 6, 4, 0};

  const float input_scale = 0.023529f;
  const float output_scale = 0.023529f;
  const int input_zero_point = -128;
  const int output_zero_point = -128;

  int8_t input_quantized[tflite::testing::kInputElements];
  int8_t filter_quantized[tflite::testing::kFilterElements];
  int32_t bias_quantized[tflite::testing::kBiasElements];
  int8_t golden_quantized[tflite::testing::kOutputElements];
  int zero_points[tflite::testing::kBiasElements + 1];
  float scales[tflite::testing::kBiasElements + 1];

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk,
      tflite::testing::TestConvQuantizedPerChannel(
          tflite::testing::kInputShape, tflite::testing::kInputData,
          input_quantized, input_scale, input_zero_point,
          tflite::testing::kFilterShape, tflite::testing::kFilterData,
          filter_quantized, tflite::testing::kBiasShape, bias_values,
          bias_quantized, scales, zero_points, tflite::testing::kOutputShape,
          golden_data, golden_quantized, output_scale, output_zero_point,
          &tflite::testing::common_conv_params, tflite::Register_CONV_2D(),
          output_data));
}

TF_LITE_MICRO_TEST(SimpleTestQuantized16x8PerChannelRelu664bBias) {
  const int output_dims_count = 12;
  int16_t output_data[output_dims_count];

  const float bias_values[] = {1, 2, -3};
  const float golden_data[] = {6, 2, 0, 6, 2, 0, 6, 4, 0, 6, 4, 0};

  const float input_scale = 0.023529f;
  const float output_scale = 0.023529f;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  int16_t input_quantized[tflite::testing::kInputElements];
  int8_t filter_quantized[tflite::testing::kFilterElements];
  std::int64_t bias_quantized[tflite::testing::kBiasElements];
  int16_t golden_quantized[tflite::testing::kOutputElements];
  int zero_points[tflite::testing::kBiasElements + 1];
  float scales[tflite::testing::kBiasElements + 1];

  TfLiteConvParams conv_params{tflite::testing::common_conv_params};
  conv_params.activation = kTfLiteActRelu6;
  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk,
      tflite::testing::TestConvQuantizedPerChannel(
          tflite::testing::kInputShape, tflite::testing::kInputData,
          input_quantized, input_scale, input_zero_point,
          tflite::testing::kFilterShape, tflite::testing::kFilterData,
          filter_quantized, tflite::testing::kBiasShape, bias_values,
          bias_quantized, scales, zero_points, tflite::testing::kOutputShape,
          golden_data, golden_quantized, output_scale, output_zero_point,
          &conv_params, tflite::Register_CONV_2D(), output_data));
}

TF_LITE_MICRO_TEST(SimpleTestQuantized16x8PerChannelRelu632bBias) {
  const int output_dims_count = 12;
  int16_t output_data[output_dims_count];

  const float bias_values[] = {1, 2, -3};
  const float golden_data[] = {6, 2, 0, 6, 2, 0, 6, 4, 0, 6, 4, 0};

  const float input_scale = 0.023529f;
  const float output_scale = 0.023529f;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  int16_t input_quantized[tflite::testing::kInputElements];
  int8_t filter_quantized[tflite::testing::kFilterElements];
  int32_t bias_quantized[tflite::testing::kBiasElements];
  int16_t golden_quantized[tflite::testing::kOutputElements];
  int zero_points[tflite::testing::kBiasElements + 1];
  float scales[tflite::testing::kBiasElements + 1];

  TfLiteConvParams conv_params{tflite::testing::common_conv_params};
  conv_params.activation = kTfLiteActRelu6;
  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk,
      tflite::testing::TestConvQuantizedPerChannel(
          tflite::testing::kInputShape, tflite::testing::kInputData,
          input_quantized, input_scale, input_zero_point,
          tflite::testing::kFilterShape, tflite::testing::kFilterData,
          filter_quantized, tflite::testing::kBiasShape, bias_values,
          bias_quantized, scales, zero_points, tflite::testing::kOutputShape,
          golden_data, golden_quantized, output_scale, output_zero_point,
          &conv_params, tflite::Register_CONV_2D(), output_data));
}

TF_LITE_MICRO_TEST(Kernel1x1QuantizedPerChannel) {
  // conv params:
  // padding, stride_<width,height>, activation, dilation_<width, height>
  TfLiteConvParams conv_params = {
      kTfLitePaddingValid, 1, 1, kTfLiteActNone, 1, 1, kTfLiteNoType};

  int input_shape[] = {4, 1, 2, 2, 4};  // [len,N,H,W,C]
  constexpr int input_elements =
      1 * 2 * 2 *
      4;  // input_shape[1] * input_shape[2] * input_shape[3] * input_shape[4];
  constexpr float input_data[input_elements] = {1, 1, 1, 1, 2, 2, 2, 2,
                                                1, 2, 3, 4, 1, 2, 3, 4};

  int filter_shape[] = {4, 3, 1, 1, 4};
  constexpr int filter_elements =
      3 * 1 * 1 * 4;  //      filter_shape[1] * filter_shape[2] *
                      //      filter_shape[3] * filter_shape[4];
  const float filter_data[filter_elements] = {1,  2, 3,  4,  -1, 1,
                                              -1, 1, -1, -1, 1,  1};

  constexpr int bias_elements = 3;  // filter_shape[1];
  int bias_shape[] = {1, bias_elements};
  constexpr float bias_data[bias_elements] = {1, 2, 3};

  int output_shape[] = {4, 1, 2, 2, bias_elements};
  constexpr int output_elements = 4 * 3;
  int8_t output_data[output_elements];

  const float golden_data[output_elements] = {11, 2, 3, 21, 2, 3,
                                              31, 4, 7, 31, 4, 7};

  const float input_scale = 0.5f;
  const float output_scale = 1.0f;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  int8_t input_quantized[input_elements];
  int8_t filter_quantized[filter_elements];
  int32_t bias_quantized[bias_elements];
  int8_t golden_quantized[output_elements];
  int zero_points[bias_elements + 1];
  float scales[bias_elements + 1];

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk, tflite::testing::TestConvQuantizedPerChannel(
                     input_shape, input_data, input_quantized, input_scale,
                     input_zero_point, filter_shape, filter_data,
                     filter_quantized, bias_shape, bias_data, bias_quantized,
                     scales, zero_points, output_shape, golden_data,
                     golden_quantized, output_scale, output_zero_point,
                     &conv_params, tflite::Register_CONV_2D(), output_data));
}

TF_LITE_MICRO_TEST(Kernel1x1QuantizedPerChannelRelu6) {
  // conv params:
  // padding, stride_<width,height>, activation, dilation_<width, height>
  TfLiteConvParams conv_params = {
      kTfLitePaddingValid, 1, 1, kTfLiteActRelu6, 1, 1, kTfLiteNoType};

  int input_shape[] = {4, 1, 2, 2, 4};  // [len,N,H,W,C]
  constexpr int input_elements =
      1 * 2 * 2 *
      4;  // input_shape[1] * input_shape[2] * input_shape[3] * input_shape[4];
  constexpr float input_data[input_elements] = {1, 1, 1, 1, 2, 2, 2, 2,
                                                1, 2, 3, 4, 1, 2, 3, 4};

  int filter_shape[] = {4, 3, 1, 1, 4};
  constexpr int filter_elements =
      3 * 1 * 1 * 4;  //      filter_shape[1] * filter_shape[2] *
                      //      filter_shape[3] * filter_shape[4];
  const float filter_data[filter_elements] = {1,  2, 3,  4,  -1, 1,
                                              -1, 1, -1, -1, 1,  1};

  constexpr int bias_elements = 3;  // filter_shape[1];
  int bias_shape[] = {1, bias_elements};
  constexpr float bias_data[bias_elements] = {1, 2, -3};

  int output_shape[] = {4, 1, 2, 2, bias_elements};
  constexpr int output_elements = 4 * 3;
  int8_t output_data[output_elements];

  const float golden_data[output_elements] = {6, 2, 0, 6, 2, 0,
                                              6, 4, 1, 6, 4, 1};

  const float input_scale = 0.023529f;
  const float output_scale = 0.023529f;
  const int input_zero_point = -128;
  const int output_zero_point = -128;

  int8_t input_quantized[input_elements];
  int8_t filter_quantized[filter_elements];
  int32_t bias_quantized[bias_elements];
  int8_t golden_quantized[output_elements];
  int zero_points[bias_elements + 1];
  float scales[bias_elements + 1];

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk, tflite::testing::TestConvQuantizedPerChannel(
                     input_shape, input_data, input_quantized, input_scale,
                     input_zero_point, filter_shape, filter_data,
                     filter_quantized, bias_shape, bias_data, bias_quantized,
                     scales, zero_points, output_shape, golden_data,
                     golden_quantized, output_scale, output_zero_point,
                     &conv_params, tflite::Register_CONV_2D(), output_data));
}

TF_LITE_MICRO_TEST(Kernel1x1Quantized16x8PerChannelRelu6) {
  // conv params:
  // padding, stride_<width,height>, activation, dilation_<width, height>
  TfLiteConvParams conv_params = {
      kTfLitePaddingValid, 1, 1, kTfLiteActRelu6, 1, 1, kTfLiteNoType};

  int input_shape[] = {4, 1, 2, 2, 4};  // [len,N,H,W,C]
  const int input_elements = 1 * 2 * 2 * 4;
  const float input_data[input_elements] = {1, 1, 1, 1, 2, 2, 2, 2,
                                            1, 2, 3, 4, 1, 2, 3, 4};

  int filter_shape[] = {4, 3, 1, 1, 4};
  const int filter_elements = 3 * 1 * 1 * 4; // [N,H,W,C]
  const float filter_data[filter_elements] = {1,  2, 3,  4,  -1, 1,
                                              -1, 1, -1, -1, 1,  1};

  const int bias_elements = 3;
  int bias_shape[] = {1, bias_elements};
  const float bias_data[bias_elements] = {1, 2, -3};

  int output_shape[] = {4, 1, 2, 2, bias_elements};
  const int output_elements = 4 * 3;
  int16_t output_data[output_elements];

  const float golden_data[output_elements] = {6, 2, 0, 6, 2, 0,
                                              6, 4, 1, 6, 4, 1};

  const float input_scale = 0.023529f;
  const float output_scale = 0.023529f;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  int16_t input_quantized[input_elements];
  int8_t filter_quantized[filter_elements];
  std::int64_t bias_quantized[bias_elements];
  int16_t golden_quantized[output_elements];
  int zero_points[bias_elements + 1];
  float scales[bias_elements + 1];

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk, tflite::testing::TestConvQuantizedPerChannel(
                     input_shape, input_data, input_quantized, input_scale,
                     input_zero_point, filter_shape, filter_data,
                     filter_quantized, bias_shape, bias_data, bias_quantized,
                     scales, zero_points, output_shape, golden_data,
                     golden_quantized, output_scale, output_zero_point,
                     &conv_params, tflite::Register_CONV_2D(), output_data));
}

TF_LITE_MICRO_TEST(Int16Input1x13x13x16Int8Filter8x3x3x16ActNone) {
  using tflite::Int16ConvCommonInput;
  using tflite::Int8ConvCommonFilter;
  using tflite::kConvInt16Input1x13x13x16Golden1x11x11x8;
  using tflite::kConvCommonZeroBias;
  using tflite::input_quantized1x13x13x16;
  using tflite::filter_quantized8x3x3x16;
  using tflite::golden_quantized1x11x11x8;
  using tflite::output_data11x11x8;
  using tflite::bias_quantized13x13x16Inp;
  constexpr int kNumFilters = 8;
  int input_shape[] = {4, 1, 13, 13, 16};  // [len,N,H,W,C]
  int filter_shape[] = {4, kNumFilters, 3, 3, 16};
  int bias_shape[] = {1, kNumFilters};
  int output_shape[] = {4, 1, 11, 11, kNumFilters};

  // conv params:
    // padding, stride_<width,height>, activation, dilation_<width, height>
    TfLiteConvParams conv_params = {
    		kTfLitePaddingUnknown, 1, 1, kTfLiteActNone, 1, 1, kTfLiteNoType};

  const float input_scale = 1.0f;
  const float output_scale = 32.0f;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  int zero_points[kNumFilters + 1];
  float scales[kNumFilters + 1];

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk, tflite::testing::TestConvQuantizedPerChannel(
                     input_shape, Int16ConvCommonInput, input_quantized1x13x13x16, input_scale,
                     input_zero_point, filter_shape, Int8ConvCommonFilter,
					 filter_quantized8x3x3x16, bias_shape, kConvCommonZeroBias, bias_quantized13x13x16Inp,
                     scales, zero_points, output_shape, kConvInt16Input1x13x13x16Golden1x11x11x8,
					 golden_quantized1x11x11x8, output_scale, output_zero_point,
                     &conv_params, tflite::Register_CONV_2D(), output_data11x11x8));
}


TF_LITE_MICRO_TEST(Int16Input1x15x15x14Int8Filter7x3x3x14ActRelu) {
  using tflite::Int16ConvCommonInput;
  using tflite::Int8ConvCommonFilter;
  using tflite::kConvInt16Input1x15x15x14Golden1x7x7x7;
  using tflite::kConvCommonZeroBias;
  using tflite::input_quantized1x15x15x14;
  using tflite::filter_quantized7x3x3x14;
  using tflite::golden_quantized1x7x7x7;
  using tflite::output_data7x7x7;
  using tflite::bias_quantized15x15x14Inp;
  constexpr int kNumFilters = 7;
  int input_shape[] = {4, 1, 15, 15, 14};  // [len,N,H,W,C]
  int filter_shape[] = {4, kNumFilters, 3, 3, 14};
  int bias_shape[] = {1, kNumFilters};
  int output_shape[] = {4, 1, 7, 7, kNumFilters};

  // conv params:
    // padding, stride_<width,height>, activation, dilation_<width, height>
    TfLiteConvParams conv_params = {
    		kTfLitePaddingUnknown, 2, 2, kTfLiteActRelu, 1, 1, kTfLiteNoType};

  const float input_scale = 1.0f;
  const float output_scale = 64.0f;
  const int input_zero_point = -128;
  const int output_zero_point = 0;

  int zero_points[kNumFilters + 1];
  float scales[kNumFilters + 1];

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk, tflite::testing::TestConvQuantizedPerChannel(
                     input_shape, Int16ConvCommonInput, input_quantized1x15x15x14, input_scale,
                     input_zero_point, filter_shape, Int8ConvCommonFilter,
					 filter_quantized7x3x3x14, bias_shape, kConvCommonZeroBias, bias_quantized15x15x14Inp,
                     scales, zero_points, output_shape, kConvInt16Input1x15x15x14Golden1x7x7x7,
					 golden_quantized1x7x7x7, output_scale, output_zero_point,
                     &conv_params, tflite::Register_CONV_2D(), output_data7x7x7));
}


TF_LITE_MICRO_TEST(Int16Input1x12x12x12Int8Filter6x5x5x12ActNone) {
  using tflite::Int16ConvCommonInput;
  using tflite::Int8ConvCommonFilter;
  using tflite::kConvInt16Input1x12x12x12Golden1x8x8x6;
  using tflite::kConvCommonZeroBias;
  using tflite::input_quantized1x12x12x12;
  using tflite::filter_quantized6x5x5x12;
  using tflite::golden_quantized1x8x8x6;
  using tflite::output_data8x8x6;
  using tflite::bias_quantized12x12x12Inp;
  constexpr int kNumFilters = 6;

  int input_shape[] = {4, 1, 12, 12, 12};  // [len,N,H,W,C]
  int filter_shape[] = {4, kNumFilters, 5, 5, 12};
  int bias_shape[] = {1, kNumFilters};
  int output_shape[] = {4, 1, 8, 8, kNumFilters};

  // conv params:
    // padding, stride_<width,height>, activation, dilation_<width, height>
    TfLiteConvParams conv_params = {
    		kTfLitePaddingUnknown, 1, 1, kTfLiteActNone, 1, 1, kTfLiteNoType};

  const float input_scale = 0.023529f;
  const float output_scale = 64.0f;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  int zero_points[kNumFilters + 1];
  float scales[kNumFilters + 1];

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk, tflite::testing::TestConvQuantizedPerChannel(
                     input_shape, Int16ConvCommonInput, input_quantized1x12x12x12, input_scale,
                     input_zero_point, filter_shape, Int8ConvCommonFilter,
					 filter_quantized6x5x5x12, bias_shape, kConvCommonZeroBias, bias_quantized12x12x12Inp,
                     scales, zero_points, output_shape, kConvInt16Input1x12x12x12Golden1x8x8x6,
					 golden_quantized1x8x8x6, output_scale, output_zero_point,
                     &conv_params, tflite::Register_CONV_2D(), output_data8x8x6));
}


TF_LITE_MICRO_TEST(Int16Input1x15x15x10Int8Filter9x5x5x10ShouldMatchGolden) {
  using tflite::Int16ConvCommonInput;
  using tflite::Int8ConvCommonFilter;
  using tflite::kConvInt16Input1x15x15x10Golden1x6x6x9;
  using tflite::kConvCommonZeroBias;
  using tflite::input_quantized15x15x10;
  using tflite::filter_quantized9x5x5x10;
  using tflite::golden_quantized6x6x9;
  using tflite::output_data8x8x6;
  using tflite::bias_quantized15x15x10Inp;
  constexpr int kNumFilters = 9;

  int input_shape[] = {4, 1, 15, 15, 10};  // [len,N,H,W,C]
  int filter_shape[] = {4, kNumFilters, 5, 5, 10};
  int bias_shape[] = {1, kNumFilters};
  int output_shape[] = {4, 1, 6, 6, kNumFilters};

  // conv params:
    // padding, stride_<width,height>, activation, dilation_<width, height>
    TfLiteConvParams conv_params = {
    		kTfLitePaddingUnknown, 2, 2, kTfLiteActNone, 1, 1, kTfLiteNoType};

  const float input_scale = 0.023529f;
  const float output_scale = 32.0f;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  int zero_points[kNumFilters + 1];
  float scales[kNumFilters + 1];

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk, tflite::testing::TestConvQuantizedPerChannel(
                     input_shape, Int16ConvCommonInput, input_quantized15x15x10, input_scale,
                     input_zero_point, filter_shape, Int8ConvCommonFilter,
					 filter_quantized9x5x5x10, bias_shape, kConvCommonZeroBias, bias_quantized15x15x10Inp,
                     scales, zero_points, output_shape, kConvInt16Input1x15x15x10Golden1x6x6x9,
					 golden_quantized6x6x9, output_scale, output_zero_point,
                     &conv_params, tflite::Register_CONV_2D(), output_data8x8x6));
}


TF_LITE_MICRO_TEST(BroadcastPerLayerQuantizationToPerChannelShouldMatchGolden) {
  const int output_dims_count = 12;
  int8_t output_data[output_dims_count];

  const float input_scale = 1.0f;
  const float filter_scale = 1.0f;
  const float output_scale = 1.0f;

  int8_t input_quantized[tflite::testing::kInputElements];
  int8_t filter_quantized[tflite::testing::kFilterElements];
  int32_t bias_quantized[tflite::testing::kBiasElements];
  int8_t golden_quantized[tflite::testing::kOutputElements];

  TfLiteIntArray* input_dims =
      tflite::testing::IntArrayFromInts(tflite::testing::kInputShape);
  TfLiteIntArray* filter_dims =
      tflite::testing::IntArrayFromInts(tflite::testing::kFilterShape);
  TfLiteIntArray* bias_dims =
      tflite::testing::IntArrayFromInts(tflite::testing::kBiasShape);
  TfLiteIntArray* output_dims =
      tflite::testing::IntArrayFromInts(tflite::testing::kOutputShape);

  // Create per-layer quantized int8_t input tensor.
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      tflite::testing::kInputData, input_quantized, input_dims, input_scale, 0);
  int input_zero_points[2] = {1, 0};
  float input_scales[2] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-layer quantized int8_t filter tensor.
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
      tflite::testing::kFilterData, filter_quantized, filter_dims, filter_scale,
      0);
  int filter_zero_points[2] = {1, 0};
  float filter_scales[2] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-layer quantized int32_t bias tensor.
  tflite::SymmetricQuantize(tflite::testing::kBiasData, bias_quantized,
                            tflite::testing::kBiasElements,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized, bias_dims);

  int bias_zero_points[2] = {1, 0};
  float bias_scales[2] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-layer quantized int8_t output tensor.
  TfLiteTensor output_tensor = tflite::testing::CreateQuantizedTensor(
      output_data, output_dims, output_scale, 0 /* quantized dimension */);
  int output_zero_points[2] = {1, 0};
  float output_scales[2] = {1, output_scale};
  TfLiteAffineQuantization output_quant = {
      tflite::testing::FloatArrayFromFloats(output_scales),
      tflite::testing::IntArrayFromInts(output_zero_points), 0};
  output_tensor.quantization = {kTfLiteAffineQuantization, &output_quant};

  constexpr int inputs_size = 3;
  constexpr int outputs_size = 1;
  constexpr int tensors_size = inputs_size + outputs_size;
  TfLiteTensor tensors[tensors_size] = {
      input_tensor,
      filter_tensor,
      bias_tensor,
      output_tensor,
  };

  tflite::Quantize(tflite::testing::kGoldenData, golden_quantized,
                   output_dims_count, output_scale, 0);

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk, tflite::testing::ValidateConvGoldens(
                     tensors, tensors_size, golden_quantized, output_dims_count,
                     &tflite::testing::common_conv_params,
                     tflite::Register_CONV_2D(), output_data));
}

TF_LITE_MICRO_TEST(Int8Filter1x3x3x1ShouldMatchGoldenEvenInputPaddingSame) {
  using tflite::ElementCount;
  using tflite::kConvFilter1x3x3x1;
  using tflite::kConvGoldenOutput4x4InputPaddingSame2x2;
  using tflite::kConvInput1x4x4x1;
  using tflite::kConvZeroBias;
  using tflite::testing::CreateTensor;
  using tflite::testing::FloatArrayFromFloats;
  using tflite::testing::IntArrayFromInts;
  using tflite::testing::ValidateConvGoldens;

  constexpr int kInDepth = 1;
  constexpr int kOutDepth = 1;

  // Input quantization parameters: same scale and zero point for all input
  // elements.
  constexpr float kInputScale = 0.00392120517f;
  constexpr int kInputZeroPoint = -128;
  float input_scales[] = {1, kInputScale};
  int input_zero_points[] = {1, kInputZeroPoint};
  TfLiteAffineQuantization input_quant = {FloatArrayFromFloats(input_scales),
                                          IntArrayFromInts(input_zero_points),
                                          0};
  // Create input tensor of size 1x4x4x1.
  int input_shape[] = {4, 1, 4, 4, kInDepth};
  TfLiteIntArray* input_dims = IntArrayFromInts(input_shape);
  TfLiteTensor input_tensor = CreateTensor(kConvInput1x4x4x1, input_dims);
  input_tensor.params = {kInputScale, kInputZeroPoint};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Filter quantization parameters.
  int filter_zero_points[kOutDepth + 1] = {kOutDepth, 0};
  float filter_scales[kOutDepth + 1] = {kOutDepth, 0.00448552053f};
  TfLiteAffineQuantization filter_quant;
  filter_quant.scale = FloatArrayFromFloats(filter_scales);
  filter_quant.zero_point = IntArrayFromInts(filter_zero_points);
  filter_quant.quantized_dimension = 0;

  // Create filter tensor of size 1x3x3x1.
  int filter_shape[] = {4, kOutDepth, 3, 3, kInDepth};
  TfLiteIntArray* filter_dims = IntArrayFromInts(filter_shape);
  TfLiteTensor filter_tensor = CreateTensor(kConvFilter1x3x3x1, filter_dims);
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Bias quantization parameters: same zero point, but different scale per
  // output channel.
  int bias_zero_points[kOutDepth + 1] = {kOutDepth, 0};
  float bias_scales[kOutDepth + 1] = {kOutDepth, 0.00001758864f};
  TfLiteAffineQuantization bias_quant;
  bias_quant.scale = FloatArrayFromFloats(bias_scales);
  bias_quant.zero_point = IntArrayFromInts(bias_zero_points);
  bias_quant.quantized_dimension = 0;

  // Create size 1 zero bias tensor.
  int bias_shape[] = {1, kOutDepth};
  TfLiteIntArray* bias_dims = IntArrayFromInts(bias_shape);
  TfLiteTensor bias_tensor = CreateTensor(kConvZeroBias, bias_dims);
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Output quantization parameters: same zero point and scale for all elements.
  const float output_scale = 0.00627814838f;
  const int output_zero_point = -7;
  float output_scales[] = {1, output_scale};
  int output_zero_points[] = {1, output_zero_point};
  TfLiteAffineQuantization output_quant = {FloatArrayFromFloats(output_scales),
                                           IntArrayFromInts(output_zero_points),
                                           0};

  // Create output tensor of 1x2x2x1.
  int8_t output_data[4 * 2 * 2 * kOutDepth];
  int output_shape[] = {4, 1, 2, 2, kOutDepth};
  TfLiteIntArray* output_dims = IntArrayFromInts(output_shape);
  const int output_dims_count = ElementCount(*output_dims);
  TfLiteTensor output_tensor = CreateTensor(output_data, output_dims);
  output_tensor.params = {output_scale, output_zero_point};
  output_tensor.quantization = {kTfLiteAffineQuantization, &output_quant};

  // The 3 inputs include the input, filter and bias tensors.
  constexpr int inputs_size = 3;
  constexpr int outputs_size = 1;
  constexpr int tensors_size = inputs_size + outputs_size;
  TfLiteTensor tensors[tensors_size] = {
      input_tensor,
      filter_tensor,
      bias_tensor,
      output_tensor,
  };

  TfLiteConvParams conv_params{tflite::testing::common_conv_params};
  conv_params.padding = kTfLitePaddingSame;

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk, ValidateConvGoldens(tensors, tensors_size,
                                     kConvGoldenOutput4x4InputPaddingSame2x2,
                                     output_dims_count, &conv_params,
                                     tflite::Register_CONV_2D(), output_data,
                                     1.0 /* tolerance */));
}


TF_LITE_MICRO_TEST(Int8Filter1x3x3x1ShouldMatchGoldenOddInputPaddingSame) {
  using tflite::ElementCount;
  using tflite::kConvFilter1x3x3x1;
  using tflite::kConvGoldenOutput5x5InputPaddingSame3x3;
  using tflite::kConvInput1x5x5x1;
  using tflite::kConvZeroBias;
  using tflite::testing::CreateTensor;
  using tflite::testing::FloatArrayFromFloats;
  using tflite::testing::IntArrayFromInts;
  using tflite::testing::ValidateConvGoldens;

  constexpr int kInDepth = 1;
  constexpr int kOutDepth = 1;

  // Input quantization parameters: same scale and zero point for all input
  // elements.
  constexpr float kInputScale = 0.00392120517f;
  constexpr int kInputZeroPoint = -128;
  float input_scales[] = {1, kInputScale};
  int input_zero_points[] = {1, kInputZeroPoint};
  TfLiteAffineQuantization input_quant = {FloatArrayFromFloats(input_scales),
                                          IntArrayFromInts(input_zero_points),
                                          0};
  // Create input tensor of size 1x5x5x1.
  int input_shape[] = {4, 1, 5, 5, kInDepth}; // [len,N,H,W,C]
  TfLiteIntArray* input_dims = IntArrayFromInts(input_shape);
  TfLiteTensor input_tensor = CreateTensor(kConvInput1x5x5x1, input_dims);
  input_tensor.params = {kInputScale, kInputZeroPoint};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Filter quantization parameters.
  int filter_zero_points[kOutDepth + 1] = {kOutDepth, 0};
  float filter_scales[kOutDepth + 1] = {kOutDepth, 0.00448552053f};
  TfLiteAffineQuantization filter_quant;
  filter_quant.scale = FloatArrayFromFloats(filter_scales);
  filter_quant.zero_point = IntArrayFromInts(filter_zero_points);
  filter_quant.quantized_dimension = 0;

  // Create filter tensor of size 1x3x3x1.
  int filter_shape[] = {4, kOutDepth, 3, 3, kInDepth};
  TfLiteIntArray* filter_dims = IntArrayFromInts(filter_shape);
  TfLiteTensor filter_tensor = CreateTensor(kConvFilter1x3x3x1, filter_dims);
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Bias quantization parameters: same zero point, but different scale per
  // output channel.
  int bias_zero_points[kOutDepth + 1] = {kOutDepth, 0};
  float bias_scales[kOutDepth + 1] = {kOutDepth, 0.00001758864f};
  TfLiteAffineQuantization bias_quant;
  bias_quant.scale = FloatArrayFromFloats(bias_scales);
  bias_quant.zero_point = IntArrayFromInts(bias_zero_points);
  bias_quant.quantized_dimension = 0;

  // Create size 1 zero bias tensor.
  int bias_shape[] = {1, kOutDepth};
  TfLiteIntArray* bias_dims = IntArrayFromInts(bias_shape);
  TfLiteTensor bias_tensor = CreateTensor(kConvZeroBias, bias_dims);
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Output quantization parameters: same zero point and scale for all elements.
  const float output_scale = 0.00627814838f;
  const int output_zero_point = -7;
  float output_scales[] = {1, output_scale};
  int output_zero_points[] = {1, output_zero_point};
  TfLiteAffineQuantization output_quant = {FloatArrayFromFloats(output_scales),
                                           IntArrayFromInts(output_zero_points),
                                           0};

  // Create output tensor.
  int8_t output_data[1 * 3 * 3 * kOutDepth];    // [N,H,W,C]
  int output_shape[] = {4, 1, 3, 3, kOutDepth};
  TfLiteIntArray* output_dims = IntArrayFromInts(output_shape);
  const int output_dims_count = ElementCount(*output_dims);
  TfLiteTensor output_tensor = CreateTensor(output_data, output_dims);
  output_tensor.params = {output_scale, output_zero_point};
  output_tensor.quantization = {kTfLiteAffineQuantization, &output_quant};

  // The 3 inputs include the input, filter and bias tensors.
  constexpr int inputs_size = 3;
  constexpr int outputs_size = 1;
  constexpr int tensors_size = inputs_size + outputs_size;
  TfLiteTensor tensors[tensors_size] = {
      input_tensor,
      filter_tensor,
      bias_tensor,
      output_tensor,
  };

  TfLiteConvParams conv_params{tflite::testing::common_conv_params};
  conv_params.padding = kTfLitePaddingSame;

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk, ValidateConvGoldens(tensors, tensors_size,
                                     kConvGoldenOutput5x5InputPaddingSame3x3,
                                     output_dims_count, &conv_params,
                                     tflite::Register_CONV_2D(), output_data,
                                     1.0 /* tolerance */));
}

TF_LITE_MICRO_TEST(FilterDimsNotMatchingAffineQuantization) {
  const int output_dims_count = 12;
  int8_t output_data[output_dims_count];

  const float input_scale = 0.5f;
  const float output_scale = 1.0f;

  int8_t input_quantized[tflite::testing::kInputElements];
  int8_t filter_quantized[tflite::testing::kFilterElements];
  int32_t bias_quantized[tflite::testing::kBiasElements];
  int8_t golden_quantized[tflite::testing::kOutputElements];
  int zero_points[tflite::testing::kBiasElements + 1];
  float scales[tflite::testing::kBiasElements + 1];

  TfLiteIntArray* input_dims =
      tflite::testing::IntArrayFromInts(tflite::testing::kInputShape);
  TfLiteIntArray* filter_dims =
      tflite::testing::IntArrayFromInts(tflite::testing::kFilterShape);
  TfLiteIntArray* bias_dims =
      tflite::testing::IntArrayFromInts(tflite::testing::kBiasShape);
  TfLiteIntArray* output_dims =
      tflite::testing::IntArrayFromInts(tflite::testing::kOutputShape);

  int filter_zero_points[5];
  float filter_scales[5];
  TfLiteAffineQuantization filter_quant;
  TfLiteAffineQuantization bias_quant;
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      tflite::testing::kInputData, input_quantized, input_dims, input_scale, 0);
  TfLiteTensor filter_tensor =
      tflite::testing::CreateSymmetricPerChannelQuantizedTensor(
          tflite::testing::kFilterData, filter_quantized, filter_dims,
          filter_scales, filter_zero_points, &filter_quant,
          0 /* quantized dimension */);
  TfLiteTensor bias_tensor =
      tflite::testing::CreatePerChannelQuantizedBiasTensor(
          tflite::testing::kBiasData, bias_quantized, bias_dims, input_scale,
          &filter_scales[1], scales, zero_points, &bias_quant, 0);
  TfLiteTensor output_tensor = tflite::testing::CreateQuantizedTensor(
      output_data, output_dims, output_scale, 0 /* quantized dimension */);

  float input_scales[] = {1, input_scale};
  int input_zero_points[] = {1, 128};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  constexpr int inputs_size = 3;
  constexpr int outputs_size = 1;
  constexpr int tensors_size = inputs_size + outputs_size;
  TfLiteTensor tensors[tensors_size] = {
      input_tensor,
      filter_tensor,
      bias_tensor,
      output_tensor,
  };

  tflite::Quantize(tflite::testing::kGoldenData, golden_quantized,
                   output_dims_count, output_scale, 0);

  // Set filter quant to mismatched dimension.
  TfLiteAffineQuantization* quant = reinterpret_cast<TfLiteAffineQuantization*>(
      filter_tensor.quantization.params);

  // Choose arbitrary incorrect scale and zero point sizes which are neither 1
  // (for broadcast case) nor the quantized dimension size.
  quant->scale->size = 2;
  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteError, tflite::testing::ValidateConvGoldens(
                        tensors, tensors_size, golden_quantized,
                        output_dims_count, &tflite::testing::common_conv_params,
                        tflite::Register_CONV_2D(), output_data));

  // Set scale back to correct dimension, and make zero point array too short.
  quant->scale->size = tflite::testing::kFilterShape[0];
  quant->zero_point->size = 2;
  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteError, tflite::testing::ValidateConvGoldens(
                        tensors, tensors_size, golden_quantized,
                        output_dims_count, &tflite::testing::common_conv_params,
                        tflite::Register_CONV_2D(), output_data));
}

TF_LITE_MICRO_TEST(Int8Input32x1Filter32x32ShouldMatchGolden) {
  using tflite::kConvInput32x1;
  using tflite::kConvFilter32x32;
  using tflite::kConvInput32x1Golden1x32;
  using tflite::kConvInput1x32ZeroBias;
  constexpr int kSampleSize = 32;
  constexpr int kNumFilters = 32;
  int input_shape[] = {4, 1, 1, 1, kSampleSize};    // [len,N,H,W,C]
  int filter_shape[] = {4, kNumFilters, 1, 1, kSampleSize};
  int bias_shape[] = {1, kSampleSize};
  int output_shape[] = {4, 1, 1, 1, kNumFilters};

  TfLiteConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_height_factor = 1;
  conv_params.dilation_width_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 1;
  conv_params.padding = kTfLitePaddingValid;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);
  const int output_dims_count = tflite::ElementCount(*output_dims);

  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  int input_zero_point = 0;
  float input_scale = 1.0f;
  int filter_zero_point = 0;
  float filter_scale = 1.0f;
  int output_zero_point = 0;
  // Output scale of 50 is needed to accomodate a float range of [-6400, 6350]
  float output_scale = 50.0f;

  // Create per-tensor quantized int8_t input tensor.
  int8_t input_quantized[kSampleSize];
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
		  kConvInput32x1, input_quantized, input_dims, input_scale, input_zero_point);
  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  int8_t filter_quantized[kNumFilters * kSampleSize];
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
		  kConvFilter32x32, filter_quantized, filter_dims, filter_scale,
      filter_zero_point);
  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, filter_zero_point};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  int32_t bias_quantized[kSampleSize];
  tflite::SymmetricQuantize(kConvInput1x32ZeroBias, bias_quantized, kNumFilters,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized, bias_dims);

  // There is a single zero point of 0, and a single scale of
  // input_scale * filter_scale.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  int8_t output_quantized[kNumFilters];
  TfLiteTensor output_tensor = tflite::testing::CreateQuantizedTensor(
      output_quantized, output_dims, output_scale, output_zero_point);
  // Set zero point and scale arrays with a single element for each.
  int output_zero_points[] = {1, output_zero_point};
  float output_scales[] = {1, output_scale};
  TfLiteAffineQuantization output_quant = {
      tflite::testing::FloatArrayFromFloats(output_scales),
      tflite::testing::IntArrayFromInts(output_zero_points), 0};
  output_tensor.quantization = {kTfLiteAffineQuantization, &output_quant};

  // The 3 inputs include the input, filter and bias tensors.
  constexpr int kInputsSize = 3;
  constexpr int kOutputsSize = 1;
  constexpr int kTensorsSize = kInputsSize + kOutputsSize;
  TfLiteTensor tensors[kTensorsSize] = {
      input_tensor,
      filter_tensor,
      bias_tensor,
      output_tensor,
  };

  int8_t golden_quantized[kNumFilters];
  tflite::Quantize(kConvInput32x1Golden1x32, golden_quantized, output_dims_count,
                   output_scale, output_zero_point);

  // Rounding errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk, tflite::testing::ValidateConvGoldens(
                     tensors, kTensorsSize, golden_quantized, output_dims_count,
                     &conv_params, tflite::Register_CONV_2D(), output_quantized,
                     kQuantizationTolerance));
}


TF_LITE_MICRO_TEST(Int8Input1x17x17x16Filter10x3x3x16ShouldMatchGolden) {
  using tflite::Int8ConvCommonInput;
  using tflite::Int8ConvCommonFilter;
  using tflite::kConvInput1x17x17x16Golden1x8x8x10;
  using tflite::kConvCommonZeroBias;
  using tflite::input_quantized1x17x17x16;
  using tflite::filter_quantized10x1x17x17x16;
  using tflite::output_quantized1x8x8x10;
  using tflite::golden_quantized1x8x8x10;
  using tflite::bias_quantized17x17x16;
  constexpr int kNumFilters = 10;
  int input_shape[] = {4, 1, 17, 17, 16};   // [len,N,H,W,C]
  int filter_shape[] = {4, kNumFilters, 3, 3, 16};
  int bias_shape[] = {1, kNumFilters};
  int output_shape[] = {4, 1, 8, 8, kNumFilters};

  TfLiteConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_height_factor = 1;
  conv_params.dilation_width_factor = 1;
  conv_params.stride_height = 2;
  conv_params.stride_width = 2;
  conv_params.padding = kTfLitePaddingValid;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);
  const int output_dims_count = tflite::ElementCount(*output_dims);

  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  int input_zero_point = 0;
  float input_scale = 1.0f;
  int filter_zero_point = 0;
  float filter_scale = 1.0f;
  int output_zero_point = 0;
  // Output scale of 225 is needed to accomodate a float range of [-128, 127]
  float output_scale = 225.0f;

  // Create per-tensor quantized int8_t input tensor.
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
		  Int8ConvCommonInput, input_quantized1x17x17x16, input_dims, input_scale, input_zero_point);
  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
		  Int8ConvCommonFilter, filter_quantized10x1x17x17x16, filter_dims, filter_scale,
      filter_zero_point);
  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, filter_zero_point};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  tflite::SymmetricQuantize(kConvCommonZeroBias, bias_quantized17x17x16, kNumFilters,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized17x17x16, bias_dims);

  // There is a single zero point of 0, and a single scale of
  // input_scale * filter_scale.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  TfLiteTensor output_tensor = tflite::testing::CreateQuantizedTensor(
		  output_quantized1x8x8x10, output_dims, output_scale, output_zero_point);
  // Set zero point and scale arrays with a single element for each.
  int output_zero_points[] = {1, output_zero_point};
  float output_scales[] = {1, output_scale};
  TfLiteAffineQuantization output_quant = {
      tflite::testing::FloatArrayFromFloats(output_scales),
      tflite::testing::IntArrayFromInts(output_zero_points), 0};
  output_tensor.quantization = {kTfLiteAffineQuantization, &output_quant};

  // The 3 inputs include the input, filter and bias tensors.
  constexpr int kInputsSize = 3;
  constexpr int kOutputsSize = 1;
  constexpr int kTensorsSize = kInputsSize + kOutputsSize;
  TfLiteTensor tensors[kTensorsSize] = {
      input_tensor,
      filter_tensor,
      bias_tensor,
      output_tensor,
  };

  tflite::Quantize(kConvInput1x17x17x16Golden1x8x8x10, golden_quantized1x8x8x10, output_dims_count,
                   output_scale, output_zero_point);

  // Rounding errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk, tflite::testing::ValidateConvGoldens(
                     tensors, kTensorsSize, golden_quantized1x8x8x10, output_dims_count,
                     &conv_params, tflite::Register_CONV_2D(), output_quantized1x8x8x10,
                     kQuantizationTolerance));
}

TF_LITE_MICRO_TEST(Int8Input1x16x16x32Filter12x3x3x32ShouldMatchGolden) {
  using tflite::Int8ConvCommonInput;
  using tflite::Int8ConvCommonFilter;
  using tflite::kConvInput1x16x16x32Golden1x14x14x12;
  using tflite::kConvCommonZeroBias;
  using tflite::input_quantized1x16x16x32;
  using tflite::filter_quantized12x1x16x16x32;
  using tflite::output_quantized1x14x14x12;
  using tflite::golden_quantized1x14x14x12;
  using tflite::bias_quantized16x16x32Inp;
  constexpr int kNumFilters = 12;
  int input_shape[] = {4, 1, 16, 16, 32};  // [len,N,H,W,C]
  int filter_shape[] = {4, kNumFilters, 3, 3, 32};
  int bias_shape[] = {1, kNumFilters};
  int output_shape[] = {4, 1, 14, 14, kNumFilters};

  TfLiteConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_height_factor = 1;
  conv_params.dilation_width_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 1;
  conv_params.padding = kTfLitePaddingValid;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);
  const int output_dims_count = tflite::ElementCount(*output_dims);

  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  int input_zero_point = 0;
  float input_scale = 1.0f;
  int filter_zero_point = 0;
  float filter_scale = 1.0f;
  int output_zero_point = 0;
  // Output scale of 200 is needed to accomodate a float range of [-128, 127]
  float output_scale = 200.0f;

  // Create per-tensor quantized int8_t input tensor.
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
		  Int8ConvCommonInput, input_quantized1x16x16x32, input_dims, input_scale, input_zero_point);
  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
		  Int8ConvCommonFilter, filter_quantized12x1x16x16x32, filter_dims, filter_scale,
      filter_zero_point);
  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, filter_zero_point};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  tflite::SymmetricQuantize(kConvCommonZeroBias, bias_quantized16x16x32Inp, kNumFilters,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized16x16x32Inp, bias_dims);

  // There is a single zero point of 0, and a single scale of
  // input_scale * filter_scale.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  TfLiteTensor output_tensor = tflite::testing::CreateQuantizedTensor(
		  output_quantized1x14x14x12, output_dims, output_scale, output_zero_point);
  // Set zero point and scale arrays with a single element for each.
  int output_zero_points[] = {1, output_zero_point};
  float output_scales[] = {1, output_scale};
  TfLiteAffineQuantization output_quant = {
      tflite::testing::FloatArrayFromFloats(output_scales),
      tflite::testing::IntArrayFromInts(output_zero_points), 0};
  output_tensor.quantization = {kTfLiteAffineQuantization, &output_quant};

  // The 3 inputs include the input, filter and bias tensors.
  constexpr int kInputsSize = 3;
  constexpr int kOutputsSize = 1;
  constexpr int kTensorsSize = kInputsSize + kOutputsSize;
  TfLiteTensor tensors[kTensorsSize] = {
      input_tensor,
      filter_tensor,
      bias_tensor,
      output_tensor,
  };

  tflite::Quantize(kConvInput1x16x16x32Golden1x14x14x12, golden_quantized1x14x14x12, output_dims_count,
                   output_scale, output_zero_point);

  // Rounding errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk, tflite::testing::ValidateConvGoldens(
                     tensors, kTensorsSize, golden_quantized1x14x14x12, output_dims_count,
                     &conv_params, tflite::Register_CONV_2D(), output_quantized1x14x14x12,
                     kQuantizationTolerance));
}


TF_LITE_MICRO_TEST(Int8Input1x20x20x32Filter16x5x5x32ShouldMatchGolden) {
  using tflite::Int8ConvCommonInput;
  using tflite::Int8ConvCommonFilter;
  using tflite::kConvInput1x20x20x32Golden1x16x16x16;
  using tflite::kConvCommonZeroBias;
  using tflite::input_quantized1x20x20x32;
  using tflite::filter_quantized16x20x20x32;
  using tflite::output_quantized1x16x16x16;
  using tflite::golden_quantized1x16x16x16;
  using tflite::bias_quantized20x20x32Inp;
  constexpr int kNumFilters = 16;
  int input_shape[] = {4, 1, 20, 20, 32};   // [len,N,H,W,C]
  int filter_shape[] = {4, kNumFilters, 5, 5, 32};
  int bias_shape[] = {1, kNumFilters};
  int output_shape[] = {4, 1, 16, 16, kNumFilters};

  TfLiteConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_height_factor = 1;
  conv_params.dilation_width_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 1;
  conv_params.padding = kTfLitePaddingValid;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);
  const int output_dims_count = tflite::ElementCount(*output_dims);

  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  int input_zero_point = -128;
  float input_scale = 1.0f;
  int filter_zero_point = 0;
  float filter_scale = 1.0f;
  int output_zero_point = 0;
  // Output scale of 300 is needed to accomodate a float range of [-128, 127]
  float output_scale = 300.0f;

  // Create per-tensor quantized int8_t input tensor.
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
		  Int8ConvCommonInput, input_quantized1x20x20x32, input_dims, input_scale, input_zero_point);
  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
		  Int8ConvCommonFilter, filter_quantized16x20x20x32, filter_dims, filter_scale,
      filter_zero_point);
  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, filter_zero_point};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  tflite::SymmetricQuantize(kConvCommonZeroBias, bias_quantized20x20x32Inp, kNumFilters,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized20x20x32Inp, bias_dims);

  // There is a single zero point of 0, and a single scale of
  // input_scale * filter_scale.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  TfLiteTensor output_tensor = tflite::testing::CreateQuantizedTensor(
		  output_quantized1x16x16x16, output_dims, output_scale, output_zero_point);
  // Set zero point and scale arrays with a single element for each.
  int output_zero_points[] = {1, output_zero_point};
  float output_scales[] = {1, output_scale};
  TfLiteAffineQuantization output_quant = {
      tflite::testing::FloatArrayFromFloats(output_scales),
      tflite::testing::IntArrayFromInts(output_zero_points), 0};
  output_tensor.quantization = {kTfLiteAffineQuantization, &output_quant};

  // The 3 inputs include the input, filter and bias tensors.
  constexpr int kInputsSize = 3;
  constexpr int kOutputsSize = 1;
  constexpr int kTensorsSize = kInputsSize + kOutputsSize;
  TfLiteTensor tensors[kTensorsSize] = {
      input_tensor,
      filter_tensor,
      bias_tensor,
      output_tensor,
  };

  tflite::Quantize(kConvInput1x20x20x32Golden1x16x16x16, golden_quantized1x16x16x16, output_dims_count,
                   output_scale, output_zero_point);

   //Rounding errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk, tflite::testing::ValidateConvGoldens(
                     tensors, kTensorsSize, golden_quantized1x16x16x16, output_dims_count,
                     &conv_params, tflite::Register_CONV_2D(), output_quantized1x16x16x16,
                     kQuantizationTolerance));
}


TF_LITE_MICRO_TEST(Int8Input1x19x19x25Filter14x5x5x25ActRelu) {
  using tflite::Int8ConvCommonInput;
  using tflite::Int8ConvCommonFilter;
  using tflite::kConvInput1x19x19x25Golden1x8x8x14;
  using tflite::kConvCommonZeroBias;
  using tflite::input_quantized19x19x25;
  using tflite::filter_quantized14x19x19x25;
  using tflite::output_quantized8x8x14;
  using tflite::golden_quantized8x8x14;
  using tflite::bias_quantized19x19x25Inp;
  constexpr int kNumFilters = 14;
  int input_shape[] = {4, 1, 19, 19, 25};   // [len,N,H,W,C]
  int filter_shape[] = {4, kNumFilters, 5, 5, 25};
  int bias_shape[] = {1, kNumFilters};
  int output_shape[] = {4, 1, 8, 8, kNumFilters};

  TfLiteConvParams conv_params;
  conv_params.activation = kTfLiteActRelu;
  conv_params.dilation_height_factor = 1;
  conv_params.dilation_width_factor = 1;
  conv_params.stride_height = 2;
  conv_params.stride_width = 2;
  conv_params.padding = kTfLitePaddingValid;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);
  const int output_dims_count = tflite::ElementCount(*output_dims);

  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  int input_zero_point = -128;
  float input_scale = 1.0f;
  int filter_zero_point = 0;
  float filter_scale = 1.0f;
  int output_zero_point = 0;
  // Output scale of 200 is needed to accomodate a float range of [-128, 127]
  float output_scale = 200.0f;

  // Create per-tensor quantized int8_t input tensor.
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
		  Int8ConvCommonInput, input_quantized19x19x25, input_dims, input_scale, input_zero_point);
  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
		  Int8ConvCommonFilter, filter_quantized14x19x19x25, filter_dims, filter_scale,
      filter_zero_point);
  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, filter_zero_point};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  tflite::SymmetricQuantize(kConvCommonZeroBias, bias_quantized19x19x25Inp, kNumFilters,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized19x19x25Inp, bias_dims);

  // There is a single zero point of 0, and a single scale of
  // input_scale * filter_scale.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  TfLiteTensor output_tensor = tflite::testing::CreateQuantizedTensor(
		  output_quantized8x8x14, output_dims, output_scale, output_zero_point);
  // Set zero point and scale arrays with a single element for each.
  int output_zero_points[] = {1, output_zero_point};
  float output_scales[] = {1, output_scale};
  TfLiteAffineQuantization output_quant = {
      tflite::testing::FloatArrayFromFloats(output_scales),
      tflite::testing::IntArrayFromInts(output_zero_points), 0};
  output_tensor.quantization = {kTfLiteAffineQuantization, &output_quant};

  // The 3 inputs include the input, filter and bias tensors.
  constexpr int kInputsSize = 3;
  constexpr int kOutputsSize = 1;
  constexpr int kTensorsSize = kInputsSize + kOutputsSize;
  TfLiteTensor tensors[kTensorsSize] = {
      input_tensor,
      filter_tensor,
      bias_tensor,
      output_tensor,
  };

  tflite::Quantize(kConvInput1x19x19x25Golden1x8x8x14, golden_quantized8x8x14, output_dims_count,
                   output_scale, output_zero_point);

   //Rounding errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk, tflite::testing::ValidateConvGoldens(
                     tensors, kTensorsSize, golden_quantized8x8x14, output_dims_count,
                     &conv_params, tflite::Register_CONV_2D(), output_quantized8x8x14,
                     kQuantizationTolerance));
}



// This test is created based on
// https://github.com/tensorflow/tflite-micro/issues/329
// Input, output and filter are all 8 bits.
// Filter tensor is of dimension 8x3x3x3 with different scales per output
// channel. Some arbitrary parameters come from the above issue.
TF_LITE_MICRO_TEST(Int8Filter8x3x3x3PerChannelScaleRelu6ShouldMatchGolden) {
  using tflite::ElementCount;
  using tflite::kConvBiasQuantized8;
  using tflite::kConvFilter8x3x3x3;
  using tflite::kConvGoldenOutput1x16x16x8;
  using tflite::kConvInput1x32x32x3;
  using tflite::testing::CreateTensor;
  using tflite::testing::FloatArrayFromFloats;
  using tflite::testing::IntArrayFromInts;
  using tflite::testing::ValidateConvGoldens;
  using tflite::output_data16x16x8;

  constexpr int kInDepth = 3;
  constexpr int kOutDepth = 8;

  // Input quantization parameters: same scale and zero point for all input
  // elements.
  constexpr float kInputScale = 0.00784313772f;
  constexpr int kInputZeroPoint = -1;
  float input_scales[] = {1, kInputScale};
  int input_zero_points[] = {1, kInputZeroPoint};
  TfLiteAffineQuantization input_quant = {FloatArrayFromFloats(input_scales),
                                          IntArrayFromInts(input_zero_points),
                                          0};
  // Create input tensor of size 1x32x32x3.
  int input_shape[] = {4, 1, 32, 32, kInDepth};
  TfLiteIntArray* input_dims = IntArrayFromInts(input_shape);
  TfLiteTensor input_tensor = CreateTensor(kConvInput1x32x32x3, input_dims);
  input_tensor.params = {kInputScale, kInputZeroPoint};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Filter quantization parameters: same zero point, but different scale per
  // output channel.
  int filter_zero_points[kOutDepth + 1] = {kOutDepth, 0, 0, 0, 0, 0, 0, 0, 0};
  float filter_scales[kOutDepth + 1] = {
      kOutDepth,      2.18926089e-05, 0.00453596329,
      0.000504297379, 0.00184638216,  0.00596635276,
      0.000199135626, 0.0047677448,   0.00193942268};
  TfLiteAffineQuantization filter_quant;
  filter_quant.scale = FloatArrayFromFloats(filter_scales);
  filter_quant.zero_point = IntArrayFromInts(filter_zero_points);
  filter_quant.quantized_dimension = 0;

  // Create filter tensor of size 8x3x3x3.
  int filter_shape[] = {4, kOutDepth, 3, 3, kInDepth};
  TfLiteIntArray* filter_dims = IntArrayFromInts(filter_shape);
  TfLiteTensor filter_tensor = CreateTensor(kConvFilter8x3x3x3, filter_dims);
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Bias quantization parameters: same zero point, but different scale per
  // output channel.
  int bias_zero_points[kOutDepth + 1] = {kOutDepth, 0, 0, 0, 0, 0, 0, 0, 0};
  float bias_scales[kOutDepth + 1] = {
      kOutDepth,      1.71706745e-07, 3.5576184e-05,
      3.95527377e-06, 1.44814294e-05, 4.67949249e-05,
      1.56184819e-06, 3.73940784e-05, 1.52111588e-05};
  TfLiteAffineQuantization bias_quant;
  bias_quant.scale = FloatArrayFromFloats(bias_scales);
  bias_quant.zero_point = IntArrayFromInts(bias_zero_points);
  bias_quant.quantized_dimension = 0;

  // Create per output channel bias of size 8
  int bias_shape[] = {1, kOutDepth};
  TfLiteIntArray* bias_dims = IntArrayFromInts(bias_shape);
  TfLiteTensor bias_tensor = CreateTensor(kConvBiasQuantized8, bias_dims);
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Output quantization parameters: same zero point and scale for all elements.
  const float output_scale = 0.0235294122f;
  const int output_zero_point = -128;
  float output_scales[] = {1, output_scale};
  int output_zero_points[] = {1, output_zero_point};
  TfLiteAffineQuantization output_quant = {FloatArrayFromFloats(output_scales),
                                           IntArrayFromInts(output_zero_points),
                                           0};

  // Create output tensor of 16x16x8
  int output_shape[] = {4, 1, 16, 16, kOutDepth};
  TfLiteIntArray* output_dims = IntArrayFromInts(output_shape);
  const int output_dims_count = ElementCount(*output_dims);
  TfLiteTensor output_tensor = CreateTensor(output_data16x16x8, output_dims);
  output_tensor.params = {output_scale, output_zero_point};
  output_tensor.quantization = {kTfLiteAffineQuantization, &output_quant};

  // The 3 inputs include the input, filter and bias tensors.
  constexpr int inputs_size = 3;
  constexpr int outputs_size = 1;
  constexpr int tensors_size = inputs_size + outputs_size;
  TfLiteTensor tensors[tensors_size] = {
      input_tensor,
      filter_tensor,
      bias_tensor,
      output_tensor,
  };

  TfLiteConvParams conv_params{tflite::testing::common_conv_params};
  conv_params.activation = kTfLiteActRelu6;

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk,
      ValidateConvGoldens(tensors, tensors_size, kConvGoldenOutput1x16x16x8,
                          output_dims_count, &conv_params,
                          tflite::Register_CONV_2D(), output_data16x16x8,
                          1.0 /* tolerance */));
}

TF_LITE_MICRO_TESTS_END
