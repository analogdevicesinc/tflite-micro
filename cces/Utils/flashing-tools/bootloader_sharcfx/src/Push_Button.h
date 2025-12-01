/*********************************************************************************
Copyright(c) 2023 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
 *********************************************************************************/

/*****************************************************************************
 * Push_Button.h
 *****************************************************************************/

#ifndef __PUSH_BUTTON_H__
#define __PUSH_BUTTON_H__

/* Add your custom header content here */



#if defined(__ADSP21835__)

/*
 * Push button 1 GPIO settings
 */

/* GPIO port to which push button 1 is connected to */
#define PUSH_BUTTON1_PORT           	(ADI_GPIO_PORT_B)

/* GPIO pin within the port to which push button 1 is connected to */
#define PUSH_BUTTON1_PIN            	(ADI_GPIO_PIN_3)

/* GPIO pint to which push button 1 is connected to */
#define PUSH_BUTTON1_PINT          		(ADI_GPIO_PIN_INTERRUPT_0)

/* Pin within the pint to which push button 1 is connected to */
#define PUSH_BUTTON1_PINT_PIN     		(ADI_GPIO_PIN_3)

/*PINT port assignment to which push button 1 is connected*/
#define PUSH_BUTTON1_PIN_ASSIGN 		(ADI_GPIO_PIN_ASSIGN_PBL_PINT0)

/*Byte assignment in the PINT block to which push button 1 is connected*/
#define PUSH_BUTTON1_PIN_ASSIGN_BYTE 	(ADI_GPIO_PIN_ASSIGN_BYTE_0)

/* Label printed on the EZ-Kit */
#define PUSH_BUTTON1_LABEL          	"SW3"

/*
 * Push button 2 GPIO settings
 */

/* GPIO port to which push button 2 is connected to */
#define PUSH_BUTTON2_PORT           	(ADI_GPIO_PORT_B)

/* GPIO pin within the port to which push button 2 is connected to */
#define PUSH_BUTTON2_PIN            	(ADI_GPIO_PIN_5)

/* GPIO pint to which push button 2 is connected to */
#define PUSH_BUTTON2_PINT	          	(ADI_GPIO_PIN_INTERRUPT_0)

/* Pin within the pint to which push button 2 is connected to */
#define PUSH_BUTTON2_PINT_PIN	     	(ADI_GPIO_PIN_5)

/*PINT port assignment to which push button 2 is connected*/
#define PUSH_BUTTON2_PIN_ASSIGN			(ADI_GPIO_PIN_ASSIGN_PBL_PINT0)

/*Byte assignment in the PINT block to which push button 1 is connected*/
#define PUSH_BUTTON2_PIN_ASSIGN_BYTE 	(ADI_GPIO_PIN_ASSIGN_BYTE_0)

/* Label printed on the EZ-Kit */
#define PUSH_BUTTON2_LABEL          	"SW4"


#elif defined(__ADSPSC835__)

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

/*
 * Push button 2 GPIO settings
 */

/* GPIO port to which push button 2 is connected to */
#define PUSH_BUTTON2_PORT           	(ADI_GPIO_PORT_C)

/* GPIO pin within the port to which push button 2 is connected to */
#define PUSH_BUTTON2_PIN            	(ADI_GPIO_PIN_10)

/* GPIO pint to which push button 2 is connected to */
#define PUSH_BUTTON2_PINT	          	(ADI_GPIO_PIN_INTERRUPT_1)

/* Pin within the pint to which push button 2 is connected to */
#define PUSH_BUTTON2_PINT_PIN	     	(ADI_GPIO_PIN_10)

/*PINT port assignment to which push button 2 is connected*/
#define PUSH_BUTTON2_PIN_ASSIGN			(ADI_GPIO_PIN_ASSIGN_PCH_PINT1)

/*Byte assignment in the PINT block to which push button 1 is connected*/
#define PUSH_BUTTON2_PIN_ASSIGN_BYTE 	(ADI_GPIO_PIN_ASSIGN_BYTE_1)

/* Label printed on the EZ-Kit */
#define PUSH_BUTTON2_LABEL          	"SW4"


#endif

#endif /* __PUSH_BUTTON_H__ */
