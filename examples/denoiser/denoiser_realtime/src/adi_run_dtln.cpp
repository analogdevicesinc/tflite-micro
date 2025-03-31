/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/
/*****************************************************************************
 * adi_run_dtln.cpp
 *****************************************************************************/

#include "adi_run_dtln.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "../../common/model/model_fp/model_float_1_data.h"
#include "../../common/model/model_fp/model_float_2_data.h"

#include <window.h>
#include <filter.h>
#include <complex.h>
#include <fft.h>
#include <vector.h>
#include <filter.h>

//#define DISPLAY_CYCLE_COUNTS
#ifdef DISPLAY_CYCLE_COUNTS   /* Enable the macros */
#define DO_CYCLE_COUNTS       //Needed internally
#define __PRE_FX_COMPATIBILITY
#include "cycle_count.h" /* Define the macros */
#endif

volatile int pReadPtr;
int pProcessReadPtr;
volatile int pWritePtr;
int pProcessWritePtr;

#define NUM_HOPS    10
#define HOP_SIZE   128
#define FFT_SIZE   512

//TFLM messes up order during conversion -
#define STATE_INPUT 0
#define MAG_INPUT 1
#define STATE_OUTPUT 0
#define MAG_OUTPUT 1

extern "C" {
	int generate_fft_twiddle_table (
		int fft_size,         /**< [in] table size in complex elements */
		float *p_twiddles);    /**< [out] table output.  Size in floats is 2 * 3 * fft_size / 4 */
}

//*2 extra for stereo, *3 extra for 48KHz decimated to 16KHz
int g_audio_data_input[NUM_HOPS*AUDIO_COUNT];//interface buffer to get data in
int g_audio_data_output[NUM_HOPS*AUDIO_COUNT];//interface buffer to pass data out
float g_process_input[FFT_SIZE];//this holds the current input data

fir_statef decimation_filter;
fir_statef interpolation_filter;

#define N_DECIMATION_COEFFS    72
float decimation_coefs[N_DECIMATION_COEFFS] = {0,-0.0000,-0.0001,0,0.0002,0.0003,0,-0.0005,-0.0007,0,0.0011,0.0014,0,-0.0022,-0.0028,
											   0,0.0040,0.0048,0,-0.0068,-0.0080,0,0.0111,0.0129,0,-0.0177,-0.0207,0,0.0287,0.0342,0,
											   -0.0513,-0.0659,0,0.1363,0.2749,0.3333,0.2749,0.1363,0,-0.0659,-0.0513,0,0.0342,0.0287,
											   0,-0.0207,-0.0177,0,0.0129,0.0111,0,-0.0080,-0.0068,0,0.0048,0.0040,0,-0.0028,-0.0022,
											   0,0.0014,0.0011,0,-0.0007,-0.0005,0,0.0003,0.0002,0,-0.0001,-0.0000};
float decimation_delay[N_DECIMATION_COEFFS + 1];

#define N_INTERPOLATION_COEFFS    72
float interpolation_coefs[N_INTERPOLATION_COEFFS] = {0,-0.0001,-0.0002,0,0.0006,0.0008,0,-0.0015,-0.0020,0,0.0034,0.0043,0,-0.0067,-0.0083,
													0,0.0121,0.0145,0,-0.0205,-0.0241,0,0.0332,0.0388,0,-0.0530,-0.0620,0,0.0861,0.1027,0,
													-0.1540,-0.1976,0,0.4088,0.8247,1.0000,0.8247,0.4088,0,-0.1976,-0.1540,0,0.1027,0.0861,0,
													-0.0620,-0.0530,0,0.0388,.0332,0,-0.0241,-0.0205,0,0.0145,0.0121,0,-0.0083,-0.0067,0,0.0043,
													0.0034,0,-0.0020,-0.0015,0,0.0008,0.0006,0,-0.0002,-0.0001};
float interpolation_delay[N_INTERPOLATION_COEFFS/3 + 1];
/* Coefficients in implementation order */
float dec_coeffs[N_DECIMATION_COEFFS];
float int_coeffs[N_INTERPOLATION_COEFFS];


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

float g_audio_state1_input[FFT_SIZE];//this retains the state for model 1
float g_audio_state2_input[FFT_SIZE];//this retains the state for model 2


complex_float g_audio_complex_input[FFT_SIZE];//holds fft output of input data
float g_process_output[FFT_SIZE];//current output
complex_float twiddle_table[FFT_SIZE];//persistant twiddle tables used for fft
float g_previous_frame_out[FFT_SIZE];//store previous frame output
float g_decimation_buffer[HOP_SIZE*3];//*3 because audio is sampled at 48KHz and then decimated to 16KHz. 16 * 3 --> 48
float g_interpolation_buffer[HOP_SIZE*3];
}  // namespace

void adi_dtln_model_setup() {
  tflite::InitializeTarget();
  #ifdef UART_REDIRECT
    set_print_func(uart_print);
  #else
    set_print_func(default_print);
  #endif
  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  model1 = tflite::GetModel(g_no_norm_model_float_1_model_data);
  model2 = tflite::GetModel(g_no_norm_model_float_2_model_data);


  if (model1->version() != TFLITE_SCHEMA_VERSION) {
	  PRINT_INFO(
        "Model provided is schema version %d not equal "
        "to supported version %d.",
		model1->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  if (model2->version() != TFLITE_SCHEMA_VERSION) {
	  PRINT_INFO(
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
    PRINT_INFO("AllocateTensors() failed");
    return;
  }

  allocate_status = interpreter2->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
	  PRINT_INFO("AllocateTensors() failed");
    return;
  }

  // Get information about the memory area to use for the model's input.
  input1 = interpreter1->input(MAG_INPUT);
  state1 = interpreter1->input(STATE_INPUT);
  input2 = interpreter2->input(0);
  state2 = interpreter2->input(1);

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

  for (int i = 0; i < (N_DECIMATION_COEFFS + 1); i++) {
	  decimation_delay[i] = 0.0F;
  }

  for (int i = 0; i < (N_INTERPOLATION_COEFFS + 1); i++) {
	  interpolation_delay[i] = 0.0F;
  }

  /* Transform the normal order coefficients from a filter design
     tool into coefficients for the fir_interp function */
  int j = N_INTERPOLATION_COEFFS;
  for (int np = 1; np <= 3; np++) {
      for (int nc = 1; nc <= (N_INTERPOLATION_COEFFS/3); nc++) {
    	  int_coeffs[--j] = interpolation_coefs[(nc * 3) - np];
      }
  }

  /* Transform the normal order coefficients from a filter design
       tool into coefficients for the fir_decimaf function */
  for (int np = 0; np < N_DECIMATION_COEFFS; np++) {
	  dec_coeffs[np] = decimation_coefs[N_DECIMATION_COEFFS - 1 - np];
  }

  //initialize fir decimation and interpolation filters
  fir_init( &decimation_filter, dec_coeffs, decimation_delay, N_DECIMATION_COEFFS, 3 );
  fir_init( &interpolation_filter, int_coeffs, interpolation_delay, N_INTERPOLATION_COEFFS/3, 3 );
}

bool adi_dtln_model_run() {

#ifdef DO_CYCLE_COUNTS
  cycle_t pre_var = 0, pre_cyc=0; //Variables for cycle counting
  START_CYCLE_COUNT (pre_var);
#endif

  //check if there is new data to process
  if(pProcessReadPtr >= pReadPtr) {
	  return true;
  }

  //slide previous samples out and read new samples
  memcpy(g_process_input,g_process_input+HOP_SIZE,(FRAME_SIZE-HOP_SIZE)*sizeof(float));
  //read new samples
  int nProcLocation = (pProcessReadPtr) % NUM_HOPS;//circular buffer loopback to beginning
  float *pFloatPtr = g_decimation_buffer;//128 samples of channel 1
  int *pIntPtr = g_audio_data_input+nProcLocation*AUDIO_COUNT;
  for(int i = 0;i < AUDIO_COUNT;i+=2){
	  float nNoiseVal = (float)(*pIntPtr++/(float)(SCALE_FACTOR));//24 bit adc = pow(2,24)
	  float nSignalVal = (float)(*pIntPtr++/(float)(SCALE_FACTOR));//24 bit adc = pow(2,24)
	  *pFloatPtr++ = nSignalVal + nNoiseVal;//combine signal with noise
  }
  pProcessReadPtr += 1;//updated process buffer location

  //decimate inputs
  float *pOutPtr = g_process_input+(FRAME_SIZE-HOP_SIZE);//128 samples of channel 1
  fir_decimaf( (const float32_t *)g_decimation_buffer, pOutPtr, HOP_SIZE*3, &decimation_filter);

  /* Calculate the FFT of a real signal */
  rfft ((const float32_t *)g_process_input,(complex_float *)g_audio_complex_input,(const complex_float *)twiddle_table,1,FFT_SIZE);

  //convert complex to magnitude
  fft_magnitude( (complex_float *)g_audio_complex_input, input1->data.f,FFT_SIZE,1);

  //Remove normalized effect of fft_magnitude
  vecsmltf(input1->data.f,FFT_SIZE/2+1,input1->data.f,FFT_SIZE/2+1);

  //set input state 1 frm global buffer
  //state from previous iteration is passed to next iteration
  memcpy(state1->data.f,g_audio_state1_input,FFT_SIZE*sizeof(float));//set input state

#ifdef DO_CYCLE_COUNTS
	STOP_CYCLE_COUNT (pre_cyc, pre_var);
    cycle_t var = 0, cyc=0; //Variables for cycle counting
	START_CYCLE_COUNT (var);
#endif

  // Run the model on this input and make sure it succeeds.
  if (kTfLiteOk != interpreter1->Invoke()) {
	  PRINT_INFO("Invoke failed.");
  }

#ifdef DO_CYCLE_COUNTS
	STOP_CYCLE_COUNT (cyc, var);
    START_CYCLE_COUNT (pre_var);
#endif

  TfLiteTensor* output1 = interpreter1->output(0);//mask
  TfLiteTensor* output2 = interpreter1->output(1);//state

  memcpy(g_audio_state1_input,output2->data.f,FFT_SIZE*sizeof(float));//copy output state to store for next iteration

    //apply mask to input, multiply complex input with nn output
    for(int i = 0;i < FFT_SIZE/2+1;i++){
  	  g_audio_complex_input[i] = g_audio_complex_input[i] * output1->data.f[i];
    }

  rifft ((const complex_float *)g_audio_complex_input,(float32_t *)input2->data.f,
		  (const complex_float *)twiddle_table,1,FFT_SIZE);

  memcpy(state2->data.f,g_audio_state2_input,FFT_SIZE*sizeof(float));//set input state from previous it

#ifdef DO_CYCLE_COUNTS
	STOP_CYCLE_COUNT (pre_cyc, pre_var);
	START_CYCLE_COUNT (var);
#endif

  // Run the model on this input and make sure it succeeds.
  if (kTfLiteOk != interpreter2->Invoke()) {
	  PRINT_INFO("Invoke failed.");
  }

#ifdef DO_CYCLE_COUNTS
	STOP_CYCLE_COUNT (cyc, var);
    START_CYCLE_COUNT (pre_var);
#endif

  TfLiteTensor* output3 = interpreter2->output(MAG_OUTPUT);//audio out
  TfLiteTensor* output4 = interpreter2->output(STATE_OUTPUT);//state

  memcpy(g_audio_state2_input,output4->data.f,FFT_SIZE*sizeof(float));//update state for next it
  memcpy(g_process_output,output3->data.f,FFT_SIZE*sizeof(float));//update output

  //overlap and add
  //next frames, add and overlap
  for(int i = 0;i < FRAME_SIZE;i++) {
	  g_previous_frame_out[i] += g_process_output[i];
  }

  //interpolate inputs
  fir_interpf( (const float32_t *)g_previous_frame_out, g_interpolation_buffer, HOP_SIZE, &interpolation_filter);

  //copy this to write buffer to next empty location
  nProcLocation = (pProcessWritePtr) % NUM_HOPS;//circular buffer loopback to beginning
  pFloatPtr = g_interpolation_buffer;//128 samples of channel 1
  pIntPtr = g_audio_data_output+nProcLocation*AUDIO_COUNT;
  for(int i = 0;i < AUDIO_COUNT;i+=2){
	  int nIntVal = (int)((*pFloatPtr++)*SCALE_FACTOR);//24 bit dac = pow(2,24)
	  *pIntPtr++ = nIntVal;//set channel 0 to denoised output
	  *pIntPtr++ = nIntVal;//set channel 1 to denoiser output
  }
  pProcessWritePtr++;

  //shift out frame discarding outputted signals
  memcpy(g_previous_frame_out,g_previous_frame_out+HOP_SIZE,(FRAME_SIZE-HOP_SIZE)*sizeof(float));
  //add new samples at the end
  memset(g_previous_frame_out+(FRAME_SIZE-HOP_SIZE),0,(HOP_SIZE)*sizeof(float));

#ifdef DO_CYCLE_COUNTS
	STOP_CYCLE_COUNT (pre_cyc, pre_var);
#endif


#ifdef DO_CYCLE_COUNTS
	PRINT_INFO("\tNumber of cycles to run preprocessing+postprocessing: \t%lu \n", pre_cyc);
	PRINT_INFO("\tNumber of cycles to run inference: \t%lu \n", cyc);
	PRINT_INFO("\n");
#endif
  return true;
}
