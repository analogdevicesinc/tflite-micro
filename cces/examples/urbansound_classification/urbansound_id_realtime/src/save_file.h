/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/
/*****************************************************************************
 * save_file.h
 *****************************************************************************/

#ifndef _SAVE_FILE_H_
#define _SAVE_FILE_H_

#include <cstdint>
#include <cstdio>
#include <string.h>
#include<cstdlib>

#define TIME_IN_SECS_TO_SAVE 8 		//seconds of data to file
#define BITRATE_16 16000 			//16KHz data
#define BITRATE_48 48000 			//48KHz data

#ifdef __cplusplus
extern "C" {
#endif

void save48KHzADCIntStereoBufferToFile(int* buffer, int32_t dataSize, const char* filename);
void save48KHzInputFloatMonoBufferToFile(float* buffer, int32_t dataSize, const char* filename);
void save16KHzDecimatedFloatMonoBufferToFile(float* buffer, int32_t dataSize, const char* filename);
void save48KHzDACIntStereoBufferToFile(int* buffer, int32_t dataSize, const char* filename);

#ifdef __cplusplus
}
#endif
#endif // _SAVE_FILE_H_
