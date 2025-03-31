/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/
/*****************************************************************************
 * uart_redirection.c
 *****************************************************************************/
#include <stdint.h>
#include "debug.h"
/* if we are using the UART to print debug info, define the following */
char UART_DEBUG_BUFFER[UART_DEBUG_BUFFER_LINE_SIZE]__attribute__((section(".L1.data"), aligned(16)));

/* Length of Tx and Rx buffers */
#define BUFFER_LENGTH     100

/* Set prescaler and divisor value */
bool Edb0 = 1u;

/* Divisor value set such that Baud rate is set to 9600 */
uint32_t Divisor = 13020;//11718;

extern int32_t uart_pinmux(void);
#if defined (__ADSP21835__)
void SoftConfig_EV_21835_SOM_UART(void);
#elif defined (__ADSPSC835__)
void SoftConfig_EV_SC835_SOM_UART(void);
#endif

/* UART Handle */
ADI_UART_HANDLE  ghUART = NULL;


/* Memory required for operating UART in interrupt mode */
uint8_t  gUARTMemory[ADI_UART_BIDIR_MEMORY_SIZE];

/* test setup */
static int test_setup(void)
{
	uart_pinmux();
	/* soft switches */
#if defined (__ADSP21835__)
	SoftConfig_EV_21835_SOM_UART();
#elif defined (__ADSPSC8xx__)
	SoftConfig_EV_SC835_SOM_UART();
#endif

	return PASSED;
}


/*
 *   Function:    Init_UART
 *   Description: Initialize the UART.
 */
int32_t Init_UART(void)
{

	/* UART return code */
	ADI_UART_RESULT    eResult = ADI_UART_FAILURE;

	/* setup pinmux and softconfig */
	test_setup();

	/* Initialize UART */
	if((eResult = adi_uart_Open(UART_DEVICE_NUM,
			ADI_UART_DIR_TRANSMIT,
			gUARTMemory,
			ADI_UART_UNIDIR_MEMORY_SIZE,
			&ghUART)) != ADI_UART_SUCCESS)
	{
		#undef UART_REDIRECT
		PRINT_INFO("Could not open UART Device 0x%08X \n", eResult); //
		return FAILED;
	}

	if(adi_uart_ConfigBaudRate(ghUART, Edb0, Divisor))
	{
		#undef UART_REDIRECT
		PRINT_INFO("Could not open UART Device Baud rate Divisor 0x%08X \n", eResult); //
		return FAILED;
	}

	return PASSED;

}

/*
 *   Function:    UART_DEBUG_PRINT
 *   Description: Prints debug info over the UART using a predefined
 *				 buffer.
 */
int32_t UART_DEBUG_PRINT(int transmit_len)
{
	ADI_UART_RESULT    eResult = ADI_UART_FAILURE;

	if((eResult = adi_uart_CoreWrite(ghUART,
			&UART_DEBUG_BUFFER,
			transmit_len
	)) != ADI_UART_SUCCESS)

	{
		PRINT_INFO("Could not do a write 0x%08X \n", eResult);
		return FAILED;
	}
	return PASSED;
}
