/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/
/**
 * @file main.cpp
 * @brief Application entry point for the DFN denoiser realtime example.
 *
 * Initialises all hardware peripherals (power, UART, SPU, GPIO, TWI, codecs,
 * SPORT) then enters the main processing loop.  Each loop iteration either:
 *  - Runs one DFN inference hop (adi_denoiser_dfn_run) in denoising mode, or
 *  - Copies one hop of raw ADC audio directly to the DAC in passthrough mode.
 *
 * Push-button SW3 toggles between denoising and passthrough modes at runtime.
 */
#include <sys/platform.h>
#include "adi_initialize.h"
#include "adi_run_dtln.h"
#include "adi_sharcfx_init.h"

bool bPassThrough = false;
volatile bool bButtonPressed = false;

void gpioCallback(ADI_GPIO_PIN_INTERRUPT ePinInt, uint32_t Data, void *pCBParam)
{
	if (ePinInt == PUSH_BUTTON1_PINT)
	{
		if (Data & PUSH_BUTTON1_PINT_PIN)
		{
			bButtonPressed = true;
		}
	}
}

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

	/* Register GPIO interrupt callback for push button (SW3) */
	Result = adi_gpio_RegisterCallback(PUSH_BUTTON1_PINT, PUSH_BUTTON1_PINT_PIN, gpioCallback, (void*)0);
	if(Result != ADI_GPIO_SUCCESS)
	{
		PRINT_INFO("GPIO callback registration failed \n");
	}

	/* Configure PINT for rising edge on SW3 */
	Result = adi_gpio_PinInt(PUSH_BUTTON1_PIN_ASSIGN, PUSH_BUTTON1_PINT_PIN, PUSH_BUTTON1_PINT,
	                         PUSH_BUTTON1_PIN_ASSIGN_BYTE, true, ADI_GPIO_SENSE_RISING_EDGE);
	if(Result != ADI_GPIO_SUCCESS)
	{
		PRINT_INFO("GPIO PinInt configuration failed \n");
	}

	/* Configure Port Pin PC_01 as output for LED blink */
	Result = adi_gpio_SetDirection(ADI_GPIO_PORT_C, ADI_GPIO_PIN_1, ADI_GPIO_DIRECTION_OUTPUT);
	if(Result != ADI_GPIO_SUCCESS)
	{
		PRINT_INFO("GPIO Initialization failed \n");
	}
	//TFLM model setup
	adi_dtln_model_setup();

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

	/* Passthrough disabled initially, toggle LED to indicate denoising mode */
	adi_gpio_Toggle(ADI_GPIO_PORT_C, ADI_GPIO_PIN_1);
	bPassThrough = false;
	/* Main processing loop */
	while(1) {
		/* Check flag set by SW3 interrupt to toggle passthrough/denoising mode */
		if (bButtonPressed)
		{
			bButtonPressed = false;
			bPassThrough = !bPassThrough;
			PRINT_INFO(bPassThrough ? "Mode: PASSTHROUGH\n" : "Mode: DENOISING\n");
			adi_gpio_Toggle(ADI_GPIO_PORT_C, ADI_GPIO_PIN_1);
		}

		if(bPassThrough == false) {
			//process next frame through denoiser
			if(pProcessReadPtr < pReadPtr) {
				adi_dtln_model_run();
			}
		} else {
			/* Passthrough mode: copy one hop of raw ADC input directly to DAC output */
			if(pReadPtr > pProcessReadPtr) {
				int nReadLoc  = (pProcessReadPtr)  % NUM_HOPS;
				int nWriteLoc = (pProcessWritePtr) % NUM_HOPS;
				int *pSrc = g_audio_data_input  + nReadLoc  * AUDIO_COUNT;
				int *pDst = g_audio_data_output + nWriteLoc * AUDIO_COUNT;
				memcpy(pDst, pSrc, AUDIO_COUNT * sizeof(int));
				pProcessReadPtr++;
				pProcessWritePtr++;
			}
		}
	}
}

