/* Copyright 2022 The TensorFlow Authors. All Rights Reserved.

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
#include "tensorflow/lite/kernels/internal/reference/pooling.h"

#include "tensorflow/lite/c/builtin_op_data.h"
#include "tensorflow/lite/kernels/kernel_util.h"
#include "tensorflow/lite/micro/kernels/kernel_util.h"
#include "tensorflow/lite/micro/kernels/pooling.h"
#include "tensorflow/lite/micro/micro_log.h"

#include "adi_sharcfx_nn.h"
namespace tflite {

int8_t pTempPool[128*128*128]__attribute__((section(".L3.data"), aligned(8)));

namespace {

TfLiteStatus AverageEval(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->builtin_data != nullptr);
  auto* params = reinterpret_cast<TfLitePoolParams*>(node->builtin_data);

  TFLITE_DCHECK(node->user_data != nullptr);
  const OpDataPooling* data =
      static_cast<const OpDataPooling*>(node->user_data);

  const TfLiteEvalTensor* input =
      micro::GetEvalInput(context, node, kPoolingInputTensor);
  TfLiteEvalTensor* output =
      micro::GetEvalOutput(context, node, kPoolingOutputTensor);

  // Inputs and outputs share the same type, guaranteed by the converter.
  switch (input->type) {
    case kTfLiteFloat32:
      AveragePoolingEvalFloat(context, node, params, data, input, output);
      break;
    case kTfLiteInt8:{
#ifdef DISPLAY_CYCLE_COUNTS
        	cycle_t var = 0, cyc=0; //Variables for cycle counting
			START_CYCLE_COUNT (var);
#endif
      AveragePoolingEvalQuantized<int8_t>(context, node, params, data, input,
                                          output);
#ifdef DISPLAY_CYCLE_COUNTS
		  STOP_CYCLE_COUNT (cyc, var);
		  PRINT_INFO("\nNumber of cycles to run Avg pooling Layer kTfLiteInt8 %lu:\n", cyc);
#endif
      break;}
    case kTfLiteInt16:
      AveragePoolingEvalQuantized<int16_t>(context, node, params, data, input,
                                           output);
      break;
    default:
      MicroPrintf("Input type %s is not currently supported",
                  TfLiteTypeGetName(input->type));
      return kTfLiteError;
  }
  return kTfLiteOk;
}

TfLiteStatus MaxEval(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->builtin_data != nullptr);
  auto* params = reinterpret_cast<TfLitePoolParams*>(node->builtin_data);

  TFLITE_DCHECK(node->user_data != nullptr);
  const OpDataPooling* data =
      static_cast<const OpDataPooling*>(node->user_data);

  const TfLiteEvalTensor* input =
      micro::GetEvalInput(context, node, kPoolingInputTensor);
  TfLiteEvalTensor* output =
      micro::GetEvalOutput(context, node, kPoolingOutputTensor);

  switch (input->type) {
    case kTfLiteFloat32:
      MaxPoolingEvalFloat(context, node, params, data, input, output);
      break;
    case kTfLiteInt8:
    {
#ifdef DISPLAY_CYCLE_COUNTS
		cycle_t var = 0, cyc=0; //Variables for cycle counting
		START_CYCLE_COUNT (var);
#endif
#ifndef USE_OPTIMIZED_MAXPOOL
        MaxPoolingEvalQuantized<int8_t>(context, node, params, data, input,
                                        output);
#else
        const RuntimeShape& input_shape = tflite::micro::GetTensorShape(input);
        const RuntimeShape& output_shape = tflite::micro::GetTensorShape(output);
        const int input_height = input_shape.Dims(1);
        const int input_width = input_shape.Dims(2);
        const int output_height = output_shape.Dims(1);
        const int output_width = output_shape.Dims(2);
        const int depth = MatchingDim(input_shape, 3, output_shape, 3);
        adi_sharcfx_maxpool_int8(
        						input_height,
        						input_width,
        						output_height,
        						output_width,
        						params->stride_height,
        						params->stride_width,
        						params->filter_height,
        						params->filter_width,
        						data->padding.height,
        						data->padding.width,
        						data->activation_min,
        						data->activation_max,
        						depth,
        						tflite::micro::GetTensorData<int8_t>(input),
        						tflite::micro::GetTensorData<int8_t>(output));
#endif
#ifdef DISPLAY_CYCLE_COUNTS
	  STOP_CYCLE_COUNT (cyc, var);
	  PRINT_INFO("\nNumber of cycles to run MaxPooling Layer :  %lu\n", cyc);
#endif
    }
      break;
    case kTfLiteInt16:
      MaxPoolingEvalQuantized<int16_t>(context, node, params, data, input,
                                       output);
      break;
    default:
      MicroPrintf("Type %s not currently supported.",
                  TfLiteTypeGetName(input->type));
      return kTfLiteError;
  }
  return kTfLiteOk;
}

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(OpDataPooling));
}

}  // namespace

TfLiteRegistration_V1 Register_AVERAGE_POOL_2D() {
  return tflite::micro::RegisterOp(Init, PoolingPrepare, AverageEval);
}

TfLiteRegistration_V1 Register_MAX_POOL_2D() {
  return tflite::micro::RegisterOp(Init, PoolingPrepare, MaxEval);
}

}  // namespace tflite
