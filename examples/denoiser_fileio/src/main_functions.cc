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

#include "main_functions.h"

//Enable this macro to run inference in hybrid int16 activation, int8 weights mode
//#define DO_QUANTIZED_INFERENCE

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#ifndef DO_QUANTIZED_INFERENCE
#include "model_fp/model_float_1_data.h"
#include "model_fp/model_float_2_data.h"
#else
#include "model_int16/model_int16_1_data.h"
#include "model_int16/model_int16_2_data.h"
#endif//DO_QUANTIZED_INFERENCE
#include "frame_provider.h"

#include <window.h>
#include <filter.h>
#include <complex.h>
#include <fft.h>
#include <vector.h>

//#define DISPLAY_CYCLE_COUNTS
#ifdef DISPLAY_CYCLE_COUNTS   /* Enable the macros */
#define DO_CYCLE_COUNTS       //Needed internally
#include "sharc/cycle_count.h" /* Define the macros */
#endif

#ifdef DO_QUANTIZED_INFERENCE
//int16 scales are fixed per model
#define STATE_SCALE1 0.00909733772277832f
#define STATE_ZERO_PT1 0
#define INPUT_SCALE1 0.002145789097994566f
#define INPUT_ZERO_PT1 0

#define OUTPUT_SCALE1   3.0517578125e-05f
#define OUTPUT_ZERO_PT1 0
#define OUTPUT_STATE_SCALE1  0.009021474979817867f
#define OUTPUT_STATE_ZERO_PT1 0

#define STATE_SCALE2  0.0050125643610954285f
#define STATE_ZERO_PT2 0
#define INPUT_SCALE2 1.6400847016484477e-05f
#define INPUT_ZERO_PT2 0

#define OUTPUT_SCALE2  7.149199518607929e-06f
#define OUTPUT_ZERO_PT2 0
#define OUTPUT_STATE_SCALE2  0.005027586594223976f
#define OUTPUT_STATE_ZERO_PT2 0
#endif

//these pointers keep track of the read and write buffer status in the circular pointers
int pReadPtr;
int pProcessReadPtr;
int pWritePtr;
int pProcessWritePtr;


//TFLM messes up order during conversion so keeping a track of the indices
#define STATE_INPUT 0
#define MAG_INPUT 1
#define STATE_OUTPUT 0
#define MAG_OUTPUT 1

extern "C" {
	int generate_fft_twiddle_table (
		int fft_size,         /**< [in] table size in complex elements */
		float *p_twiddles);    /**< [out] table output.  Size in floats is 2 * 3 * fft_size / 4 */
}
// Globals, used for compatibility with Arduino-style sketches.
namespace {
const tflite::Model* model1 = nullptr;
const tflite::Model* model2 = nullptr;
tflite::MicroInterpreter* interpreter1 = nullptr;
tflite::MicroInterpreter* interpreter2 = nullptr;
TfLiteTensor* input1 = nullptr;
TfLiteTensor* input2 = nullptr;
TfLiteTensor* state1 = nullptr;
TfLiteTensor* state2 = nullptr;

// An area of memory to use for input, output, and intermediate arrays.
constexpr int kTensorArenaSize = (72 + 10) * 1024;
static uint8_t tensor_arena[kTensorArenaSize]__attribute__((section(".L1.data"), aligned(16)));

//For audio file read
float g_audio_data_input[NUM_HOPS*HOP_SIZE];//interface buffer to get data in
float g_process_input[FFT_SIZE];//this holds the current input data
float g_audio_state1_input[FFT_SIZE];//this retains the state for model 1
float g_audio_state2_input[FFT_SIZE];//this retains the state for model 2

#ifdef DO_QUANTIZED_INFERENCE
//temporary buffers used for converting int <-> float
float g_temp_inout[FFT_SIZE];
#endif

complex_float g_audio_complex_input[FFT_SIZE];//holds fft output of input data
float g_process_output[FFT_SIZE];//current output
float g_audio_data_output[NUM_HOPS*HOP_SIZE];//interface buffer to pass data out
complex_float twiddle_table[FFT_SIZE];//persistant twiddle tables used for fft
float g_previous_frame_out[FFT_SIZE];//store previous frame output
}  // namespace

void setup() {
  tflite::InitializeTarget();

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
#ifndef DO_QUANTIZED_INFERENCE
  model1 = tflite::GetModel(g_no_norm_model_float_1_model_data);
  model2 = tflite::GetModel(g_no_norm_model_float_2_model_data);
#else
  model1 = tflite::GetModel(g_no_norm_model_int16_1_model_data);
  model2 = tflite::GetModel(g_no_norm_model_int16_2_model_data);
#endif

  if (model1->version() != TFLITE_SCHEMA_VERSION) {
    MicroPrintf(
        "Model provided is schema version %d not equal "
        "to supported version %d.",
		model1->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  if (model2->version() != TFLITE_SCHEMA_VERSION) {
    MicroPrintf(
        "Model provided is schema version %d not equal "
        "to supported version %d.",
		model2->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  // Pull in only the operation implementations we need.
  // This relies on a complete list of all the ops needed by this graph.

  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroMutableOpResolver<23> micro_op_resolver;
  micro_op_resolver.AddConv2D();
  micro_op_resolver.AddUnidirectionalSequenceLSTM();
  micro_op_resolver.AddRelu();
  micro_op_resolver.AddLogistic();
  micro_op_resolver.AddSqueeze();
  micro_op_resolver.AddTranspose();
  micro_op_resolver.AddReshape();
  micro_op_resolver.AddUnpack();
  micro_op_resolver.AddFullyConnected();
  micro_op_resolver.AddSplit();
  micro_op_resolver.AddAdd();
  micro_op_resolver.AddMul();
  micro_op_resolver.AddTanh();
  micro_op_resolver.AddStridedSlice();

  micro_op_resolver.AddPack();
  micro_op_resolver.AddMean();
  micro_op_resolver.AddSquaredDifference();
  micro_op_resolver.AddSub();
  micro_op_resolver.AddRsqrt();
  micro_op_resolver.AddPad();

  micro_op_resolver.AddLog();
  micro_op_resolver.AddDequantize();
  micro_op_resolver.AddQuantize();

  // Build an interpreter to run the model with.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroInterpreter static_interpreter1(
      model1, micro_op_resolver, tensor_arena, kTensorArenaSize);
  interpreter1 = &static_interpreter1;

  static tflite::MicroInterpreter static_interpreter2(
      model2, micro_op_resolver, tensor_arena, kTensorArenaSize);
  interpreter2 = &static_interpreter2;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter1->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    MicroPrintf("AllocateTensors() failed");
    return;
  }

  allocate_status = interpreter2->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    MicroPrintf("AllocateTensors() failed");
    return;
  }

  // Get information about the memory area to use for the model's input.
  input1 = interpreter1->input(MAG_INPUT);
  state1 = interpreter1->input(STATE_INPUT);
  input2 = interpreter2->input(0);
  state2 = interpreter2->input(1);

  //Open audio input/output file
  if (kTfLiteOk != OpenInputFile()) {
	MicroPrintf("Failed to read the input file.");
	exit(0);
  }

  if (kTfLiteOk != OpenOutputFile()) {
	MicroPrintf("Failed to open output file in write mode.");
	exit(0);
  }

  /* Initialize the twiddle table */
  generate_fft_twiddle_table(FFT_SIZE,(float *)twiddle_table);

  memset(g_process_input,0,FFT_SIZE*sizeof(float));
  //set initial states to 0
  memset(g_audio_state1_input,0,FFT_SIZE*sizeof(float));
  memset(g_audio_state2_input,0,FFT_SIZE*sizeof(float));
  //set previous frame output to 0
  memset(g_previous_frame_out,0,(FFT_SIZE)*sizeof(float));

  //set read and write pointers
  pReadPtr = 0;
  pWritePtr = 0;
  pProcessReadPtr= 0;
  pProcessWritePtr = 0;
}

bool loop() {
  TfLiteStatus status = kTfLiteOk;
  // Get audio frame from provider.
  status = GetFrame(g_audio_data_input,g_previous_frame_out,&pReadPtr);
  if (status == kTfLiteCancelled) {
	MicroPrintf("Cancellation triggered! End of File reached!");
  }
  else if (status == kTfLiteError) {
	MicroPrintf("Audio capture failed.");
	return false;
  }
  else if (status == kTfLiteOk) {
	MicroPrintf("Audio capture passed, next frame being processed!");
  }

#ifdef DO_CYCLE_COUNTS
  long int pre_var = 0, pre_cyc=0; //Variables for cycle counting
  START_CYCLE_COUNT (pre_var);
#endif

  //check if there is new data to process
  if(pProcessWritePtr >= pReadPtr) {
	  return true;
  }
  //slide previous samples out and read new samples
  memcpy(g_process_input,g_process_input+HOP_SIZE,(FRAME_SIZE-HOP_SIZE)*sizeof(float));
  //read new samples
  int nProcLocation = (pProcessReadPtr) % NUM_HOPS;//circular buffer loopback to beginning
  memcpy(g_process_input+(FRAME_SIZE-HOP_SIZE),g_audio_data_input+nProcLocation*HOP_SIZE,HOP_SIZE*sizeof(float));
  pProcessReadPtr += 1;//updated process buffer location

  /* Calculate the FFT of a real signal */
  rfft ((const float32_t *)g_process_input,(complex_float *)g_audio_complex_input,(const complex_float *)twiddle_table,1,FFT_SIZE);

#ifdef DO_QUANTIZED_INFERENCE
  //convert complex to magnitude
  fft_magnitude( (complex_float *)g_audio_complex_input, g_temp_inout,FFT_SIZE,1);

  //Remove normalized
  vecsmltf(g_temp_inout,FFT_SIZE/2+1,g_temp_inout,FFT_SIZE/2+1);
  //note: we are using a g_temp_input instead of input buffer because of size variations of float vs int16

  //state from previous iteration is passed to next iteration
#else
  //convert complex to magnitude
  fft_magnitude( (complex_float *)g_audio_complex_input, input1->data.f,FFT_SIZE,1);

  //Remove normalized effect of fft_magnitude
  vecsmltf(input1->data.f,FFT_SIZE/2+1,input1->data.f,FFT_SIZE/2+1);

  //set input state 1 frm global buffer
  //state from previous iteration is passed to next iteration
  memcpy(state1->data.f,g_audio_state1_input,FFT_SIZE*sizeof(float));//set input state
#endif

#ifdef DO_QUANTIZED_INFERENCE
  //Quantize inputs, convert float to int16 or int8
	for(int i = 0;i < FFT_SIZE/2+1;i++) {
		input1->data.i16[i] = (int16)(g_temp_inout[i]/INPUT_SCALE1 + INPUT_ZERO_PT1);
	}
	for(int i = 0;i < FFT_SIZE;i++) {
		state1->data.i16[i] = (int8)(g_audio_state1_input[i]/STATE_SCALE1 + STATE_ZERO_PT1);
	}
#endif

#ifdef DO_CYCLE_COUNTS
	STOP_CYCLE_COUNT (pre_cyc, pre_var);
#endif

#ifdef DO_CYCLE_COUNTS
    long int var = 0, cyc=0; //Variables for cycle counting
	START_CYCLE_COUNT (var);
#endif

  // Run the model on this input and make sure it succeeds.
  if (kTfLiteOk != interpreter1->Invoke()) {
    MicroPrintf("Invoke failed.");
    return false;
  }

#ifdef DO_CYCLE_COUNTS
	STOP_CYCLE_COUNT (cyc, var);
#endif

#ifdef DO_CYCLE_COUNTS
  START_CYCLE_COUNT (pre_var);
#endif

  TfLiteTensor* output1 = interpreter1->output(0);//mask
  TfLiteTensor* output2 = interpreter1->output(1);//state

#ifdef DO_QUANTIZED_INFERENCE
  //Dequantize outputs and convert mask to float
  for(int i = 0;i < FFT_SIZE/2+1;i++) {
	  g_temp_inout[i] = (float)(OUTPUT_SCALE1 * ((float)output1->data.i16[i] - OUTPUT_ZERO_PT1));
  }
  for(int i = 0;i < FFT_SIZE;i++) {
	  //copy output state to store for next iteration
	  g_audio_state1_input[i] = (float)(OUTPUT_STATE_SCALE1 * ((float)output2->data.i16[i] - OUTPUT_STATE_ZERO_PT1));
  }

  //apply mask to input, multiply complex input with nn output for denoising
  for(int i = 0;i < FFT_SIZE/2+1;i++){
	  g_audio_complex_input[i] = g_audio_complex_input[i] * g_temp_inout[i];
  }
#else
  memcpy(g_audio_state1_input,output2->data.f,FFT_SIZE*sizeof(float));//copy output state to store for next iteration

    //apply mask to input, multiply complex input with nn output
    for(int i = 0;i < FFT_SIZE/2+1;i++){
  	  g_audio_complex_input[i] = g_audio_complex_input[i] * output1->data.f[i];
    }
#endif

#ifdef DO_QUANTIZED_INFERENCE
   rifft ((const complex_float *)g_audio_complex_input,(float32_t *)g_temp_inout,
		  (const complex_float *)twiddle_table,1,FFT_SIZE);

  //Quantize inputs, convert float to int16 or int8
   //set input state 2 from global buffer
   	//state from previous iteration is passed to next iteration
  for(int i = 0;i < FFT_SIZE;i++) {
	input2->data.i16[i] = (int16)(g_temp_inout[i]/INPUT_SCALE2 + INPUT_ZERO_PT2);
	state2->data.i16[i] = (int16)(g_audio_state2_input[i]/STATE_SCALE2 + STATE_ZERO_PT2);
  }
#else
  rifft ((const complex_float *)g_audio_complex_input,(float32_t *)input2->data.f,
		  (const complex_float *)twiddle_table,1,FFT_SIZE);

  memcpy(state2->data.f,g_audio_state2_input,FFT_SIZE*sizeof(float));//set input state from previous it
#endif

#ifdef DO_CYCLE_COUNTS
	STOP_CYCLE_COUNT (pre_cyc, pre_var);
#endif

#ifdef DO_CYCLE_COUNTS
	START_CYCLE_COUNT (var);
#endif

  // Run the model on this input and make sure it succeeds.
  if (kTfLiteOk != interpreter2->Invoke()) {
    MicroPrintf("Invoke failed.");
    return false;
  }

#ifdef DO_CYCLE_COUNTS
	STOP_CYCLE_COUNT (cyc, var);
#endif

#ifdef DO_CYCLE_COUNTS
  START_CYCLE_COUNT (pre_var);
#endif

  TfLiteTensor* output3 = interpreter2->output(MAG_OUTPUT);//audio out
  TfLiteTensor* output4 = interpreter2->output(STATE_OUTPUT);//state

#ifdef DO_QUANTIZED_INFERENCE
  //Dequantize outputs and convert output to float
  for(int i = 0;i < FFT_SIZE;i++) {
	g_process_output[i] = (float)(OUTPUT_SCALE2 * ((float)output3->data.i16[i] - OUTPUT_ZERO_PT2));
	g_audio_state2_input[i] = (float)(OUTPUT_STATE_SCALE2 * ((float)output4->data.i16[i] - OUTPUT_STATE_ZERO_PT2));
  }
#else
  memcpy(g_audio_state2_input,output4->data.f,FFT_SIZE*sizeof(float));//update state for next it
  memcpy(g_process_output,output3->data.f,FFT_SIZE*sizeof(float));//update output
#endif

  //overlap and add
  //next frames, add and overlap
  for(int i = 0;i < FRAME_SIZE;i++) {
	  g_previous_frame_out[i] += g_process_output[i];
  }

  //copy this to write buffer to next empty location
  nProcLocation = (pProcessWritePtr) % NUM_HOPS;//circular buffer loopback to beginning
  memcpy(g_audio_data_output+nProcLocation*HOP_SIZE , g_previous_frame_out, HOP_SIZE*sizeof(float));//copy output
  pProcessWritePtr++;

  //shift out frame discarding outputted signals
  memcpy(g_previous_frame_out,g_previous_frame_out+HOP_SIZE,(FRAME_SIZE-HOP_SIZE)*sizeof(float));
  //add new samples at the end
  memset(g_previous_frame_out+(FRAME_SIZE-HOP_SIZE),0,(HOP_SIZE)*sizeof(float));

#ifdef DO_CYCLE_COUNTS
	STOP_CYCLE_COUNT (pre_cyc, pre_var);
#endif

  //writetofile with overlap
  if (kTfLiteOk != WriteFrame(g_audio_data_output,&pWritePtr,pProcessWritePtr)) {
	MicroPrintf("Image write failed.");
  return false;
  }

#ifdef DO_CYCLE_COUNTS
  printf("\tNumber of cycles to run preprocessing+postprocessing: \t%ld \n", pre_cyc);
  printf("\tNumber of cycles to run inference: \t%ld \n", cyc);
  printf("\n");
#endif
  return true;
}
