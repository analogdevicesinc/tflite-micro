/*******************************************************************************
 Copyright(c) 2024 Analog Devices, Inc. All Rights Reserved. This software is
 proprietary & confidential to Analog Devices, Inc. and its licensors. By using
 this software you agree to the terms of the associated Analog Devices License
 Agreement.
*******************************************************************************/

#include "adi_run_kws.h"

#define DO_QUANTIZED_INFERENCE
#define USE_DS_CNN_SMALL_INT8_MODEL

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

#ifdef DO_QUANTIZED_INFERENCE
#ifdef USE_DS_CNN_SMALL_INT8_MODEL
#include "model/ds_cnn_s_quantized_model_data.h"
#else
#include "model/model_data.h"
#endif
#else
#include "model/ds_cnn_l_fp32_model_data.h"
#endif
#include "adi_input_provider.h"
#include "preprocessing_utils.h"

#include <window.h>
#include <filter.h>
#include <complex.h>
#include <fft.h>
#include <vector.h>
#include <matrix.h>
#define M_PI 3.141592653589793

//#define DISPLAY_CYCLE_COUNTS
#ifdef DISPLAY_CYCLE_COUNTS   /* Enable the macros */
#define DO_CYCLE_COUNTS       //Needed internally
#define __PRE_FX_COMPATIBILITY
#include "cycle_count.h" /* Define the macros */
#endif

#ifdef DO_QUANTIZED_INFERENCE
//int16 scales fixed per model
#ifdef USE_DS_CNN_SMALL_INT8_MODEL
#define INPUT_SCALE 1.084193468093872f
#define INPUT_ZERO_PT 100
#else
#define INPUT_SCALE 1.1021944284439087f
#define INPUT_ZERO_PT 96
#endif
#define OUTPUT_SCALE 0.00390625f
#define OUTPUT_ZERO_PT -128
#endif

int imax = 0;
// Function to find discrete cosine transform and print it
void  custom_DCT(float matrix[DCT_SIZE])
{
    int k, l;
    float dct[DCT_SIZE];// dct will store the discrete cosine transform
    float sum=0;// sum will temporarily store the sum of
    float scale = 1/sqrt((int)2*DCT_SIZE);
    uint16_t ind=0;
    for (k = 0; k < DCT_SIZE; k++) {
        for (l = 0; l < DCT_SIZE; l++) {
            sum += matrix[l] * dct_multipliers[k*DCT_SIZE+l];
        }
        dct[k] =(float)(sum*2*scale);
        sum=0;
    }
    memcpy(matrix,dct,(DCT_SIZE)*sizeof(float));
}


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
#ifdef USE_DS_CNN_SMALL_INT8_MODEL
constexpr int kTensorArenaSize = (180) * 1024;
static uint8_t tensor_arena[kTensorArenaSize]__attribute__((section(".L2.data"), aligned(8)));
#else
constexpr int kTensorArenaSize = (400) * 1024;
static uint8_t tensor_arena[kTensorArenaSize]__attribute__((section(".L2.data"), aligned(8)));
#endif
//static uint8_t tensor_arena[kTensorArenaSize];

//For audio file read
float g_audio_data_input[FFT_SIZE]__attribute__((section(".L2.bss"), aligned(16)));
float g_audio_windowed_data_input[FFT_SIZE]__attribute__((section(".L2.bss"), aligned(16)));
complex_float g_audio_complex_input[FFT_SIZE]__attribute__((section(".L2.bss"), aligned(16)));
float g_stft_clip[FFT_SIZE/2+1]__attribute__((section(".L2.bss"), aligned(16)));
complex_float twiddle_table[FFT_SIZE]__attribute__((section(".L2.data"), aligned(16)));
float sig_spec[WINDOW_COUNT][FFT_SIZE/2+1]__attribute__((section(".L2.bss"), aligned(16)));
float log_mel_spec[WINDOW_COUNT][MEL_RESOLUTION]__attribute__((section(".L2.bss"), aligned(16)));
float model_input[WINDOW_COUNT*DCT_COEFF]__attribute__((section(".L1.bss"), aligned(16)));
float scores[NUM_CLASSES]__attribute__((section(".L2.bss"), aligned(16)));
}  // namespace

// The name of this function is important for Arduino compatibility.
void kws_model_setup() {
  tflite::InitializeTarget();

  set_print_func(default_print);

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
#ifdef DO_QUANTIZED_INFERENCE
#ifdef USE_DS_CNN_SMALL_INT8_MODEL
  model = tflite::GetModel(g_ds_cnn_s_quantized_model_data);
#else
  model = tflite::GetModel(g_model);
#endif
#else
  model = tflite::GetModel(g_ds_cnn_l_fp32_model_data);
#endif
//  printf("Read in model weights");

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
  static tflite::MicroMutableOpResolver<7> micro_op_resolver;
  micro_op_resolver.AddConv2D();
  micro_op_resolver.AddDepthwiseConv2D();
  micro_op_resolver.AddRelu();
  micro_op_resolver.AddReshape();
  micro_op_resolver.AddFullyConnected();
  micro_op_resolver.AddAveragePool2D();
  micro_op_resolver.AddSoftmax();

  // Build an interpreter to run the model with.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    PRINT_INFO("AllocateTensors() failed");
    return;
  }

  // Get information about the memory area to use for the model's input.
  input = interpreter->input(0);

  //Open audio file
  if (kTfLiteOk != OpenInputFile()) {
    PRINT_INFO("\nFailed to read the input file. Check if the keyword_audio.bin is available in input folder.Refer keyword_spotter/Readme.\n");
    exit(0);
  }

  /* Initialize the twiddle table */
  generate_fft_twiddle_table(FFT_SIZE,(float *)twiddle_table);

  memset(g_audio_data_input,0,FFT_SIZE*sizeof(float));
}

// The name of this function is important for Arduino compatibility.
bool kws_model_run() {
    if(CheckFileOpen()){
#ifdef DO_CYCLE_COUNTS
  cycle_t pre_var = 0, pre_cyc=0; //Variables for cycle counting
  START_CYCLE_COUNT (pre_var);
#endif
        //Loop over number of windows
        for(int32_t nWindow=0; nWindow<WINDOW_COUNT;nWindow++){
          // Get audio frame from provider.
          if (kTfLiteOk != GetFrame(g_audio_data_input)) {
            PRINT_INFO("\nAudio capture failed.\n");
          }
          //do window operations + stft and pass to input
          //this is identical to the one done by librosa.stft
          //https://librosa.org/doc/main/generated/librosa.stft.html
          //we do a windowing, fftmag
          //our fft_magnitude does a normalization so we have to remove it
          /* Windowing */
          vecvmltf(g_audio_data_input, g_window, g_audio_windowed_data_input, FFT_SIZE);
          /* Calculate the FFT of a real signal */
          rfft ((const float32_t *)g_audio_windowed_data_input,(complex_float *)g_audio_complex_input,(const complex_float *)twiddle_table,1,FFT_SIZE);

          //convert complex to magnitude
          fft_magnitude( (complex_float *)g_audio_complex_input, g_stft_clip,FFT_SIZE,1);
          //Remove normalized
          vecsmltf(g_stft_clip,FFT_SIZE/2+1,g_stft_clip,FFT_SIZE/2+1);
          //Square STFT outputs
          vecvmltf(g_stft_clip,g_stft_clip,g_stft_clip,FFT_SIZE/2+1);

          memcpy(sig_spec[nWindow], g_stft_clip,(FFT_SIZE/2+1)*sizeof(float));

        }

        //Map frequency into Mel scale
        matmmltf(*sig_spec, WINDOW_COUNT, FFT_SIZE/2+1, *mel_weight_mat,MEL_RESOLUTION,*log_mel_spec);

        for(int i=0; i<WINDOW_COUNT; i++){
        //Take log of squared magnitude of STFT outputs
            veclogf(log_mel_spec[i], log_mel_spec[i], MEL_RESOLUTION);
        }
        //Apply DCT
        for (int i=0; i<WINDOW_COUNT;i++){
            custom_DCT(log_mel_spec[i]);
        }
        int ind =0;
        for(int i=0; i< WINDOW_COUNT;i++){
            for(int j=0; j< 10;j++){
                model_input[ind++] = log_mel_spec[i][j];
            }
        }
        //PREPROCESSING END
    //    PRINT_INFO("Preprocessing done");
#ifdef DO_CYCLE_COUNTS
    STOP_CYCLE_COUNT (pre_cyc, pre_var);
#endif

        //RUN MODEL
    #ifdef DO_CYCLE_COUNTS
        cycle_t var = 0, cyc=0; //Variables for cycle counting
        START_CYCLE_COUNT (var);
    #endif

    #ifdef DO_QUANTIZED_INFERENCE
      //Quantize inputs, convert float to int16
        for(int32_t i = 0;i < WINDOW_COUNT*DCT_COEFF;i++) {
            input->data.int8[i] = (int8)(model_input[i]/INPUT_SCALE + INPUT_ZERO_PT);
        }
    #else
        for(int32_t i = 0;i < WINDOW_COUNT*DCT_COEFF;i++) {
            input->data.f[i] = (float)model_input[i];
        }
    #endif

      // Run the model on this input and make sure it succeeds.
      if (kTfLiteOk != interpreter->Invoke()) {
        PRINT_INFO("\nInvoke failed.\n");
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

      for (int i = 1; i < NUM_CLASSES; ++i)
      {
          if (scores[imax] < scores[i])
              imax = i;
      }
      PRINT_INFO("\n\nDetected Class \" %s\" with score %f\n\n", classes_list[imax], scores[imax]);
      PRINT_INFO("");


    #ifdef DO_CYCLE_COUNTS
        STOP_CYCLE_COUNT (cyc, var);
    #endif


    #ifdef DO_CYCLE_COUNTS
        PRINT_INFO("\tNumber of cycles to run preprocessing: \t%lu \n", pre_cyc);
        PRINT_INFO("\tNumber of cycles to run inference: \t%lu \n", cyc);
        PRINT_INFO("\n");
    #endif
      return true;
    }
    else{
    	PRINT_INFO("\n Finished processing audio file\n");
        return false;
    }
}
