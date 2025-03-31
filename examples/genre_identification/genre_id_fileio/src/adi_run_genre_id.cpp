/*******************************************************************************
 Copyright(c) 2024 Analog Devices, Inc. All Rights Reserved. This software is
 proprietary & confidential to Analog Devices, Inc. and its licensors. By using
 this software you agree to the terms of the associated Analog Devices License
 Agreement.
*******************************************************************************/

#include "adi_run_genre_id.h"

#define DO_QUANTIZED_INFERENCE

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

#ifdef DO_QUANTIZED_INFERENCE
#include "../../common/model/int8_genre_ID/int8_genre_ID_model_data.h"
#else
#include "../../common/model/float_genre_ID/float32_genre_ID_model_data.h"
#endif
#include "adi_input_provider.h"
#include "../../common/preprocessing_utils.h"

#include <window.h>
#include <filter.h>
#include <complex.h>
#include <fft.h>
#include <vector.h>
#include <matrix.h>
#define M_PI 3.141592653589793

//#define DISPLAY_CYCLE_COUNTS

#ifdef DISPLAY_CYCLE_COUNTS   /* Enable the macros */
#define __PRE_FX_COMPATIBILITY
#define DO_CYCLE_COUNTS       //Needed internally
#include "cycle_count.h" /* Define the macros */
#endif

#ifdef DO_QUANTIZED_INFERENCE
//int16 scales fixed per model
#define INPUT_SCALE 4.748456954956055f
#define INPUT_ZERO_PT -128
#define OUTPUT_SCALE 0.00390625f
#define OUTPUT_ZERO_PT -128
#endif


///
extern "C" {
    int generate_fft_twiddle_table (
        int fft_size,         /**< [in] table size in complex elements */
        float *p_twiddles);    /**< [out] table output.  Size in floats is 2 * 3 * fft_size / 4 */
}
// Globals, used for compatibility with Arduino-style sketches.
namespace {
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;

// In order to use optimized tensorflow lite kernels, a signed int8_t quantized
// model is preferred over the legacy unsigned model format. This means that
// throughout this project, input images must be converted from unisgned to
// signed format. The easiest and quickest way to convert from unsigned to
// signed 8-bit integers is to subtract 128 from the unsigned value to get a
// signed value.

// An area of memory to use for input, output, and intermediate arrays.
#ifdef DO_QUANTIZED_INFERENCE
constexpr int kTensorArenaSize = (1300) * 1024;
#else
constexpr int kTensorArenaSize = (6000) * 1024;
#endif
static uint8_t tensor_arena[kTensorArenaSize]__attribute__((section(".L3.data"), aligned(8)));
}  // namespace

//For audio file read
float g_audio_data_input[NUM_HOPS*WINDOWS_PER_FRAME*HOP_SIZE]__attribute__((section(".L3.data"), aligned(8)));
int pReadPtr;
int pProcessReadPtr;
float g_audio_input[FFT_SIZE]__attribute__((section(".L2.data"), aligned(8)));
float g_audio_windowed_data_input[FFT_SIZE]__attribute__((section(".L2.data"), aligned(8)));
complex_float g_audio_complex_input[FFT_SIZE]__attribute__((section(".L2.data"), aligned(8)));
float g_stft_clip[FFT_SIZE/2+1]__attribute__((section(".L2.data"), aligned(8)));
complex_float twiddle_table[FFT_SIZE]__attribute__((section(".L2.data"), aligned(8)));
//float sig_spec[MEL_RESOLUTION][FFT_SIZE/2+1]__attribute__((section(".L2.data"), aligned(8)));
float model_input[MEL_RESOLUTION*MEL_RESOLUTION]__attribute__((section(".L2.data"), aligned(8)));
float mel_spec[MEL_RESOLUTION][MEL_RESOLUTION]__attribute__((section(".L2.data"), aligned(8)));
float mel_spec_temp[MEL_RESOLUTION]__attribute__((section(".L2.data"), aligned(8)));
float scores[NUM_CLASSES]__attribute__((section(".L2.data"), aligned(8)));
int nFrame;
bool FirstFrame;
int counter;

// The name of this function is important for Arduino compatibility.
void adi_model_setup() {
  tflite::InitializeTarget();

  //File IO always use printf
  set_print_func(default_print);

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  #ifdef DO_QUANTIZED_INFERENCE
    model = tflite::GetModel(g_int8_model_data);
  #else
    model = tflite::GetModel(g_float32_model_data);
  #endif

  if (model->version() != TFLITE_SCHEMA_VERSION) {
    PRINT_INFO(
        "\nModel provided is schema version %d not equal "
        "to supported version %d.\n",
        model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  // Pull in only the operation implementations we need.
  // This relies on a complete list of all the ops needed by this graph.

  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroMutableOpResolver<16> micro_op_resolver;
  micro_op_resolver.AddConv2D();
  micro_op_resolver.AddRelu();
  micro_op_resolver.AddReshape();
  micro_op_resolver.AddFullyConnected();
  micro_op_resolver.AddMaxPool2D();
  micro_op_resolver.AddSoftmax();
  micro_op_resolver.AddTranspose();
  micro_op_resolver.AddShape();
  micro_op_resolver.AddGather();
  micro_op_resolver.AddConcatenation();
  micro_op_resolver.AddEqual();
  // micro_op_resolver.AddWhere();

  // Build an interpreter to run the model with.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

//   Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    PRINT_INFO("\nAllocateTensors() failed\n");
//    return;
  }

//   Get information about the memory area to use for the model's input.
  input = interpreter->input(0);

  //Open audio file
  if (kTfLiteOk != OpenInputFile()) {
    PRINT_INFO("\nFailed to read the input file. Check if the genre_audio.bin is available in input folder.Refer genre_identification/Readme.\n");
    exit(0);
  }

//  gen_hanningf (g_window, 1, FFT_SIZE);
  //TODO:Hanning window is not matching at the ends
  //g_window[0] = 0;
  //g_window[FFT_SIZE -1] = 0;
  /* Initialize the twiddle table */
  generate_fft_twiddle_table(FFT_SIZE,(float *)twiddle_table);

  memset(g_audio_data_input,0,NUM_HOPS*WINDOWS_PER_FRAME*HOP_SIZE*sizeof(float));
  memset(g_audio_windowed_data_input,0,FFT_SIZE*sizeof(float));
  memset(g_audio_input,0,FFT_SIZE*sizeof(float));

  for(int32_t nWindow=0; nWindow<MEL_RESOLUTION;nWindow++){
        memset(mel_spec[nWindow],0,(MEL_RESOLUTION)*sizeof(float));
  }

  nFrame = 0;
  FirstFrame = true;
  pReadPtr = 0;//consider first 3 samples as 0 (FFT_SIZE = 2048, HOP_SIZE=512)
  pProcessReadPtr = 0;
}
void GetData(float* image_data, int* pReadPtr, int counter){
	//Loop over number of windows
	TfLiteStatus status = kTfLiteOk;
	if(CheckFileOpen()){
	  for (int j = 0;j < counter ;j++) {
		  // Get audio frame(s) for 500msec from provider.
		status = GetFrame(image_data,pReadPtr);
		if (status == kTfLiteCancelled) {
			PRINT_INFO("\nCancellation triggered! End of File reached!\n");
		}
		else if (status == kTfLiteError) {
			PRINT_INFO("\nAudio capture failed.\n");
			exit(0);
		}
		else if (status == kTfLiteOk) {
        	PRINT_INFO("\nAudio capture passed, next frame being processed!\n");
		}
	  }
	}
}
// The name of this function is important for Arduino compatibility.
void adi_model_run() {
#ifdef DO_CYCLE_COUNTS
    long int pre_var = 0, pre_cyc=0; //Variables for cycle counting
    START_CYCLE_COUNT (pre_var);
#endif
    if(FirstFrame == true){
		GetData(g_audio_data_input,&pReadPtr,((FFT_SIZE - HOP_SIZE)/HOP_SIZE));
		int nInitialProcLocation = (pProcessReadPtr) % (NUM_HOPS * WINDOWS_PER_FRAME);//circular buffer loopback to beginning
		float *pInBuffer = g_audio_data_input + nInitialProcLocation * HOP_SIZE;
		memcpy(g_audio_input,pInBuffer,(int)((FFT_SIZE - HOP_SIZE)/HOP_SIZE)*HOP_SIZE*sizeof(float));
		pProcessReadPtr += (int)((FFT_SIZE - HOP_SIZE)/HOP_SIZE);
    }
    GetData(g_audio_data_input,&pReadPtr,WINDOWS_PER_FRAME);
    if(pReadPtr - pProcessReadPtr >= WINDOWS_PER_FRAME) {
        nFrame = MEL_RESOLUTION - WINDOWS_PER_FRAME;
        for (int nWindow = 0;nWindow < WINDOWS_PER_FRAME;nWindow++) {
			//get next window
			int nProcLocation = (pProcessReadPtr) % (NUM_HOPS * WINDOWS_PER_FRAME);//circular buffer loopback to beginning
			float *pOutBuffer = g_audio_data_input + nProcLocation * HOP_SIZE;
			memcpy(g_audio_input + (FFT_SIZE - HOP_SIZE),pOutBuffer,HOP_SIZE*sizeof(float));
			pProcessReadPtr += 1;//updated process buffer location
			  //do window operations + stft and pass to input
			  /* Windowing */
			  vecvmltf(g_audio_input, g_window, g_audio_windowed_data_input, FFT_SIZE);

			  /* Calculate the FFT of a real signal */
			  rfft ((const float32_t *)g_audio_windowed_data_input,(complex_float *)g_audio_complex_input,(const complex_float *)twiddle_table,1,FFT_SIZE);

			  //convert complex to magnitude
			  fft_magnitude( (complex_float *)g_audio_complex_input, g_stft_clip,FFT_SIZE,1);
			  //Remove normalization and revert to normal scales
			  vecsmltf(g_stft_clip,FFT_SIZE/2+1,g_stft_clip,FFT_SIZE/2+1);
			  //Square STFT outputs
			  vecvmltf(g_stft_clip,g_stft_clip,g_stft_clip,FFT_SIZE/2+1);


			  //Map frequency into Mel scale
			  memcpy(&mel_spec[0], &mel_spec[WINDOWS_PER_FRAME],(MEL_RESOLUTION - WINDOWS_PER_FRAME)* MEL_RESOLUTION * sizeof(float));
			  matmmltf(g_stft_clip, 1, FFT_SIZE/2+1, *mel_weight_mat,MEL_RESOLUTION,mel_spec[nFrame++]);
			  //copy last 3 hop frames into beginning
			  memcpy(g_audio_input,g_audio_input + HOP_SIZE,(FFT_SIZE - HOP_SIZE)*sizeof(float));
        }
	FirstFrame = false;

#ifdef DO_QUANTIZED_INFERENCE
	//Quantize inputs, convert float to int8
	int ind =0;
	for(int i=0; i< MEL_RESOLUTION;i++){
		for(int j=0; j< MEL_RESOLUTION;j++){
			//input->data.int8[ind++] = (int8)round(mel_spec[(iteration_number+i)%MEL_RESOLUTION][j]/INPUT_SCALE + INPUT_ZERO_PT);//index calculation for mel_spec(combined with how each new row is populated) enables it to act like a circular buffer
			input->data.int8[ind++] = (int8)round(mel_spec[i][j]/INPUT_SCALE + INPUT_ZERO_PT);//index calculation for mel_spec(combined with how each new row is populated) enables it to act like a circular buffer
		}
	}
#else
	for(int32_t i = 0;i < MEL_RESOLUTION*MEL_RESOLUTION;i++) {
		input->data.f[i] = (float)model_input[i];
	}
#endif
	//PREPROCESSING END
#ifdef DO_CYCLE_COUNTS
	STOP_CYCLE_COUNT (pre_cyc, pre_var);
	cycle_t var = 0, cyc=0; //Variables for cycle counting
	START_CYCLE_COUNT (var);
#endif

	// Run the model on this input and make sure it succeeds.
	if (kTfLiteOk != interpreter->Invoke()) {
	PRINT_INFO("\nInvoke failed.\n");
	exit(0);
	}

	TfLiteTensor* output = interpreter->output(0);

#ifdef DO_QUANTIZED_INFERENCE
	for(int i=0; i<NUM_CLASSES; i++){
	 scores[i] = (float)(OUTPUT_SCALE * ((float)output->data.int8[i] - OUTPUT_ZERO_PT));
	}
#else
	for(int i=0; i<NUM_CLASSES; i++){
	 scores[i] = (float)output->data.f[i];
	}
#endif
	int imax = 0;
	for (int i = 1; i < NUM_CLASSES; ++i)
	{
	  if (scores[imax] < scores[i])
		  imax = i;
	}
	PRINT_INFO("Detected Class \" %s\" with score %f\n\n", classes_list[imax], scores[imax]);
	PRINT_INFO("");


#ifdef DO_CYCLE_COUNTS
	STOP_CYCLE_COUNT (cyc, var);
#endif


#ifdef DO_CYCLE_COUNTS
	PRINT_INFO("\tNumber of cycles to run preprocessing: \t%lu \n", pre_cyc);
	PRINT_INFO("\tNumber of cycles to run inference: \t%lu \n", cyc);
	PRINT_INFO("\n");
#endif
    }
}
