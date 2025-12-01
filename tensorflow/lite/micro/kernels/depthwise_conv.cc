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

#include "tensorflow/lite/micro/kernels/depthwise_conv.h"

#include "tensorflow/lite/c/builtin_op_data.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/kernels/internal/portable_tensor_utils.h"
#include "tensorflow/lite/kernels/internal/reference/depthwiseconv_float.h"
#include "tensorflow/lite/kernels/internal/reference/integer_ops/depthwise_conv.h"
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
  TFLITE_DCHECK(node->user_data != nullptr);
  TFLITE_DCHECK(node->builtin_data != nullptr);

  auto& params =
      *(reinterpret_cast<TfLiteDepthwiseConvParams*>(node->builtin_data));
  const OpDataConv& data = *(static_cast<const OpDataConv*>(node->user_data));

  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kDepthwiseConvOutputTensor);
  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kDepthwiseConvInputTensor);
  const TfLiteEvalTensor* filter =
      tflite::micro::GetEvalInput(context, node, kDepthwiseConvWeightsTensor);
  const TfLiteEvalTensor* bias =
      (NumInputs(node) == 3)
          ? tflite::micro::GetEvalInput(context, node, kDepthwiseConvBiasTensor)
          : nullptr;

  switch (input->type) {  // Already know in/out types are same.
    case kTfLiteFloat32: {
      tflite::reference_ops::DepthwiseConv(
          DepthwiseConvParamsFloat(params, data),
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
          reference_integer_ops::DepthwiseConvPerChannel(
              DepthwiseConvParamsQuantized(params, data),
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
#ifndef USE_OPTIMIZED_DEPTHCONV
          reference_integer_ops::DepthwiseConvPerChannel(
              DepthwiseConvParamsQuantized(params, data),
              data.per_channel_output_multiplier, data.per_channel_output_shift,
              tflite::micro::GetTensorShape(input),
              tflite::micro::GetTensorData<int8_t>(input),
              tflite::micro::GetTensorShape(filter),
              tflite::micro::GetTensorData<int8_t>(filter),
              tflite::micro::GetTensorShape(bias),
              tflite::micro::GetOptionalTensorData<int32_t>(bias),
              tflite::micro::GetTensorShape(output),
              tflite::micro::GetTensorData<int8_t>(output));
#else
          //TODO:assuming that input is now non-interleaved
          DepthwiseParams params_read = DepthwiseConvParamsQuantized(params, data);
          RuntimeShape input_shape = tflite::micro::GetTensorShape(input);
          RuntimeShape filter_shape = tflite::micro::GetTensorShape(filter);
		  RuntimeShape output_shape = tflite::micro::GetTensorShape(output);

          const int input_depth = input_shape.Dims(3);
          const int input_height = input_shape.Dims(1);
          const int input_width = input_shape.Dims(2);

          const int output_depth = output_shape.Dims(3);
          const int output_height = output_shape.Dims(1);
          const int output_width = output_shape.Dims(2);

          const int filter_depth = filter_shape.Dims(3);
          const int filter_height = filter_shape.Dims(1);
          const int filter_width = filter_shape.Dims(2);

          const int stride_width = params_read.stride_width;
          const int stride_height = params_read.stride_height;

          const int pad_height = params_read.padding_values.height;
          const int pad_width = params_read.padding_values.width;
          const int depth_multiplier = params_read.depth_multiplier;

          const int32_t output_activation_min = params_read.quantized_activation_min;
          const int32_t output_activation_max = params_read.quantized_activation_max;

          const int dilation_width_factor = params_read.dilation_width_factor;
          const int dilation_height_factor = params_read.dilation_height_factor;


#ifndef USE_NON_INTERLEAVED_DEPTHCONV
          //temp_buffer is used as scratch for padding so check if this fits before calling the routine
          if(output_depth * output_height * output_width < TEMP_BUFFER_SIZE_L3 &&
        		  dilation_width_factor == 1 && dilation_height_factor == 1) {
			  //Use this routine if data is in interleaved format similar to default TFLM format
			  //using formula output_size = (Size-FilterSize + 2*padding)/stride + 1
			  int nPadWidth = (output_width - 1) * stride_width - input_width + filter_width;
			  int nPadHeight = (output_height - 1) * stride_height - input_height + filter_height;
			  adi_sharcfx_depthconv2d_int8(
							  tflite::micro::GetTensorData<int8_t>(input),			//Input buffer
							  tflite::micro::GetTensorData<int8_t>(output),			//Output buffer
							  tflite::micro::GetTensorData<int8_t>(filter),			//Weights buffer
							  tflite::micro::GetOptionalTensorData<int32_t>(bias),	//Bias
							  input_width,											//Width of input buffer
							  input_height,											//Height of input buffer
							  depth_multiplier,									    //Depth mult
							  input_depth,											//In Channels
							  output_depth,											//Out Channels
							  filter_width,											//Kernel size width
							  filter_height,										//Kernel size height
							  nPadWidth,                                            //total padding for width
							  nPadHeight,								            //Total padding for height
							  data.per_channel_output_multiplier,		            //Quantized multiplier
							  data.per_channel_output_shift,						//Quantized scale
							  params_read.input_offset,								//Input Zero Point
							  params_read.output_offset,                            //Output Zero Point
							  stride_width,											//width stride
							  stride_height,                                        //height stride
							  output_activation_min,                                //activation min
							  output_activation_max);         						//activation max
          } else {
              reference_integer_ops::DepthwiseConvPerChannel(
                  DepthwiseConvParamsQuantized(params, data),
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
    	  //If data is in non-interleaved custom format use these routines
          if(stride_width == 1){
        	  adi_sharcfx_depthconv2d_stride1_noninterleaved_int8(
        		  tflite::micro::GetTensorData<int8_t>(input),			//Input buffer
				  tflite::micro::GetTensorData<int8_t>(output),			//Output buffer
				  tflite::micro::GetTensorData<int8_t>(filter),			//Weights buffer
				  tflite::micro::GetOptionalTensorData<int32_t>(bias),	//Bias
				  input_width,											//Width of input buffer
				  depth_multiplier,									    //Depth mult
				  filter_depth,											//In Channels
				  output_depth,											//Out Channels
				  filter_width,											//Kernel size
				  2,											        //Total padding
				  (uint32_t *)data.per_channel_output_multiplier,		//Quantized multiplier
				  data.per_channel_output_shift,						//Quantized scale
				  params_read.input_offset,								//Input Zero Point
				  (uint32_t)params_read.output_offset);         		//Output Zero Point
          } else if (stride_width ==2) {
        	  if(filter_height==10){
        		  adi_sharcfx_depthconv2d_stride2_kernel8x10_noninterleaved_int8(
						tflite::micro::GetTensorData<int8_t>(input), 			//const ADI_ACTIVATION_DATATYPE *pInputBuffer,			//Input buffer
						tflite::micro::GetTensorData<int8_t>(filter),			//Weights buffer
						tflite::micro::GetOptionalTensorData<int32_t>(bias),	//Bias
						tflite::micro::GetTensorData<int8_t>(output),			//ADI_ACTIVATION_DATATYPE *pOutputBuffer,					//Output buffer
						input_width,											//f input buffer
						input_height,						//Length of input buffer
						input_depth, 							//Depth of input buffer- In Channels
						output_width,						//Width of output buffer
						output_height,						//Length of output buffer
						output_depth,						//Depth of output buffer- Out Channels
						filter_width,						//Width of filter
						filter_height,					//Length of filter
						pad_width,					//Width of padding
						pad_height,				//Length of padding
						(uint32_t *)data.per_channel_output_multiplier,		//Quantized multiplier
						data.per_channel_output_shift,						//Quantized scale
						(int32_t)params_read.input_offset,								//Input Zero Point
						(int32_t)params_read.output_offset,         		//Output Zero Point
						params_read.quantized_activation_min,
						params_read.quantized_activation_max);
				  } else{
					  adi_sharcfx_depthconv2d_stride2_noninterleaved_int8(
						  tflite::micro::GetTensorData<int8_t>(input),			//Input buffer
						  tflite::micro::GetTensorData<int8_t>(output),			//Output buffer
						  tflite::micro::GetTensorData<int8_t>(filter),			//Weights buffer
						  tflite::micro::GetOptionalTensorData<int32_t>(bias),	//Bias
						  input_width,											//Width of input buffer
						  depth_multiplier,									    //Depth mult
						  filter_depth,									        //In Channels
						  output_depth,											//Out Channels
						  filter_width,											//Kernel size
						  1,											//Total padding
						  (uint32_t *)data.per_channel_output_multiplier,		//Quantized multiplier
						  data.per_channel_output_shift,						//Quantized scale
						  params_read.input_offset,								//Input Zero Point
						  (uint32_t)params_read.output_offset);         		//Output Zero Point
				  }
		  } else {
			  reference_integer_ops::DepthwiseConvPerChannel(
				  DepthwiseConvParamsQuantized(params, data),
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
#endif
#endif
#ifdef DISPLAY_CYCLE_COUNTS
		  STOP_CYCLE_COUNT (cyc, var);
		  PRINT_INFO("\tNumber of cycles to run Depthwise Layer : \t%lu \n", cyc);
#endif

          break;
        }
        default:
          MicroPrintf("Filter type %s (%d) not supported.",
                      TfLiteTypeGetName(filter->type), filter->type);
          return kTfLiteError;
      }
      break;
    }
    default:
      MicroPrintf("Input type %s (%d) not supported.",
                  TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
  }
  return kTfLiteOk;
}

}  // namespace

TfLiteRegistration_V1 Register_DEPTHWISE_CONV_2D() {
  return tflite::micro::RegisterOp(Init, DepthwiseConvPrepare, Eval);
}

}  // namespace tflite
