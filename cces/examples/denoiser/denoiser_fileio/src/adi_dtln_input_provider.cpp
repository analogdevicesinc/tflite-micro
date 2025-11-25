/*******************************************************************************
 Copyright(c) 2024 Analog Devices, Inc. All Rights Reserved. This software is
 proprietary & confidential to Analog Devices, Inc. and its licensors. By using
 this software you agree to the terms of the associated Analog Devices License
 Agreement.
*******************************************************************************/


#include "adi_dtln_input_provider.h"

#include <fstream>

FILE *fpRead;
FILE *fpWrite;

TfLiteStatus OpenInputFile() {
	fpRead = fopen("../src/input/noisy_audio.bin","rb");
	if (fpRead==NULL){
		return kTfLiteDelegateDataNotFound;
	}
	return kTfLiteOk;
}

TfLiteStatus OpenOutputFile() {
	fpWrite = fopen("../src/output/denoised_audio.bin","wb");
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
