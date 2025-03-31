/*********************************************************************************

Copyright(c) 2018 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

 *********************************************************************************/

/*!
* @file      adi_clockrates_2156x_config.h
*
* @brief     Header file for clock rate configurations.
*
* @details
*
*/

#ifndef INC_ADI_CLOCK_2156X_CONFIG_H_
#define INC_ADI_CLOCK_2156X_CONFIG_H_

#include <stdint.h>

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_5_1:"Allow identifiers to be more than 31 characters")
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define TimerNum 5
#define OVERRIDE_CONFIG_FILE
uint32_t adi_uart_byte_delay(void);
uint32_t adi_uart_baud_read(void);
uint32_t adi_uart_baud_init(uint32_t UART_Baud_Rate_Val);
uint32_t adi_spi_clock_rate(uint32_t SPI_Clock_Rate_Val);
uint32_t adi_ospi_clock_rate(uint32_t OSPI_Clock_Rate_Val, uint8_t Readcapdelay);

#ifdef __cplusplus
}
#endif

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

#endif /* INC_ADI_CLOCKRATES_2156X_CONFIG_H_ */
