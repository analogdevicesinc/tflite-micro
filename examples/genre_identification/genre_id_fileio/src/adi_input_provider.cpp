/*******************************************************************************
 Copyright(c) 2024 Analog Devices, Inc. All Rights Reserved. This software is
 proprietary & confidential to Analog Devices, Inc. and its licensors. By using
 this software you agree to the terms of the associated Analog Devices License
 Agreement.
*******************************************************************************/


#include "adi_input_provider.h"

#include <fstream>

FILE *fpRead;
FILE *fpWrite;
int32_t g_latest_audio_timestamp;

bool CheckFileOpen(){
	if (fpRead!=NULL){
		return true;
	}
	return false;
}
TfLiteStatus OpenInputFile() {
	fpRead = fopen("../src/input/genre_audio.bin","rb");
	g_latest_audio_timestamp = 0;
	if (fpRead==NULL){
		return kTfLiteDelegateDataNotFound;
	}
	return kTfLiteOk;
}

TfLiteStatus GetFrame(float* image_data, int *pReadPtr) {
    // Note: Read HOP_SIZE elements for each call, read FRAME_SIZE at the start
    if (fpRead != NULL) {
        // Read new frame into long circular buffer
        int nReadLocation = (*pReadPtr) % (NUM_HOPS * WINDOWS_PER_FRAME); // Circular buffer loopback to beginning
        int nBuffSize = fread(image_data + nReadLocation * HOP_SIZE, sizeof(float), HOP_SIZE, fpRead);
        
        if (nBuffSize < HOP_SIZE) {
            // If fewer elements are read, pad the remaining buffer with zeros
            memset(image_data + nReadLocation * HOP_SIZE + nBuffSize, 0, (HOP_SIZE - nBuffSize) * sizeof(float));
            
            if (ferror(fpRead)) {
                // Handle fread error
                return kTfLiteError;
            }

            // Close file and reset fpRead to NULL
            if (fclose(fpRead) != 0) {
                // Handle fclose error (optional, rare case)
                return kTfLiteError;
            }
            fpRead = NULL;
			return kTfLiteCancelled;
        }
        // Update the read pointer only when data is read
        *pReadPtr += 1;
    } else {
		return kTfLiteError;
	}
    return kTfLiteOk;
}

