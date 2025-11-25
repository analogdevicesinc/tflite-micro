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

#include "adi_input_provider.h"

FILE *fpRead;
FILE *fpWrite;
int32_t g_latest_audio_timestamp;
float g_previous_frame_out[FRAME_SIZE];
float g_previous_window_out[FRAME_SIZE];

bool CheckFileOpen(){
	if (fpRead!=NULL){
		return true;
	}
	return false;
}

TfLiteStatus OpenInputFile() {
	fpRead = fopen("../src/input/keyword_audio.bin","rb");
	g_latest_audio_timestamp = 0;
	if (fpRead==NULL){
		return kTfLiteDelegateDataNotFound;
	}
	return kTfLiteOk;
}

TfLiteStatus GetFrame(float* image_data) {
    // Note: Read HOP_SIZE elements for each call, read FRAME_SIZE at the start
    if (fpRead != NULL) {
        if (g_latest_audio_timestamp == 0) {
            // First frame
            int nBuffSize = fread(image_data, sizeof(float), FFT_SIZE, fpRead);
            if (nBuffSize < FRAME_SIZE) {
                if (ferror(fpRead)) {
                    // Handle fread error
                    return kTfLiteError;
                }
                if (fclose(fpRead) != 0) {
                    // Handle fclose error
                    return kTfLiteError;
                }
                fpRead = NULL;
            }
            g_latest_audio_timestamp++;
        } else {
            // Subsequent frames: Copy previous data and read new data
            memcpy(image_data, image_data + HOP_SIZE, (FFT_SIZE - HOP_SIZE) * sizeof(float));
            int nBuffSize = fread(image_data + FFT_SIZE - HOP_SIZE, sizeof(float), HOP_SIZE, fpRead);
            if (nBuffSize < HOP_SIZE) {
                if (ferror(fpRead)) {
                    // Handle fread error
                    return kTfLiteError;
                }
                if (fclose(fpRead) != 0) {
                    // Handle fclose error
                    return kTfLiteError;
                }
                fpRead = NULL;
            }
            g_latest_audio_timestamp++;
        }
    }
    return kTfLiteOk;
}
