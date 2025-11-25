/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/
/*****************************************************************************
 * main.cpp
 *****************************************************************************/
#include <sys/platform.h>
#include "adi_initialize.h"
#include "adi_run_kws.h"

int main() {
	/**
	 * Initialize managed drivers and/or services that have been added to
	 * the project.
	 * @return zero on success
	 */
	adi_initComponents();
	kws_model_setup();
	while (kws_model_run()==true);
	return 0;
}
