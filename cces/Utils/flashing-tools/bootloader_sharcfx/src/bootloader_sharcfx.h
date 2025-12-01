/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/
/*****************************************************************************
 * bootloader_sharcfx.h
 *****************************************************************************/

#ifndef __BOOTLOADER_SHARFX_H__
#define __BOOTLOADER_SHARFX_H__

#include <services/gpio/adi_gpio.h>
#include <services/pwr/adi_pwr.h>
#include <drivers/twi/adi_twi.h>

//#define CASE14 /*SPI Master Normal Boot*/
#define SPI_MASTER

#if defined(__ADSP218xx__)
void SoftConfig_EV_21835_SOM(void);
#elif defined(__ADSPSC8xx__)
void SoftConfig_EV_SC835_SOM(void);
#endif
uint32_t adi_pwr_cfg0_init();
void Init_Twi1PinMux(void);
void Init_Twi2PinMux(void);

/* TWI2 Pin mux  */
#define TWI2_SCL_PORTA_MUX  ((uint32_t) ((uint32_t) 0<<28))
#define TWI2_SDA_PORTA_MUX  ((uint32_t) ((uint32_t) 0<<30))

#define TWI2_SCL_PORTA_FER  ((uint32_t) ((uint32_t) 1<<14))
#define TWI2_SDA_PORTA_FER  ((uint32_t) ((uint32_t) 1<<15))

#define TWI1_SCL_PORTB_MUX  ((uint16_t) ((uint16_t) 1<<0))
#define TWI1_SDA_PORTB_MUX  ((uint16_t) ((uint16_t) 1<<2))

#define TWI1_SCL_PORTB_FER  ((uint16_t) ((uint16_t) 1<<0))
#define TWI1_SDA_PORTB_FER  ((uint16_t) ((uint16_t) 1<<1))

#define GPIO_MEMORY_SIZE (ADI_GPIO_CALLBACK_MEM_SIZE*2)
static uint8_t gpioMemory[GPIO_MEMORY_SIZE];

#endif /* __BOOTLOADER_SHARFX_H__ */
