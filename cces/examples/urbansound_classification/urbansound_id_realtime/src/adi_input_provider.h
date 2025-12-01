/*******************************************************************************
 Copyright(c) 2024 Analog Devices, Inc. All Rights Reserved. This software is
 proprietary & confidential to Analog Devices, Inc. and its licensors. By using
 this software you agree to the terms of the associated Analog Devices License
 Agreement.
*******************************************************************************/


#ifndef _ADI_INPUT_PROVIDER_H_
#define _ADI_INPUT_PROVIDER_H_

#include "tensorflow/lite/c/common.h"

TfLiteStatus OpenInputFile();
TfLiteStatus OpenOutputFile();
bool CheckFileOpen();
TfLiteStatus GetFrame(float* image_data, int *pReadPtr);
TfLiteStatus WriteFrame(float* image_data, float* window);

#endif  // _ADI_INPUT_PROVIDER_H_