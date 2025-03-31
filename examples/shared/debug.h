/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/
/*****************************************************************************
 * debug.h
 *****************************************************************************/
#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>
#include <stdarg.h>
#include "uart_redirection.h"

#ifdef __cplusplus
extern "C" {
#endif

// Function pointer type for logging
typedef void (*p_print_func_t)(const char *, ...);

// Externally defined function pointer
//PRINT_INFO dynamically configured for default printf or uart printf, to be active in Release and Debug Mode
extern p_print_func_t PRINT_INFO;
//Specific to Debug logs to be used when Debug builds are run
extern p_print_func_t DEBUG_PRINT;

// Default function (used if not overridden)
void default_print(const char *fmt, ...);

//Uart print function when in release mode for realtime applications with uart enable
void uart_print(const char *fmt, ...);

// Function to set pritnf dynamically
void set_print_func(p_print_func_t func);

#ifdef __cplusplus
}
#endif

#endif /* _DEBUG_H_ */
