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

#include "tensorflow/lite/micro/kernels/fully_connected.h"

#include "tensorflow/lite/c/builtin_op_data.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/kernels/internal/portable_tensor_utils.h"
#include "tensorflow/lite/kernels/internal/reference/fully_connected.h"
#include "tensorflow/lite/kernels/internal/reference/integer_ops/fully_connected.h"
#include "tensorflow/lite/micro/kernels/kernel_util.h"
#include "tensorflow/lite/micro/micro_log.h"

#include "primitives.h"
#define BUFFER_LENGTH 25*20*8 //Buffer length is calculated based on expected input dimensions for FC layer(here, calculated for micro_speech)
int8_t pInputBuffer_8bit[BUFFER_LENGTH]__attribute__((section(".L2.data"), aligned(16)));
int16_t pInputBuffer_16bit[BUFFER_LENGTH]__attribute__((section(".L2.data"), aligned(16)));

namespace tflite {
namespace {

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context,
                                           sizeof(OpDataFullyConnected));
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  MicroContext* micro_context = GetMicroContext(context);

  TFLITE_DCHECK(node->user_data != nullptr);
  TFLITE_DCHECK(node->builtin_data != nullptr);

  auto* data = static_cast<OpDataFullyConnected*>(node->user_data);
  const auto params =
      static_cast<const TfLiteFullyConnectedParams*>(node->builtin_data);

  TfLiteTensor* input =
      micro_context->AllocateTempInputTensor(node, kFullyConnectedInputTensor);
  TF_LITE_ENSURE(context, input != nullptr);
  TfLiteTensor* filter = micro_context->AllocateTempInputTensor(
      node, kFullyConnectedWeightsTensor);
  TF_LITE_ENSURE(context, filter != nullptr);
  TfLiteTensor* bias =
      micro_context->AllocateTempInputTensor(node, kFullyConnectedBiasTensor);
  TfLiteTensor* output = micro_context->AllocateTempOutputTensor(
      node, kFullyConnectedOutputTensor);
  TF_LITE_ENSURE(context, output != nullptr);
  TF_LITE_ENSURE_TYPES_EQ(context, input->type, output->type);

  if ((input->type == kTfLiteFloat32 && filter->type != kTfLiteFloat32) ||
      (input->type == kTfLiteInt8 &&
       (filter->type != kTfLiteInt8 && filter->type != kTfLiteInt4)) ||
      (input->type == kTfLiteInt16 && filter->type != kTfLiteInt8)) {
    MicroPrintf("Input type: %s with filter type : %s not supported.",
                TfLiteTypeGetName(input->type),
                TfLiteTypeGetName(filter->type));
    return kTfLiteError;
  }

  if (filter->type == kTfLiteInt4) {
    int filter_size =
        RuntimeShape(filter->dims->size,
                     reinterpret_cast<const int32_t*>(filter->dims->data))
            .FlatSize();
    context->RequestScratchBufferInArena(context, filter_size,
                                         &data->filter_buffer_index);
  }

  TF_LITE_ENSURE_OK(context, CalculateOpDataFullyConnected(
                                 context, params->activation, input->type,
                                 input, filter, bias, output, data));

  micro_context->DeallocateTempTfLiteTensor(input);
  micro_context->DeallocateTempTfLiteTensor(filter);
  if (bias != nullptr) {
    micro_context->DeallocateTempTfLiteTensor(bias);
  }
  micro_context->DeallocateTempTfLiteTensor(output);
  return kTfLiteOk;
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->builtin_data != nullptr);
  const auto* params =
      static_cast<const TfLiteFullyConnectedParams*>(node->builtin_data);

  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kFullyConnectedInputTensor);
  const TfLiteEvalTensor* filter =
      tflite::micro::GetEvalInput(context, node, kFullyConnectedWeightsTensor);
  const TfLiteEvalTensor* bias =
      tflite::micro::GetEvalInput(context, node, kFullyConnectedBiasTensor);
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kFullyConnectedOutputTensor);

  TFLITE_DCHECK(node->user_data != nullptr);

  const auto& data =
      *(static_cast<const OpDataFullyConnected*>(node->user_data));

  // Checks in Prepare ensure input, output and filter types are all the same.
  switch (input->type) {
    case kTfLiteFloat32: {
      tflite::reference_ops::FullyConnected(
          FullyConnectedParamsFloat(params->activation),
          tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<float>(input),
          tflite::micro::GetTensorShape(filter),
          tflite::micro::GetTensorData<float>(filter),
          tflite::micro::GetTensorShape(bias),
          tflite::micro::GetOptionalTensorData<float>(bias),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<float>(output));
      break;
    }

    case kTfLiteInt8: {
      switch (filter->type) {
        case kTfLiteInt4: {
          int8_t* unpacked_filter_data = static_cast<int8_t*>(
              context->GetScratchBuffer(context, data.filter_buffer_index));
          tflite::tensor_utils::UnpackDenseInt4IntoInt8(
              tflite::micro::GetTensorData<int8_t>(filter),
              tflite::micro::GetTensorShape(filter).FlatSize(),
              unpacked_filter_data);
          tflite::reference_integer_ops::FullyConnected(
              FullyConnectedParamsQuantized(data),
              tflite::micro::GetTensorShape(input),
              tflite::micro::GetTensorData<int8_t>(input),
              tflite::micro::GetTensorShape(filter), unpacked_filter_data,
              tflite::micro::GetTensorShape(bias),
              tflite::micro::GetOptionalTensorData<int32_t>(bias),
              tflite::micro::GetTensorShape(output),
              tflite::micro::GetTensorData<int8_t>(output));
          break;
        }
        case kTfLiteInt8: {
#ifdef DISPLAY_CYCLE_COUNTS
        	long int var = 0, cyc=0; //Variables for cycle counting
			START_CYCLE_COUNT (var);
#endif
#ifndef USE_OPTIMIZED_FC
          tflite::reference_integer_ops::FullyConnected(
              FullyConnectedParamsQuantized(data),
              tflite::micro::GetTensorShape(input),
              tflite::micro::GetTensorData<int8_t>(input),
              tflite::micro::GetTensorShape(filter),
              tflite::micro::GetTensorData<int8_t>(filter),
              tflite::micro::GetTensorShape(bias),
              tflite::micro::GetOptionalTensorData<int32_t>(bias),
              tflite::micro::GetTensorShape(output),
              tflite::micro::GetTensorData<int8_t>(output));
#else
#ifdef USE_OPTIMIZED_DEPTHCONV
		  RuntimeShape input_shape = tflite::micro::GetTensorShape(input);
		  const int input_width2 = input_shape.Dims(1);//25
		  const int input_width1 = input_shape.Dims(2);//20
		  const int input_channels = input_shape.Dims(3);//8
		  /* Current optimised depthconv function processes data in non-interleaved format.
		   * All optimised functions will support interleaved data moving forward.
		   * Until depthconv is re-worked to make it work with interleaved data format,
		   * this is a temporary fix to maintain compatibility with the new optimised
		   * and reference TFLM functions.
		   *
		   * The following code interleaves the input data, in case the previous layer
		   * is an optimised depthconv layer (which gives non-interleaved output data),
		   * which is the case in micro_speech example application.
		   */

		  //Check if input dimensions are within the buffer range to perform a data interleaving operation, if not fall back to reference TFLM code
		  if((input_width2 * input_width1 * input_channels) <= BUFFER_LENGTH){
			  const int8_t *pInTemp = tflite::micro::GetTensorData<int8_t>(input);
			  for(int nCh = 0;nCh < input_channels;nCh++)
			  {
				  for(int nCol = 0;nCol < input_width2;nCol++)
				  {
					  for(int nRow = 0;nRow < input_width1;nRow++)
					  {
						  pInputBuffer_8bit[nCol * input_width1 * input_channels + nRow * input_channels + nCh] =
								  pInTemp[nCh*input_width2*input_width1 + nCol*input_width1 + nRow];//

					  }
				  }
			  }
		  }
		  else{
	          tflite::reference_integer_ops::FullyConnected(
	              FullyConnectedParamsQuantized(data),
	              tflite::micro::GetTensorShape(input),
	              tflite::micro::GetTensorData<int8_t>(input),
	              tflite::micro::GetTensorShape(filter),
	              tflite::micro::GetTensorData<int8_t>(filter),
	              tflite::micro::GetTensorShape(bias),
	              tflite::micro::GetOptionalTensorData<int32_t>(bias),
	              tflite::micro::GetTensorShape(output),
	              tflite::micro::GetTensorData<int8_t>(output));
			  break;
		  }
#endif
          FullyConnectedParams params_read = FullyConnectedParamsQuantized(data);
          RuntimeShape filter_shape = tflite::micro::GetTensorShape(filter);
          RuntimeShape output_shape = tflite::micro::GetTensorShape(output);
          fullyConnected_asm_8bit(
#ifdef USE_OPTIMIZED_DEPTHCONV
		    pInputBuffer_8bit,
#else
        	tflite::micro::GetTensorData<int8_t>(input),
#endif
			tflite::micro::GetTensorData<int8_t>(filter),
			tflite::micro::GetOptionalTensorData<int32_t>(bias),
			tflite::micro::GetTensorData<int8_t>(output),
			filter_shape.Dims(filter_shape.DimensionsCount()-1),					//filter_shape.Dims(filter_dim_count - 1),;
			output_shape.Dims(output_shape.DimensionsCount() - 1),
			FlatSizeSkipDim(output_shape, output_shape.DimensionsCount() - 1),//batches
			params_read.output_multiplier,
			params_read.output_shift,
			params_read.input_offset,
			params_read.weights_offset,
			params_read.output_offset,
			params_read.quantized_activation_min,
			params_read.quantized_activation_max);
#endif
#ifdef DISPLAY_CYCLE_COUNTS
		  STOP_CYCLE_COUNT (cyc, var);
		  printf("\tNumber of cycles to run FullyConnected Layer : \t%ld \n", cyc);
#endif
#ifdef SAVE_OUTPUTS
	int8_t * temp =  tflite::micro::GetTensorData<int8_t>(output);
#ifdef USE_OPTIMIZED_FC
	FILE *fp = fopen("optimised-output-FC.txt","w");
	for (int i=0; i<4; i++)
	{
		fprintf(fp, "%d\t", *temp++);
	}
	fclose(fp);
#else
	FILE *fp = fopen("reference-output-FC.txt","w");
	for (int i=0; i<4; i++)
	{
		fprintf(fp, "%d\t", *temp++);
	}
	fclose(fp);
#endif
#endif
          break;
        }
        default: {
          MicroPrintf("Filter type %s (%d) not supported.",
                      TfLiteTypeGetName(filter->type), input->type);
          return kTfLiteError;
        }
      }
      break;
    }

    case kTfLiteInt16: {
      switch (filter->type) {
        case kTfLiteInt8: {
#ifdef DISPLAY_CYCLE_COUNTS
        	long int var = 0, cyc=0; //Variables for cycle counting
			START_CYCLE_COUNT (var);
#endif
#ifndef USE_OPTIMIZED_FC
          tflite::reference_integer_ops::FullyConnected(
              FullyConnectedParamsQuantized(data),
              tflite::micro::GetTensorShape(input),
              tflite::micro::GetTensorData<int16_t>(input),
              tflite::micro::GetTensorShape(filter),
              tflite::micro::GetTensorData<int8_t>(filter),
              tflite::micro::GetTensorShape(bias),
              tflite::micro::GetOptionalTensorData<int64_t>(bias),
              tflite::micro::GetTensorShape(output),
              tflite::micro::GetTensorData<int16_t>(output));
#else
#ifdef USE_OPTIMIZED_DEPTHCONV
		  //FIXME: Change the depthconv to work with interleaved data. Once that is done, the following interleaving step can be removed.
		  RuntimeShape input_shape = tflite::micro::GetTensorShape(input);
		  const int input_width2 = input_shape.Dims(1);//25
		  const int input_width1 = input_shape.Dims(2);//20
		  const int input_channels = input_shape.Dims(3);//8

		  /* Current optimised depthconv function processes data in non-interleaved format.
		   * All optimised functions will support interleaved data moving forward.
		   * Until depthconv is re-worked to make it work with interleaved data format,
		   * this is a temporary fix to maintain compatibility with the new optimised
		   * and reference TFLM functions.
		   *
		   * The following code interleaves the input data, in case the previous layer
		   * is an optimised depthconv layer (which gives non-interleaved output data),
		   * which is the case in micro_speech example application.
		   */

		  //Check if input dimensions are within the buffer range to perform a data interleaving operation, if not fall back to reference TFLM code
		  if((input_width2 * input_width1 * input_channels) <= BUFFER_LENGTH){

			  const int16_t *pInTemp = tflite::micro::GetTensorData<int16_t>(input);
			  for(int nCh = 0;nCh < input_channels;nCh++)
			  {
				  for(int nCol = 0;nCol < input_width2;nCol++)
				  {
					  for(int nRow = 0;nRow < input_width1;nRow++)
					  {
						  pInputBuffer_16bit[nCol * input_width1 * input_channels + nRow * input_channels + nCh] =
								  pInTemp[nCh*input_width2*input_width1 + nCol*input_width1 + nRow];//

					  }
				  }
			  }
		  }
		  else{
	          tflite::reference_integer_ops::FullyConnected(
	              FullyConnectedParamsQuantized(data),
	              tflite::micro::GetTensorShape(input),
	              tflite::micro::GetTensorData<int16_t>(input),
	              tflite::micro::GetTensorShape(filter),
	              tflite::micro::GetTensorData<int8_t>(filter),
	              tflite::micro::GetTensorShape(bias),
	              tflite::micro::GetOptionalTensorData<int64_t>(bias),
	              tflite::micro::GetTensorShape(output),
	              tflite::micro::GetTensorData<int16_t>(output));
			  break;
		  }
#endif
          FullyConnectedParams params_read = FullyConnectedParamsQuantized(data);
          RuntimeShape filter_shape = tflite::micro::GetTensorShape(filter);
          RuntimeShape output_shape = tflite::micro::GetTensorShape(output);
          fullyConnected_asm_16bit(
#ifdef USE_OPTIMIZED_DEPTHCONV
		    pInputBuffer_16bit,
#else
        	tflite::micro::GetTensorData<int16_t>(input),
#endif
			tflite::micro::GetTensorData<int8_t>(filter),
			tflite::micro::GetOptionalTensorData<int32_t>(bias),
			tflite::micro::GetTensorData<int16_t>(output),
			filter_shape.Dims(filter_shape.DimensionsCount()-1),					//filter_shape.Dims(filter_dim_count - 1),;
			output_shape.Dims(output_shape.DimensionsCount() - 1),
			FlatSizeSkipDim(output_shape, output_shape.DimensionsCount() - 1),//batches
			params_read.output_multiplier,
			params_read.output_shift,
			params_read.input_offset,
			params_read.weights_offset,
			params_read.output_offset,
			params_read.quantized_activation_min,
			params_read.quantized_activation_max);
#endif
          break;
        }
        default: {
          MicroPrintf("Filter type %s (%d) not supported.",
                      TfLiteTypeGetName(filter->type), input->type);
          return kTfLiteError;
        }
      }
      break;
    }

    default: {
      MicroPrintf("Input type %s (%d) not supported.",
                  TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
    }
  }
  return kTfLiteOk;
}

}  // namespace

TfLiteRegistration_V1 Register_FULLY_CONNECTED() {
  return tflite::micro::RegisterOp(Init, Prepare, Eval);
}

}  // namespace tflite
