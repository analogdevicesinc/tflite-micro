/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/
/*****************************************************************************
 * adi_run_kws.h
 *****************************************************************************/
#ifndef _ADI_RUN_KWS_H_
#define _ADI_RUN_KWS_H_
#include "debug.h"
#include "adi_sharcfx_init.h"
#define FRAME_SIZE 640         //40ms @ 16k SR
#define HOP_SIZE   320         //20ms @ 16k SR
#define FFT_SIZE   1024
#define DCT_SIZE   40
#define WINDOW_COUNT 49        // Number if windows processed together
#define MEL_RESOLUTION 40      // Number of the Mel filterbanks used
#define DCT_COEFF  10
#define NUM_CLASSES 12


//Definitions for taking in realtime input
#define NUM_HOPS    10

#define DETECTION_THRESHOLD 0.8
#define EVAL_WINDOW_SIZE 42
#define OVERLAP_WINDOW_SIZE 21

#ifdef __cplusplus
extern "C" {
#endif
// Initializes all data needed for the example.
void kws_model_setup();

// Runs one iteration of data gathering and inference. This should be called
// repeatedly from the application code. 
bool kws_model_run();

#ifdef __cplusplus
}
#endif

#endif  // _ADI_RUN_KWS_H_
