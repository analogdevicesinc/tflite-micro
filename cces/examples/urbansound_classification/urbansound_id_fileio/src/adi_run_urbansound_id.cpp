/*******************************************************************************
 Copyright(c) 2024 Analog Devices, Inc. All Rights Reserved. This software is
 proprietary & confidential to Analog Devices, Inc. and its licensors. By using
 this software you agree to the terms of the associated Analog Devices License
 Agreement.
*******************************************************************************/

#include "adi_run_urbansound_id.h"


#define DO_QUANTIZED_INFERENCE

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

#ifdef DO_QUANTIZED_INFERENCE
#include "../../common/model/int8_urbansound_class_ID/int8_urbansound_ID_model_data.h"
#endif
#include "adi_input_provider.h"
#include "../../common/preprocessing_utils.h"

#include <window.h>
#include <filter.h>
#include <complex.h>
#include <fft.h>
#include <vector.h>
#include <matrix.h>
#include <stdlib.h>

#define M_PI 3.141592653589793

//#define DISPLAY_CYCLE_COUNTS

#ifdef DISPLAY_CYCLE_COUNTS   /* Enable the macros */
#define __PRE_FX_COMPATIBILITY
#define DO_CYCLE_COUNTS       //Needed internally
#include "cycle_count.h" /* Define the macros */
#endif

#ifdef DO_QUANTIZED_INFERENCE
//int16 scales fixed per model
#define INPUT_SCALE 0.0006427989574149251f
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
    0.0779379681f, 0.0727755353f, 0.0748316869f, 0.0770610049f, 0.0782691911f, 0.0826157257f, 0.0785092339f, 0.0690027624f,
    0.0594748147f, 0.0694236979f, 0.0588885881f, 0.0591512546f, 0.0677784681f, 0.0739689320f, 0.0701336414f, 0.0782994628f,
    0.0799887553f, 0.0671363994f, 0.0697447136f, 0.0790622905f, 0.0797198862f, 0.0797846466f, 0.0814602077f, 0.0829809606f,
    0.0799147114f, 0.0835658833f, 0.0813181475f, 0.0793696120f, 0.0804833844f, 0.0770415440f, 0.0681432709f, 0.0709076822f,
    0.0693485141f, 0.0733557940f, 0.0603705347f, 0.0634412542f, 0.0639470890f, 0.0759073347f, 0.0611093640f, 0.0628804788f,
    0.0629007220f, 0.0562330596f, 0.0527972877f, 0.0555638112f, 0.0605159253f, 0.0733513683f, 0.0607014596f, 0.0433086343f,
    0.0543178618f, 0.0432658717f, 0.0354015939f, 0.0399728157f, 0.0495995693f, 0.0317482539f, 0.0334755145f, 0.0487873256f,
    0.0336168446f, 0.0375899300f, 0.0361109301f, 0.0406890363f, 0.0365769640f, 0.0322911590f, 0.0379427075f, 0.0377016850f,
    0.0444516912f, 0.0580824241f, 0.0615519769f, 0.0445884168f, 0.0291664414f, 0.0320255160f, 0.0289179590f, 0.0430259332f,
    0.0273590162f, 0.0238949321f, 0.0277973805f, 0.0377030261f, 0.0429896712f, 0.0523487143f, 0.0566235185f, 0.0529954061f,
    0.0542568862f, 0.0448758304f, 0.0316523463f, 0.0291070417f, 0.0333619975f, 0.0250525381f, 0.0326409824f, 0.0110763349f,
    0.0128415804f, 0.0209528077f, 0.0150773553f, 0.0320615210f, 0.0309348255f, 0.0219026804f, 0.0214501210f, 0.0263661165f,
    0.0378931686f, 0.0230823420f, 0.0134827849f, 0.0137377148f, 0.0176963396f, 0.0230338834f, 0.0284511019f, 0.0201662648f,
    0.0228843577f, 0.0204377007f, 0.0270680469f, 0.0299377721f, 0.0452050939f, 0.0463657565f, 0.0654862747f, 0.0516212732f,
    0.0277110711f, 0.0176518969f, 0.0085745472f, 0.0099084219f, 0.0089271674f, 0.0071124700f, 0.0060318876f, 0.0093464572f,
    0.0115090311f, 0.0105805127f, 0.0057678241f, 0.0055719479f, 0.0069069732f, 0.0035822620f, 0.0014276973f, 0.0013291521f
};


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
constexpr int kTensorArenaSize = (1300+72) * 1024;
#endif
static uint8_t tensor_arena[kTensorArenaSize]__attribute__((section(".L3.data"), aligned(32)));
}  // namespace

//For audio file read
float g_audio_data_input[NUM_HOPS*WINDOWS_PER_FRAME*HOP_SIZE]__attribute__((section(".L3.data"), aligned(32)));
int pReadPtr;
int pProcessReadPtr;
float g_audio_input[FFT_SIZE]__attribute__((section(".L3.data"), aligned(32)));
float g_audio_windowed_data_input[FFT_SIZE]__attribute__((section(".L3.data"), aligned(32)));
complex_float g_audio_complex_input[FFT_SIZE]__attribute__((section(".L3.data"), aligned(32)));
float g_stft_clip[MEL_RESOLUTION][(FFT_SIZE/2)+1]__attribute__((section(".L3.data"), aligned(32)));
complex_float twiddle_table[FFT_SIZE]__attribute__((section(".L3.data"), aligned(32)));
float model_input[MEL_RESOLUTION*MEL_RESOLUTION]__attribute__((section(".L3.data"), aligned(32)));
float mel_spec[MEL_RESOLUTION][MEL_RESOLUTION]__attribute__((section(".L3.data"), aligned(32)));
float scores[NUM_CLASSES]__attribute__((section(".L3.data"), aligned(32)));
float denom[MEL_RESOLUTION]__attribute__((section(".L3.data"), aligned(32)));
int nFrame;
int frameCount;
float global_max;
bool FirstFrame;
int counter;
char filename[64];

// The name of this function is important for Arduino compatibility.
void adi_model_setup() {
	tflite::InitializeTarget();

	//File IO always use printf
	set_print_func(default_print);

	// Map the model into a usable data structure. This doesn't involve any
	// copying or parsing, it's a very lightweight operation.
	#ifdef DO_QUANTIZED_INFERENCE
		model = tflite::GetModel(g_int8_model_data);
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

	// Build an interpreter to run the model with.
	// NOLINTNEXTLINE(runtime-global-variables)
	static tflite::MicroInterpreter static_interpreter(
		model, micro_op_resolver, tensor_arena, kTensorArenaSize);
	interpreter = &static_interpreter;

	//   Allocate memory from the tensor_arena for the model's tensors.
	TfLiteStatus allocate_status = interpreter->AllocateTensors();
	if (allocate_status != kTfLiteOk) {
		PRINT_INFO("\nAllocateTensors() failed\n");
	   	exit(0);
	}

	//   Get information about the memory area to use for the model's input.
	input = interpreter->input(0);

	//Open audio file
	if (kTfLiteOk != OpenInputFile()) {
		PRINT_INFO("\nFailed to read the input file. Check if the genre_audio.bin is available in input folder.Refer genre_identification/Readme.\n");
		exit(0);
	}

	/* Initialize the twiddle table */
	generate_fft_twiddle_table(FFT_SIZE,(float *)twiddle_table);

	memset(g_audio_data_input,0,NUM_HOPS*WINDOWS_PER_FRAME*HOP_SIZE*sizeof(float));
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
	// train_max and train_min are the input mel bins min and max values observed during training
	// denom is used to normalize the input mel bins
	for (int i = 0; i < MEL_RESOLUTION; i++){
		denom[i] = train_max[i] - train_min[i] + 1e-10f;
	}
}

void GetData(float* image_data, int* pReadPtr, int counter){
	//Loop over number of windows
	TfLiteStatus status = kTfLiteOk;
	if(CheckFileOpen()){
		for (int j = 0;j < counter ;j++) {
			// Get audio frame(s) for 1sec from provider.
			status = GetFrame(image_data,pReadPtr);
			if (status == kTfLiteCancelled) {
				PRINT_INFO("\nCancellation triggered! End of File reached!\n");
				exit(0);
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
			    nFrame = MEL_RESOLUTION - WINDOWS_PER_FRAME + nWindow;

			//get next window
			int nProcLocation = (pProcessReadPtr) % (NUM_HOPS * WINDOWS_PER_FRAME);//circular buffer loopback to beginning
			float *pOutBuffer = g_audio_data_input + nProcLocation * HOP_SIZE;

			memcpy(g_audio_input + (FFT_SIZE - HOP_SIZE),pOutBuffer,HOP_SIZE*sizeof(float));

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

			//Increment process pointer
			pProcessReadPtr += 1;//updated process buffer location

			//copy last 3 hop frames into beginning
			memcpy(g_audio_input,g_audio_input + HOP_SIZE,(FFT_SIZE - HOP_SIZE)*sizeof(float));
        }

		// Map the STFT frequency bins to the Mel scale
		// Compute the Mel spectrogram frame index based on the current frame count and each audio inference duration (WINDOWS_PER_FRAME)
		// Then apply the Mel filter bank to the current STFT frame and store the result in mel_spec[nFrame]
        frameCount+=1;
      
        for (int i = 0; i < MEL_RESOLUTION; i++)
            for (int j = 0; j < ((FFT_SIZE/2)+1); j++)
                if (g_stft_clip[i][j] > global_max) global_max = g_stft_clip[i][j];
				
        // The addition of a small constant ensures we never divide by zero
		global_max += 1e-9f;

        //normalize STFT
        for (int i = 0; i < MEL_RESOLUTION; i++)
          for (int j = 0; j < ((FFT_SIZE/2)+1); j++)
            g_stft_clip[i][j] /= global_max;


		matmmltf((const float32_t *)g_stft_clip, MEL_RESOLUTION , (FFT_SIZE/2)+1, (const float32_t *)mel_weight_mat, MEL_RESOLUTION, (float32_t *)mel_spec);

        global_max = 0.0f;
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
		PRINT_INFO("\tNumber of cycles to run preprocessing: \t%lu \n", pre_cyc);
		PRINT_INFO("\tNumber of cycles to run inference: \t%lu \n", cyc);
#endif

		PRINT_INFO("Detected Class \" %s\" with score %f\n\n", classes_list[imax], scores[imax]);
		PRINT_INFO("");
		// Copy the overlapping melspectorgram to the beginning
		memcpy(&mel_spec[0], &mel_spec[WINDOWS_PER_FRAME],(MEL_RESOLUTION - WINDOWS_PER_FRAME)* MEL_RESOLUTION * sizeof(float));
    }
}
