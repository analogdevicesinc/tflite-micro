/*******************************************************************************
 Copyright(c) 2024 Analog Devices, Inc. All Rights Reserved. This software is
 proprietary & confidential to Analog Devices, Inc. and its licensors. By using
 this software you agree to the terms of the associated Analog Devices License
 Agreement.
*******************************************************************************/


/*============= I N C L U D E S =============*/
#include <sys/platform.h>
#include "adi_initialize.h"
#include "adi_run_dtln.h"

/*============= D E F I N E S =============*/
#define USE_DTLN_MODEL

/*============= C O D E =============*/
int main() {
	/**
	 * Initialize managed drivers and/or services that have been added to
	 * the project.
	 * @return zero on success
	 */
	adi_initComponents();
#ifdef USE_DTLN_MODEL
	adi_dtln_model_setup();
	while (adi_dtln_model_run() == true);//keep processing until we get new frames
#endif //USE_DTLN_MODEL

	return 0;
}
