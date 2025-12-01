/*******************************************************************************
 Copyright(c) 2024 Analog Devices, Inc. All Rights Reserved. This software is
 proprietary & confidential to Analog Devices, Inc. and its licensors. By using
 this software you agree to the terms of the associated Analog Devices License
 Agreement.
*******************************************************************************/
/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef _ADI_INPUT_PROVIDER_H_
#define _ADI_INPUT_PROVIDER_H_

#include "tensorflow/lite/c/common.h"
#include <fstream>

#define FRAME_SIZE 640 //40ms @ 16k SR
#define HOP_SIZE   320 //20ms @ 16k SR
#define FFT_SIZE   1024
#define DCT_SIZE   40 //Only 40*4 needed but Must be power of 2
#define WINDOW_COUNT 49

#define MEL_RESOLUTION 40 // Number of the Mel filterbanks used
#define DCT_COEFF  10

#define NUM_CLASSES 12

TfLiteStatus OpenInputFile();
TfLiteStatus OpenOutputFile();
bool CheckFileOpen();
TfLiteStatus GetFrame(float* image_data);
TfLiteStatus WriteFrame(float* image_data, float* window);

#endif  // _ADI_INPUT_PROVIDER_H_
