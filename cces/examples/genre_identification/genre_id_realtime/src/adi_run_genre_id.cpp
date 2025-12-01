/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/
/*****************************************************************************
 * adi_run_genre_id.cpp
 *****************************************************************************/
#include "adi_run_genre_id.h"
#include <stdint.h>

#define DO_QUANTIZED_INFERENCE

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

#ifdef DO_QUANTIZED_INFERENCE
#include "../../common/model/int8_genre_ID/int8_genre_ID_model_data.h"
#endif
#include "preprocessing_utils.h"
#include "adi_sharcfx_init.h"
#include <window.h>
#include <filter.h>
#include <complex.h>
#include <fft.h>
#include <vector.h>
#include <filter.h>
#include <matrix.h>
#include <stdlib.h>
#include "save_file.h"

#define M_PI 3.141592653589793

// #define DISPLAY_CYCLE_COUNTS
#ifdef DISPLAY_CYCLE_COUNTS   /* Enable the macros */
#define DO_CYCLE_COUNTS       //Needed internally
#define __PRE_FX_COMPATIBILITY
#include "cycle_count.h" /* Define the macros */
cycle_t pre_var = 0, pre_cyc=0; //Variables for cycle counting
#endif

#ifdef DO_QUANTIZED_INFERENCE
//int16 scales fixed per model
#define INPUT_SCALE 0.0002986959589179605f
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
static float train_min[MEL_RESOLUTION] = {
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f
};

static float train_max[MEL_RESOLUTION] = {
    0.0544787347f, 0.0709493980f, 0.0757559687f, 0.0761674717f, 0.0696647987f, 0.0711646006f, 0.0658979416f, 0.0470140278f,
    0.0560819656f, 0.0634767339f, 0.0640369430f, 0.0570543110f, 0.0582593530f, 0.0557297207f, 0.0460934006f, 0.0502725281f,
    0.0605143942f, 0.0472433046f, 0.0448874086f, 0.0568433329f, 0.0547614507f, 0.0585558899f, 0.0459594950f, 0.0330935977f,
    0.0439633168f, 0.0504311658f, 0.0352341868f, 0.0491012260f, 0.0398206040f, 0.0421081334f, 0.0522768125f, 0.0453265049f,
    0.0483472906f, 0.0394321010f, 0.0494496897f, 0.0498018190f, 0.0486189276f, 0.0421792790f, 0.0353340618f, 0.0447653346f,
    0.0266452096f, 0.0358084217f, 0.0326985009f, 0.0340347700f, 0.0334696099f, 0.0269841533f, 0.0370768271f, 0.0187110901f,
    0.0354591385f, 0.0224874020f, 0.0293756574f, 0.0211157557f, 0.0209773742f, 0.0214666128f, 0.0203053206f, 0.0316305049f,
    0.0268627033f, 0.0295013115f, 0.0248801745f, 0.0308168735f, 0.0287385471f, 0.0238769818f, 0.0364112034f, 0.0249824431f,
    0.0189381298f, 0.0155275920f, 0.0227175541f, 0.0197891444f, 0.0322422683f, 0.0158448294f, 0.0125988144f, 0.0105447005f,
    0.0236575324f, 0.0299205035f, 0.0262722429f, 0.0207796823f, 0.0077753020f, 0.0046684397f, 0.0034947151f, 0.0093648126f,
    0.0252236836f, 0.0195619352f, 0.0182084180f, 0.0062436522f, 0.0058284802f, 0.0053312765f, 0.0123204133f, 0.0142368888f,
    0.0151284477f, 0.0133968657f, 0.0120246662f, 0.0089507774f, 0.0049243718f, 0.0067697992f, 0.0252167005f, 0.0095890304f,
    0.0197974276f, 0.0194418095f, 0.0113818347f, 0.0154679120f, 0.0172325242f, 0.0219069589f, 0.0086016031f, 0.0017610170f,
    0.0000287543f, 0.0000248290f, 0.0000292272f, 0.0000217099f, 0.0000239099f, 0.0000201102f, 0.0000183580f, 0.0000216984f,
    0.0000190141f, 0.0000197959f, 0.0000163066f, 0.0000150121f, 0.0000152907f, 0.0000151520f, 0.0000141960f, 0.0000131747f,
    0.0000125319f, 0.0000123339f, 0.0000114128f, 0.0000115483f, 0.0000121078f, 0.0000108588f, 0.0000107925f, 0.0000106576f
};
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
constexpr int kTensorArenaSize = (1300+72) * 1024;
#endif
static uint8_t tensor_arena[kTensorArenaSize]__attribute__((section(".L3.data"), aligned(32)));
}  // namespace

//For audio file read
//#define FILE_IO
// Please comment pReadPtr++ in SPORTRxCallback, adi_sharcfx_init.cpp if FILE_IO is defined
//***************************** file *************************************************
#ifdef FILE_IO
#include "adi_input_provider.h"
float g_audio_data_input_file[HOP_SIZE*CIRCULAR_BUFFER_LENGTH]__attribute__((section(".L3.data"), aligned(32)));
#endif
//***************************** file *************************************************
int g_audio_data_input[AUDIO_COUNT*CIRCULAR_BUFFER_LENGTH]__attribute__((section(".L3.data"), aligned(32)));
volatile uint32_t pReadPtr;
uint32_t pProcessReadPtr;
volatile uint32_t pWritePtr;
uint32_t pProcessWritePtr;
int g_audio_data_output[AUDIO_COUNT*CIRCULAR_BUFFER_LENGTH]__attribute__((section(".L3.data"), aligned(32)));//holds raw audio output of model2
float g_audio_input[FFT_SIZE]__attribute__((section(".L3.data"), aligned(32)));
float g_audio_windowed_data_input[FFT_SIZE]__attribute__((section(".L3.data"), aligned(32)));
complex_float g_audio_complex_input[FFT_SIZE]__attribute__((section(".L3.data"), aligned(32)));
float g_stft_clip[MEL_RESOLUTION][(FFT_SIZE/2)+1]__attribute__((section(".L3.data"), aligned(32)));
complex_float twiddle_table[FFT_SIZE]__attribute__((section(".L3.data"), aligned(32)));
float mel_spec[MEL_RESOLUTION][MEL_RESOLUTION]__attribute__((section(".L3.data"), aligned(32)));
float scores[NUM_CLASSES]__attribute__((section(".L3.data"), aligned(32)));
float denom[MEL_RESOLUTION]__attribute__((section(".L3.data"), aligned(32)));

int nFrame;
int frameCount;
float global_max;
bool FirstFrame;
// The name of this function is important for Arduino compatibility.
void adi_genre_id_model_setup() {

	tflite::InitializeTarget();

#ifdef UART_REDIRECT
	set_print_func(uart_print);
#else
	set_print_func(default_print);
#endif

	// Map the model into a usable data structure. This doesn't involve any
	// copying or parsing, it's a very lightweight operation.
	#ifdef DO_QUANTIZED_INFERENCE
	model = tflite::GetModel(g_int8_model_data);
	#endif

	if (model->version() != TFLITE_SCHEMA_VERSION) {
		PRINT_INFO(
			"Model provided is schema version %d not equal "
			"to supported version %d.",
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

	// Build an interpreter to run the model with.
	// NOLINTNEXTLINE(runtime-global-variables)
	static tflite::MicroInterpreter static_interpreter(
		model, micro_op_resolver, tensor_arena, kTensorArenaSize);

	interpreter = &static_interpreter;

	//   Allocate memory from the tensor_arena for the model's tensors.
	TfLiteStatus allocate_status = interpreter->AllocateTensors();
	if (allocate_status != kTfLiteOk) {
		PRINT_INFO("AllocateTensors() failed");
		exit(0);
	}

	//   Get information about the memory area to use for the model's input.
	input = interpreter->input(0);
#ifdef FILE_IO
	//Open audio file
	if (kTfLiteOk != OpenInputFile()) {
		PRINT_INFO("\nFailed to read the input file. Check if the genre_audio.bin is available in input folder.Refer genre_identification/Readme.\n");
		exit(0);
	}
#endif

	/* Initialize the twiddle table */
	generate_fft_twiddle_table(FFT_SIZE,(float *)twiddle_table);

	memset(g_audio_data_input,0,CIRCULAR_BUFFER_LENGTH*AUDIO_COUNT*sizeof(int));
	memset(g_audio_data_output,0,CIRCULAR_BUFFER_LENGTH*AUDIO_COUNT*sizeof(int));
	memset(g_audio_windowed_data_input,0,FFT_SIZE*sizeof(float));
	memset(g_audio_input,0,FFT_SIZE*sizeof(float));

	for(int32_t nWindow=0; nWindow<MEL_RESOLUTION;nWindow++){
		memset(mel_spec[nWindow],0,(MEL_RESOLUTION)*sizeof(float));
	}
	for(int32_t nWindow=0; nWindow<MEL_RESOLUTION;nWindow++){
		memset(g_stft_clip[nWindow],0,((FFT_SIZE/2)+1)*sizeof(float));
	}

	nFrame = 0;
	frameCount = 0;
	global_max = 0.0f;
	FirstFrame = true;
	pReadPtr = 0;//consider first 3 samples as 0 (FFT_SIZE = 2048, HOP_SIZE=512)
	pProcessReadPtr = 0;
	pWritePtr = 0;
	pProcessWritePtr = 0;

	// train_max and train_min are the input mel bins min and max values observed during training
  	// denom is used to normalize the input mel bins
	for (int i = 0; i < MEL_RESOLUTION; i++){
		denom[i] = train_max[i] - train_min[i] + 1e-10f;
	}
}
#ifdef FILE_IO
void GetData(float* image_data, int* pReadPtr, int counter){
	//Loop over number of windows
	TfLiteStatus status = kTfLiteOk;
	if(CheckFileOpen()){
	  	for (int j = 0;j < counter ;j++) {
			// Get audio frame(s) for 500msec from provider.
			status = GetFrame(image_data,pReadPtr);
			if (status == kTfLiteCancelled) {
				PRINT_INFO("Cancellation triggered! End of File reached!");
				exit(0);
			}
			else if (status == kTfLiteError) {
				PRINT_INFO("Audio capture failed.");
				exit(0);
			}
			else if (status == kTfLiteOk) {
				PRINT_INFO("\nAudio capture passed, next frame being processed!\n");
			}
	  	}
	}
}
#endif

// The name of this function is important for Arduino compatibility.
void adi_genre_id_model_run() {
#ifdef DO_CYCLE_COUNTS
	if(frameCount == 0){
		pre_var = 0, pre_cyc=0; //Variables for cycle counting
		START_CYCLE_COUNT (pre_var);
	}
#endif

#ifdef FILE_IO
	if(FirstFrame == true){
		GetData(g_audio_data_input_file,(int *)&pReadPtr,((FFT_SIZE - HOP_SIZE)/HOP_SIZE)); // (int)((FFT_SIZE - HOP_SIZE)/HOP_SIZE));
		int nInitialProcLocation = (pProcessReadPtr) % (NUM_HOPS * WINDOWS_PER_FRAME);//circular buffer loopback to beginning
		float *pInBuffer = g_audio_data_input_file + nInitialProcLocation * HOP_SIZE;
		memcpy(g_audio_input,pInBuffer,(int)((FFT_SIZE - HOP_SIZE)/HOP_SIZE)*HOP_SIZE*sizeof(float));
		pProcessReadPtr += (int)((FFT_SIZE - HOP_SIZE)/HOP_SIZE);
		FirstFrame = false;
		}
	GetData(g_audio_data_input_file,(int *)&pReadPtr,1);
#endif

	if(pReadPtr > pProcessReadPtr) {
		nFrame = MEL_RESOLUTION - WINDOWS_PER_FRAME + frameCount;
	    //do window operations + stft and pass to input
	    //this is identical to the one done by librosa.stft
	    //https://librosa.org/doc/main/generated/librosa.stft.html
	    //we do a windowing, fftmag
	    //our fft_magnitude does a normalization so we have to remove it
	    //get next window
	    //read new samples
		int nProcLocation = (pProcessReadPtr) % (CIRCULAR_BUFFER_LENGTH);//circular buffer loopback to beginning

#ifdef FILE_IO
		float *pInBuffer = g_audio_data_input_file + (nProcLocation * HOP_SIZE);
		memcpy(g_audio_input + (FFT_SIZE - HOP_SIZE),pInBuffer,HOP_SIZE*sizeof(float));
#else
		int *pInBuffer = g_audio_data_input + (nProcLocation * AUDIO_COUNT);
		// Dump the input buffer to file using the save_file.cpp utility
		// Uncomment the below line to save the input buffer to a file
		// Change TIME_IN_SECS_TO_SAVE in save_file.h accordingly
		// save48KHzADCIntStereoBufferToFile(pInBuffer, AUDIO_COUNT, "genre_ADC_int_stereo_48KHz.bin");

	    float *pFloatPtr = g_audio_input + (FFT_SIZE - HOP_SIZE);
	    /* pragma no_simd is added because we noticed when memcpy or loop vectorization is enabled then there is memory corruption
		   and the inference results get corrupted */
		#pragma no_simd
	    for(int i = 0;i < AUDIO_COUNT;i += 2){
			float nLeftChannel1 = (float)(*pInBuffer++/(float)(SCALE_FACTOR));//24 bit adc = pow(2,24)
			float nRightChannel1 = (float)(*pInBuffer++/(float)(SCALE_FACTOR));//24 bit adc = pow(2,24)
			*pFloatPtr++ = ((nLeftChannel1+nRightChannel1)/2);
	    }
#endif
		frameCount+=1;

	    /* Windowing */
	    vecvmltf(g_audio_input, g_window, g_audio_windowed_data_input, FFT_SIZE);

	    /* Calculate the FFT of a real signal */
	    rfft ((const float32_t *)g_audio_windowed_data_input,(complex_float *)g_audio_complex_input,(const complex_float *)twiddle_table,1,FFT_SIZE);

	    //convert complex to magnitude
	    fft_magnitude( (complex_float *)g_audio_complex_input, g_stft_clip[nFrame],FFT_SIZE,1);

		//Remove normalization and revert to normal scales
	    vecsmltf(g_stft_clip[nFrame],((FFT_SIZE/2)),g_stft_clip[nFrame],(FFT_SIZE/2)+1);

		//Square STFT outputs
	    vecvmltf(g_stft_clip[nFrame],g_stft_clip[nFrame],g_stft_clip[nFrame],(FFT_SIZE/2)+1);

		pProcessReadPtr += 1;

		//copy last 3 hop frames into beginning
	    memcpy(g_audio_input,g_audio_input + HOP_SIZE,(FFT_SIZE - HOP_SIZE)*sizeof(float));
	    

	    if ( frameCount == WINDOWS_PER_FRAME) {
	    	// Map the STFT frequency bins to the Mel scale
			// Compute the Mel spectrogram frame index based on the current frame count and each audio inference duration (WINDOWS_PER_FRAME)
			// Then apply the Mel filter bank to the current STFT frame and store the result in mel_spec[nFrame]

			for (int i = 0; i < MEL_RESOLUTION; i++)
			    for (int j = 0; j < ((FFT_SIZE/2)+1); j++)
			        if (g_stft_clip[i][j] > global_max) global_max = g_stft_clip[i][j];

			global_max += 1e-9f;

			//Normalize STFT clip
			for (int i = 0; i < MEL_RESOLUTION; i++)
				for (int j = 0; j < ((FFT_SIZE/2)+1); j++)
					g_stft_clip[i][j] /= global_max;

			matmmltf((const float32_t *)g_stft_clip, MEL_RESOLUTION , (FFT_SIZE/2)+1, (const float32_t *)mel_weight_mat, MEL_RESOLUTION, (float32_t *)mel_spec);
			global_max = 0.0f;

	        frameCount=0;
	        FirstFrame = false;
#ifdef DO_QUANTIZED_INFERENCE
			//Quantize inputs, convert float to int8
			int ind =0;
			for(int i=0; i< MEL_RESOLUTION;i++){
				for(int j=0; j< MEL_RESOLUTION;j++){
					float data = (mel_spec[i][j] - train_min[j]) / denom[j];
					float round_data = round(data/INPUT_SCALE + INPUT_ZERO_PT);
					input->data.int8[ind++] = (int8)(fmax(-128.0f, fmin(127.0f, round_data)));
				}
			}
#endif
	//PREPROCESSING END
#ifdef DO_CYCLE_COUNTS
			STOP_CYCLE_COUNT (pre_cyc, pre_var);
			cycle_t var = 0, cyc=0; //Variables for cycle counting
			START_CYCLE_COUNT (var);
#endif

			// Reset the TFLite interpreter to clear previous states and prepare for a fresh inference cycle
			interpreter->Reset();
			// Run the model on this input and make sure it succeeds.
			if (kTfLiteOk != interpreter->Invoke()) {
				PRINT_INFO("Invoke failed.");
				exit(0);
			}
			TfLiteTensor* output = interpreter->output(0);

#ifdef DO_QUANTIZED_INFERENCE
			for(int i=0; i<NUM_CLASSES; i++){
			  scores[i] = (float)(OUTPUT_SCALE * ((float)output->data.int8[i] - OUTPUT_ZERO_PT));
			}
#endif

			int imax = 0;
			for (int i = 1; i < NUM_CLASSES; ++i)
			{
				if (scores[imax] < scores[i])
					imax = i;
			}

#ifdef DO_CYCLE_COUNTS
			STOP_CYCLE_COUNT (cyc, var);
			PRINT_INFO("\nNumber of cycles to run preprocessing: %lu \n", pre_cyc);
			PRINT_INFO("\nNumber of cycles to run inference: %lu\n", cyc);
#endif
			PRINT_INFO("\nDetected Class \" %s\" with score %f\n", classes_list[imax], scores[imax]);

			// Copy the overlapping melspectorgram to the beginning
			memcpy(&mel_spec[0], &mel_spec[WINDOWS_PER_FRAME],(MEL_RESOLUTION - WINDOWS_PER_FRAME)* MEL_RESOLUTION * sizeof(float));

	    }
	}
}
