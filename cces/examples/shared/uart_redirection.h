/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/
/*****************************************************************************
 * uart_redirection.h
 *****************************************************************************/

#ifndef _UART_REDIRECTION_H_
#define _UART_REDIRECTION_H_

#include <drivers/uart/adi_uart.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* UART Device Number to test */
#define UART_DEVICE_NUM           0u
#define BAUD_RATE                 9600u

#define UART_DEBUG_BUFFER_LINE_SIZE 256

#define FAILED              	-1
#define PASSED              	 0

/* Clock Definitions */
#define CGU_DEV       (0)
#define CGU_DEV_1     (1)
#define MHZ           (1000000u)
#define CLKIN         (25u * MHZ)

// Function declaration
int32_t Init_UART(void);
int32_t UART_DEBUG_PRINT(int transmit_len);

#ifdef __cplusplus
}
#endif

#endif /* _UART_REDIRECTION_H_ */
