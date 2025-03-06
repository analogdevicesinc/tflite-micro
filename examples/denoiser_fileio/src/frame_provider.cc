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

#include "frame_provider.h"

#include <fstream>

FILE *fpRead;
FILE *fpWrite;

TfLiteStatus OpenInputFile() {
	fpRead = fopen("../src/input/audio_in.bin","rb");
	if (fpRead==NULL){
		return kTfLiteDelegateDataNotFound;
	}
	return kTfLiteOk;
}

TfLiteStatus OpenOutputFile() {
	fpWrite = fopen("../audio_out.bin","wb");
	if(fpWrite){
		return kTfLiteOk;
	}
	return kTfLiteError;
}

TfLiteStatus GetFrame(float* image_data, float *previous_frame, int *pReadPtr) {
    // Note: Read HOP_SIZE elements for each call, read FRAME_SIZE at the start
    if (fpRead != NULL) {
        // Copy previous data to start, then read remaining
        int nReadLocation = (*pReadPtr) % NUM_HOPS; // Circular buffer loopback to beginning
        int nBuffSize = fread(image_data + (nReadLocation) * HOP_SIZE, sizeof(float), HOP_SIZE, fpRead);
        
        if (nBuffSize < HOP_SIZE) {
            // Write remaining elements in previous_frame buffer
            if (fpWrite != NULL) {
                int writtenSize = fwrite(previous_frame, sizeof(float), FRAME_SIZE - HOP_SIZE, fpWrite);
                if (writtenSize < FRAME_SIZE - HOP_SIZE) {
                    // Handle write error
                    return kTfLiteError;
                }
            } else {
                return kTfLiteError;
            }
            fclose(fpRead);
            fclose(fpWrite);
            fpRead = NULL;
            fpWrite = NULL;
            return kTfLiteCancelled;
        }
        *pReadPtr += 1; // Update read buffer location
    } else {
		return kTfLiteError;
	}
    return kTfLiteOk;
}

TfLiteStatus WriteFrame(float* image_data,int *pWritePtr, int nProcVal) {
	//Note write HOP_SIZE elements for each call, write FRAME_SIZE at the end
	if(*pWritePtr >= nProcVal) {
		//no new samples to write
		return kTfLiteOk;
	}
	if (fpWrite != NULL) {
	  int nWriteLocation = (*pWritePtr) % NUM_HOPS;//circular buffer loopback to beginning
	  int nBuffSize = fwrite(image_data+(nWriteLocation)*HOP_SIZE, sizeof(float), HOP_SIZE, fpWrite);
	  *pWritePtr += 1;
	}
	return kTfLiteOk;
}
