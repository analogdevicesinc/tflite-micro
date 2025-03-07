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
	fpRead = fopen("../src/audio.bin","rb");
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

TfLiteStatus GetFrame(float* image_data,float* previous_frame) {
	//Note read HOP_SIZE elements for each call, read FRAME_SIZE at the start
	if (fpRead != NULL) {
	  //copy previous data to start, then read remaining
	  memcpy(image_data,image_data+HOP_SIZE,(FRAME_SIZE-HOP_SIZE)*sizeof(float));
	  int nBuffSize = fread(image_data+FRAME_SIZE-HOP_SIZE, sizeof(float), HOP_SIZE, fpRead);
	  if(nBuffSize < HOP_SIZE){
		  //write remaining elements in previous_frame buffer
		  int nBuffSize = fwrite(previous_frame, sizeof(float), FRAME_SIZE-HOP_SIZE, fpWrite);
		  fclose(fpRead);
		  fclose(fpWrite);
		  fpRead = NULL;
		  fpWrite = NULL;
		  return kTfLiteCancelled;
	  }
	}
	return kTfLiteOk;
}

TfLiteStatus WriteFrame(float* image_data,float *previous_frame) {
	//Note write HOP_SIZE elements for each call, write FRAME_SIZE at the end
	if (fpWrite != NULL) {
	  //next frames, add and overlap
	  for(int i = 0;i < FRAME_SIZE;i++) {
		  previous_frame[i] += image_data[i];
	  }

	  int nBuffSize = fwrite(previous_frame, sizeof(float), HOP_SIZE, fpWrite);
	  //shift out frame
	  //discard HOP_SIZE samples
	  memcpy(previous_frame,previous_frame+HOP_SIZE,(FRAME_SIZE-HOP_SIZE)*sizeof(float));
	  memset(previous_frame+(FRAME_SIZE-HOP_SIZE),0,(HOP_SIZE)*sizeof(float));
	}
	return kTfLiteOk;
}
