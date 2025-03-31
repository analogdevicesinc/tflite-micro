/*******************************************************************************
 Copyright(c) 2024 Analog Devices, Inc. All Rights Reserved. This software is
 proprietary & confidential to Analog Devices, Inc. and its licensors. By using
 this software you agree to the terms of the associated Analog Devices License
 Agreement.
*******************************************************************************/


#ifndef _ADI_INPUT_PROVIDER_H_
#define _ADI_INPUT_PROVIDER_H_

#include "tensorflow/lite/c/common.h"
///////////////// samplerate 22050
#define FRAME_SIZE 2048
#define HOP_SIZE   512
#define FFT_SIZE   2048
#define WINDOW_COUNT 10
#define MEL_RESOLUTION 128 // Number of the Mel filterbanks used
#define WINDOWS_PER_FRAME 128 //number derived from how long the model takes 400MMAC and how many
//windows we get in that time @22050 (22 windows)
#define NUM_HOPS 10
#define NUM_CLASSES 10

TfLiteStatus OpenInputFile();
TfLiteStatus OpenOutputFile();
bool CheckFileOpen();
TfLiteStatus GetFrame(float* image_data, int *pReadPtr);
TfLiteStatus WriteFrame(float* image_data, float* window);

#endif  // _ADI_INPUT_PROVIDER_H_
