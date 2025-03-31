/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

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

#include "tensorflow/lite/micro/kernels/conv.h"

#include "tensorflow/lite/c/builtin_op_data.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/kernels/internal/portable_tensor_utils.h"
#include "tensorflow/lite/kernels/internal/reference/conv.h"
#include "tensorflow/lite/kernels/internal/reference/integer_ops/conv.h"
#include "tensorflow/lite/kernels/kernel_util.h"
#include "tensorflow/lite/micro/kernels/kernel_util.h"
#include "tensorflow/lite/micro/micro_log.h"

#include "adi_sharcfx_nn.h"

namespace tflite {
namespace {

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(OpDataConv));
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kConvInputTensor);
  const TfLiteEvalTensor* filter =
      tflite::micro::GetEvalInput(context, node, kConvWeightsTensor);
  const TfLiteEvalTensor* bias =
      (NumInputs(node) == 3)
          ? tflite::micro::GetEvalInput(context, node, kConvBiasTensor)
          : nullptr;
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kConvOutputTensor);

  TFLITE_DCHECK(node->builtin_data != nullptr);
  const auto& params =
      *(reinterpret_cast<TfLiteConvParams*>(node->builtin_data));
  TFLITE_DCHECK(node->user_data != nullptr);
  const auto& data = *(static_cast<const OpDataConv*>(node->user_data));

  TF_LITE_ENSURE_EQ(context, input->type, output->type);
  TF_LITE_ENSURE_MSG(
      context,
      input->type == filter->type ||
          (input->type == kTfLiteInt16 && filter->type == kTfLiteInt8) ||
          (input->type == kTfLiteInt8 && filter->type == kTfLiteInt4),
      "Hybrid models are not supported on TFLite Micro.");

  switch (input->type) {  // Already know in/out types are same.
    case kTfLiteFloat32: {
      tflite::reference_ops::Conv(
          ConvParamsFloat(params, data), tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<float>(input),
          tflite::micro::GetTensorShape(filter),
          tflite::micro::GetTensorData<float>(filter),
          tflite::micro::GetTensorShape(bias),
          tflite::micro::GetOptionalTensorData<float>(bias),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<float>(output),
          tflite::micro::GetTensorShape(nullptr), nullptr);
      break;
    }
    case kTfLiteInt16: {
      switch (bias->type) {
        case kTfLiteInt32: {
          reference_integer_ops::ConvPerChannel(
              ConvParamsQuantized(params, data),
              data.per_channel_output_multiplier, data.per_channel_output_shift,
              tflite::micro::GetTensorShape(input),
              tflite::micro::GetTensorData<int16_t>(input),
              tflite::micro::GetTensorShape(filter),
              tflite::micro::GetTensorData<int8_t>(filter),
              tflite::micro::GetTensorShape(bias),
              tflite::micro::GetOptionalTensorData<std::int32_t>(bias),
              tflite::micro::GetTensorShape(output),
              tflite::micro::GetTensorData<int16_t>(output));
          break;
        }
        case kTfLiteInt64: {
          reference_integer_ops::ConvPerChannel(
              ConvParamsQuantized(params, data),
              data.per_channel_output_multiplier, data.per_channel_output_shift,
              tflite::micro::GetTensorShape(input),
              tflite::micro::GetTensorData<int16_t>(input),
              tflite::micro::GetTensorShape(filter),
              tflite::micro::GetTensorData<int8_t>(filter),
              tflite::micro::GetTensorShape(bias),
              tflite::micro::GetOptionalTensorData<std::int64_t>(bias),
              tflite::micro::GetTensorShape(output),
              tflite::micro::GetTensorData<int16_t>(output));
          break;
        }
        default:
          MicroPrintf("Bias type %s (%d) not supported.",
                      TfLiteTypeGetName(bias->type), bias->type);
          return kTfLiteError;
      }
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
          reference_integer_ops::ConvPerChannel(
              ConvParamsQuantized(params, data),
              data.per_channel_output_multiplier, data.per_channel_output_shift,
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
        	cycle_t var = 0, cyc=0; //Variables for cycle counting
			START_CYCLE_COUNT (var);
#endif
#if defined(USE_OPTIMIZED_1x1_CONV) || (USE_OPTIMIZED_3x3_CONV)
		  ConvParams params_read = ConvParamsQuantized(params, data);
		  RuntimeShape input_shape = tflite::micro::GetTensorShape(input);
		  const int input_width = input_shape.Dims(2);
		  const int input_height = input_shape.Dims(1);
		  const int input_depth = input_shape.Dims(3);

		  RuntimeShape filter_shape = tflite::micro::GetTensorShape(filter);
		  RuntimeShape output_shape = tflite::micro::GetTensorShape(output);
		  const int output_height = output_shape.Dims(1);
		  const int output_width = output_shape.Dims(2);
		  const int output_depth = MatchingDim(filter_shape, 0, output_shape, 3);
		  const int filter_input_depth = filter_shape.Dims(3);
		  const int stride_width = params_read.stride_width;
		  const int stride_height = params_read.stride_height;
		  const int pad_height = params_read.padding_values.height;
		  const int pad_width = params_read.padding_values.width;

		  const int32_t output_activation_min = params_read.quantized_activation_min;
		  const int32_t output_activation_max = params_read.quantized_activation_max;

		  const int dilation_width_factor = params_read.dilation_width_factor;
		  const int dilation_height_factor = params_read.dilation_height_factor;
		  const int filter_height = filter_shape.Dims(1);
		  const int filter_width = filter_shape.Dims(2);
		  const int nBatchSize = MatchingDim(input_shape, 0, output_shape, 0);

		  int32_t nTotalPadWidth, nTotalPadHeight;
		  nTotalPadWidth = (output_width-1)*stride_width + filter_width - input_width;
		  nTotalPadHeight = (output_height-1)*stride_height + filter_height - input_height;
		  uint32_t BufSizeNeeded = (output_depth*filter_height*filter_width*input_depth)*2+(input_depth*(input_width+nTotalPadWidth)*(input_height+nTotalPadHeight))+100;

	  if(dilation_width_factor==1 && dilation_height_factor==1 && BufSizeNeeded<=TEMP_BUFFER_SIZE)
	  {
		  adi_sharcfx_conv2d_dilation1x1_int8(
				  tflite::micro::GetTensorData<int8_t>(input),			//Input buffer
				  tflite::micro::GetTensorData<int8_t>(filter),			//Weights buffer
				  tflite::micro::GetOptionalTensorData<int32_t>(bias),	//Bias
				  tflite::micro::GetTensorData<int8_t>(output),			//Output buffer
				  (int32_t) nBatchSize,
				  (int32_t)input_depth,
				  (int32_t)output_depth,
				  (int32_t)filter_height,											//Kernel size width
				  (int32_t)filter_width,										//Kernel size height
				  (int32_t)MatchingDim(filter_shape, 0, output_shape, 3),									//Number of filters
				  (int32_t)input_width,											//Width of input buffer
				  (int32_t) input_height,											//Height of input buffer
				  (int32_t)stride_height,
				  (int32_t) stride_width,
				  (int32_t) pad_height,                                            //total padding for width
				  (int32_t) pad_width,								            //Total padding for height
				  (int32_t)output_height,
				  (int32_t)output_width,
				  (int32_t*)data.per_channel_output_multiplier,
				  (int32_t*)data.per_channel_output_shift,
				  (int32_t)params_read.input_offset,
				  (int32_t)params_read.output_offset,
				  (int32_t)params_read.weights_offset,
				  (int32_t) output_activation_min,                                //activation min
				  (int32_t) output_activation_max);
      }
	  else  if(filter_height == 1 && filter_width == 1){
#ifndef USE_NON_INTERLEAVED_1x1_CONV
		  adi_sharcfx_conv2d_kernel1x1_int8(tflite::micro::GetTensorData<int8_t>(input),
					  	  	  	  	  	  	  tflite::micro::GetTensorData<int8_t>(filter),
											  tflite::micro::GetOptionalTensorData<int32_t>(bias),
            		  	  	  	  	  	  	  tflite::micro::GetTensorData<int8_t>(output),
											  nBatchSize,
											  filter_input_depth,
											  output_depth,
											  input_width*input_height,
											  data.per_channel_output_multiplier,
											  data.per_channel_output_shift,
											  params_read.input_offset,
											  params_read.output_offset);
#else
		  adi_sharcfx_conv2d_kernel1x1_noninterleaved_int8(tflite::micro::GetTensorData<int8_t>(input),
            		                  tflite::micro::GetTensorData<int8_t>(output),
    								  tflite::micro::GetTensorData<int8_t>(filter),
    								  tflite::micro::GetOptionalTensorData<int32_t>(bias),
    								  input_width,
    								  filter_input_depth,
    								  output_depth,
    								  (uint32_t *)data.per_channel_output_multiplier,
    								  data.per_channel_output_shift,
    								  params_read.input_offset,
    								  (uint32_t)params_read.output_offset);
#endif
          }else if(filter_height == 3 && filter_width == 3){
        	  if(params.stride_height == 1 && params.stride_width == 1 && params.padding == kTfLitePaddingSame){
        		  adi_sharcfx_conv2d_kernel3x3_stride1_same_pad_int8(tflite::micro::GetTensorData<int8_t>(input),
									  tflite::micro::GetTensorData<int8_t>(filter),
									  tflite::micro::GetOptionalTensorData<int32_t>(bias),
									  tflite::micro::GetTensorData<int8_t>(output),
									  (int32_t)filter_input_depth,
									  (int32_t)output_depth,
									  (int32_t)input_width,
									  (int32_t)input_height,
									  (int32_t)output_depth,
									  (int32_t*)data.per_channel_output_multiplier,
									  (int32_t*)data.per_channel_output_shift,
									  (int32_t)params_read.input_offset,
									  (int32_t)params_read.output_offset);
        	  } else if(params.stride_height == 1 && params.stride_width == 1 && params.padding == kTfLitePaddingValid) {
        		  adi_sharcfx_conv2d_kernel3x3_stride1_valid_pad_int8(tflite::micro::GetTensorData<int8_t>(input),
									  tflite::micro::GetTensorData<int8_t>(filter),
									  tflite::micro::GetOptionalTensorData<int32_t>(bias),
									  tflite::micro::GetTensorData<int8_t>(output),
									  (int32_t)filter_input_depth,
									  (int32_t)output_depth,
									  (int32_t)input_width,
									  (int32_t)input_height,
									  (int32_t)output_depth,
									  (int32_t*)data.per_channel_output_multiplier,
									  (int32_t*)data.per_channel_output_shift,
									  (int32_t)params_read.input_offset,
									  (int32_t)params_read.output_offset);
        	  } else if(params.stride_height == 2 && params.stride_width == 2 && params.padding == kTfLitePaddingValid){
        		  adi_sharcfx_conv2d_kernel3x3_stride2_valid_pad_int8(tflite::micro::GetTensorData<int8_t>(input),
									  tflite::micro::GetTensorData<int8_t>(filter),
									  tflite::micro::GetOptionalTensorData<int32_t>(bias),
									  tflite::micro::GetTensorData<int8_t>(output),
									  (int32_t)filter_input_depth,
									  (int32_t)output_depth,
									  (int32_t)input_width,
									  (int32_t)input_height,
									  (int32_t)output_depth,
									  (int32_t*)data.per_channel_output_multiplier,
									  (int32_t*)data.per_channel_output_shift,
									  (int32_t)params_read.input_offset,
									  (int32_t)params_read.output_offset
						  );
			  } else {
	        	  reference_integer_ops::ConvPerChannel(
	        	  				  ConvParamsQuantized(params, data),
								  data.per_channel_output_multiplier, data.per_channel_output_shift,
	        	  				  tflite::micro::GetTensorShape(input),
	        	  				  tflite::micro::GetTensorData<int8_t>(input),
	        	  				  tflite::micro::GetTensorShape(filter),
	        	  				  tflite::micro::GetTensorData<int8_t>(filter),
	        	  				  tflite::micro::GetTensorShape(bias),
	        	  				  tflite::micro::GetOptionalTensorData<int32_t>(bias),
	        	  				  tflite::micro::GetTensorShape(output),
								  tflite::micro::GetTensorData<int8_t>(output));
			  }

          }
          else{
              reference_integer_ops::ConvPerChannel(
                  ConvParamsQuantized(params, data),
                  data.per_channel_output_multiplier, data.per_channel_output_shift,
                  tflite::micro::GetTensorShape(input),
                  tflite::micro::GetTensorData<int8_t>(input),
                  tflite::micro::GetTensorShape(filter),
                  tflite::micro::GetTensorData<int8_t>(filter),
                  tflite::micro::GetTensorShape(bias),
                  tflite::micro::GetOptionalTensorData<int32_t>(bias),
                  tflite::micro::GetTensorShape(output),
                  tflite::micro::GetTensorData<int8_t>(output));
          }
#else
	  reference_integer_ops::ConvPerChannel(
		  ConvParamsQuantized(params, data),
		  data.per_channel_output_multiplier, data.per_channel_output_shift,
		  tflite::micro::GetTensorShape(input),
		  tflite::micro::GetTensorData<int8_t>(input),
		  tflite::micro::GetTensorShape(filter),
		  tflite::micro::GetTensorData<int8_t>(filter),
		  tflite::micro::GetTensorShape(bias),
		  tflite::micro::GetOptionalTensorData<int32_t>(bias),
		  tflite::micro::GetTensorShape(output),
		  tflite::micro::GetTensorData<int8_t>(output)
	  );
#endif

#ifdef DISPLAY_CYCLE_COUNTS
		  STOP_CYCLE_COUNT (cyc, var);
		  PRINT_INFO("\tNumber of cycles to run Conv 1x1 Layer: \t %lu \n", cyc);
#endif
          break;
        }
        default:
          MicroPrintf("Weight type %s (%d) not supported.",
                      TfLiteTypeGetName(filter->type), filter->type);
          return kTfLiteError;
      }
      break;
    }
    default:
      MicroPrintf("Type %s (%d) not supported.", TfLiteTypeGetName(input->type),
                  input->type);
      return kTfLiteError;
  }
  return kTfLiteOk;
}

}  // namespace

TfLiteRegistration_V1 Register_CONV_2D() {
  return tflite::micro::RegisterOp(Init, ConvPrepare, Eval);
}

}  // namespace tflite
