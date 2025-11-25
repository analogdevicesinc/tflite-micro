/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/

/*****************************************************************************
 * adi_sharcfx_init.h
 *****************************************************************************/

#ifndef _ADI_SHARCFX_INIT_H_
#define _ADI_SHARCFX_INIT_H_

#include <stdint.h>
#include <string.h>
#include <sys/platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/adi_core.h>
#include "adi_initialize.h"
#include <services/int/adi_int.h>
#include <drivers/sport/adi_sport.h>
#include <services/spu/adi_spu.h>
#include <drivers/twi/adi_twi.h>
#include "ADAU_1962Common.h"
#include "ADAU_1979Common.h"
#include "math.h"
#include <sru.h>
#include <services/gpio/adi_gpio.h>
#include "adi_run_kws.h"
#include <services/pwr/adi_pwr.h>
#include "debug.h"

/*=============  L O C A L    F U N C T I O N    P R O T O T Y P E S =============*/
//TFLM denoiser specific defines
extern int g_audio_data_input[];//this holds the current input data
extern int g_audio_data_output[];//holds raw audio output of model2
extern volatile int pReadPtr;
extern volatile int pWritePtr;
extern int pProcessReadPtr;
extern int pProcessWritePtr;
extern float g_process_input[];//this holds the current input data

int ADAU_1962_Pllinit(void);
int ADAU_1979_Pllinit(void);
void Switch_Configurator(void);
void SRU_Init(void);

extern int ADAU_1962_init(void);
extern int ADAU_1979_init(void);
extern int Sport_Init(void);
extern int Sport_Stop(void);
extern int SPU_init(void);
extern int Init_TWI(void);
extern int Stop_TWI(void);

#ifdef __cplusplus
extern "C" {
#endif
	void ConfigSoftSwitches_ADC_DAC(void);
	void ConfigSoftSwitches_ADAU_Reset(void);
	#if defined(__ADSP218xx__)
	void SoftConfig_EV_21835_SOM(void);
	#elif defined(__ADSPSC8xx__)
	void SoftConfig_EV_SC835_SOM(void);
	#endif
	uint32_t adi_pwr_cfg0_init();
	void Init_Twi1PinMux(void);
	void Init_Twi2PinMux(void);
#ifdef __cplusplus
}
#endif


/*==============  D E F I N E S  ===============*/
#define AUDIO_COUNT                 HOP_SIZE*6 /* 2(mono/stereo) * 3 for 48KHz to 16KHz * HOP_SIZE */
#define SCALE_FACTOR 				16777216 	/*this is the scaling factor to quantize actual real world analog samples to digital
												since it is a 24 bit dac, this scaling factor corresponds to 2^24 */

#define SPORT_DEVICE_4A 			4u			/* SPORT device number */
#define SPORT_DEVICE_4B 			4u			/* SPORT device number */
#define TWIDEVNUM     				2u         /* TWI device number */

#define BITRATE       				(100u)      /* kHz */
#define DUTYCYCLE     				(50u)       /* percent */
#define PRESCALEVALUE 				(12u)       /* fSCLK/10MHz (112.5 sclk0_0) */
#define BUFFER_SIZE   				(8u)

#define TARGETADDR    				(0x38u)     /* hardware address */
#define TARGETADDR_1962    			(0x04u)     /* hardware address of adau1962 DAC */
#define TARGETADDR_1979    			(0x11u)     /* hardware address of adau1979 ADC */

#define SPORT_4A_SPU  					66
#define SPORT_4B_SPU   					67

#define DMA_NUM_DESC 				2u

#define SUCCESS   0
#define FAILED   -1

#define CHECK_RESULT(eResult) \
        if(eResult != 0)\
		{\
			return (1);\
        }

#if defined(__ADSPSC835__)

/*
 * Push button 1 GPIO settings
 */

/* GPIO port to which push button 1 is connected to */
#define PUSH_BUTTON1_PORT           	(ADI_GPIO_PORT_C)

/* GPIO pin within the port to which push button 1 is connected to */
#define PUSH_BUTTON1_PIN            	(ADI_GPIO_PIN_9)

/* GPIO pint to which push button 1 is connected to */
#define PUSH_BUTTON1_PINT          		(ADI_GPIO_PIN_INTERRUPT_1)

/* Pin within the pint to which push button 1 is connected to */
#define PUSH_BUTTON1_PINT_PIN     		(ADI_GPIO_PIN_9)

/*PINT port assignment to which push button 1 is connected*/
#define PUSH_BUTTON1_PIN_ASSIGN 		(ADI_GPIO_PIN_ASSIGN_PCH_PINT1)

/*Byte assignment in the PINT block to which push button 1 is connected*/
#define PUSH_BUTTON1_PIN_ASSIGN_BYTE 	(ADI_GPIO_PIN_ASSIGN_BYTE_1)

/* Label printed on the EZ-Kit */
#define PUSH_BUTTON1_LABEL          	"SW3"

#endif

#define REPORT_ERROR        	 PRINT_INFO
#define DEBUG_INFORMATION        PRINT_INFO

#define GPIO_MEMORY_SIZE (ADI_GPIO_CALLBACK_MEM_SIZE*2)

/*=============  D A T A  =============*/
static uint8_t gpioMemory[GPIO_MEMORY_SIZE];

/* TWI2 Pin mux  */
#define TWI2_SCL_PORTA_MUX  ((uint32_t) ((uint32_t) 0<<28))
#define TWI2_SDA_PORTA_MUX  ((uint32_t) ((uint32_t) 0<<30))

#define TWI2_SCL_PORTA_FER  ((uint32_t) ((uint32_t) 1<<14))
#define TWI2_SDA_PORTA_FER  ((uint32_t) ((uint32_t) 1<<15))

#define TWI1_SCL_PORTB_MUX  ((uint16_t) ((uint16_t) 1<<0))
#define TWI1_SDA_PORTB_MUX  ((uint16_t) ((uint16_t) 1<<2))

#define TWI1_SCL_PORTB_FER  ((uint16_t) ((uint16_t) 1<<0))
#define TWI1_SDA_PORTB_FER  ((uint16_t) ((uint16_t) 1<<1))
#endif /* _ADI_SHARCFX_INIT_H_ */
