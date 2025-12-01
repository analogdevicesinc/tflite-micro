/*******************************************************************************
 Copyright(c) 2024 Analog Devices, Inc. All Rights Reserved. This software is
 proprietary & confidential to Analog Devices, Inc. and its licensors. By using
 this software you agree to the terms of the associated Analog Devices License
 Agreement.
*******************************************************************************/


#ifndef _ADI_INPUT_PROVIDER_H_
#define _ADI_INPUT_PROVIDER_H_

#include "tensorflow/lite/c/common.h"

#define FRAME_SIZE 512
#define HOP_SIZE   128
#define FFT_SIZE   512
#define NUM_HOPS    10

TfLiteStatus OpenInputFile();
TfLiteStatus OpenOutputFile();
TfLiteStatus GetFrame(float* image_data,float *previous_frame, int *pReadPtr);
TfLiteStatus WriteFrame(float* image_data,int *pWritePtr, int nProcVal);

#endif  // _ADI_INPUT_PROVIDER_H_
