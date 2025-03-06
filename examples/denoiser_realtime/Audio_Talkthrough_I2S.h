/*********************************************************************************
Copyright(c) 2023 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/

/*****************************************************************************
 * Audio_Talkthrough_I2S.h
 *****************************************************************************/

#ifndef __AUDIO_TALKTHROUGH_I2S_H__
#define __AUDIO_TALKTHROUGH_I2S_H__

/* Add your custom header content here */

#include <stdint.h>

#define COUNT 128*2*3//*2 to convert mono to stereo, *3 for 48KHz to 16KHz

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
#define REPORT_ERROR        	 printf
#define DEBUG_INFORMATION        printf


#endif /* __AUDIO_TALKTHROUGH_I2S_H__ */
