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

#ifndef TENSORFLOW_LITE_MICRO_EXAMPLES_PERSON_DETECTION_IMAGE_PROVIDER_H_
#define TENSORFLOW_LITE_MICRO_EXAMPLES_PERSON_DETECTION_IMAGE_PROVIDER_H_

#include "tensorflow/lite/c/common.h"

#define FRAME_SIZE 512
#define HOP_SIZE   128
#define FFT_SIZE   512
#define NUM_HOPS    10

TfLiteStatus OpenInputFile();
TfLiteStatus OpenOutputFile();
TfLiteStatus GetFrame(float* image_data,float *previous_frame, int *pReadPtr);
TfLiteStatus WriteFrame(float* image_data,int *pWritePtr, int nProcVal);

#endif  // TENSORFLOW_LITE_MICRO_EXAMPLES_PERSON_DETECTION_IMAGE_PROVIDER_H_
