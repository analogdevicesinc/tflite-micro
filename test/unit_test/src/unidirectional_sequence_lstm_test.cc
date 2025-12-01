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
#include "tensorflow/lite/c/builtin_op_data.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/kernels/internal/portable_tensor_utils.h"
#include "tensorflow/lite/kernels/internal/types.h"
#include "tensorflow/lite/micro/kernels/kernel_runner.h"
#include "tensorflow/lite/micro/kernels/lstm_shared.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "lstm_test_data.h"
#include "tensorflow/lite/micro/test_helpers.h"
#include "micro_test.h"
#include "unit_test.h"



namespace tflite {
namespace testing {
namespace {

constexpr int kLstmMaxNumInputOutputTensors = 24 + 1;
constexpr int kLstmIntermediateTensorBase = kLstmMaxNumInputOutputTensors + 1;

// Validate the output result array with golden values
template <typename T>
void ValidateResultGoldens(const T* golden, const T* output_data,
                           const int output_len, const float tolerance) {
  for (int i = 0; i < output_len; ++i) {
    TF_LITE_MICRO_EXPECT_NEAR(golden[i], output_data[i], tolerance);
  }
}

template <typename ActivationType, typename WeightType, typename BiasType,
          typename CellType, int batch_size, int time_steps,
          int input_dimension, int state_dimension>
void TestUnidirectionalLSTMInteger(
    const LstmEvalCheckData<
        batch_size * time_steps * input_dimension, batch_size * state_dimension,
        batch_size * state_dimension * time_steps>& eval_check_data,
    const float hidden_state_tolerance, const float cell_state_tolerance,
    LstmNodeContent<ActivationType, WeightType, BiasType, CellType, batch_size,
                    time_steps, input_dimension, state_dimension>&
        node_contents) {
  TfLiteTensor tensors[kLstmMaxNumInputOutputTensors + 1 + 5];
  memcpy(tensors, node_contents.GetTensors(),
         kLstmMaxNumInputOutputTensors * sizeof(TfLiteTensor));

  // Provide also intermediate tensors needed by older LSTM implementations
  int intermediate_array_data[6] = {5,
                                    kLstmIntermediateTensorBase,
                                    kLstmIntermediateTensorBase + 1,
                                    kLstmIntermediateTensorBase + 2,
                                    kLstmIntermediateTensorBase + 3,
                                    kLstmIntermediateTensorBase + 4};
  int input_zero_points[2] = {1, -21};
  float input_scales[2] = {1, 0.004705882165580988};
  TfLiteAffineQuantization input_quant = {
      tflite::testing::FloatArrayFromFloats(input_scales),
      tflite::testing::IntArrayFromInts(input_zero_points), 0};
  int intermediate_dim[2] = {1, 0};
  for (int i = 0; i < 5; ++i) {
    tensors[kLstmIntermediateTensorBase + i] =
        CreateTensor<int16_t>(nullptr, IntArrayFromInts(intermediate_dim));
    tensors[kLstmIntermediateTensorBase + i].quantization = {
        kTfLiteAffineQuantization, &input_quant};
  }

  //const TFLMRegistration registration = Register_UNIDIRECTIONAL_SEQUENCE_LSTM(); //original line
  const TfLiteRegistration_V1 registration = Register_UNIDIRECTIONAL_SEQUENCE_LSTM();
  auto buildin_data = node_contents.BuiltinData();
  micro::KernelRunner runner(
      registration, tensors, kLstmMaxNumInputOutputTensors + 1 + 5,
      node_contents.KernelInputs(), node_contents.KernelOutputs(),
      reinterpret_cast<void*>(&buildin_data),
      IntArrayFromInts(intermediate_array_data));
  TF_LITE_MICRO_EXPECT_EQ(kTfLiteOk, runner.InitAndPrepare());
  TF_LITE_MICRO_EXPECT_EQ(kTfLiteOk, runner.Invoke());

  const auto& quantization_settings = node_contents.QuantizationSettings();

  float dequantized_hidden_state[batch_size * state_dimension] = {};
  Dequantize(node_contents.GetHiddenStateData(), batch_size * state_dimension,
             quantization_settings.hidden_state.scale,
             quantization_settings.hidden_state.zero_point,
             dequantized_hidden_state);

  ValidateResultGoldens(eval_check_data.expected_hidden_state,
                        dequantized_hidden_state, batch_size * state_dimension,
                        hidden_state_tolerance);

  float dequantized_cell_state[batch_size * state_dimension] = {};
  Dequantize(node_contents.GetCellStateData(), batch_size * state_dimension,
             quantization_settings.cell_state.scale,
             quantization_settings.cell_state.zero_point,
             dequantized_cell_state);
  ValidateResultGoldens(eval_check_data.expected_cell_state,
                        dequantized_cell_state, batch_size * state_dimension,
                        cell_state_tolerance);

  float dequantized_output[batch_size * state_dimension * time_steps] = {};
  Dequantize(node_contents.GetOutputData(),
             batch_size * state_dimension * time_steps,
             quantization_settings.output.scale,
             quantization_settings.output.zero_point, dequantized_output);
  ValidateResultGoldens(eval_check_data.expected_output, dequantized_output,
                        batch_size * state_dimension, hidden_state_tolerance);
}

template <int batch_size, int time_steps, int input_dimension,
          int state_dimension>
void TestUnidirectionalLSTMFloat(
    const LstmEvalCheckData<
        batch_size * time_steps * input_dimension, batch_size * state_dimension,
        batch_size * state_dimension * time_steps>& eval_check_data,
    const float hidden_state_tolerance, const float cell_state_tolerance,
    LstmNodeContent<float, float, float, float, batch_size, time_steps,
                    input_dimension, state_dimension>& node_contents) {
  //const TFLMRegistration registration = Register_UNIDIRECTIONAL_SEQUENCE_LSTM(); //original line
  const TfLiteRegistration_V1 registration = Register_UNIDIRECTIONAL_SEQUENCE_LSTM();
  auto buildin_data = node_contents.BuiltinData();
  micro::KernelRunner runner(
      registration, node_contents.GetTensors(), kLstmMaxNumInputOutputTensors,
      node_contents.KernelInputs(), node_contents.KernelOutputs(),
      reinterpret_cast<void*>(&buildin_data));
  TF_LITE_MICRO_EXPECT_EQ(kTfLiteOk, runner.InitAndPrepare());
  TF_LITE_MICRO_EXPECT_EQ(kTfLiteOk, runner.Invoke());

  ValidateResultGoldens(eval_check_data.expected_hidden_state,
                        node_contents.GetHiddenStateData(),
                        batch_size * state_dimension, hidden_state_tolerance);
  ValidateResultGoldens(eval_check_data.expected_cell_state,
                        node_contents.GetCellStateData(),
                        batch_size * state_dimension, cell_state_tolerance);
  ValidateResultGoldens(eval_check_data.expected_output,
                        node_contents.GetOutputData(),
                        batch_size * state_dimension, hidden_state_tolerance);
}

}  // namespace
}  // namespace testing
}  // namespace tflite

using namespace micro_test;


///*****************************************CUSTOM TEST CASES***********************************************************/

tflite::testing::LstmEvalCheckData<100, 10, 50> GetCustomLstmEvalCheckData() {
    tflite::testing::LstmEvalCheckData<100, 10, 50> eval_data;
  const float input_data[100] = {201, -10, 80, 247, 102, -66, 118, 148, 197, 5, 176, 191, -208, 216, -218, -42, 17, -54, -22, -132, -61, -96, 203, -56, 83, 107, -106, 235, 49, 194, 176, -209, 97, 194, -1, 201, -251, -7, 99, -177, 206, 63, 136, 5, 193, 182, -184, -169, -224, -251, 130, 75, 66, 224, -103, 230, 128, 233, 104, -131, -224, 13, -126, 54, 28, 246, -94, 15, -191, 204, -76, 201, 216, 15, -195, 243, 160, -2, 196, 75, 145, -161, 101, 25, 252, 193, 26, 121, 134, 218, -168, -66, -75, -214, 250, 22, 192, -55, 138, -203};
  std::memcpy(eval_data.input_data, input_data, 100 * sizeof(float));

  // Initialize hidden state as zeros
  const float hidden_state[10] = {};
  std::memcpy(eval_data.hidden_state, hidden_state, 10 * sizeof(float));

  // The expected model output after 3 time steps using the fixed input and
  // parameters
  //expected output data is generated from TFLM REF C kernel
  const float expected_output[50] = {7.690664388 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 9.613330485 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 10.04058962 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 10.04058962 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 10.04058962 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 };
  std::memcpy(eval_data.expected_output, expected_output, 50 * sizeof(float));

  const float expected_hidden_state[10] = {1.004058962 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0};
  std::memcpy(eval_data.expected_hidden_state, expected_hidden_state,
              10 * sizeof(float));

  const float expected_cell_state[10] = {4.995605366, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0};
  std::memcpy(eval_data.expected_cell_state, expected_cell_state,
              10 * sizeof(float));
  return eval_data;
}



constexpr TfLiteUnidirectionalSequenceLSTMParams kDefaultBuiltinData = {
    /*.activation=*/kTfLiteActTanh,
    /*.cell_clip=*/6,
    /*.proj_clip=*/3,
    /*.time_major=*/false,
    /*.asymmetric_quantize_inputs=*/true,
    /*diagonal_recurrent_tensors=*/false};

tflite::testing::LstmNodeContent<float, float, float, float, 1, 5, 20, 10>
CreateCustomFloatNodeContents(const float* input_data,
                               const float* hidden_state_data,
                               const float* cell_state_data) {
  // Parameters for different gates
  // negative large weights for forget gate to make it really forget
  const tflite::testing::GateData<float, float, 20, 10> forget_gate_data = {
      /*.activation_weight=*/{1},
      /*.recurrent_weight=*/{2},
      /*.fused_bias=*/{3},
      /*activation_zp_folded_bias=*/{2},
      /*recurrent_zp_folded_bias=*/{2}};
  // positive large weights for input gate to make it really remember
  const tflite::testing::GateData<float, float, 20, 10> input_gate_data = {
      /*.activation_weight=*/{1},
      /*.recurrent_weight=*/{2},
      /*.fused_bias=*/{3},
      /*activation_zp_folded_bias=*/{2},
      /*recurrent_zp_folded_bias=*/{2}};
  // all ones to test the behavior of tanh at normal range (-1,1)
  const tflite::testing::GateData<float, float, 20, 10> cell_gate_data = {
      /*.activation_weight=*/{1},
      /*.recurrent_weight=*/{2},
      /*.fused_bias=*/{3},
      /*activation_zp_folded_bias=*/{2},
      /*recurrent_zp_folded_bias=*/{2}};
  // all ones to test the behavior of sigmoid at normal range (-1. 1)
  const tflite::testing::GateData<float, float, 20, 10> output_gate_data = {
      /*.activation_weight=*/{1},
      /*.recurrent_weight=*/{2},
      /*.fused_bias=*/{3},
      /*activation_zp_folded_bias=*/{2},
      /*recurrent_zp_folded_bias=*/{2}};

  tflite::testing::LstmNodeContent<float, float, float, float, 1, 5, 20, 10> float_node_contents(
          kDefaultBuiltinData, forget_gate_data, input_gate_data, cell_gate_data,
      output_gate_data);

  if (input_data != nullptr) {
    float_node_contents.SetInputData(input_data);
  }
  if (hidden_state_data != nullptr) {
    float_node_contents.SetHiddenStateData(hidden_state_data);
  }
  if (cell_state_data != nullptr) {
    float_node_contents.SetCellStateData(cell_state_data);
  }
  return float_node_contents;
}

tflite::testing::LstmNodeContent<int16_t, int8_t, int64_t, int16_t, 1, 5, 20, 10>
CreateCustomInt16NodeContents(const float* input_data = nullptr,
                               const float* hidden_state = nullptr,
                               const float* cell_state = nullptr) {
  auto float_node_content = CreateCustomFloatNodeContents(input_data, hidden_state, cell_state);
  const auto quantization_settings = tflite::testing::Get2X2Int16LstmQuantizationSettings_CustomTest1();
  return tflite::testing::CreateIntegerNodeContents<int16_t, int8_t, int64_t, int16_t, 1, 5, 20, 10>(   quantization_settings,
                                                                                                              /*fold_zero_point=*/false,
                                                                                                              float_node_content);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
tflite::testing::LstmEvalCheckData<200, 20, 200> GetCustomTest2LstmEvalCheckData() {
    tflite::testing::LstmEvalCheckData<200, 20, 200> eval_data;
  const float input_data[200] = {184, -176, -98, 168, 94, -78, 136, 74, 35, -47, -40, 193, -125, -162, -205, -113, -56, -4, -173, 248, -230, 181, 225, -34, -10, 117, 152, 6, -15, -110, -91, -135, -120, 55, -209, -105, -178, 104, -56, -122, -54, 239, -158, 168, 205, -80, 34, 212, 39, -54, -179, -210, 205, 29, -85, 13, 137, -161, -108, 81, -118, -62, -156, -212, 69, -230, 57, -162, 116, 96, 25, -249, 77, -59, 130, -146, 223, -83, 97, 47, 194, -168, 56, -244, 8, -105, 176, -106, 39, 203, -24, -205, 107, -110, 15, 134, -234, 248, -227, 212, -189, -56, -173, 185, -11, 227, -40, -249, -213, -230, 221, -205, 184, 190, -214, 190, 187, -158, -166, -54, 94, -250, 152, 28, 47, 227, 177, 242, 111, -170, 11, 96, 143, 210, -176, 13, -159, -158, 191, -245, -212, 234, 32, 190, -34, -226, 130, 27, -185, -140, -234, 145, -41, 197, -143, -117, -15, 83, -128, 51, -11, -198, -36, 146, 41, -126, 77, 20, 44, 193, 104, -139, 222, -43, 249, 206, 32, 161, -200, -22, -66, -63, 239, 81, 95, -27, -120, 192, 167, 172, -100, -162, 27, 210, 9, -180, 246, 46, 30, -149};
  std::memcpy(eval_data.input_data, input_data, 200 * sizeof(float));

  // Initialize hidden state
  const float hidden_state[20] = {5};
  std::memcpy(eval_data.hidden_state, hidden_state, 20 * sizeof(float));

  // The expected model output after 3 time steps using the fixed input and
  // parameters
  //expected output data is generated from TFLM REF C kernel
  const float expected_output[200] = {856.654561 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 803.2471694 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 805.3834651 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 811.7923521 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 843.836787 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 826.7464217 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 848.1093783 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 826.7464217 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 820.3375347 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 820.3375347 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0};
  std::memcpy(eval_data.expected_output, expected_output, 200 * sizeof(float));

  const float expected_hidden_state[20] = {0.820337535 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0};
  std::memcpy(eval_data.expected_hidden_state, expected_hidden_state,
              20 * sizeof(float));

  const float expected_cell_state[20] = {3.930663982 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0};
  std::memcpy(eval_data.expected_cell_state, expected_cell_state,
              20 * sizeof(float));
  return eval_data;
}

tflite::testing::LstmNodeContent<float, float, float, float, 1, 10, 20, 20>
CreateCustomTest2FloatNodeContents(const float* input_data,
                               const float* hidden_state_data,
                               const float* cell_state_data) {
  // Parameters for different gates
  // negative large weights for forget gate to make it really forget
  const tflite::testing::GateData<float, float, 20, 20> forget_gate_data = {
      /*.activation_weight=*/{1},
      /*.recurrent_weight=*/{2},
      /*.fused_bias=*/{3},
      /*activation_zp_folded_bias=*/{2},
      /*recurrent_zp_folded_bias=*/{2}};
  // positive large weights for input gate to make it really remember
  const tflite::testing::GateData<float, float, 20, 20> input_gate_data = {
      /*.activation_weight=*/{1},
      /*.recurrent_weight=*/{2},
      /*.fused_bias=*/{3},
      /*activation_zp_folded_bias=*/{2},
      /*recurrent_zp_folded_bias=*/{2}};
  // all ones to test the behavior of tanh at normal range (-1,1)
  const tflite::testing::GateData<float, float, 20, 20> cell_gate_data = {
      /*.activation_weight=*/{1},
      /*.recurrent_weight=*/{2},
      /*.fused_bias=*/{3},
      /*activation_zp_folded_bias=*/{2},
      /*recurrent_zp_folded_bias=*/{2}};
  // all ones to test the behavior of sigmoid at normal range (-1. 1)
  const tflite::testing::GateData<float, float, 20, 20> output_gate_data = {
      /*.activation_weight=*/{1},
      /*.recurrent_weight=*/{2},
      /*.fused_bias=*/{3},
      /*activation_zp_folded_bias=*/{2},
      /*recurrent_zp_folded_bias=*/{2}};

  tflite::testing::LstmNodeContent<float, float, float, float, 1, 10, 20, 20> float_node_contents(
          kDefaultBuiltinData, forget_gate_data, input_gate_data, cell_gate_data,
      output_gate_data);

  if (input_data != nullptr) {
    float_node_contents.SetInputData(input_data);
  }
  if (hidden_state_data != nullptr) {
    float_node_contents.SetHiddenStateData(hidden_state_data);
  }
  if (cell_state_data != nullptr) {
    float_node_contents.SetCellStateData(cell_state_data);
  }
  return float_node_contents;
}

tflite::testing::LstmNodeContent<int16_t, int8_t, int64_t, int16_t, 1, 10, 20, 20>
CreateCustomTest2Int16NodeContents(const float* input_data = nullptr,
                               const float* hidden_state = nullptr,
                               const float* cell_state = nullptr) {
  auto float_node_content = CreateCustomTest2FloatNodeContents(input_data, hidden_state, cell_state);
  const auto quantization_settings = tflite::testing::Get2X2Int16LstmQuantizationSettings_CustomTest2();
  return tflite::testing::CreateIntegerNodeContents<int16_t, int8_t, int64_t, int16_t, 1, 10, 20, 20>(   quantization_settings,
                                                                                                              /*fold_zero_point=*/false,
                                                                                                              float_node_content);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
tflite::testing::LstmEvalCheckData<250, 10, 50> GetCustomTest3LstmEvalCheckData() {
    tflite::testing::LstmEvalCheckData<250, 10, 50> eval_data;
  const float input_data[250] = {-57, 145, -111, 254, 58, 76, -58, -186, 75, 238, 157, 169, -20, 20, 90, -94, -97, -45, 71, 145, 232, 90, 187, 49, -146, 79, -145, 21, 119, 27, -93, 252, 0, -57, -211, -13, 174, -225, -209, -37, 246, -227, 62, 176, -24, 151, -71, -220, -75, 3, 95, -2, 52, -168, 149, 101, -179, 193, 85, -13, 186, 61, 168, -129, -101, 9, -88, 224, 104, 65, -163, 113, 189, 167, 41, 91, 37, 231, 187, 129, 60, 70, -165, 139, -166, -108, -75, -246, 205, -164, 48, 42, -256, -252, -72, -225, 146, -19, -57, -118, -237, 230, 180, -221, -130, -195, 179, 111, 70, -210, -127, -130, -175, 167, 123, 253, 79, -153, -55, 30, -103, -135, -112, -238, 144, 66, 41, 144, -198, 36, -159, -73, -67, -126, -87, 175, 183, 173, 39, -172, 112, 125, -22, 181, 42, 115, 136, -121, 61, 233, 206, -238, -179, 49, -226, 188, 89, -44, 184, -191, -8, -176, 206, 144, 168, 98, 103, 57, 232, 38, 139, 11, 15, -251, -24, 181, 185, -254, -254, 212, 236, 35, -69, 27, -26, -163, 81, -76, 189, -16, -17, 130, -66, -149, 68, 89, -80, -158, 70, 12, 73, 217, -158, -226, 48, -239, 136, -224, -112, 50, 61, -246, -19, -136, -140, -33, -234, 40, -217, 40, 64, -217, -91, 34, 11, 0, 109, 58, -141, -112, 67, 166, -84, -207, -180, 105, 45, -83, 54, 229, -202, 29, 92, 128, 53, -184, -55, 76, 142, 3};
  std::memcpy(eval_data.input_data, input_data, 250 * sizeof(float));

  // Initialize hidden state as zeros
  const float hidden_state[10] = {};
  std::memcpy(eval_data.hidden_state, hidden_state, 10 * sizeof(float));

  // The expected model output after 3 time steps using the fixed input and
  // parameters
  //expected output data is generated from TFLM REF C kernel
  const float expected_output[50] = {-0.004279904 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0.057778703 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0.002139952 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0.068478463 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0.096297839 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0};
  std::memcpy(eval_data.expected_output, expected_output, 50 * sizeof(float));

  const float expected_hidden_state[10] = {0.917871133, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0};
  std::memcpy(eval_data.expected_hidden_state, expected_hidden_state,
              10 * sizeof(float));

  const float expected_cell_state[10] = {1.70898434, 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0};
  std::memcpy(eval_data.expected_cell_state, expected_cell_state,
              10 * sizeof(float));
  return eval_data;
}

tflite::testing::LstmNodeContent<float, float, float, float, 1, 5, 50, 10>
CreateCustomTest3FloatNodeContents(const float* input_data,
                               const float* hidden_state_data,
                               const float* cell_state_data) {
  // Parameters for different gates
  // negative large weights for forget gate to make it really forget
  const tflite::testing::GateData<float, float, 50, 10> forget_gate_data = {
      /*.activation_weight=*/{1},
      /*.recurrent_weight=*/{2},
      /*.fused_bias=*/{3},
      /*activation_zp_folded_bias=*/{2},
      /*recurrent_zp_folded_bias=*/{2}};
  // positive large weights for input gate to make it really remember
  const tflite::testing::GateData<float, float, 50, 10> input_gate_data = {
      /*.activation_weight=*/{1},
      /*.recurrent_weight=*/{2},
      /*.fused_bias=*/{3},
      /*activation_zp_folded_bias=*/{2},
      /*recurrent_zp_folded_bias=*/{2}};
  // all ones to test the behavior of tanh at normal range (-1,1)
  const tflite::testing::GateData<float, float, 50, 10> cell_gate_data = {
      /*.activation_weight=*/{1},
      /*.recurrent_weight=*/{2},
      /*.fused_bias=*/{3},
      /*activation_zp_folded_bias=*/{2},
      /*recurrent_zp_folded_bias=*/{2}};
  // all ones to test the behavior of sigmoid at normal range (-1. 1)
  const tflite::testing::GateData<float, float, 50, 10> output_gate_data = {
      /*.activation_weight=*/{1},
      /*.recurrent_weight=*/{2},
      /*.fused_bias=*/{3},
      /*activation_zp_folded_bias=*/{2},
      /*recurrent_zp_folded_bias=*/{2}};

  tflite::testing::LstmNodeContent<float, float, float, float, 1, 5, 50, 10> float_node_contents(
          kDefaultBuiltinData, forget_gate_data, input_gate_data, cell_gate_data,
      output_gate_data);

  if (input_data != nullptr) {
    float_node_contents.SetInputData(input_data);
  }
  if (hidden_state_data != nullptr) {
    float_node_contents.SetHiddenStateData(hidden_state_data);
  }
  if (cell_state_data != nullptr) {
    float_node_contents.SetCellStateData(cell_state_data);
  }
  return float_node_contents;
}

tflite::testing::LstmNodeContent<int16_t, int8_t, int64_t, int16_t, 1, 5, 50, 10>
CreateCustomTest3Int16NodeContents(const float* input_data = nullptr,
                               const float* hidden_state = nullptr,
                               const float* cell_state = nullptr) {
  auto float_node_content = CreateCustomTest3FloatNodeContents(input_data, hidden_state, cell_state);
  const auto quantization_settings = tflite::testing::Get2X2Int16LstmQuantizationSettings_CustomTest3();
  return tflite::testing::CreateIntegerNodeContents<int16_t, int8_t, int64_t, int16_t, 1, 5, 50, 10>(   quantization_settings,
                                                                                                              /*fold_zero_point=*/false,
                                                                                                              float_node_content);
}
/*****************************************CUSTOM TEST CASES***********************************************************/

int unidirectionalSequenceLstmTest() {
  micro_test::tests_passed = 0;
  micro_test::tests_failed = 0;
  tflite::InitializeTest();
// TODO(b/230666079) enable below tests for xtensa when the xtensa
// kernel is reconciled with reference kernel
#if !defined(XTENSA)
TF_LITE_MICRO_TEST(TestUnidirectionalLSTMFloat) {
  const tflite::testing::LstmEvalCheckData<12, 4, 12> kernel_eval_data =
      tflite::testing::Get2X2LstmEvalCheckData();
  tflite::testing::LstmNodeContent<float, float, float, float, 2, 3, 2, 2>
      float_node_contents = tflite::testing::Create2x3x2X2FloatNodeContents(
          kernel_eval_data.input_data, kernel_eval_data.hidden_state);

  const float tolerance = 1e-6;
  tflite::testing::TestUnidirectionalLSTMFloat(kernel_eval_data, tolerance,
                                               tolerance, float_node_contents);
}

TF_LITE_MICRO_TEST(TestUnidirectionalLSTMInt8) {
  const tflite::testing::LstmEvalCheckData<12, 4, 12> kernel_eval_data =
      tflite::testing::Get2X2LstmEvalCheckData();
  tflite::testing::LstmNodeContent<int8_t, int8_t, int32_t, int16_t, 2, 3, 2, 2>
      int8_node_contents = tflite::testing::Create2x3x2X2Int8NodeContents(
          kernel_eval_data.input_data, kernel_eval_data.hidden_state);

  const float hidden_state_tolerance = 1e-2;
  // cell state degrade due to integer overflow
  const float cell_state_tolerance = 1e-2;
  tflite::testing::TestUnidirectionalLSTMInteger(
      kernel_eval_data, hidden_state_tolerance, cell_state_tolerance,
      int8_node_contents);
}

TF_LITE_MICRO_TEST(TestUnidirectionalLSTMInt16) {
  const tflite::testing::LstmEvalCheckData<12, 4, 12> kernel_eval_data =
      tflite::testing::Get2X2LstmEvalCheckData();
  tflite::testing::LstmNodeContent<int16_t, int8_t, int64_t, int16_t, 2, 3, 2,
                                   2>
      int16_node_contents = tflite::testing::Create2x3x2X2Int16NodeContents(
          kernel_eval_data.input_data, kernel_eval_data.hidden_state);

  const float hidden_state_tolerance = 1e-2;  // reduced tolerance to compensate for sigmoid/tanh limited resolution
  // cell state degrade due to integer overflow
  const float cell_state_tolerance = 1e-2;
  tflite::testing::TestUnidirectionalLSTMInteger(
      kernel_eval_data, hidden_state_tolerance, cell_state_tolerance,
      int16_node_contents);
}

TF_LITE_MICRO_TEST(TestUnidirectionalLSTMInt16_CUSTOM) {
  const tflite::testing::LstmEvalCheckData<100, 10, 50> kernel_eval_data = GetCustomLstmEvalCheckData();
  /*LstmNodeContent<typename ActivationType, typename WeightType, typename BiasType, typename CellType, int 1_size, int time_steps, int input_dimension, int 10ension>*/
  tflite::testing::LstmNodeContent<int16_t, int8_t, int64_t, int16_t, 1, 5, 20,10>
  int16_node_contents = CreateCustomInt16NodeContents(kernel_eval_data.input_data, kernel_eval_data.hidden_state);

  const float hidden_state_tolerance = 1e-2;
  // cell state degrade due to integer overflow
  const float cell_state_tolerance = 1e-2;
  tflite::testing::TestUnidirectionalLSTMInteger(
      kernel_eval_data, hidden_state_tolerance, cell_state_tolerance,
      int16_node_contents);
}

TF_LITE_MICRO_TEST(TestUnidirectionalLSTMInt16_CUSTOM_TEST2) {
  const tflite::testing::LstmEvalCheckData<200, 20, 200> kernel_eval_data = GetCustomTest2LstmEvalCheckData();
  /*LstmNodeContent<typename ActivationType, typename WeightType, typename BiasType, typename CellType, int 1_size, int time_steps, int input_dimension, int 20ension>*/
  tflite::testing::LstmNodeContent<int16_t, int8_t, int64_t, int16_t, 1, 10, 20,20>
  int16_node_contents = CreateCustomTest2Int16NodeContents(kernel_eval_data.input_data, kernel_eval_data.hidden_state);

  const float hidden_state_tolerance = 1e-2;
  // cell state degrade due to integer overflow
  const float cell_state_tolerance = 1e-2;
  tflite::testing::TestUnidirectionalLSTMInteger(
      kernel_eval_data, hidden_state_tolerance, cell_state_tolerance,
      int16_node_contents);
}

TF_LITE_MICRO_TEST(TestUnidirectionalLSTMInt16_CUSTOM_TEST3) {
  const tflite::testing::LstmEvalCheckData<250, 10, 50> kernel_eval_data = GetCustomTest3LstmEvalCheckData();
  /*LstmNodeContent<typename ActivationType, typename WeightType, typename BiasType, typename CellType, int 1_size, int time_steps, int input_dimension, int 10ension>*/
  tflite::testing::LstmNodeContent<int16_t, int8_t, int64_t, int16_t, 1, 5, 50,10>
  int16_node_contents = CreateCustomTest3Int16NodeContents(kernel_eval_data.input_data, kernel_eval_data.hidden_state);

  const float hidden_state_tolerance = 1e-2;
  // cell state degrade due to integer overflow
  const float cell_state_tolerance = 1e-2;
  tflite::testing::TestUnidirectionalLSTMInteger(
      kernel_eval_data, hidden_state_tolerance, cell_state_tolerance,
      int16_node_contents);
}

#endif  // !defined(XTENSA)
TF_LITE_MICRO_TESTS_END
