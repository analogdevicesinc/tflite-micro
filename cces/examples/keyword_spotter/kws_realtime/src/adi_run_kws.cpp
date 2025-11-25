/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/
/*****************************************************************************
 * adi_run_kws.cpp
 *****************************************************************************/

#include <stdint.h>

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
#endif

#include "preprocessing_utils.h"
#include "adi_run_kws.h"

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



volatile int pReadPtr;
int pProcessReadPtr;
volatile int pWritePtr;
int pProcessWritePtr;

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


//*2 extra for stereo, *3 extra for 48KHz decimated to 16KHz
int g_audio_data_input[NUM_HOPS*AUDIO_COUNT]__attribute__((section(".L3.data"), aligned(8)));//interface buffer to get data in
int g_audio_data_output[NUM_HOPS*AUDIO_COUNT]__attribute__((section(".L3.data"), aligned(8)));//interface buffer to pass data out
float g_process_input[FFT_SIZE]__attribute__((section(".L3.data"), aligned(8)));//this holds the current input data

fir_statef decimation_filter;
fir_statef interpolation_filter;

//Decimation and interpolation coeffs are preocomputed for a Fs/3 decimation and Fs*3 interpolation of a signal.
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
float g_decimation_buffer[HOP_SIZE*3];//__attribute__((section(".L3.data"), aligned(8)));//interface buffer to get data in;
float g_interpolation_buffer[HOP_SIZE*3];//__attribute__((section(".L3.data"), aligned(8)));//interface buffer to get data in;


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
constexpr int kTensorArenaSize = (180) * 1024;
static uint8_t tensor_arena[kTensorArenaSize]__attribute__((section(".L2.data"), aligned(8)));
//static uint8_t tensor_arena[kTensorArenaSize];

//For audio file read
//float g_audio_data_input[FFT_SIZE];
float g_audio_windowed_data_input[FFT_SIZE];
complex_float g_audio_complex_input[FFT_SIZE];
float g_stft_clip[FFT_SIZE/2+1];
complex_float twiddle_table[FFT_SIZE]__attribute__((section(".L2.bss"), aligned(16)));
float sig_spec[WINDOW_COUNT*(FFT_SIZE/2+1)];
float log_mel_spec[WINDOW_COUNT][MEL_RESOLUTION];
float model_input[WINDOW_COUNT*DCT_COEFF];
float scores[NUM_CLASSES];
}  // namespace

// The name of this function is important for Arduino compatibility.
void kws_model_setup() {
  tflite::InitializeTarget();

  #ifdef UART_REDIRECT
    set_print_func(uart_print);
  #else
    set_print_func(default_print);
  #endif

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
#ifdef DO_QUANTIZED_INFERENCE
#ifdef USE_DS_CNN_SMALL_INT8_MODEL
  model = tflite::GetModel(g_ds_cnn_s_quantized_model_data);
#else
  model = tflite::GetModel(g_model);
#endif
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

  /* Initialize the twiddle table */
  generate_fft_twiddle_table(FFT_SIZE,(float *)twiddle_table);

//  memset(g_audio_data_input,0,FFT_SIZE*sizeof(float));

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
//  printf("setup Done");
}

static uint32_t count =0;
static int nms[50]={0};
// The name of this function is important for Arduino compatibility.
bool kws_model_run() {
    if(pReadPtr < pProcessReadPtr)
        return true;
	#ifdef DO_CYCLE_COUNTS
      cycle_t pre_var = 0, pre_cyc=0; //Variables for cycle counting
      START_CYCLE_COUNT (pre_var);
    #endif
    if(pReadPtr > pProcessReadPtr) {
        //Ensure we have enough windows to process
        int nProcLocation = (pProcessReadPtr) % (NUM_HOPS);//circular buffer loopback to beginning
        int *pIntPtr = g_audio_data_input+nProcLocation*AUDIO_COUNT;
        // for (int nWindow = 0;nWindow < WINDOW_COUNT;nWindow++) {
        //slide previous samples out and read new samples
        memcpy(g_process_input,g_process_input+HOP_SIZE,(FFT_SIZE-HOP_SIZE)*sizeof(float));
        //read new samples
        float *pFloatPtr = g_decimation_buffer;//128 samples of channel 1
        for(int i = 0;i < AUDIO_COUNT;i+=2){//combining stereo to mono
          float nNoiseVal = (float)(*pIntPtr++/(float)(SCALE_FACTOR));//24 bit adc = pow(2,24)
          float nSignalVal = (float)(*pIntPtr++/(float)(SCALE_FACTOR));//24 bit adc = pow(2,24)
          *pFloatPtr++ = nSignalVal + nNoiseVal;//combine signal with noise
        }
        pProcessReadPtr += 1;//updated process buffer location
        pIntPtr += AUDIO_COUNT;
        //decimate inputs
        float *pOutPtr = g_process_input+(FFT_SIZE-HOP_SIZE);//320 samples of channel 1
        fir_decimaf( (const float32_t *)g_decimation_buffer, pOutPtr, HOP_SIZE*3, &decimation_filter);


        //do window operations + stft and pass to input
        //this is identical to the one done by librosa.stft
        //https://librosa.org/doc/main/generated/librosa.stft.html
        //we do a windowing, fftmag
        //our fft_magnitude does a normalization so we have to remove it
        /* Windowing */
        vecvmltf((const float32_t *)g_process_input, (const float32_t *)g_window, (float32_t *)g_audio_windowed_data_input, FFT_SIZE);
        /* Calculate the FFT of a real signal */
        rfft ((const float32_t *)g_audio_windowed_data_input,(complex_float *)g_audio_complex_input,(const complex_float *)twiddle_table,1,FFT_SIZE);

        //convert complex to magnitude
        fft_magnitude( (complex_float *)g_audio_complex_input, g_stft_clip,FFT_SIZE,1);
        //Remove normalization and revert to normal scales.
        //After normalization, the length of the FFT sequence gets reduced to (FFT_SIZE/2+1)
        vecsmltf(g_stft_clip,FFT_SIZE/2+1,g_stft_clip,FFT_SIZE/2+1);

        //Square STFT outputs
        vecvmltf(g_stft_clip,g_stft_clip,g_stft_clip,FFT_SIZE/2+1);

        // memcpy(sig_spec[nWindow], g_stft_clip,(FFT_SIZE/2+1)*sizeof(float));
        // }
        memcpy(sig_spec, sig_spec+FFT_SIZE/2+1,(WINDOW_COUNT-1)*(FFT_SIZE/2+1)*sizeof(float));
        memcpy(sig_spec+(WINDOW_COUNT-1)*(FFT_SIZE/2+1), g_stft_clip, (FFT_SIZE/2+1)*sizeof(float));
        //Map frequency into Mel scale
        matmmltf(sig_spec, WINDOW_COUNT, FFT_SIZE/2+1, *mel_weight_mat,MEL_RESOLUTION,*log_mel_spec);

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
            for(int j=0; j< DCT_COEFF;j++){
                model_input[ind++] = log_mel_spec[i][j];
            }
        }
    
      //Quantize inputs, convert float to int16
        for(int32_t i = 0;i < WINDOW_COUNT*DCT_COEFF;i++) {
            input->data.int8[i] = (int8)(model_input[i]/INPUT_SCALE + INPUT_ZERO_PT);
        }

	
	#ifdef DO_CYCLE_COUNTS
        STOP_CYCLE_COUNT (pre_cyc, pre_var);

	//RUN MODEL

        cycle_t var = 0, cyc=0; //Variables for cycle counting
        START_CYCLE_COUNT (var);
    #endif

      // Run the model on this input and make sure it succeeds.
      if (kTfLiteOk != interpreter->Invoke()) {
        PRINT_INFO("Invoke failed.");
      }

      TfLiteTensor* output = interpreter->output(0);

      for(int i=0; i<NUM_CLASSES; i++){
         scores[i] = (float)(OUTPUT_SCALE * ((float)output->data.int8[i] - OUTPUT_ZERO_PT));
      }

      int imax = 0;
      for (int i = 1; i < NUM_CLASSES; ++i)
      {
          if (scores[imax] < scores[i])
              imax = i;
      }

	  if (scores[imax]>DETECTION_THRESHOLD)
	      nms[count++] = imax;

	  if (count>EVAL_WINDOW_SIZE)
	  {
	      count=0;
	      int count_array[NUM_CLASSES]={0};
	      for(int i=0; i<EVAL_WINDOW_SIZE; i++)
	          count_array[nms[i]]++;

	      int imax2 = 1;//Ignore "Silence"(index 0) class.
	      for (int i = 2; i < NUM_CLASSES; ++i)
	      {
	          if (count_array[imax2] < count_array[i])
	              imax2 = i;
	      }
	      if(count_array[imax2]>OVERLAP_WINDOW_SIZE)
	          PRINT_INFO("Detected Class \" %s\" with count %d\n", classes_list[imax2], count_array[imax2]);

	  }


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
    return true;
}
