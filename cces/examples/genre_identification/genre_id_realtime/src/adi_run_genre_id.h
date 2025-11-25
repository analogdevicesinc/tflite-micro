/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/
/*****************************************************************************
 * adi_run_genre_id.h
 *****************************************************************************/
#ifndef _ADI_RUN_GENRE_ID_H_
#define _ADI_RUN_GENRE_ID_H_
#include "debug.h"
#ifdef __cplusplus
extern "C" {
#endif

///////////////// samplerate 48000
#define FRAME_SIZE 4096
#define HOP_SIZE   1024
#define FFT_SIZE   4096
#define MEL_RESOLUTION 128 		// Number of the Mel filterbanks used
#define WINDOWS_PER_FRAME 128 	//Model works with 2.7secs of input audio which makes 128*128 melspectorgam
#define NUM_HOPS 3 				//3 inference data we can save in circular buffer enough data for seamless execution
#define NUM_CLASSES 6			// The genre identification model trained for 6 classes

void adi_genre_id_model_setup();


void adi_genre_id_model_run();

#ifdef __cplusplus
}
#endif

#endif  // _ADI_RUN_GENRE_ID_H_