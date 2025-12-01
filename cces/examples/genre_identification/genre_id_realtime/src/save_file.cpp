/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/
/*****************************************************************************
 * save_file.cpp
 *****************************************************************************/

#include "save_file.h"
#include "debug.h"
int8_t L3Buf[(2 * BITRATE_48 * TIME_IN_SECS_TO_SAVE * sizeof(int))]__attribute__((section(".L3.data"), aligned(16)));			/*float L3 buffer 10 sec*/
uint32_t nSampleCount = 0;

void save48KHzADCIntStereoBufferToFile(int* buffer, int32_t dataSize, const char* filename) {
	int* pL3Buf = (int*)L3Buf;
	memcpy(pL3Buf + nSampleCount, buffer, dataSize * sizeof(int));/* Copy only the new 2 * 3 * HOP_SIZE data */
	nSampleCount += dataSize;
	int32_t nTotalSize = 2*BITRATE_48*TIME_IN_SECS_TO_SAVE;//210*dataSize;//2*BITRATE_48*TIME_IN_SECS_TO_SAVE;

	if(nSampleCount == nTotalSize){
		FILE* fp = fopen(filename, "wb");  // Open in binary mode
		if (fp) {
			fwrite(L3Buf, sizeof(int), nTotalSize, fp);
			fclose(fp);
			exit(0);
		}
	}
}

void save48KHzInputFloatMonoBufferToFile(float* buffer, int32_t dataSize, const char* filename) {
	float* pL3Buf = (float*)L3Buf;
	memcpy(pL3Buf + nSampleCount, buffer, dataSize *sizeof(float));/* Copy only the new 3 * HOP_SIZE data */
	nSampleCount += dataSize;
	int32_t nTotalSize = 210*dataSize; //BITRATE_48*TIME_IN_SECS_TO_SAVE;
//	PRINT_INFO("nSampleCount \n %d", nSampleCount);
	if(nSampleCount == nTotalSize){
		FILE* fp = fopen(filename, "wb");  // Open in binary mode
		if (fp) {
			fwrite(L3Buf, sizeof(float), nTotalSize, fp);
			fclose(fp);
			exit(0);
		}
	}
}

void save16KHzDecimatedFloatMonoBufferToFile(float* buffer, int32_t dataSize, const char* filename) {
	float* pL3Buf = (float*)L3Buf;
	memcpy(pL3Buf + nSampleCount, buffer, dataSize * sizeof(float));/* Copy only the new HOP_SIZE data */
	nSampleCount += dataSize;
	int32_t nTotalSize = BITRATE_16 * TIME_IN_SECS_TO_SAVE;

	if(nSampleCount == nTotalSize){
		FILE* fp = fopen(filename, "wb");  // Open in binary mode
		if (fp) {
			fwrite(L3Buf, sizeof(float), nTotalSize, fp);
			fclose(fp);
			exit(0);
		}
	}
}
void save48KHzDACIntStereoBufferToFile(int* buffer, int32_t dataSize, const char* filename) {
	int* pL3Buf = (int*)L3Buf;
	memcpy(pL3Buf + nSampleCount, buffer, dataSize * sizeof(int));/* Copy only the new 2 * 3 * HOP_SIZE data */
	nSampleCount += dataSize;
	int32_t nTotalSize = 2 * BITRATE_48 * TIME_IN_SECS_TO_SAVE;

	if(nSampleCount == nTotalSize){
		FILE* fp = fopen(filename, "wb");  // Open in binary mode
		if (fp) {
			fwrite(L3Buf, sizeof(int), nTotalSize, fp);
			fclose(fp);
			exit(0);
		}
	}
}