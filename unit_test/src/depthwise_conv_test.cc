
/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

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
#include <cstddef>
#include <cstdint>
#include "tensorflow/lite/c/builtin_op_data.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "tensorflow/lite/micro/kernels/kernel_runner.h"
#include "tensorflow/lite/micro/test_helpers.h"
#include "micro_test.h"

namespace tflite {
namespace testing {
namespace {

// Index of the output tensor in context->tensors, specific to
// DepthwiseConv.
constexpr int kOutputTensorIndex = 3;

constexpr int kMaxFilterChannels = 64;
constexpr int kMaxBiasChannels = 64;

// Creates a DepthwiseConv opeerator, calls it with the provided input tensors
// and some defaults parameters, and compares the output with
// expected_output_data.
//
// The tensors parameter contains both the input tensors as well as a
// preallocated output tensor into which the output is stored.
template <typename T>
TfLiteStatus ValidateDepthwiseConvGoldens(
    const T* expected_output_data, int output_length,
    TfLiteDepthwiseConvParams* conv_params, float tolerance, int tensors_size,
    TfLiteTensor* tensors) {
  int inputs_array_data[] = {3, 0, 1, 2};
  TfLiteIntArray* inputs_array = IntArrayFromInts(inputs_array_data);
  int outputs_array_data[] = {1, 3};
  TfLiteIntArray* outputs_array = IntArrayFromInts(outputs_array_data);

  //const TFLMRegistration registration = Register_DEPTHWISE_CONV_2D();
  const TfLiteRegistration_V1 registration = Register_DEPTHWISE_CONV_2D();
  micro::KernelRunner runner(registration, tensors, tensors_size, inputs_array,
                             outputs_array,
                             reinterpret_cast<void*>(conv_params));

  int input_depth = tensors[0].dims->data[3];
  int output_depth = tensors[1].dims->data[3];
  int depth_mul = output_depth / input_depth;

  conv_params->padding = kTfLitePaddingValid;
  conv_params->depth_multiplier = depth_mul;

  const char* init_data = reinterpret_cast<const char*>(conv_params);

  // TODO(b/154240825): Use a test macro here which fails and returns.
  TfLiteStatus status = runner.InitAndPrepare(init_data);
  if (status != kTfLiteOk) {
    return status;
  }
  TF_LITE_MICRO_EXPECT_EQ(kTfLiteOk, runner.Invoke());

  const T* output_data = tflite::GetTensorData<T>(&tensors[kOutputTensorIndex]);

  for (int i = 0; i < output_length; ++i) {
    TF_LITE_MICRO_EXPECT_NEAR(expected_output_data[i], output_data[i],
                              tolerance);
  }
  return kTfLiteOk;
}

template <typename T, typename BiasT>
void TestDepthwiseConvQuantizedPerChannel(
    int* input_dims_data, const float* input_data, T* input_quantized,
    float input_scale, int input_zero_point, int* filter_dims_data,
    const float* filter_data, int8_t* filter_data_quantized,
    int* bias_dims_data, const float* bias_data, BiasT* bias_data_quantized,
    int* output_dims_data, const float* expected_output_data,
    T* expected_output_data_quantized, T* output_data, float output_scale,
    int output_zero_point, TfLiteDepthwiseConvParams* conv_params,
    TfLiteType filter_packed_type = kTfLiteNoType) {
  TfLiteIntArray* input_dims = IntArrayFromInts(input_dims_data);
  TfLiteIntArray* filter_dims = IntArrayFromInts(filter_dims_data);
  TfLiteIntArray* bias_dims = IntArrayFromInts(bias_dims_data);
  TfLiteIntArray* output_dims = IntArrayFromInts(output_dims_data);
  const int output_dims_count = ElementCount(*output_dims);

  int filter_zero_points[kMaxFilterChannels];
  float filter_scales[kMaxFilterChannels];
  int bias_zero_points[kMaxBiasChannels];
  float bias_scales[kMaxBiasChannels];
  TfLiteAffineQuantization filter_quant;
  TfLiteAffineQuantization bias_quant;
  TfLiteTensor input_tensor = CreateQuantizedTensor(
      input_data, input_quantized, input_dims, input_scale, input_zero_point);
  TfLiteTensor filter_tensor = CreateSymmetricPerChannelQuantizedTensor(
      filter_data, filter_data_quantized, filter_dims, filter_scales,
      filter_zero_points, &filter_quant, 3 /* quantized dimension */, false,
      filter_packed_type);
  TfLiteTensor bias_tensor = CreatePerChannelQuantizedBiasTensor(
      bias_data, bias_data_quantized, bias_dims, input_scale, &filter_scales[1],
      bias_scales, bias_zero_points, &bias_quant, 3 /* quantized dimension */
  );
  TfLiteTensor output_tensor = CreateQuantizedTensor(
      output_data, output_dims, output_scale, input_zero_point);

  // TODO(njeff): Affine Quantization Params should be set on tensor creation.
  float input_scales[] = {1, input_scale};
  int input_zero_points[] = {1, input_zero_point};
  TfLiteAffineQuantization input_quant = {FloatArrayFromFloats(input_scales),
                                          IntArrayFromInts(input_zero_points),
                                          0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  float output_scales[] = {1, output_scale};
  int output_zero_points[] = {1, output_zero_point};
  TfLiteAffineQuantization output_quant = {FloatArrayFromFloats(output_scales),
                                           IntArrayFromInts(output_zero_points),
                                           0};
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

  Quantize(expected_output_data, expected_output_data_quantized,
           output_dims_count, output_scale, output_zero_point);

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk, ValidateDepthwiseConvGoldens(expected_output_data_quantized,
                                              output_dims_count, conv_params,
                                              1.0, tensors_size, tensors));
}

void TestDepthwiseConvQuantizedPerChannel(
    int* input_dims_data, const float* input_data, int8_t* input_quantized,
    float input_scale, int input_zero_point, int* filter_dims_data,
    const float* filter_data, int8_t* filter_data_quantized,
    int* bias_dims_data, const float* bias_data, int32_t* bias_data_quantized,
    int* output_dims_data, const float* expected_output_data,
    int8_t* expected_output_data_quantized, int8_t* output_data,
    float output_scale, int output_zero_point,
    TfLiteDepthwiseConvParams* conv_params,
    TfLiteType filter_packed_type = kTfLiteNoType) {
  return TestDepthwiseConvQuantizedPerChannel<int8_t, int32_t>(
      input_dims_data, input_data, input_quantized, input_scale,
      input_zero_point, filter_dims_data, filter_data, filter_data_quantized,
      bias_dims_data, bias_data, bias_data_quantized, output_dims_data,
      expected_output_data, expected_output_data_quantized, output_data,
      output_scale, output_zero_point, conv_params, filter_packed_type);
}

void TestDepthwiseConvQuantizedPerChannel(
    int* input_dims_data, const float* input_data, int16_t* input_quantized,
    float input_scale, int input_zero_point, int* filter_dims_data,
    const float* filter_data, int8_t* filter_data_quantized,
    int* bias_dims_data, const float* bias_data, int64_t* bias_data_quantized,
    int* output_dims_data, const float* expected_output_data,
    int16_t* expected_output_data_quantized, int16_t* output_data,
    float output_scale, int output_zero_point,
    TfLiteDepthwiseConvParams* conv_params,
    TfLiteType filter_packed_type = kTfLiteNoType) {
  return TestDepthwiseConvQuantizedPerChannel<int16_t, int64_t>(
      input_dims_data, input_data, input_quantized, input_scale,
      input_zero_point, filter_dims_data, filter_data, filter_data_quantized,
      bias_dims_data, bias_data, bias_data_quantized, output_dims_data,
      expected_output_data, expected_output_data_quantized, output_data,
      output_scale, output_zero_point, conv_params, filter_packed_type);
}

// Xtensa kernels do not support float activations., and the corresponding tests
// are disabled. As a result, helper functions that are only needed for float
// kernel tests also need to be ifdef'd out to avoid build errors due to unused
// functions.
#if !defined(XTENSA)
void TestDepthwiseConvFloat(int* input_dims_data, const float* input_data,
                            int* filter_dims_data, const float* filter_data,
                            int* bias_dims_data, const float* bias_data,
                            const float* expected_output_data,
                            int* output_dims_data,
                            TfLiteDepthwiseConvParams* conv_params,
                            float* output_data) {
  TfLiteIntArray* input_dims = IntArrayFromInts(input_dims_data);
  TfLiteIntArray* filter_dims = IntArrayFromInts(filter_dims_data);
  TfLiteIntArray* bias_dims = IntArrayFromInts(bias_dims_data);
  TfLiteIntArray* output_dims = IntArrayFromInts(output_dims_data);
  const int output_dims_count = ElementCount(*output_dims);

  constexpr int inputs_size = 3;
  constexpr int outputs_size = 1;
  constexpr int tensors_size = inputs_size + outputs_size;
  TfLiteTensor tensors[tensors_size] = {
      CreateTensor(input_data, input_dims),
      CreateTensor(filter_data, filter_dims),
      CreateTensor(bias_data, bias_dims),
      CreateTensor(output_data, output_dims),
  };

  ValidateDepthwiseConvGoldens(expected_output_data, output_dims_count,
                               conv_params, 1e-5, tensors_size, tensors);
}

#endif  // !defined(XTENSA)

}  // namespace
}  // namespace testing
}  // namespace tflite

using namespace micro_test;

int DepthWise_conv() {
  micro_test::tests_passed = 0;
  micro_test::tests_failed = 0;
  tflite::InitializeTest();

#if !defined(XTENSA)  // TODO(b/170322965): xtensa kernels are less general than
                      // reference kernels and we ifdef out test cases that are
                      // currently known to fail.
TF_LITE_MICRO_TEST(SimpleTest) {
  int input_shape[] = {4, 1, 3, 2, 2};
  const float input_values[] = {1, 2, 7, 8, 3, 4, 9, 10, 5, 6, 11, 12};
  int filter_shape[] = {4, 1, 2, 2, 4};
  const float filter_values[] = {1, 2, 3, 4, -9, 10,  -11, 12,
                                 5, 6, 7, 8, 13, -14, 15,  -16};
  int bias_shape[] = {4, 1, 1, 1, 4};
  const float bias_values[] = {1, 2, 3, 4};
  const float golden[] = {
      71, -34, 99, -20, 91, -26, 127, -4,
  };
  int output_shape[] = {4, 1, 2, 1, 4};
  const int output_dims_count = 8;
  float output_data[output_dims_count];

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 1;

  tflite::testing::TestDepthwiseConvFloat(
      input_shape, input_values, filter_shape, filter_values, bias_shape,
      bias_values, golden, output_shape, &conv_params, output_data);
}

TF_LITE_MICRO_TEST(SimpleTestRelu) {
  int input_shape[] = {4, 1, 3, 2, 2};
  const float input_values[] = {1, 2, 7, 8, 3, 4, 9, 10, 5, 6, 11, 12};
  int filter_shape[] = {4, 1, 2, 2, 4};
  const float filter_values[] = {1, 2, 3, 4, -9, 10,  -11, 12,
                                 5, 6, 7, 8, 13, -14, 15,  -16};
  int bias_shape[] = {4, 1, 1, 1, 4};
  const float bias_values[] = {1, 2, 3, 4};
  int output_shape[] = {4, 1, 2, 1, 4};
  const int output_dims_count = 8;
  const float golden_relu[] = {71, 0, 99, 0, 91, 0, 127, 0};
  float output_data[output_dims_count];

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActRelu;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 1;

  tflite::testing::TestDepthwiseConvFloat(
      input_shape, input_values, filter_shape, filter_values, bias_shape,
      bias_values, golden_relu, output_shape, &conv_params, output_data);
}

TF_LITE_MICRO_TEST(SimpleTestQuantizedPerChannelDepthMultiplier1) {
  const int input_elements = 12;
  int input_shape[] = {4, 1, 3, 2, 2};
  const float input_values[] = {1, 2, 7, 8, 3, 4, 9, 10, 5, 6, 11, 12};
  const int filter_elements = 8;
  int filter_shape[] = {4, 1, 2, 2, 2};
  const float filter_values[] = {1, 2, 3, 4, -9, 10, -11, 12};
  const int bias_elements = 2;
  int bias_shape[] = {4, 1, 1, 1, 2};
  const int output_elements = 4;
  const float bias_values[] = {1, 2};
  const float golden[] = {
      -103,
      127,
      -128,
      127,
  };
  int output_shape[] = {4, 1, 2, 1, 2};
  const int output_dims_count = 4;
  int8_t output_data[output_dims_count];

  const float input_scale = 1.0f;
  const float output_scale = 1.0f;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  int8_t input_quantized[input_elements];
  int8_t filter_quantized[filter_elements];
  int32_t bias_quantized[bias_elements];
  int8_t golden_quantized[output_elements];

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 1;

  tflite::testing::TestDepthwiseConvQuantizedPerChannel(
      input_shape, input_values, input_quantized, input_scale, input_zero_point,
      filter_shape, filter_values, filter_quantized, bias_shape, bias_values,
      bias_quantized, output_shape, golden, golden_quantized, output_data,
      output_scale, output_zero_point, &conv_params);
}

TF_LITE_MICRO_TEST(TestQuantizedPerChannelDepthMultiplier1Relu6) {
  const int input_elements = 24;
  int input_shape[] = {4, 1, 3, 2, 4};
  const float input_values[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  const int filter_elements = 16;
  int filter_shape[] = {4, 1, 2, 2, 4};
  const float filter_values[] = {0,  1, 8,   -2, -1, 2, -10, 0,
                                 -1, 3, -18, 0,  0,  4, 20,  -3};
  const int bias_elements = 4;
  int bias_shape[] = {4, 1, 1, 1, 4};
  const int output_elements = 8;
  const float bias_values[] = {1, 2, 3, 4};
  const float golden[] = {
      0, 6, 3, 0, 0, 6, 3, 0,
  };
  int output_shape[] = {4, 1, 2, 1, 4};
  int8_t output_data[output_elements];

  const float input_scale = 0.023529f;
  const float output_scale = 0.023529f;
  const int input_zero_point = -128;
  const int output_zero_point = -128;

  int8_t input_quantized[input_elements];
  int8_t filter_quantized[filter_elements];
  int32_t bias_quantized[bias_elements];
  int8_t golden_quantized[output_elements];

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActRelu6;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 1;

  tflite::testing::TestDepthwiseConvQuantizedPerChannel(
      input_shape, input_values, input_quantized, input_scale, input_zero_point,
      filter_shape, filter_values, filter_quantized, bias_shape, bias_values,
      bias_quantized, output_shape, golden, golden_quantized, output_data,
      output_scale, output_zero_point, &conv_params);
}

TF_LITE_MICRO_TEST(SimpleTestDilatedQuantizedPerChannel) {
  const int input_elements = 48;
  int input_shape[] = {4, 1, 4, 6, 2};
  const float input_values[] = {1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2,   // h = 0
                                3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4,   // h = 1
                                1, 2, 3, 4, 5, 6, 2, 6, 2, 4, 4, 2,   // h = 2
                                3, 2, 6, 5, 1, 4, 1, 2, 1, 4, 6, 3};  // h = 3
  const int filter_elements = 16;
  int filter_shape[] = {4, 1, 2, 2, 4};
  const float filter_values[] = {1, 2, 3, 4, -9, 10,  -11, 12,
                                 5, 6, 7, 8, 13, -14, 15,  -16};
  const int bias_elements = 4;
  int bias_shape[] = {4, 1, 1, 1, 4};
  const int output_elements = 24;
  const float bias_values[] = {1, 2, 3, 4};
  const float golden[] = {
      15, 2,  88, -48, 25, 14, 72, 0,  61, -2,  56, 48,  // h = 0
      -4, 52, 12, 48,  11, 70, 63, 40, 51, -30, 41, 48   // h = 1
  };
  int output_shape[] = {4, 1, 2, 3, 4};
  int8_t output_data[output_elements];

  const float input_scale = 0.5;
  const float output_scale = 1.0f;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  int8_t input_quantized[input_elements];
  int8_t filter_quantized[filter_elements];
  int32_t bias_quantized[bias_elements];
  int8_t golden_quantized[output_elements];

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 3;
  conv_params.dilation_height_factor = 2;
  conv_params.stride_height = 1;
  conv_params.stride_width = 1;

  tflite::testing::TestDepthwiseConvQuantizedPerChannel(
      input_shape, input_values, input_quantized, input_scale, input_zero_point,
      filter_shape, filter_values, filter_quantized, bias_shape, bias_values,
      bias_quantized, output_shape, golden, golden_quantized, output_data,
      output_scale, output_zero_point, &conv_params);
}

TF_LITE_MICRO_TEST(TestQuantizedPerChannelCompareWithFloat) {
  int input_dims[] = {4, 1, 2, 3, 2};
  const float input_data[] = {3, 2, 1, -1, -2, -3, 4, 3, 2, -2, -3, -4};
  int filter_dims[] = {4, 1, 2, 2, 4};
  const float filter_data[] = {1, 2, 3, 4, 3, 4, 5, 6, 7, 8, 5, 6, 3, 4, 1, 2};
  int bias_dims[] = {4, 1, 1, 1, 4};
  const float bias_data[] = {3, -2, 4, 6};
  int output_dims[] = {4, 1, 1, 2, 4};
  const float golden[] = {43, 48, 18, 22, 3, -4, -28, -36};

  const int input_size = 12;
  const int filter_size = 16;
  const int output_size = 8;
  const int bias_size = 4;
  int8_t input_quantized[input_size];
  int8_t filter_quantized[filter_size];
  int32_t bias_quantized[bias_size];
  int8_t golden_quantized[output_size];
  int8_t output_data[output_size];
  float output_float[output_size];

  const float input_scale = 0.5;
  const float output_scale = 1.0;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 1;

  tflite::testing::TestDepthwiseConvQuantizedPerChannel(
      input_dims, input_data, input_quantized, input_scale, input_zero_point,
      filter_dims, filter_data, filter_quantized, bias_dims, bias_data,
      bias_quantized, output_dims, golden, golden_quantized, output_data,
      output_scale, output_zero_point, &conv_params);

  tflite::testing::TestDepthwiseConvFloat(
      input_dims, input_data, filter_dims, filter_data, bias_dims, bias_data,
      golden, output_dims, &conv_params, output_float);
}

TF_LITE_MICRO_TEST(PerChannelBroadcastQuantizationParams) {
  const float input_scale = 1.0f;
  const float filter_scale = 1.0f;
  const float output_scale = 1.0f;

  const int input_elements = 12;
  int input_shape[] = {4, 1, 3, 2, 2};
  const float input_values[] = {1, 2, 7, 8, 3, 4, 9, 10, 5, 6, 11, 12};
  const int filter_elements = 16;
  int filter_shape[] = {4, 1, 2, 2, 4};
  const float filter_values[] = {1, 2, 3, 4, -9, 10,  -11, 12,
                                 5, 6, 7, 8, 13, -14, 15,  -16};
  const int bias_elements = 4;
  int bias_shape[] = {4, 1, 1, 1, 4};
  const int output_elements = 8;
  const float bias_values[] = {1, 2, 3, 4};
  const float golden[] = {
      71, -34, 99, -20, 91, -26, 127, -4,
  };
  int output_shape[] = {4, 1, 2, 1, 4};
  const int output_dims_count = 8;
  int8_t output_data[output_dims_count];

  int8_t input_quantized[input_elements];
  int8_t filter_quantized[filter_elements];
  int32_t bias_quantized[bias_elements];
  int8_t golden_quantized[output_elements];

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);

  // Create per-layer quantized int8_t input tensor.
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      input_values, input_quantized, input_dims, input_scale, 0);
  int input_zero_points[2] = {1, 0};
  float input_scales[2] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-layer quantized int8_t filter tensor.
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
      filter_values, filter_quantized, filter_dims, filter_scale, 0);
  int filter_zero_points[2] = {1, 0};
  float filter_scales[2] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-layer quantized int32_t bias tensor.
  tflite::SymmetricQuantize(bias_values, bias_quantized, bias_elements,
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
      output_data, output_dims, output_scale, 0);
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

  tflite::Quantize(golden, golden_quantized, output_dims_count, output_scale,
                   0);

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 1;

  TF_LITE_MICRO_EXPECT_EQ(
      kTfLiteOk, tflite::testing::ValidateDepthwiseConvGoldens(
                     golden_quantized, output_dims_count, &conv_params, 1e-5,
                     tensors_size, tensors));
}

#endif  // !defined(XTENSA)

TF_LITE_MICRO_TEST(FilterDimsNotMatchingAffineQuantization) {
  int input_shape[] = {4, 1, 2, 3, 2};
  const float input_data[] = {3, 2, 1, -1, -2, -3, 4, 3, 2, -2, -3, -4};
  int filter_shape[] = {4, 1, 2, 2, 4};
  const float filter_data[] = {1, 2, 3, 4, 3, 4, 5, 6, 7, 8, 5, 6, 3, 4, 1, 2};
  int bias_shape[] = {4, 1, 1, 1, 4};
  const float bias_data[] = {3, -2, 4, 6};
  int output_shape[] = {4, 1, 1, 2, 4};

  const int input_size = 12;
  const int filter_size = 16;
  const int output_size = 8;
  const int bias_size = 4;
  int8_t input_quantized[input_size];
  int8_t filter_quantized[filter_size];
  int32_t bias_quantized[bias_size];
  int8_t golden_quantized[output_size] = {};
  int zero_points[bias_size + 1];
  float scales[bias_size + 1];
  int8_t output_data[output_size];

  const float input_scale = 0.5;
  const float output_scale = 1.0;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);

  int filter_zero_points[5];
  float filter_scales[5];
  TfLiteAffineQuantization filter_quant;
  TfLiteAffineQuantization bias_quant;
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      input_data, input_quantized, input_dims, input_scale, input_zero_point);
  TfLiteTensor filter_tensor =
      tflite::testing::CreateSymmetricPerChannelQuantizedTensor(
          filter_data, filter_quantized, filter_dims, filter_scales,
          filter_zero_points, &filter_quant, 0 /* quantized dimension */);
  TfLiteTensor bias_tensor =
      tflite::testing::CreatePerChannelQuantizedBiasTensor(
          bias_data, bias_quantized, bias_dims, input_scale, &filter_scales[1],
          scales, zero_points, &bias_quant, 0);
  TfLiteTensor output_tensor = tflite::testing::CreateQuantizedTensor(
      output_data, output_dims, output_scale, output_zero_point);

  float input_scales[] = {1, input_scale};
  int input_zero_points[] = {1, input_zero_point};
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

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 1;

  // Set filter quant to mismatched dimension.
  TfLiteAffineQuantization* quant = reinterpret_cast<TfLiteAffineQuantization*>(
      filter_tensor.quantization.params);
  quant->scale->size = 2;
  TF_LITE_MICRO_EXPECT_EQ(kTfLiteError,
                          tflite::testing::ValidateDepthwiseConvGoldens(
                              golden_quantized, output_size, &conv_params, 1e-5,
                              tensors_size, tensors));

  // Set scale back to correct dimension, and make zero point array too short.
  quant->scale->size = filter_shape[0];
  quant->zero_point->size = 2;
  TF_LITE_MICRO_EXPECT_EQ(kTfLiteError,
                          tflite::testing::ValidateDepthwiseConvGoldens(
                              golden_quantized, output_size, &conv_params, 1e-5,
                              tensors_size, tensors));
}

TF_LITE_MICRO_TEST(Int8Input32x4Filter32x4ShouldMatchGolden) {
  const int input_elements = 32 * 4;
  const int filter_elements = 32 * 4;
  const int bias_elements = 32;
  const int output_elements = 32;
  int input_shape[] = {4, 1, 4, 1, 32};
  int filter_shape[] = {4, 1, 4, 1, 32};
  int bias_shape[] = {1, 32};
  int output_shape[] = {4, 1, 1, 1, 32};
  const float input_values[] = {
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001,
      11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001,
      9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942,
      9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
      11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177,
      11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177,
      9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765,
      9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
      10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766,
      11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118,
      9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589,
      8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
      11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648,
      10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354,
      9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707,
      9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883};
  const float filter_values[] = {
      -0.1617, -0.1948, 0.1419,  -0.2311, -0.0891, 0.1551,  0.0033,  0.3037,
      -0.1683, 0.1353,  0.1518,  -0.1683, -0.1386, 0.1452,  0.1816,  0.1716,
      -0.1948, 0.2080,  0.2245,  -0.1981, -0.2410, 0.1849,  0.1981,  0.1584,
      0.2509,  0.1783,  -0.2146, -0.1518, 0.2080,  -0.2872, 0.2014,  0.2476,
      -0.4126, -0.0561, -0.3235, -0.0594, -0.0957, 0.2014,  -0.1056, 0.1386,
      -0.2542, -0.1617, 0.1287,  -0.1816, -0.0363, 0.1419,  -0.0594, 0.2344,
      -0.0099, 0.4192,  0.1287,  -0.2311, -0.2212, -0.0528, -0.2080, 0.1816,
      -0.1452, 0.1221,  0.1254,  -0.1056, -0.0759, 0.1221,  0.1023,  0.1485,
      0.2707,  0.1716,  -0.1882, -0.1783, 0.1650,  -0.2740, 0.1915,  0.2080,
      -0.2971, -0.2575, -0.3169, 0.0198,  -0.0231, 0.2410,  -0.0429, 0.0660,
      -0.1816, 0.1981,  0.2014,  -0.1386, -0.1915, 0.1716,  0.1320,  0.1419,
      0.1320,  0.1353,  -0.1386, -0.1716, 0.1320,  -0.1650, 0.1386,  0.0825,
      -0.1419, -0.1023, 0.1783,  0.0462,  0.2047,  -0.2179, -0.1518, -0.1551,
      0.1518,  0.3334,  0.3103,  -0.2047, -0.2047, -0.0957, -0.1650, 0.1221,
      0.0990,  0.1353,  -0.1617, -0.1485, 0.1650,  -0.1816, 0.1518,  0.1254,
      -0.0363, -0.1254, 0.1386,  0.0429,  0.2113,  -0.2839, -0.1056, -0.2278};
  const float bias_values[] = {
      0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
      0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
      0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
      0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000};
  const float golden[] = {
      -5.1194, -2.0075, -2.1751, -4.7958, 1.7073,  -1.2963, -0.4641, 5.0416,
      -6.4424, 0.3836,  2.4684,  -4.7643, -3.8913, 3.8382,  -0.5164, 5.4304,
      -2.7400, 7.7016,  3.6115,  -6.8545, -3.6290, 0.8509,  2.3247,  5.6117,
      1.8215,  2.7645,  -0.7032, -3.2156, 3.9689,  -5.4583, 2.4346,  1.7731};

  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  const float input_scale = 0.058824;
  const float filter_scale = 0.003301;
  const float output_scale = 0.092596;
  const int input_zero_point = -128;
  const int output_zero_point = 0;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);

  // Create per-tensor quantized int8_t input tensor.
  int8_t input_quantized[input_elements];
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      input_values, input_quantized, input_dims, input_scale, input_zero_point);

  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  int8_t filter_quantized[filter_elements];
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
      filter_values, filter_quantized, filter_dims, filter_scale, 0);

  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, 0};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  int32_t bias_quantized[bias_elements];
  // See https://www.tensorflow.org/lite/performance/quantization_spec for a
  // detailed explanation of why bias scale is input_scale * filter_scale.
  tflite::SymmetricQuantize(bias_values, bias_quantized, bias_elements,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized, bias_dims);

  // Set zero point and scale arrays with a single element for each.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  int8_t output_quantized[output_elements];
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

  int8_t golden_quantized[output_elements];
  tflite::Quantize(golden, golden_quantized, output_elements, output_scale, 0);

  // Errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 1;
  tflite::testing::ValidateDepthwiseConvGoldens(
      golden_quantized, output_elements, &conv_params, kQuantizationTolerance,
      kTensorsSize, tensors);
}


TF_LITE_MICRO_TEST(Int8Input15x5x5Filter4x1x5ShouldMatchGolden) {
  const int input_elements = 15 * 5 * 5;
  const int filter_elements = 4 *1 * 5;
  const int bias_elements = 5;
  const int output_elements = 7 * 5 * 5;;
  int input_shape[] = {4, 1, 5, 15, 5};
  int filter_shape[] = {4, 1, 1, 4, 5};
  int bias_shape[] = {1, 5};
  int output_shape[] = {4, 1, 5, 7, 5};
  const float input_values[] = {
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
      11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
      10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
      11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177};
  const float filter_values[] = {
		  -0.1617, -0.1948, 0.1419,  -0.2311, -0.0891, 0.1551,  0.0033,  0.3037,
		  -0.1683, 0.1353,  0.1518,  -0.1683, -0.1386, 0.1452,  0.1816,  0.1716,
		  -0.1948, 0.2080,  0.2245,  -0.1981};
  const float bias_values[] = {
      0.0000, 0.0000, 0.0000, 0.0000, 0.0000};
  const float golden[] = {
		  2.5927, -5.4632, 4.8150, -1.2037, 0.0926, 2.5001, -4.7224, 4.4446, -0.4630, 0.6482, 3.1483, -4.9076, 4.0742, 0.1852, 0.2778, 3.7964, -4.6298, 5.0002, -0.7408, 0.4630, 2.5001, -5.4632, 4.9076, -0.3704, 0.0926, 2.9631, -5.3706, 5.0928, -0.1852, 0.3704, 1.8519, -3.3335,
		  2.8705, -1.8519, 1.9445, 3.0557, -4.8150, 4.5372, 0.0926, 0.4630, 2.7779, -5.2780, 4.0742, 0.1852, 0.0000, 3.2409, -5.4632, 5.0928, -0.8334, 0.3704, 2.5927, -4.6298, 4.6298, -0.4630, 0.4630, 3.2409, -5.0002, 5.1854, 0.2778, 0.3704, 3.3335, -5.0002, 4.5372, -0.8334,
		  0.0926, 1.0186, -3.7038, 2.5927, -2.5001, 1.9445, 3.4261, -5.4632, 4.2594, 0.9260, 0.0000, 2.5927, -5.4632, 5.0928, -1.1112, -0.0926, 2.6853, -4.9076, 4.4446, -0.4630, 0.6482, 3.2409, -4.8150, 4.2594, 0.3704, 0.3704, 3.7964, -4.5372, 5.1854, -0.7408, 0.5556, 2.5001,
		  -5.3706, 4.9076, -0.2778, 0.1852, 1.1112, -3.3335, 2.8705, -2.5001, 2.4075, 2.5927, -5.2780, 5.4632, -0.7408, 0.4630, 2.9631, -5.0002, 4.6298, 0.1852, 0.3704, 2.7779, -5.4632, 4.4446, 0.0926, 0.0926, 3.4261, -5.4632, 5.0928, -1.2037, 0.2778, 2.5001, -4.7224, 4.7224,
		  -0.4630, 0.5556, 2.9631, -4.9076, 4.8150, 0.4630, 0.4630, 1.9445, -3.3335, 2.7779, -2.7779, 1.8519, 2.4075, -4.8150, 4.3520, -0.3704, 0.4630, 3.2409, -5.4632, 4.4446, 0.7408, -0.0926, 2.7779, -5.4632, 5.0002, -1.0186, -0.0926, 2.7779, -4.9076, 4.2594, -0.5556, 0.5556,
		  3.1483, -4.9076, 3.9816, 0.1852, 0.3704, 3.7038, -4.6298, 5.0928, -0.6482, 0.5556, 1.1112, -3.6112, 2.9631, -2.3149, 1.8519};


  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  const float input_scale = 0.058824;
  const float filter_scale = 0.003301;
  const float output_scale = 0.092596;
  const int input_zero_point = -128;
  const int output_zero_point = 0;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);

  // Create per-tensor quantized int8_t input tensor.
  int8_t input_quantized[input_elements];
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      input_values, input_quantized, input_dims, input_scale, input_zero_point);

  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  int8_t filter_quantized[filter_elements];
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
      filter_values, filter_quantized, filter_dims, filter_scale, 0);

  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, 0};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  int32_t bias_quantized[bias_elements];
  // See https://www.tensorflow.org/lite/performance/quantization_spec for a
  // detailed explanation of why bias scale is input_scale * filter_scale.
  tflite::SymmetricQuantize(bias_values, bias_quantized, bias_elements,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized, bias_dims);

  // Set zero point and scale arrays with a single element for each.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  int8_t output_quantized[output_elements];
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

  int8_t golden_quantized[output_elements];
  tflite::Quantize(golden, golden_quantized, output_elements, output_scale, 0);

  // Errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 2;
  tflite::testing::ValidateDepthwiseConvGoldens(
      golden_quantized, output_elements, &conv_params, kQuantizationTolerance,
      kTensorsSize, tensors);
}


TF_LITE_MICRO_TEST(Int8Input8x8x6Filter2x2x6ActRelu) {
  const int input_elements = 8 * 8 * 6;
  const int filter_elements = 2 *2 * 6;
  const int bias_elements = 6;
  const int output_elements = 4 * 4 * 6;;
  int input_shape[] = {4, 1, 8, 8, 6};
  int filter_shape[] = {4, 1, 2, 2, 6};
  int bias_shape[] = {1, 6};
  int output_shape[] = {4, 1, 4, 4, 6};
  const float input_values[] = {
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
      11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
      10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
      11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883};
  const float filter_values[] = {
		  -0.1617, -0.1948, 0.1419,  -0.2311, -0.0891, 0.1551,  0.0033,  0.3037,
		  -0.1683, 0.1353,  0.1518,  -0.1683, -0.1386, 0.1452,  0.1816,  0.1716,
		  -0.1948, 0.2080,  0.2245,  -0.1981, -0.2410, 0.1849,  0.1981,  0.1584,};
  const float bias_values[] = {
      1.0000, 2.0000, 3.0000, 4.0000, 5.0000, 6.0000};
  const float golden[] = {
		  0.0000, 0.4191, 0.0000, 2.4310, 1.1736, 3.4370, 0.0000, 0.3353, 0.0000, 2.3472, 0.5868, 3.8561, 0.0000, 0.3353, 0.0000, 2.7664, 1.3413, 3.1017, 0.0000, 0.6706, 0.0000, 2.3472, 0.0838, 3.6046, 0.0000, 0.1677, 0.0000, 2.4310, 0.9221, 3.3532, 0.0000, 0.1677,
		  0.0000, 2.4310, 0.5868, 3.9400, 0.0000, 0.5030, 0.0000, 2.9340, 1.3413, 3.2693, 0.0000, 0.5868, 0.0000, 2.3472, 0.2515, 3.6046, 0.0000, 0.4191, 0.0000, 2.4310, 1.0059, 3.7723, 0.0000, 0.4191, 0.0000, 2.4310, 0.5030, 3.9400, 0.0000, 0.3353, 0.0000, 2.8502,
		  1.2574, 3.3532, 0.0000, 0.5030, 0.0000, 2.3472, 0.2515, 3.3532, 0.0000, 0.4191, 0.0000, 2.5149, 1.0898, 3.4370, 0.0000, 0.4191, 0.0000, 2.2634, 0.3353, 3.9400, 0.0000, 0.2515, 0.0000, 2.8502, 1.3413, 3.2693, 0.0000, 0.6706, 0.0000, 2.5149, 0.0838, 3.5208};


  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  const float input_scale = 0.071157;
  const float filter_scale = 0.005503;
  const float output_scale = 0.083829;
  const int input_zero_point = -128;
  const int output_zero_point = 0;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);

  // Create per-tensor quantized int8_t input tensor.
  int8_t input_quantized[input_elements];
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      input_values, input_quantized, input_dims, input_scale, input_zero_point);

  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  int8_t filter_quantized[filter_elements];
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
      filter_values, filter_quantized, filter_dims, filter_scale, 0);

  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, 0};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  int32_t bias_quantized[bias_elements];
  // See https://www.tensorflow.org/lite/performance/quantization_spec for a
  // detailed explanation of why bias scale is input_scale * filter_scale.
  tflite::SymmetricQuantize(bias_values, bias_quantized, bias_elements,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized, bias_dims);

  // Set zero point and scale arrays with a single element for each.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  int8_t output_quantized[output_elements];
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

  int8_t golden_quantized[output_elements];
  tflite::Quantize(golden, golden_quantized, output_elements, output_scale, 0);

  // Errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActRelu;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 2;
  conv_params.stride_width = 2;
  tflite::testing::ValidateDepthwiseConvGoldens(
      golden_quantized, output_elements, &conv_params, kQuantizationTolerance,
      kTensorsSize, tensors);
}


TF_LITE_MICRO_TEST(Int8Input10x10x3Filter4x4x3Relu6) {
  const int input_elements = 10 * 10 * 3;
  const int filter_elements = 4 *4 * 3;
  const int bias_elements = 3;
  const int output_elements = 4 * 4 * 3;
  int input_shape[] = {4, 1, 10, 10, 3};
  int filter_shape[] = {4, 1, 4, 4, 3};
  int bias_shape[] = {1, 3};
  int output_shape[] = {4, 1, 4, 4, 3};
  const float input_values[] = {
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
      11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
      10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
      11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236};
  const float filter_values[] = {
		  -0.1617, -0.1948, 0.1419,  -0.2311, -0.0891, 0.1551,  0.0033,  0.3037,
		  -0.1683, 0.1353,  0.1518,  -0.1683, -0.1386, 0.1452,  0.1816,  0.1716,
		  -0.1948, 0.2080,  0.2245,  -0.1981, -0.2410, 0.1849,  0.1981,  0.1584,
		   0.2509,  0.1783,  -0.2146, -0.1518, 0.2080,  -0.2872, 0.2014,  0.2476,
		  -0.4126, -0.0561, -0.3235, -0.0594, -0.0957, 0.2014,  -0.1056, 0.1386,
		  -0.2542, -0.1617, 0.1287,  -0.1816, -0.0363, 0.1419,  -0.0594, 0.2344};
  const float bias_values[] = {
      0.0000, 1.0000, 2.0000};
  const float golden[] = {
		  6.0187, 1.1112, 0.0000, 6.0187, 1.2037, 0.0000, 6.0187, 2.0371, 0.0000, 6.0187, 1.1112, 0.0000, 6.0187, 1.2037, 0.0000, 6.0187, 0.0000, 0.0000, 6.0187, 2.3149, 0.0000, 6.0187, 1.1112, 0.0000, 6.0187, 2.5927, 0.0000, 6.0187, 1.1112, 0.0000, 6.0187, 0.8334,
		  0.0000, 5.8335, 0.5556, 0.0000, 6.0187, 0.8334, 0.0000, 6.0187, 2.5927, 0.0000, 6.0187, 1.7593, 0.0000, 6.0187, 1.2963, 0.0000};


  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  const float input_scale = 0.058824;
  const float filter_scale = 0.003301;
  const float output_scale = 0.092596;
  const int input_zero_point = -128;
  const int output_zero_point = 0;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);

  // Create per-tensor quantized int8_t input tensor.
  int8_t input_quantized[input_elements];
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      input_values, input_quantized, input_dims, input_scale, input_zero_point);

  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  int8_t filter_quantized[filter_elements];
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
      filter_values, filter_quantized, filter_dims, filter_scale, 0);

  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, 0};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  int32_t bias_quantized[bias_elements];
  // See https://www.tensorflow.org/lite/performance/quantization_spec for a
  // detailed explanation of why bias scale is input_scale * filter_scale.
  tflite::SymmetricQuantize(bias_values, bias_quantized, bias_elements,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized, bias_dims);

  // Set zero point and scale arrays with a single element for each.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  int8_t output_quantized[output_elements];
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

  int8_t golden_quantized[output_elements];
  tflite::Quantize(golden, golden_quantized, output_elements, output_scale, 0);

  // Errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActRelu6;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 2;
  conv_params.stride_width = 2;
  tflite::testing::ValidateDepthwiseConvGoldens(
      golden_quantized, output_elements, &conv_params, kQuantizationTolerance,
      kTensorsSize, tensors);
}

TF_LITE_MICRO_TEST(Int8Input12x5x5Filter4x4x5ShouldMatchGolden) {
  const int input_elements = 12 * 5 * 5;
  const int filter_elements = 4 *4 * 5;
  const int bias_elements = 5;
  const int output_elements = 4 * 5 * 5;
  int input_shape[] = {4, 1, 5, 12, 5};
  int filter_shape[] = {4, 1, 4, 4, 5};
  int bias_shape[] = {1, 5};
  int output_shape[] = {4, 1, 5, 4, 5};
  const float input_values[] = {
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
      11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
      10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
      11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236};
  const float filter_values[] = {
		  -0.1617, -0.1948, 0.1419,  -0.2311, -0.0891, 0.1551,  0.0033,  0.3037, -0.1683, 0.1353,  0.1518,  -0.1683, -0.1386, 0.1452,  0.1816,  0.1716,
		  -0.1948, 0.2080,  0.2245,  -0.1981, -0.2410, 0.1849,  0.1981,  0.1584, 0.2509,  0.1783,  -0.2146, -0.1518, 0.2080,  -0.2872, 0.2014,  0.2476,
		  -0.4126, -0.0561, -0.3235, -0.0594, -0.0957, 0.2014,  -0.1056, 0.1386, -0.2542, -0.1617, 0.1287,  -0.1816, -0.0363, 0.1419,  -0.0594, 0.2344,
		  -0.0099, 0.4192,  0.1287,  -0.2311, -0.2212, -0.0528, -0.2080, 0.1816, -0.1452, 0.1221,  0.1254,  -0.1056, -0.0759, 0.1221,  0.1023,  0.1485,
		   0.2707,  0.1716,  -0.1882, -0.1783, 0.1650,  -0.2740, 0.1915,  0.2080, -0.2971, -0.2575, -0.3169, 0.0198,  -0.0231, 0.2410,  -0.0429, 0.0660};
  const float bias_values[] = {
      1.0000, 2.0000, 3.0000, 4.0000, 5.0000};
  const float golden[] = {
		  8.9813, -10.0524, 2.4719, -0.5768, -2.8839, 7.4981, -8.4869, 3.6255, 0.7416, -2.7191, 9.3933, -8.8989, 4.6142, 1.2360, -6.3446, 4.6966, -3.9551, -3.1311, -0.7416, -3.5431, 9.5581, -7.5805, 4.3670, 0.9888, -4.6142, 8.5693, -7.1685, 4.6142, 1.4831, -2.4719, 9.3109, -7.1685,
		  4.9438, -0.1648, -3.0487, 5.0262, -4.1198, -2.2247, -0.4120, -0.9888, 5.3558, -10.5468, 5.2734, 0.4120, -0.9064, 5.3558, -10.5468, 5.6854, -0.2472, 0.4120, 5.6854, -9.3109, 6.0150, 2.3895, -1.6479, 2.8839, -5.4382, 0.9888, -1.8127, 0.4120, 3.8727, -4.1198, 3.4607, 2.2247,
		  -0.8240, 4.6966, -3.3783, 3.2959, 2.4719, -1.5655, 4.2022, -3.7903, 3.9551, 0.9888, -0.6592, 3.0487, -1.8127, -0.9888, 0.4944, -0.4944, 2.5543, -4.9438, 5.4382, 0.3296, -0.2472, 3.4607, -5.2734, 5.2734, -0.9888, 0.4944, 2.9663, -4.4494, 4.3670, 0.9064, -0.0824, 1.9775,
		  -3.1311, 2.9663, -2.6367, 2.0599};


  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  const float input_scale = 0.068824;
  const float filter_scale = 0.004301;
  const float output_scale = 0.082397;
  const int input_zero_point = -128;
  const int output_zero_point = 0;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);

  // Create per-tensor quantized int8_t input tensor.
  int8_t input_quantized[input_elements];
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      input_values, input_quantized, input_dims, input_scale, input_zero_point);

  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  int8_t filter_quantized[filter_elements];
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
      filter_values, filter_quantized, filter_dims, filter_scale, 0);

  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, 0};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  int32_t bias_quantized[bias_elements];
  // See https://www.tensorflow.org/lite/performance/quantization_spec for a
  // detailed explanation of why bias scale is input_scale * filter_scale.
  tflite::SymmetricQuantize(bias_values, bias_quantized, bias_elements,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized, bias_dims);

  // Set zero point and scale arrays with a single element for each.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  int8_t output_quantized[output_elements];
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

  int8_t golden_quantized[output_elements];
  tflite::Quantize(golden, golden_quantized, output_elements, output_scale, 0);

  // Errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 3;
  tflite::testing::ValidateDepthwiseConvGoldens(
      golden_quantized, output_elements, &conv_params, kQuantizationTolerance,
      kTensorsSize, tensors);
}

TF_LITE_MICRO_TEST(Int8Input11x11x3Filter4x4x3padding2x2ActTanh) {
  const int input_elements = 11 * 11 * 3;
  const int filter_elements = 4 * 4 * 3;
  const int bias_elements = 3;
  const int output_elements = 4 * 4 * 3;;
  int input_shape[] = {4, 1, 11, 11, 3};
  int filter_shape[] = {4, 1, 4, 4, 3};
  int bias_shape[] = {1, 3};
  int output_shape[] = {4, 1, 4, 4, 3};
  const float input_values[] = {
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
      11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
      10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
      11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883};
  const float filter_values[] = {
		  -0.1617, -0.1948, 0.1419,  -0.2311, -0.0891, 0.1551,  0.0033,  0.3037, -0.1683, 0.1353,  0.1518,  -0.1683, -0.1386, 0.1452,  0.1816,  0.1716,
		  -0.1948, 0.2080,  0.2245,  -0.1981, -0.2410, 0.1849,  0.1981,  0.1584, 0.2509,  0.1783,  -0.2146, -0.1518, 0.2080,  -0.2872, 0.2014,  0.2476,
		  -0.4126, -0.0561, -0.3235, -0.0594, -0.0957, 0.2014,  -0.1056, 0.1386, -0.2542, -0.1617, 0.1287,  -0.1816, -0.0363, 0.1419,  -0.0594, 0.2344};
  const float bias_values[] = {
      1.0000, 2.0000, 3.0000};
  const float golden[] = {
		  7.4151, 1.7112, -7.9040, 6.7633, 1.7112, -6.6003, 5.7854, 1.3038, -5.8669, -1.6297, -0.6519, -1.1408, 6.6818, 1.2223, -6.5188, 5.9484, 1.9556, -6.3558, 5.7039, 1.1408, -5.3780, -1.4667, 0.8963, -0.0815, 6.6818, 1.9556, -5.9484, 6.4373, 1.3852, -6.8447, 7.8226, 0.5704,
		  -8.4744, -2.0371, 0.1630, -0.8963, 1.7927, 1.7112, 2.7705, 1.4667, 0.8149, 2.3631, 2.6890, 1.5482, 2.2001, -3.3409, -3.4224, 6.7633};


  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  const float input_scale = 0.069935;
  const float filter_scale = 0.004412;
  const float output_scale = 0.081485;
  const int input_zero_point = -128;
  const int output_zero_point = 0;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);

  // Create per-tensor quantized int8_t input tensor.
  int8_t input_quantized[input_elements];
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      input_values, input_quantized, input_dims, input_scale, input_zero_point);

  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  int8_t filter_quantized[filter_elements];
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
      filter_values, filter_quantized, filter_dims, filter_scale, 0);

  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, 0};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  int32_t bias_quantized[bias_elements];
  // See https://www.tensorflow.org/lite/performance/quantization_spec for a
  // detailed explanation of why bias scale is input_scale * filter_scale.
  tflite::SymmetricQuantize(bias_values, bias_quantized, bias_elements,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized, bias_dims);

  // Set zero point and scale arrays with a single element for each.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  int8_t output_quantized[output_elements];
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

  int8_t golden_quantized[output_elements];
  tflite::Quantize(golden, golden_quantized, output_elements, output_scale, 0);

  // Errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActTanh;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 3;
  conv_params.stride_width = 3;
  tflite::testing::ValidateDepthwiseConvGoldens(
      golden_quantized, output_elements, &conv_params, kQuantizationTolerance,
      kTensorsSize, tensors);
}

TF_LITE_MICRO_TEST(Int8Input13x13x4Filter4x4x4ActSigmoid) {
  const int input_elements = 13 * 13 * 4;
  const int filter_elements = 4 * 4 * 4;
  const int bias_elements = 4;
  const int output_elements = 10 * 10 * 4;;
  int input_shape[] = {4, 1, 13, 13, 4};
  int filter_shape[] = {4, 1, 4, 4, 4};
  int bias_shape[] = {1, 4};
  int output_shape[] = {4, 1, 10, 10, 4};
  const float input_values[] = {
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
      11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
      10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
      11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354};
  const float filter_values[] = {
		  -0.1617, -0.1948, 0.1419,  -0.2311, -0.0891, 0.1551,  0.0033,  0.3037, -0.1683, 0.1353,  0.1518,  -0.1683, -0.1386, 0.1452,  0.1816,  0.1716,
		  -0.1948, 0.2080,  0.2245,  -0.1981, -0.2410, 0.1849,  0.1981,  0.1584, 0.2509,  0.1783,  -0.2146, -0.1518, 0.2080,  -0.2872, 0.2014,  0.2476,
		  -0.4126, -0.0561, -0.3235, -0.0594, -0.0957, 0.2014,  -0.1056, 0.1386, -0.2542, -0.1617, 0.1287,  -0.1816, -0.0363, 0.1419,  -0.0594, 0.2344,
		  -0.0099, 0.4192,  0.1287,  -0.2311, -0.2212, -0.0528, -0.2080, 0.1816, -0.1452, 0.1221,  0.1254,  -0.1056, -0.0759, 0.1221,  0.1023,  0.1485};
  const float bias_values[] = {
      0.0000, 1.0000, 3.0000, 5.0000};
  const float golden[] = {
		  -9.2717, 9.1992, 6.7365, 3.6218, -9.2717, 9.1992, 6.0121, 1.8109, -9.2717, 9.1992, 5.5775, 2.6077, -9.2717, 9.1992, 6.0845, 3.4044, -9.2717, 9.1992, 6.6640, 1.6660, -9.2717, 9.1992, 5.9397, 3.1871, -9.2717, 9.1992, 5.4326, 2.0282, -9.2717, 9.1992, 6.0121, 2.9698,
		  -9.2717, 9.1992, 6.4467, 3.5493, -9.2717, 9.1992, 5.3602, 2.0282, -9.2717, 9.1992, 6.0845, 3.0423, -9.2717, 9.1992, 5.6499, 2.1730, -9.2717, 9.1992, 6.2294, 2.9698, -9.2717, 9.1992, 6.5191, 3.8391, -9.2717, 9.1992, 5.8672, 1.7384, -9.2717, 9.1992, 6.2294, 2.8974,
		  -9.2717, 9.1992, 6.4467, 2.8974, -9.2717, 9.1992, 6.5191, 1.9557, -9.2717, 9.1992, 5.9397, 2.7525, -9.2717, 9.1992, 5.9397, 2.6077, -9.2717, 9.1992, 6.2294, 2.8250, -9.2717, 9.1992, 6.0121, 2.8250, -9.2717, 9.1992, 6.4467, 2.1006, -9.2717, 9.1992, 6.3743, 2.7525,
		  -9.2717, 9.1992, 5.7948, 2.4628, -9.2717, 9.1992, 6.0121, 2.8974, -9.2717, 9.1992, 6.7365, 3.6218, -9.2717, 9.1992, 6.0121, 1.8109, -9.2717, 9.1992, 5.5775, 2.6077, -9.2717, 9.1992, 6.0845, 3.4044, -9.2717, 9.1992, 6.0121, 2.9698, -9.2717, 9.1992, 6.4467, 3.5493,
		  -9.2717, 9.1992, 5.3602, 2.0282, -9.2717, 9.1992, 5.9397, 2.4628, -9.2717, 9.1992, 6.5916, 3.3320, -9.2717, 9.1992, 6.5916, 1.8109, -9.2717, 9.1992, 6.0845, 3.0423, -9.2717, 9.1992, 5.6499, 2.1730, -9.2717, 9.1992, 6.2294, 2.9698, -9.2717, 9.1992, 6.5191, 3.8391,
		  -9.2717, 9.1992, 6.5191, 1.9557, -9.2717, 9.1992, 5.9397, 2.7525, -9.2717, 9.1992, 5.9397, 2.6077, -9.2717, 9.1992, 5.6499, 2.6801, -9.2717, 9.1992, 6.3018, 3.9115, -9.2717, 9.1992, 5.8672, 1.5936, -9.2717, 9.1992, 6.2294, 2.8250, -9.2717, 9.1992, 6.0121, 2.8250,
		  -9.2717, 9.1992, 6.4467, 2.1006, -9.2717, 9.1992, 6.3743, 2.7525, -9.2717, 9.1992, 6.0121, 1.8109, -9.2717, 9.1992, 5.5775, 2.6077, -9.2717, 9.1992, 6.0845, 3.4044, -9.2717, 9.1992, 6.6640, 1.6660, -9.2717, 9.1992, 5.9397, 3.1871, -9.2717, 9.1992, 5.4326, 2.0282,
		  -9.2717, 9.1992, 6.0121, 2.9698, -9.2717, 9.1992, 6.4467, 3.5493, -9.2717, 9.1992, 5.3602, 2.0282, -9.2717, 9.1992, 5.9397, 2.4628, -9.2717, 9.1992, 5.6499, 2.1730, -9.2717, 9.1992, 6.2294, 2.9698, -9.2717, 9.1992, 6.5191, 3.8391, -9.2717, 9.1992, 5.8672, 1.7384,
		  -9.2717, 9.1992, 6.2294, 2.8974, -9.2717, 9.1992, 6.4467, 2.8974, -9.2717, 9.1992, 6.5191, 1.9557, -9.2717, 9.1992, 5.9397, 2.7525, -9.2717, 9.1992, 5.9397, 2.6077, -9.2717, 9.1992, 5.6499, 2.6801, -9.2717, 9.1992, 6.0121, 2.8250, -9.2717, 9.1992, 6.4467, 2.1006,
		  -9.2717, 9.1992, 6.3743, 2.7525, -9.2717, 9.1992, 5.7948, 2.4628, -9.2717, 9.1992, 6.0121, 2.8974, -9.2717, 9.1992, 6.7365, 3.6218, -9.2717, 9.1992, 6.0121, 1.8109, -9.2717, 9.1992, 5.5775, 2.6077, -9.2717, 9.1992, 6.0845, 3.4044, -9.2717, 9.1992, 6.6640, 1.6660,
		  -9.2717, 9.1992, 6.4467, 3.5493, -9.2717, 9.1992, 5.3602, 2.0282, -9.2717, 9.1992, 5.9397, 2.4628, -9.2717, 9.1992, 6.5916, 3.3320, -9.2717, 9.1992, 6.5916, 1.8109, -9.2717, 9.1992, 6.0845, 3.0423, -9.2717, 9.1992, 5.6499, 2.1730, -9.2717, 9.1992, 6.2294, 2.9698,
		  -9.2717, 9.1992, 6.5191, 3.8391, -9.2717, 9.1992, 5.8672, 1.7384, -9.2717, 9.1992, 5.9397, 2.7525, -9.2717, 9.1992, 5.9397, 2.6077, -9.2717, 9.1992, 5.6499, 2.6801, -9.2717, 9.1992, 6.3018, 3.9115, -9.2717, 9.1992, 5.8672, 1.5936, -9.2717, 9.1992, 6.2294, 2.8250,
		  -9.2717, 9.1992, 6.0121, 2.8250, -9.2717, 9.1992, 6.4467, 2.1006, -9.2717, 9.1992, 6.3743, 2.7525, -9.2717, 9.1992, 5.7948, 2.4628};


  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  const float input_scale = 0.068824;
  const float filter_scale = 0.004301;
  const float output_scale = 0.072435;
  const int input_zero_point = -128;
  const int output_zero_point = 0;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);

  // Create per-tensor quantized int8_t input tensor.
  int8_t input_quantized[input_elements];
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      input_values, input_quantized, input_dims, input_scale, input_zero_point);

  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  int8_t filter_quantized[filter_elements];
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
      filter_values, filter_quantized, filter_dims, filter_scale, 0);

  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, 0};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  int32_t bias_quantized[bias_elements];
  // See https://www.tensorflow.org/lite/performance/quantization_spec for a
  // detailed explanation of why bias scale is input_scale * filter_scale.
  tflite::SymmetricQuantize(bias_values, bias_quantized, bias_elements,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized, bias_dims);

  // Set zero point and scale arrays with a single element for each.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  int8_t output_quantized[output_elements];
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

  int8_t golden_quantized[output_elements];
  tflite::Quantize(golden, golden_quantized, output_elements, output_scale, 0);

  // Errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActSigmoid;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 1;
  tflite::testing::ValidateDepthwiseConvGoldens(
      golden_quantized, output_elements, &conv_params, kQuantizationTolerance,
      kTensorsSize, tensors);
}


TF_LITE_MICRO_TEST(Int8Input12x10x4Filter5x5x4ShouldMatchGolden) {
  const int input_elements = 12 * 10 * 4;
  const int filter_elements = 5 * 5 * 4;
  const int bias_elements = 4;
  const int output_elements = 8 * 6 * 4;;
  int input_shape[] = {4, 1, 10, 12, 4};
  int filter_shape[] = {4, 1, 5, 5, 4};
  int bias_shape[] = {1, 4};
  int output_shape[] = {4, 1, 6, 8, 4};
  const float input_values[] = {
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
      11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
      10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
      11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530};
  const float filter_values[] = {
		  -0.1617, -0.1948, 0.1419,  -0.2311, -0.0891, 0.1551,  0.0033,  0.3037, -0.1683, 0.1353,  0.1518,  -0.1683, -0.1386, 0.1452,  0.1816,  0.1716,
		  -0.1948, 0.2080,  0.2245,  -0.1981, -0.2410, 0.1849,  0.1981,  0.1584, 0.2509,  0.1783,  -0.2146, -0.1518, 0.2080,  -0.2872, 0.2014,  0.2476,
		  -0.4126, -0.0561, -0.3235, -0.0594, -0.0957, 0.2014,  -0.1056, 0.1386, -0.2542, -0.1617, 0.1287,  -0.1816, -0.0363, 0.1419,  -0.0594, 0.2344,
		  -0.0099, 0.4192,  0.1287,  -0.2311, -0.2212, -0.0528, -0.2080, 0.1816, -0.1452, 0.1221,  0.1254,  -0.1056, -0.0759, 0.1221,  0.1023,  0.1485,
		   0.2707,  0.1716,  -0.1882, -0.1783, 0.1650,  -0.2740, 0.1915,  0.2080, -0.2971, -0.2575, -0.3169, 0.0198,  -0.0231, 0.2410,  -0.0429, 0.0660,
		  -0.1816, 0.1981,  0.2014,  -0.1386, -0.1915, 0.1716,  0.1320,  0.1419, 0.1320,  0.1353,  -0.1386, -0.1716, 0.1320,  -0.1650, 0.1386,  0.0825,
		  -0.1419, -0.1023, 0.1783,  0.0462};
  const float bias_values[] = {
      0.0000, 0.0000, 0.0000, 0.0000};
  const float golden[] = {
		  -11.8523, 11.7597, 9.6300, 2.8705, -11.8523, 11.7597, 5.0002, 2.0371, -11.8523, 10.4633, 7.7781, 4.3520, -11.8523, 11.3893, 5.0002, 2.5927, -11.8523, 11.7597, 9.7226, 2.2223, -11.8523, 11.7597, 6.7595, 4.5372, -11.8523, 11.7597, 8.6114, 1.3889, -11.8523, 11.7597, 5.2780, 4.4446,
		  -11.8523, 11.7597, 9.9078, 2.2223, -11.8523, 11.7597, 6.9447, 4.5372, -11.8523, 11.7597, 8.4262, 1.3889, -11.8523, 11.7597, 5.8335, 4.3520, -11.8523, 11.7597, 9.3522, 2.7779, -11.8523, 11.7597, 5.0002, 2.1297, -11.8523, 11.2967, 8.3336, 4.4446, -11.8523, 11.7597, 4.8150, 2.5001,
		  -11.8523, 11.7597, 9.1670, 3.0557, -11.8523, 11.7597, 5.2780, 2.0371, -11.8523, 11.2041, 8.1484, 4.7224, -11.8523, 11.7597, 4.8150, 2.3149, -11.8523, 11.7597, 9.8152, 2.5927, -11.8523, 11.7597, 7.1299, 4.2594, -11.8523, 11.7597, 9.1670, 1.7593, -11.8523, 11.7597, 5.6484, 4.3520,
		  -11.8523, 11.7597, 9.7226, 2.2223, -11.8523, 11.7597, 6.7595, 4.5372, -11.8523, 11.7597, 8.6114, 1.3889, -11.8523, 11.7597, 5.2780, 4.4446, -11.8523, 11.7597, 9.4448, 2.5927, -11.8523, 11.7597, 4.8150, 2.2223, -11.8523, 10.2782, 8.5188, 4.1668, -11.8523, 11.3893, 5.0002, 2.6853,
		  -11.8523, 11.7597, 9.3522, 2.7779, -11.8523, 11.7597, 5.0002, 2.1297, -11.8523, 11.2967, 8.3336, 4.4446, -11.8523, 11.7597, 4.8150, 2.5001, -11.8523, 11.7597, 9.5374, 2.5001, -11.8523, 11.7597, 6.5743, 4.3520, -11.8523, 11.7597, 8.3336, 1.7593, -11.8523, 11.7597, 5.8335, 4.1668,
		  -11.8523, 11.7597, 9.8152, 2.5927, -11.8523, 11.7597, 7.1299, 4.2594, -11.8523, 11.7597, 9.1670, 1.7593, -11.8523, 11.7597, 5.6484, 4.3520, -11.8523, 11.7597, 9.6300, 2.8705, -11.8523, 11.7597, 5.0002, 2.0371, -11.8523, 10.4633, 7.7781, 4.3520, -11.8523, 11.3893, 5.0002, 2.5927};


  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  const float input_scale = 0.058824;
  const float filter_scale = 0.003301;
  const float output_scale = 0.092596;
  const int input_zero_point = -128;
  const int output_zero_point = 0;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);

  // Create per-tensor quantized int8_t input tensor.
  int8_t input_quantized[input_elements];
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      input_values, input_quantized, input_dims, input_scale, input_zero_point);

  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  int8_t filter_quantized[filter_elements];
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
      filter_values, filter_quantized, filter_dims, filter_scale, 0);

  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, 0};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  int32_t bias_quantized[bias_elements];
  // See https://www.tensorflow.org/lite/performance/quantization_spec for a
  // detailed explanation of why bias scale is input_scale * filter_scale.
  tflite::SymmetricQuantize(bias_values, bias_quantized, bias_elements,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized, bias_dims);

  // Set zero point and scale arrays with a single element for each.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  int8_t output_quantized[output_elements];
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

  int8_t golden_quantized[output_elements];
  tflite::Quantize(golden, golden_quantized, output_elements, output_scale, 0);

  // Errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 1;
  tflite::testing::ValidateDepthwiseConvGoldens(
      golden_quantized, output_elements, &conv_params, kQuantizationTolerance,
      kTensorsSize, tensors);
}


TF_LITE_MICRO_TEST(Int8Input11x10x5Filter5x5x5ShouldMatchGolden) {
  const int input_elements = 11 * 10 * 5;
  const int filter_elements = 5 * 5 * 5;
  const int bias_elements = 5;
  const int output_elements = 4 * 3 * 5;;
  int input_shape[] = {4, 1, 10, 11, 5};
  int filter_shape[] = {4, 1, 5, 5, 5};
  int bias_shape[] = {1, 5};
  int output_shape[] = {4, 1, 3, 4, 5};
  const float input_values[] = {
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
      11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
      10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
      11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942};
  const float filter_values[] = {
		  -0.1617, -0.1948, 0.1419,  -0.2311, -0.0891, 0.1551,  0.0033,  0.3037, -0.1683, 0.1353,  0.1518,  -0.1683, -0.1386, 0.1452,  0.1816,  0.1716,
		  -0.1948, 0.2080,  0.2245,  -0.1981, -0.2410, 0.1849,  0.1981,  0.1584, 0.2509,  0.1783,  -0.2146, -0.1518, 0.2080,  -0.2872, 0.2014,  0.2476,
		  -0.4126, -0.0561, -0.3235, -0.0594, -0.0957, 0.2014,  -0.1056, 0.1386, -0.2542, -0.1617, 0.1287,  -0.1816, -0.0363, 0.1419,  -0.0594, 0.2344,
		  -0.0099, 0.4192,  0.1287,  -0.2311, -0.2212, -0.0528, -0.2080, 0.1816, -0.1452, 0.1221,  0.1254,  -0.1056, -0.0759, 0.1221,  0.1023,  0.1485,
		   0.2707,  0.1716,  -0.1882, -0.1783, 0.1650,  -0.2740, 0.1915,  0.2080, -0.2971, -0.2575, -0.3169, 0.0198,  -0.0231, 0.2410,  -0.0429, 0.0660,
		  -0.1816, 0.1981,  0.2014,  -0.1386, -0.1915, 0.1716,  0.1320,  0.1419, 0.1320,  0.1353,  -0.1386, -0.1716, 0.1320,  -0.1650, 0.1386,  0.0825,
		  -0.1419, -0.1023, 0.1783,  0.0462,  0.2047,  -0.2179, -0.1518, -0.1551, 0.1518,  0.3334,  0.3103,  -0.2047, -0.2047, -0.0957, -0.1650, 0.1221,
		   0.0990,  0.1353,  -0.1617, -0.1485, 0.1650,  -0.1816, 0.1518,  0.1254, -0.0363, -0.1254, 0.1386,  0.0429,  0.2113};
  const float bias_values[] = {
      0.0000, 0.0000, 0.0000, 0.0000, 0.0000};
  const float golden[] = {
		  9.9078, -5.0002, 6.1113, -0.9260, -0.4630, 10.3708, -5.4632, 3.3335, 1.1112, 0.7408, 9.6300, -6.2965, 3.9816, 0.6482, 2.1297, 8.8892, -4.0742, 5.4632, -1.7593, -0.6482, 10.0004, -5.0002, 2.7779, -0.2778, -1.7593, 7.8707, -6.4817, 6.2965, 1.6667, -0.9260, 9.9078, -6.3891,
		  5.8335, 0.8334, 1.4815, 8.4262, -6.0187, 2.5927, -1.0186, -1.2963, 8.7966, -8.1484, 6.6669, 0.8334, -1.3889, 8.0559, -9.1670, 7.2225, 0.6482, -0.5556, 7.8707, -6.9447, 8.7040, -0.8334, -1.6667, 7.8707, -8.2410, 6.0187, 1.6667, -2.0371};


  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  const float input_scale = 0.058824;
  const float filter_scale = 0.003301;
  const float output_scale = 0.092596;
  const int input_zero_point = -128;
  const int output_zero_point = 0;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);

  // Create per-tensor quantized int8_t input tensor.
  int8_t input_quantized[input_elements];
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      input_values, input_quantized, input_dims, input_scale, input_zero_point);

  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  int8_t filter_quantized[filter_elements];
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
      filter_values, filter_quantized, filter_dims, filter_scale, 0);

  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, 0};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  int32_t bias_quantized[bias_elements];
  // See https://www.tensorflow.org/lite/performance/quantization_spec for a
  // detailed explanation of why bias scale is input_scale * filter_scale.
  tflite::SymmetricQuantize(bias_values, bias_quantized, bias_elements,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized, bias_dims);

  // Set zero point and scale arrays with a single element for each.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  int8_t output_quantized[output_elements];
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

  int8_t golden_quantized[output_elements];
  tflite::Quantize(golden, golden_quantized, output_elements, output_scale, 0);

  // Errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 3;
  conv_params.stride_width = 2;
  tflite::testing::ValidateDepthwiseConvGoldens(
      golden_quantized, output_elements, &conv_params, kQuantizationTolerance,
      kTensorsSize, tensors);
}


TF_LITE_MICRO_TEST(Int8Input15x6x5Filter5x5x5ShouldMatchGolden) {
  const int input_elements = 15 * 6 * 5;
  const int filter_elements = 5 * 5 * 5;
  const int bias_elements = 5;
  const int output_elements = 4 * 6 * 5;;
  int input_shape[] = {4, 1, 6, 15, 5};
  int filter_shape[] = {4, 1, 5, 5, 5};
  int bias_shape[] = {1, 5};
  int output_shape[] = {4, 1, 6, 4, 5};
  const float input_values[] = {
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
      11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
      10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
      11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824};
  const float filter_values[] = {
		  -0.1617, -0.1948, 0.1419,  -0.2311, -0.0891, 0.1551,  0.0033,  0.3037, -0.1683, 0.1353,  0.1518,  -0.1683, -0.1386, 0.1452,  0.1816,  0.1716,
		  -0.1948, 0.2080,  0.2245,  -0.1981, -0.2410, 0.1849,  0.1981,  0.1584, 0.2509,  0.1783,  -0.2146, -0.1518, 0.2080,  -0.2872, 0.2014,  0.2476,
		  -0.4126, -0.0561, -0.3235, -0.0594, -0.0957, 0.2014,  -0.1056, 0.1386, -0.2542, -0.1617, 0.1287,  -0.1816, -0.0363, 0.1419,  -0.0594, 0.2344,
		  -0.0099, 0.4192,  0.1287,  -0.2311, -0.2212, -0.0528, -0.2080, 0.1816, -0.1452, 0.1221,  0.1254,  -0.1056, -0.0759, 0.1221,  0.1023,  0.1485,
		   0.2707,  0.1716,  -0.1882, -0.1783, 0.1650,  -0.2740, 0.1915,  0.2080, -0.2971, -0.2575, -0.3169, 0.0198,  -0.0231, 0.2410,  -0.0429, 0.0660,
		  -0.1816, 0.1981,  0.2014,  -0.1386, -0.1915, 0.1716,  0.1320,  0.1419, 0.1320,  0.1353,  -0.1386, -0.1716, 0.1320,  -0.1650, 0.1386,  0.0825,
		  -0.1419, -0.1023, 0.1783,  0.0462,  0.2047,  -0.2179, -0.1518, -0.1551, 0.1518,  0.3334,  0.3103,  -0.2047, -0.2047, -0.0957, -0.1650, 0.1221,
		   0.0990,  0.1353,  -0.1617, -0.1485, 0.1650,  -0.1816, 0.1518,  0.1254, -0.0363, -0.1254, 0.1386,  0.0429,  0.2113};
  const float bias_values[] = {
      0.0000, 0.0000, 0.0000, 0.0000, 0.0000};
  const float golden[] = {
		  8.7966, -6.3891, 6.3891, -0.6482, 1.7593, 9.4448, -6.2965, 3.7964, 0.0926, 0.2778, 8.8892, -5.6484, 5.9261, 0.7408, 0.2778, 11.0189, -2.0371, 3.0557, -2.1297, -2.6853, 9.6300, -5.4632, 5.4632, 1.9445, 0.5556, 10.1856, -6.9447, 7.1299, -0.0926, 1.6667, 8.8892, -5.7410,
		  5.0928, -0.0926, -0.8334, 10.0004, -1.7593, 2.5001, -2.0371, -3.2409, 8.7966, -7.8707, 6.9447, 1.0186, -3.1483, 7.5003, -10.2782, 8.0559, 1.1112, -1.4815, 9.2596, -7.1299, 8.1484, 0.6482, -1.3889, 6.5743, -2.9631, 4.4446, 0.3704, -2.4075, 7.8707, -8.0559, 3.2409, 1.6667,
		  -3.3335, 7.2225, -8.3336, 0.6482, 1.2037, -5.5558, 7.2225, -8.9818, 2.3149, 1.1112, -3.4261, 7.4077, -6.6669, -0.8334, -0.3704, -2.1297, 1.5741, -5.6484, 8.0559, -0.4630, 1.8519, 2.8705, -6.7595, 7.0373, -1.0186, 2.7779, 2.8705, -5.8335, 5.1854, 0.7408, 0.7408, 4.1668,
		  -3.6112, -0.5556, -2.1297, -1.7593, 0.9260, -3.7964, 6.0187, 2.0371, 2.9631, 0.8334, -2.7779, 6.6669, 1.3889, 3.1483, 1.0186, -3.9816, 6.8521, 0.1852, 2.5927, 1.1112, -2.6853, 2.9631, -1.7593, 2.2223};


  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  const float input_scale = 0.058824;
  const float filter_scale = 0.003301;
  const float output_scale = 0.092596;
  const int input_zero_point = -128;
  const int output_zero_point = 0;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);

  // Create per-tensor quantized int8_t input tensor.
  int8_t input_quantized[input_elements];
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      input_values, input_quantized, input_dims, input_scale, input_zero_point);

  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  int8_t filter_quantized[filter_elements];
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
      filter_values, filter_quantized, filter_dims, filter_scale, 0);

  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, 0};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  int32_t bias_quantized[bias_elements];
  // See https://www.tensorflow.org/lite/performance/quantization_spec for a
  // detailed explanation of why bias scale is input_scale * filter_scale.
  tflite::SymmetricQuantize(bias_values, bias_quantized, bias_elements,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized, bias_dims);

  // Set zero point and scale arrays with a single element for each.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  int8_t output_quantized[output_elements];
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

  int8_t golden_quantized[output_elements];
  tflite::Quantize(golden, golden_quantized, output_elements, output_scale, 0);

  // Errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 4;
  tflite::testing::ValidateDepthwiseConvGoldens(
      golden_quantized, output_elements, &conv_params, kQuantizationTolerance,
      kTensorsSize, tensors);
}


TF_LITE_MICRO_TEST(Int8Input9x9x5Filter3x3x5Dilation2x2x5ShouldMatchGolden) {
  const int input_elements = 9 * 9 * 5;
  const int filter_elements = 3 * 3 * 5;
  const int bias_elements = 5;
  const int output_elements = 3 * 3 * 5;;
  int input_shape[] = {4, 1, 9, 9, 5};
  int filter_shape[] = {4, 1, 3, 3, 5};
  int bias_shape[] = {1, 5};
  int output_shape[] = {4, 1, 3, 3, 5};
  const float input_values[] = {
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
      11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
      10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
      11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824};
  const float filter_values[] = {
		  -0.1617, -0.1948, 0.1419,  -0.2311, -0.0891, 0.1551,  0.0033,  0.3037, -0.1683, 0.1353,  0.1518,  -0.1683, -0.1386, 0.1452,  0.1816,  0.1716,
		  -0.1948, 0.2080,  0.2245,  -0.1981, -0.2410, 0.1849,  0.1981,  0.1584, 0.2509,  0.1783,  -0.2146, -0.1518, 0.2080};
  const float bias_values[] = {
      0.0000, 0.0000, 0.0000, 0.0000, 0.0000};
  const float golden[] = {
		  5.9261, -1.4815, 11.7597, 11.7597, 11.5745, 5.6484, -2.9631, 11.7597, 11.7597, 8.9818, -1.1112, -1.2963, 11.7597, 6.1113, 9.2596, 6.8521, -0.8334, 11.7597, 11.7597, 9.9078, 5.8335, -2.1297, 11.7597, 11.7597, 8.7040, -0.7408, -1.8519, 11.7597, 7.1299, 8.0559, 2.9631, -5.9261,
		  4.5372, 3.2409, -2.4075, 2.1297, -4.8150, 5.8335, 2.3149, -1.2037, -0.8334, -2.2223, 7.8707, 0.3704, 0.4630};


  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  const float input_scale = 0.058824;
  const float filter_scale = 0.003301;
  const float output_scale = 0.092596;
  const int input_zero_point = -128;
  const int output_zero_point = 0;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);

  // Create per-tensor quantized int8_t input tensor.
  int8_t input_quantized[input_elements];
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      input_values, input_quantized, input_dims, input_scale, input_zero_point);

  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  int8_t filter_quantized[filter_elements];
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
      filter_values, filter_quantized, filter_dims, filter_scale, 0);

  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, 0};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  int32_t bias_quantized[bias_elements];
  // See https://www.tensorflow.org/lite/performance/quantization_spec for a
  // detailed explanation of why bias scale is input_scale * filter_scale.
  tflite::SymmetricQuantize(bias_values, bias_quantized, bias_elements,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized, bias_dims);

  // Set zero point and scale arrays with a single element for each.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  int8_t output_quantized[output_elements];
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

  int8_t golden_quantized[output_elements];
  tflite::Quantize(golden, golden_quantized, output_elements, output_scale, 0);

  // Errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 2;
  conv_params.dilation_height_factor = 2;
  conv_params.stride_height = 3;
  conv_params.stride_width = 3;
  tflite::testing::ValidateDepthwiseConvGoldens(
      golden_quantized, output_elements, &conv_params, kQuantizationTolerance,
      kTensorsSize, tensors);
}



TF_LITE_MICRO_TEST(Int8Input15x15x2Filter5x5x2ShouldMatchGolden) {
  const int input_elements = 15 * 15 * 5;
  const int filter_elements = 5 * 5 * 2;
  const int bias_elements = 2;
  const int output_elements = 3 * 3 * 2;;
  int input_shape[] = {4, 1, 15, 15, 2};
  int filter_shape[] = {4, 1, 5, 5, 2};
  int bias_shape[] = {1, 2};
  int output_shape[] = {4, 1, 3, 3, 2};
  const float input_values[] = {
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
      11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
      10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
      11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824};
  const float filter_values[] = {
		  -0.1617, -0.1948, 0.1419,  -0.2311, -0.0891, 0.1551,  0.0033,  0.3037, -0.1683, 0.1353,  0.1518,  -0.1683, -0.1386, 0.1452,  0.1816,  0.1716,
		  -0.1948, 0.2080,  0.2245,  -0.1981, -0.2410, 0.1849,  0.1981,  0.1584, 0.2509,  0.1783,  -0.2146, -0.1518, 0.2080,  -0.2872, 0.2014,  0.2476,
		  -0.4126, -0.0561, -0.3235, -0.0594, -0.0957, 0.2014,  -0.1056, 0.1386, -0.2542, -0.1617, 0.1287,  -0.1816, -0.0363, 0.1419,  -0.0594, 0.2344,
		  -0.0099, 0.4192};
  const float bias_values[] = {
      0.0000, 0.0000};
  const float golden[] = {
		  -7.6855, 11.7597, -7.5929, 11.7597, -8.4262, 11.5745, -6.8521, 11.7597, -7.5929, 11.7597, -7.5003, 11.7597, -9.1670, 11.7597, -6.4817, 11.6671, -7.5003, 11.7597};


  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  const float input_scale = 0.058824;
  const float filter_scale = 0.003301;
  const float output_scale = 0.092596;
  const int input_zero_point = -128;
  const int output_zero_point = 0;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);

  // Create per-tensor quantized int8_t input tensor.
  int8_t input_quantized[input_elements];
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      input_values, input_quantized, input_dims, input_scale, input_zero_point);

  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  int8_t filter_quantized[filter_elements];
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
      filter_values, filter_quantized, filter_dims, filter_scale, 0);

  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, 0};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  int32_t bias_quantized[bias_elements];
  // See https://www.tensorflow.org/lite/performance/quantization_spec for a
  // detailed explanation of why bias scale is input_scale * filter_scale.
  tflite::SymmetricQuantize(bias_values, bias_quantized, bias_elements,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized, bias_dims);

  // Set zero point and scale arrays with a single element for each.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  int8_t output_quantized[output_elements];
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

  int8_t golden_quantized[output_elements];
  tflite::Quantize(golden, golden_quantized, output_elements, output_scale, 0);

  // Errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 5;
  conv_params.stride_width = 5;
  tflite::testing::ValidateDepthwiseConvGoldens(
      golden_quantized, output_elements, &conv_params, kQuantizationTolerance,
      kTensorsSize, tensors);
}


TF_LITE_MICRO_TEST(Int8Input12x14x3Filter8x10x3ShouldMatchGolden) {
  const int input_elements = 12 * 14 * 3;
  const int filter_elements = 8 * 10 * 3;
  const int bias_elements = 3;
  const int output_elements = 3 * 3 * 3;
  int input_shape[] = {4, 1, 14, 12, 3};
  int filter_shape[] = {4, 1, 10, 8, 3};
  int bias_shape[] = {1, 3};
  int output_shape[] = {4, 1, 3, 3, 3};
  const float input_values[] = {
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
      11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
      10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
      11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707, 9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	  11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001, 11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001, 9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942, 9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	  11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177, 11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177, 9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765, 9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	  10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766, 11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118, 9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589, 8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	  11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648, 10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354, 9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707};
  const float filter_values[] = {
		  -0.1617, -0.1948, 0.1419,  -0.2311, -0.0891, 0.1551,  0.0033,  0.3037, -0.1683, 0.1353,  0.1518,  -0.1683, -0.1386, 0.1452,  0.1816,  0.1716,
		  -0.1948, 0.2080,  0.2245,  -0.1981, -0.2410, 0.1849,  0.1981,  0.1584, 0.2509,  0.1783,  -0.2146, -0.1518, 0.2080,  -0.2872, 0.2014,  0.2476,
		  -0.4126, -0.0561, -0.3235, -0.0594, -0.0957, 0.2014,  -0.1056, 0.1386, -0.2542, -0.1617, 0.1287,  -0.1816, -0.0363, 0.1419,  -0.0594, 0.2344,
		  -0.0099, 0.4192,  0.1287,  -0.2311, -0.2212, -0.0528, -0.2080, 0.1816, -0.1452, 0.1221,  0.1254,  -0.1056, -0.0759, 0.1221,  0.1023,  0.1485,
		   0.2707,  0.1716,  -0.1882, -0.1783, 0.1650,  -0.2740, 0.1915,  0.2080, -0.2971, -0.2575, -0.3169, 0.0198,  -0.0231, 0.2410,  -0.0429, 0.0660,
		  -0.1816, 0.1981,  0.2014,  -0.1386, -0.1915, 0.1716,  0.1320,  0.1419, 0.1320,  0.1353,  -0.1386, -0.1716, 0.1320,  -0.1650, 0.1386,  0.0825,
		  -0.1419, -0.1023, 0.1783,  0.0462,  0.2047,  -0.2179, -0.1518, -0.1551, 0.1518,  0.3334,  0.3103,  -0.2047, -0.2047, -0.0957, -0.1650, 0.1221,
		   0.0990,  0.1353,  -0.1617, -0.1485, 0.1650,  -0.1816, 0.1518,  0.1254, -0.0363, -0.1254, 0.1386,  0.0429,  0.2113,  -0.2839, -0.1056, -0.2278,
		  -0.1617, -0.1948, 0.1419,  -0.2311, -0.0891, 0.1551,  0.0033,  0.3037, -0.1683, 0.1353,  0.1518,  -0.1683, -0.1386, 0.1452,  0.1816,  0.1716,
		  -0.1948, 0.2080,  0.2245,  -0.1981, -0.2410, 0.1849,  0.1981,  0.1584, 0.2509,  0.1783,  -0.2146, -0.1518, 0.2080,  -0.2872, 0.2014,  0.2476,
		  -0.4126, -0.0561, -0.3235, -0.0594, -0.0957, 0.2014,  -0.1056, 0.1386, -0.2542, -0.1617, 0.1287,  -0.1816, -0.0363, 0.1419,  -0.0594, 0.2344,
		  -0.0099, 0.4192,  0.1287,  -0.2311, -0.2212, -0.0528, -0.2080, 0.1816, -0.1452, 0.1221,  0.1254,  -0.1056, -0.0759, 0.1221,  0.1023,  0.1485,
		   0.2707,  0.1716,  -0.1882, -0.1783, 0.1650,  -0.2740, 0.1915,  0.2080, -0.2971, -0.2575, -0.3169, 0.0198,  -0.0231, 0.2410,  -0.0429, 0.0660,
		  -0.1816, 0.1981,  0.2014,  -0.1386, -0.1915, 0.1716,  0.1320,  0.1419, 0.1320,  0.1353,  -0.1386, -0.1716, 0.1320,  -0.1650, 0.1386,  0.0825,
		  -0.1419, -0.1023, 0.1783,  0.0462,  0.2047,  -0.2179, -0.1518, -0.1551, 0.1518,  0.3334,  0.3103,  -0.2047, -0.2047, -0.0957, -0.1650, 0.1221};
  const float bias_values[] = {
      0.0000, 0.0000, 0.0000};
  const float golden[] = {
		  5.2780, 9.4448, -6.4817, 1.4815, 6.8521, -2.4075, 3.9816, 9.1670, -4.1668, 3.5186, 8.8892, -2.5001, 6.7595, 9.8152, -6.5743, 6.2039, 6.2039, -8.8892, 6.0187, 7.9633, -5.4632, 6.1113, 6.2965, -3.6112, 6.2039, 6.8521, -8.7966};


  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  const float input_scale = 0.058824;
  const float filter_scale = 0.003301;
  const float output_scale = 0.092596;
  const int input_zero_point = -128;
  const int output_zero_point = 0;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);

  // Create per-tensor quantized int8_t input tensor.
  int8_t input_quantized[input_elements];
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      input_values, input_quantized, input_dims, input_scale, input_zero_point);

  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  int8_t filter_quantized[filter_elements];
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
      filter_values, filter_quantized, filter_dims, filter_scale, 0);

  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, 0};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  int32_t bias_quantized[bias_elements];
  // See https://www.tensorflow.org/lite/performance/quantization_spec for a
  // detailed explanation of why bias scale is input_scale * filter_scale.
  tflite::SymmetricQuantize(bias_values, bias_quantized, bias_elements,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized, bias_dims);

  // Set zero point and scale arrays with a single element for each.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  int8_t output_quantized[output_elements];
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

  int8_t golden_quantized[output_elements];
  tflite::Quantize(golden, golden_quantized, output_elements, output_scale, 0);

  // Errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 2;
  conv_params.stride_width = 2;
  tflite::testing::ValidateDepthwiseConvGoldens(
      golden_quantized, output_elements, &conv_params, kQuantizationTolerance,
      kTensorsSize, tensors);
}




TF_LITE_MICRO_TEST(Int8Input3x3x32Filter3x3x32ShouldMatchGolden) {
  const int input_elements = 3*3*32;
  const int filter_elements = 3*3*32;
  const int bias_elements = 32;
  const int output_elements = 32;
  int input_shape[] = {4, 1, 3, 3, 32};
  int filter_shape[] = {4, 1, 3, 3, 32};
  int bias_shape[] = {1,32};
  int output_shape[] = {4, 1, 1, 1, 32};
  const float input_values[] = {
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001,
      11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001,
      9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942,
      9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
      11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177,
      11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177,
      9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765,
      9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
      10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766,
      11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118,
      9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589,
      8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
      11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648,
      10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354,
      9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707,
      9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	        11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001,
	        11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001,
	        9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942,
	        9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	        11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177,
	        11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177,
	        9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765,
	        9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	        10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766,
	        11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118,
	        9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589,
	        8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	        11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648,
	        10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354,
	        9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707,
			9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
			      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001,
			      11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001,
			      9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942,
			      9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530};
  const float filter_values[] = {
      -0.1617, -0.1948, 0.1419,  -0.2311, -0.0891, 0.1551,  0.0033,  0.3037,
      -0.1683, 0.1353,  0.1518,  -0.1683, -0.1386, 0.1452,  0.1816,  0.1716,
      -0.1948, 0.2080,  0.2245,  -0.1981, -0.2410, 0.1849,  0.1981,  0.1584,
      0.2509,  0.1783,  -0.2146, -0.1518, 0.2080,  -0.2872, 0.2014,  0.2476,
      -0.4126, -0.0561, -0.3235, -0.0594, -0.0957, 0.2014,  -0.1056, 0.1386,
      -0.2542, -0.1617, 0.1287,  -0.1816, -0.0363, 0.1419,  -0.0594, 0.2344,
      -0.0099, 0.4192,  0.1287,  -0.2311, -0.2212, -0.0528, -0.2080, 0.1816,
      -0.1452, 0.1221,  0.1254,  -0.1056, -0.0759, 0.1221,  0.1023,  0.1485,
      0.2707,  0.1716,  -0.1882, -0.1783, 0.1650,  -0.2740, 0.1915,  0.2080,
      -0.2971, -0.2575, -0.3169, 0.0198,  -0.0231, 0.2410,  -0.0429, 0.0660,
      -0.1816, 0.1981,  0.2014,  -0.1386, -0.1915, 0.1716,  0.1320,  0.1419,
      0.1320,  0.1353,  -0.1386, -0.1716, 0.1320,  -0.1650, 0.1386,  0.0825,
      -0.1419, -0.1023, 0.1783,  0.0462,  0.2047,  -0.2179, -0.1518, -0.1551,
      0.1518,  0.3334,  0.3103,  -0.2047, -0.2047, -0.0957, -0.1650, 0.1221,
      0.0990,  0.1353,  -0.1617, -0.1485, 0.1650,  -0.1816, 0.1518,  0.1254,
      -0.0363, -0.1254, 0.1386,  0.0429,  0.2113,  -0.2839, -0.1056, -0.2278,
	        -0.1617, -0.1948, 0.1419,  -0.2311, -0.0891, 0.1551,  0.0033,  0.3037,
	        -0.1683, 0.1353,  0.1518,  -0.1683, -0.1386, 0.1452,  0.1816,  0.1716,
	        -0.1948, 0.2080,  0.2245,  -0.1981, -0.2410, 0.1849,  0.1981,  0.1584,
	        0.2509,  0.1783,  -0.2146, -0.1518, 0.2080,  -0.2872, 0.2014,  0.2476,
	        -0.4126, -0.0561, -0.3235, -0.0594, -0.0957, 0.2014,  -0.1056, 0.1386,
	        -0.2542, -0.1617, 0.1287,  -0.1816, -0.0363, 0.1419,  -0.0594, 0.2344,
	        -0.0099, 0.4192,  0.1287,  -0.2311, -0.2212, -0.0528, -0.2080, 0.1816,
	        -0.1452, 0.1221,  0.1254,  -0.1056, -0.0759, 0.1221,  0.1023,  0.1485,
	        0.2707,  0.1716,  -0.1882, -0.1783, 0.1650,  -0.2740, 0.1915,  0.2080,
	        -0.2971, -0.2575, -0.3169, 0.0198,  -0.0231, 0.2410,  -0.0429, 0.0660,
	        -0.1816, 0.1981,  0.2014,  -0.1386, -0.1915, 0.1716,  0.1320,  0.1419,
	        0.1320,  0.1353,  -0.1386, -0.1716, 0.1320,  -0.1650, 0.1386,  0.0825,
	        -0.1419, -0.1023, 0.1783,  0.0462,  0.2047,  -0.2179, -0.1518, -0.1551,
	        0.1518,  0.3334,  0.3103,  -0.2047, -0.2047, -0.0957, -0.1650, 0.1221,
	        0.0990,  0.1353,  -0.1617, -0.1485, 0.1650,  -0.1816, 0.1518,  0.1254,
	        -0.0363, -0.1254, 0.1386,  0.0429,  0.2113,  -0.2839, -0.1056, -0.2278,
			      -0.1617, -0.1948, 0.1419,  -0.2311, -0.0891, 0.1551,  0.0033,  0.3037,
			      -0.1683, 0.1353,  0.1518,  -0.1683, -0.1386, 0.1452,  0.1816,  0.1716,
			      -0.1948, 0.2080,  0.2245,  -0.1981, -0.2410, 0.1849,  0.1981,  0.1584,
			      0.2509,  0.1783,  -0.2146, -0.1518, 0.2080,  -0.2872, 0.2014,  0.2476};
  const float bias_values[] = {
      0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
	  0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
	  0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
	  0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000};
  const float golden[] = {
		  -11.852288, -6.111336, -2.77788, -11.852288, 2.407496, -1.111152, -0.92596, 11.759692,
		  -11.852288, 2.222304, 6.296528, -11.018924, -9.167004, 8.981812, 0.370384, 11.759692,
		  -7.315084, 11.759692, 9.2596, -11.852288, -9.074408, 3.24086, 6.111336, 11.759692,
		  5.926144, 7.037296, -3.148264, -7.778064, 9.629984, -11.852288, 6.389124, 5.370568};


  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  const float input_scale = 0.058824;
  const float filter_scale = 0.003301;
  const float output_scale = 0.092596;
  const int input_zero_point = -128;
  const int output_zero_point = 0;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);

  // Create per-tensor quantized int8_t input tensor.
  int8_t input_quantized[input_elements];
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      input_values, input_quantized, input_dims, input_scale, input_zero_point);

  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  int8_t filter_quantized[filter_elements];
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
      filter_values, filter_quantized, filter_dims, filter_scale, 0);

  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, 0};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  int32_t bias_quantized[bias_elements];
  // See https://www.tensorflow.org/lite/performance/quantization_spec for a
  // detailed explanation of why bias scale is input_scale * filter_scale.
  tflite::SymmetricQuantize(bias_values, bias_quantized, bias_elements,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized, bias_dims);

  // Set zero point and scale arrays with a single element for each.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  int8_t output_quantized[output_elements];
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

  int8_t golden_quantized[output_elements];
  tflite::Quantize(golden, golden_quantized, output_elements, output_scale, 0);

  // Errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 1;
  tflite::testing::ValidateDepthwiseConvGoldens(
      golden_quantized, output_elements, &conv_params, kQuantizationTolerance,
      kTensorsSize, tensors);
}


TF_LITE_MICRO_TEST(Int8Input9x9x3Filter3x3x3ShouldMatchGolden) {
  const int input_elements = 9*9*3;
  const int filter_elements = 3*3*3;
  const int bias_elements = 3;
  const int output_elements = 4*4*3;
  int input_shape[] = {4, 1, 9, 9, 3};
  int filter_shape[] = {4, 1, 3, 3, 3};
  int bias_shape[] = {1,3};
  int output_shape[] = {4, 1, 4, 4, 3};

  const float input_values[] = {
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001,
      11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001,
      9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942,
      9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
      11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177,
      11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177,
      9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765,
      9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
      10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766,
      11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118,
      9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589,
      8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
      11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648,
      10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354,
      9.7060,  8.1177,  9.2942,  9.5883,  7.7648,  9.6471, 9.1177, 9.4707,
      9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883,
	        11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001,
	        11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001,
	        9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942,
	        9.1177,  8.5883,  8.2354,  8.6471,  8.0589,  8.0001, 7.4118, 7.3530,
	        11.0001, 11.1177, 11.0589, 11.2354, 10.5883, 9.2942, 9.2942, 10.1177,
	        11.2354, 10.8824, 8.9412,  8.8236,  9.2354,  8.8824, 7.0001, 9.1177,
	        9.5883,  8.2354,  9.1765,  9.5295,  7.4118,  8.5883, 8.1177, 9.1765,
	        9.0001,  9.0589,  8.9412,  8.2942,  7.8824,  8.4118, 7.2942, 7.2354,
	        10.4118, 10.8824, 11.1177, 11.0001, 10.0001, 9.7060, 9.7648, 10.1766,
	        11.1766, 10.6471, 8.6471,  8.5295,  9.5295,  9.0001, 7.0001, 9.4118,
	        9.8236,  8.0001,  9.2354,  9.5883,  7.5295,  9.0001, 8.5295, 9.0589,
	        8.9412,  9.1177,  8.9412,  8.0001,  8.0589,  8.8824, 7.0589, 7.3530,
	        11.3530, 11.0589, 10.7060, 10.7648, 9.9413,  9.1177, 9.1177, 9.7648,
	        10.7060, 10.2354, 8.5883,  8.8236,  9.7648,  9.2942, 7.5295, 9.2354,
	        9.7060,  8.1177,  9.2942};
  const float filter_values[] = {
      -0.1617, -0.1948, 0.1419,  -0.2311, -0.0891, 0.1551,  0.0033,  0.3037,
      -0.1683, 0.1353,  0.1518,  -0.1683, -0.1386, 0.1452,  0.1816,  0.1716,
      -0.1948, 0.2080,  0.2245,  -0.1981, -0.2410, 0.1849,  0.1981,  0.1584,
      0.2509,  0.1783,  -0.2146};
  const float bias_values[] = {
      0.0000, 0.0000, 0.0000};
  const float golden[] = {
		  3.148264, 1.574132, 1.759324, 3.611244, 3.148264, 1.574132, 5.09278, 3.611244,
		  0.092596, 3.981628, 2.3149, 0.277788, 4.444608, 2.685284, 1.018556, 4.814992,
		  3.426052, 0.277788, 2.963072, 2.3149, 1.203748, 4.6298, 3.611244, 0.370384,
		  4.907588, 3.24086, -0.092596, 3.796436, 2.129708, 0.185192, 3.333456, 2.222304,
		  0.370384, 3.24086, 2.222304, 1.759324, 2.963072, 2.129708, 1.203748, 4.259416,
		  3.426052, 0.185192, 4.907588, 2.592688, 0.277788, 4.352012, 2.500092, 0.277788};


  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  const float input_scale = 0.058824;
  const float filter_scale = 0.003301;
  const float output_scale = 0.092596;
  const int input_zero_point = -128;
  const int output_zero_point = 0;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);

  // Create per-tensor quantized int8_t input tensor.
  int8_t input_quantized[input_elements];
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      input_values, input_quantized, input_dims, input_scale, input_zero_point);

  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  int8_t filter_quantized[filter_elements];
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
      filter_values, filter_quantized, filter_dims, filter_scale, 0);

  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, 0};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  int32_t bias_quantized[bias_elements];
  // See https://www.tensorflow.org/lite/performance/quantization_spec for a
  // detailed explanation of why bias scale is input_scale * filter_scale.
  tflite::SymmetricQuantize(bias_values, bias_quantized, bias_elements,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized, bias_dims);

  // Set zero point and scale arrays with a single element for each.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  int8_t output_quantized[output_elements];
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

  int8_t golden_quantized[output_elements];
  tflite::Quantize(golden, golden_quantized, output_elements, output_scale, 0);

  // Errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 2;
  conv_params.stride_width = 2;
  tflite::testing::ValidateDepthwiseConvGoldens(
      golden_quantized, output_elements, &conv_params, kQuantizationTolerance,
      kTensorsSize, tensors);
}




TF_LITE_MICRO_TEST(Int8Input32x1Filter32x1ShouldMatchGolden) {
  const int input_elements = 32 * 1;
  const int filter_elements = 32 * 1;
  const int bias_elements = 32;
  const int output_elements = 32;
  int input_shape[] = {4, 1, 1, 1, 32};
  int filter_shape[] = {4, 1, 1, 1, 32};
  int bias_shape[] = {1, 32};
  int output_shape[] = {4, 1, 1, 1, 32};
  const float input_values[] = {
      11.0589, 10.8824, 11.1766, 11.5295, 10.8236, 9.5295, 9.5295, 10.0001,
      11.2354, 10.8824, 9.1765,  9.0589,  9.6471,  8.9412, 7.9412, 9.0001,
      9.3530,  7.5295,  9.2354,  9.5883,  7.5883,  8.1765, 7.5883, 9.2942,
      9.3530,  8.8236,  8.5295,  8.0589,  8.6471,  9.5883, 7.4118, 7.5883};
  const float filter_values[] = {
      -0.1419, -0.1023, 0.1783,  0.0462,  0.2047,  -0.2179, -0.1518, -0.1551,
      0.1518,  0.3334,  0.3103,  -0.2047, -0.2047, -0.0957, -0.1650, 0.1221,
      0.0990,  0.1353,  -0.1617, -0.1485, 0.1650,  -0.1816, 0.1518,  0.1254,
      -0.0363, -0.1254, 0.1386,  0.0429,  0.2113,  -0.2839, -0.1056, -0.2278};
  const float bias_values[] = {
      0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
      0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
      0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
      0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000};
  const float golden[] = {
      -1.5741, -1.1112, 2.0371,  0.5556,  2.2223,  -2.0371, -1.4815, -1.5741,
      1.6667,  3.6112,  2.8705,  -1.8519, -1.9445, -0.8334, -1.2963, 1.1112,
      0.9260,  1.0186,  -1.4815, -1.3889, 1.2963,  -1.4815, 1.1112,  1.2037,
      -0.3704, -1.1112, 1.2037,  0.3704,  1.8519,  -2.6853, -0.7408, -1.7593};

  // Quantization Parameters.  All scales except output are 1.0, and all zero
  // points are 0. This direct-maps the values to floating point and makes it
  // easy to reson about them.
  const float input_scale = 0.058824;
  const float filter_scale = 0.003301;
  const float output_scale = 0.092596;
  const int input_zero_point = -128;
  const int output_zero_point = 0;

  TfLiteIntArray* input_dims = tflite::testing::IntArrayFromInts(input_shape);
  TfLiteIntArray* filter_dims = tflite::testing::IntArrayFromInts(filter_shape);
  TfLiteIntArray* bias_dims = tflite::testing::IntArrayFromInts(bias_shape);
  TfLiteIntArray* output_dims = tflite::testing::IntArrayFromInts(output_shape);

  // Create per-tensor quantized int8_t input tensor.
  int8_t input_quantized[input_elements];
  TfLiteTensor input_tensor = tflite::testing::CreateQuantizedTensor(
      input_values, input_quantized, input_dims, input_scale, input_zero_point);

  // Set zero point and scale arrays with a single element for each.
  int input_zero_points[] = {1, input_zero_point};
  float input_scales[] = {1, input_scale};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  input_tensor.quantization = {kTfLiteAffineQuantization, &input_quant};

  // Create per-tensor quantized int8_t filter tensor.
  int8_t filter_quantized[filter_elements];
  TfLiteTensor filter_tensor = tflite::testing::CreateQuantizedTensor(
      filter_values, filter_quantized, filter_dims, filter_scale, 0);

  // Set zero point and scale arrays with a single element for each.
  int filter_zero_points[] = {1, 0};
  float filter_scales[] = {1, filter_scale};
  TfLiteAffineQuantization filter_quant = {
      tflite::testing::FloatArrayFromFloats(filter_scales),
      tflite::testing::IntArrayFromInts(filter_zero_points), 0};
  filter_tensor.quantization = {kTfLiteAffineQuantization, &filter_quant};

  // Create per-tensor quantized int32_t bias tensor.
  int32_t bias_quantized[bias_elements];
  // See https://www.tensorflow.org/lite/performance/quantization_spec for a
  // detailed explanation of why bias scale is input_scale * filter_scale.
  tflite::SymmetricQuantize(bias_values, bias_quantized, bias_elements,
                            input_scale * output_scale);
  TfLiteTensor bias_tensor =
      tflite::testing::CreateTensor(bias_quantized, bias_dims);

  // Set zero point and scale arrays with a single element for each.
  int bias_zero_points[] = {1, 0};
  float bias_scales[] = {1, input_scale * filter_scale};
  TfLiteAffineQuantization bias_quant = {
      tflite::testing::FloatArrayFromFloats(bias_scales),
      tflite::testing::IntArrayFromInts(bias_zero_points), 0};
  bias_tensor.quantization = {kTfLiteAffineQuantization, &bias_quant};

  // Create per-tensor quantized int8_t output tensor.
  int8_t output_quantized[output_elements];
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

  int8_t golden_quantized[output_elements];
  tflite::Quantize(golden, golden_quantized, output_elements, output_scale, 0);

  // Errors due to quantization should not exceed 1.
  constexpr int kQuantizationTolerance = 1;

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 2;
  conv_params.stride_width = 2;
  TF_LITE_MICRO_EXPECT_EQ(kTfLiteOk,
                          tflite::testing::ValidateDepthwiseConvGoldens(
                              golden_quantized, output_elements, &conv_params,
                              kQuantizationTolerance, kTensorsSize, tensors));
}

// TODO(b/268384678): xtensa vision p6 kernels break
// this test, will if def till properly investigated.

// Quantizing int8-ranged filter values down to int4 doesn't always yield the
// accuracy sufficient to meet the golden values. So this test was created by
// handcrafting filter values within the int4 range, and the golden data was
// obtained by running TestDepthwiseConvQuantizedPerChannel() with int8
// quantization, and ensuring that int4 quantization yields the same outputs.
TF_LITE_MICRO_TEST(SimpleTestQuantizedPerChannelInt4Filter) {
  const int input_elements = 12;
  int input_shape[] = {4, 1, 3, 2, 2};
  const float input_values[] = {1, 2, 7, 8, 3, 4, 9, 10, 5, 6, 11, 12};
  const int filter_elements = 16;
  int filter_shape[] = {4, 1, 2, 2, 4};
  const float filter_values[] = {1, 2, 3, 4, -5, 7,  -6, 7,
                                 5, 6, 7, 4, 2,  -5, 4,  0};
  const int bias_elements = 4;
  int bias_shape[] = {4, 1, 1, 1, 4};
  const int output_elements = 8;
  const float bias_values[] = {1, 2, 3, 4};
  const float golden[] = {
      0, 26, 29, 84, 6, 46, 45, 114,
  };
  int output_shape[] = {4, 1, 2, 1, 4};
  const int output_dims_count = 8;
  int8_t output_data[output_dims_count];

  const float input_scale = 0.5;
  const float output_scale = 1.0f;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  int8_t input_quantized[input_elements];
  int8_t filter_quantized[filter_elements];
  int32_t bias_quantized[bias_elements];
  int8_t golden_quantized[output_elements];

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 1;

  tflite::testing::TestDepthwiseConvQuantizedPerChannel(
      input_shape, input_values, input_quantized, input_scale, input_zero_point,
      filter_shape, filter_values, filter_quantized, bias_shape, bias_values,
      bias_quantized, output_shape, golden, golden_quantized, output_data,
      output_scale, output_zero_point, &conv_params, kTfLiteInt4);
}

TF_LITE_MICRO_TEST(SimpleTestQuantizedPerChannel) {
  const int input_elements = 12;
  int input_shape[] = {4, 1, 3, 2, 2};
  const float input_values[] = {1, 2, 7, 8, 3, 4, 9, 10, 5, 6, 11, 12};
  const int filter_elements = 16;
  int filter_shape[] = {4, 1, 2, 2, 4};
  const float filter_values[] = {1, 2, 3, 4, -9, 10,  -11, 12,
                                 5, 6, 7, 8, 13, -14, 15,  -16};
  const int bias_elements = 4;
  int bias_shape[] = {4, 1, 1, 1, 4};
  const int output_elements = 8;
  const float bias_values[] = {1, 2, 3, 4};
  const float golden[] = {
      71, -34, 99, -20, 91, -26, 127, -4,
  };
  int output_shape[] = {4, 1, 2, 1, 4};
  const int output_dims_count = 8;
  int8_t output_data[output_dims_count];

  const float input_scale = 0.5;
  const float output_scale = 1.0f;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  int8_t input_quantized[input_elements];
  int8_t filter_quantized[filter_elements];
  int32_t bias_quantized[bias_elements];
  int8_t golden_quantized[output_elements];

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 1;

  tflite::testing::TestDepthwiseConvQuantizedPerChannel(
      input_shape, input_values, input_quantized, input_scale, input_zero_point,
      filter_shape, filter_values, filter_quantized, bias_shape, bias_values,
      bias_quantized, output_shape, golden, golden_quantized, output_data,
      output_scale, output_zero_point, &conv_params);
}

TF_LITE_MICRO_TEST(SimpleTestQuantizedPerChannelInt16InputInt8Filter) {
  const int input_elements = 12;
  int input_shape[] = {4, 1, 3, 2, 2};
  const float input_values[] = {-547, 108, -682, 540,  -161, -539, 9,    -482,
                                -859, 84,  153,  -726, 523,  702,  -172, -936};
  const int filter_elements = 16;
  int filter_shape[] = {4, 1, 2, 2, 4};
  const float filter_values[] = {1, 2, 3, 4, -9, 10,  -11, 12,
                                 5, 6, 7, 8, 13, -14, 15,  -16};
  const int bias_elements = 4;
  int bias_shape[] = {4, 1, 1, 1, 4};
  const int output_elements = 8;
  const float bias_values[] = {1, 2, 3, 4};
  const float golden[] = {
      4894, -9009, -16596, 10268, -2564, -7483, -6599, 4356,
  };
  int output_shape[] = {4, 1, 2, 1, 4};
  const int output_dims_count = 8;
  int16_t output_data[output_dims_count];

  const float input_scale = 0.5;
  const float output_scale = 1.0f;
  const int input_zero_point = 0;
  const int output_zero_point = 0;

  int16_t input_quantized[input_elements];
  int8_t filter_quantized[filter_elements];
  int64_t bias_quantized[bias_elements];
  int16_t golden_quantized[output_elements];

  TfLiteDepthwiseConvParams conv_params;
  conv_params.activation = kTfLiteActNone;
  conv_params.dilation_width_factor = 1;
  conv_params.dilation_height_factor = 1;
  conv_params.stride_height = 1;
  conv_params.stride_width = 1;

  tflite::testing::TestDepthwiseConvQuantizedPerChannel(
      input_shape, input_values, input_quantized, input_scale, input_zero_point,
      filter_shape, filter_values, filter_quantized, bias_shape, bias_values,
      bias_quantized, output_shape, golden, golden_quantized, output_data,
      output_scale, output_zero_point, &conv_params);
}
TF_LITE_MICRO_TESTS_END
