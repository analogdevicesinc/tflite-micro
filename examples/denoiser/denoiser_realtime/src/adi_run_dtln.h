/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/
/*****************************************************************************
 * adi_run_dtln.h
 *****************************************************************************/

#ifndef _ADI_RUN_DTLN_H_
#define _ADI_RUN_DTLN_H_

#include "debug.h"
#include "adi_sharcfx_init.h"
#define FRAME_SIZE 512
#define HOP_SIZE   128
#define FFT_SIZE   512
#define NUM_HOPS    10


#ifdef __cplusplus
extern "C" {
#endif

// Initializes all data needed for the example. The name is important, and needs
// to be setup() for Arduino compatibility.
void adi_dtln_model_setup();

// Runs one iteration of data gathering and inference. This should be called
// repeatedly from the application code. The name needs to be loop() for Arduino
// compatibility.
bool adi_dtln_model_run();

#ifdef __cplusplus
}
#endif

#endif  // _ADI_RUN_DTLN_H_
