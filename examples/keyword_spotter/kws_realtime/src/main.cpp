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
#include "adi_sharcfx_init.h"

/*
 * Main function
 */
int main(int argc, char *argv[])
{
	/**
	 * Initialize managed drivers and/or services that have been added to
	 * the project.
	 * @return zero on success
	 */
	uint32_t Result=0;

	adi_initComponents();

	/* Initialize the power service to CLKIN=25MHz. This API is called so that we can use other low level APIs to
	 * modify individual clocks. adi_pwr_SetFreq() is not called and the default CGU0 settings done in init_code
	 * are used
	 * (For ADSP-21593 - CCLK: 1000 MHz , SYSCLK: 500 MHz) */
	adi_pwr_Init(CGU_DEV, CLKIN);


	#ifdef UART_REDIRECT
		/* Initialize the TWI pin mux for any of the soft config access through TWI1 (SOM board) and TWI2 (EZLITE board) */
		Init_Twi1PinMux();
		Init_Twi2PinMux();
	#endif

	#if defined(__ADSP218xx__)
		SoftConfig_EV_21835_SOM();
	#elif defined(__ADSPSC8xx__)
		SoftConfig_EV_SC835_SOM();
	#endif

	#ifdef UART_REDIRECT
		// Initialize UART
		if (Init_UART() != PASSED)
		{
			PRINT_INFO("Could not Initialize \n");
			return FAILED;
		}
	#endif

    /* SPU initialization */
	if (Result==0u)
	{
		Result=SPU_init();
	}

    /* Switch Configuration */
	Switch_Configurator();

	/*Configure the GPIO as output for PB1 and PB2*/
	Result = adi_gpio_PortInit(PUSH_BUTTON1_PORT,PUSH_BUTTON1_PIN,ADI_GPIO_DIRECTION_INPUT,true);
	if(Result != ADI_GPIO_SUCCESS)
	{
		PRINT_INFO("GPIO Initialization failed \n");
	}

	uint32_t gpiocallbacks;
	Result = adi_gpio_Init((void*)gpioMemory,GPIO_MEMORY_SIZE,&gpiocallbacks);
	if(Result != ADI_GPIO_SUCCESS)
	{
		PRINT_INFO("GPIO Initialization failed \n");
	}

	/*Configure the Port Pin PC_01 as output for LED blink*/
	Result = adi_gpio_SetDirection(ADI_GPIO_PORT_C,ADI_GPIO_PIN_1, ADI_GPIO_DIRECTION_OUTPUT);
	if(Result!= ADI_GPIO_SUCCESS)
	{
		PRINT_INFO("GPIO Initialization failed \n");
	}

	/* SRU Configuration */
	SRU_Init();

	/* TWI Initialization */
	if (Result==0u)
	{
		Result=Init_TWI();
	}

	/* ADAU1962 Initialization */
	if (Result==0u)
	{
		Result=ADAU_1962_init();
	}

	/* ADAU1979 Initialization */
	if (Result==0u)
	{
		Result=ADAU_1979_init();
	}

	/* SPORT Initialization */
	if (Result==0u)
	{
		Result=Sport_Init();
	}

	/* Close TWI */
	if (Result==0u)
	{
		Result=Stop_TWI();
	}

	//TFLM model setup
	kws_model_setup();

	while(1){
		if(pProcessReadPtr < pReadPtr) {
			kws_model_run();
		}
	}
}
