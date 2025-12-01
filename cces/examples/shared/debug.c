/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/
/*****************************************************************************
 * debug.c
 *****************************************************************************/

#include "debug.h"
#include <stdarg.h>

// Initialize function pointer with default function
p_print_func_t PRINT_INFO = default_print;
p_print_func_t DEBUG_PRINT = default_print;

void default_print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);  // Default to printf
    va_end(args);
}

// External buffer and function declaration
extern char UART_DEBUG_BUFFER[];
extern int32_t UART_DEBUG_PRINT(int length);

// Function to handle UART printing
void uart_print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    // Format output into UART buffer
    int outLen = vsnprintf(UART_DEBUG_BUFFER, UART_DEBUG_BUFFER_LINE_SIZE, fmt, args);
    va_end(args);

    // Send the formatted string via UART
    UART_DEBUG_PRINT(outLen);

    // Handle buffer overflow case
    if (outLen >= UART_DEBUG_BUFFER_LINE_SIZE) {
        va_start(args, fmt);
        outLen = snprintf(UART_DEBUG_BUFFER, UART_DEBUG_BUFFER_LINE_SIZE, "\n  ...(output truncated)...\n");
        va_end(args);
        UART_DEBUG_PRINT(outLen);
    }
}

// Function to set printf dynamically
void set_print_func(p_print_func_t func) {
	PRINT_INFO = func;
}
