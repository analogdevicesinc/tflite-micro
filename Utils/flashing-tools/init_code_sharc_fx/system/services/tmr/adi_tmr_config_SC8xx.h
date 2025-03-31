/******************************************************************************

Copyright (c) 2022-2023 Analog Devices.  All Rights Reserved.

This software is proprietary.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*******************************************************************************/

/** @addtogroup Timer_Service GP Timer Service
*  @{
*/

/** @addtogroup Timer_Service_Static_Configuration GP Timer Service Static Configuration
 *  @{
 */
 
/*!
* @file      adi_tmr_config_SC8xx.h
*
* @brief     Timer driver static configuration Header file
*
* @details
*            Timer driver static configuration Header file
*/

#ifndef __ADI_TMR_CONFIG_SC8XX_H__
#define __ADI_TMR_CONFIG_SC8XX_H__

#include <sys/platform.h>
#include <stdint.h>

#if defined(__ADSPSHARCFX__)
/*!Specifies if the SEC interrupts should be used for TMR instances
 * If set as 1, SEC interrupt path is enabled for all TMR instances
 * If set as 0, direct interrupt path is enabled for all TMR instances*/
#define ADI_TMR_CONFIG_SEC_INT	(0u)
#endif


/* Assign the macros to 1 for those timers for which static configuration needs to be done. If macro is 
assigned to 1, then static configuration will be done for the corresponding timer. If macro is assigned to 0,
static configuration will not be done and the default reset configuration is retained. */

/*!Enable the static configuration for Timer 0*/
#define ADI_TMR_TMR0	1
/*!Enable the static configuration for Timer 1*/
#define ADI_TMR_TMR1	1
/*!Enable the static configuration for Timer 2*/
#define ADI_TMR_TMR2	1
/*!Enable the static configuration for Timer 3*/
#define ADI_TMR_TMR3	1
/*!Enable the static configuration for Timer 4*/
#define ADI_TMR_TMR4	1
/*!Enable the static configuration for Timer 5*/
#define ADI_TMR_TMR5	1
/*!Enable the static configuration for Timer 6*/
#define ADI_TMR_TMR6	1
/*!Enable the static configuration for Timer 7*/
#define ADI_TMR_TMR7	1
/*!Enable the static configuration for Timer 8*/
#define ADI_TMR_TMR8	1
/*!Enable the static configuration for Timer 9*/
#define ADI_TMR_TMR9	1

/*------------------TIMER 0------------------------------------*/
#if (ADI_TMR_TMR0==1)
/* TIMER0 Instance */
/*! Select TIMER0 Instance */
#define ADI_TIMER0_INSTANCE                     1u

/*!Selects the mode of operation for the Timer0. 
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_MODE_IDLE     		 	- Idle mode          
    
	ADI_TMR_MODE_PERIOD_WD  		- Period watchdog mode 
   
    ADI_TMR_MODE_WIDTH_WD 			- Width watchdog mode 
    
	ADI_TMR_MODE_CAPTURE_ASSERT 	- Width capture mode at asserting edge 
    
	ADI_TMR_MODE_CAPTURE_DEASSERT 	- Width capture mode at de-asserting edge
    
	ADI_TMR_MODE_CONTINUOUS_PWMOUT 	- Continuous PWM out mode 
    
	ADI_TMR_MODE_SINGLE_PWMOUT   	- Single PWM out mode     
    
	ADI_TMR_MODE_EXTCLK             - External clock mode
    
	ADI_TMR_MODE_PINT               - Pin interrupt mode
*/
#define ADI_TIMER0_MODE        		ADI_TMR_MODE_CONTINUOUS_PWMOUT

/*!Selects the IRQ Mode of operation of the Timer0.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_IRQMODE_PULSE           - Generate interrupt every active edge.This IRQ Mode is valid for the PIN Interrupt mode only.
									  
    ADI_TMR_IRQMODE_DELAY           - Generate interrupt after timer counts till TMR_DELAY register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.
									  
    ADI_TMR_IRQMODE_WIDTH_DELAY     - Generate interrupt after timer counts till TMR_DELAY + TMR_WIDTH register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.

    ADI_TMR_IRQMODE_INSIDE_WINDOW   - Generate interrupt if de-asserting edge of timer input falls within width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_OUTSIDE_WINDOW  - Generate interrupt if de-asserting edge of timer input falls outside width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_PERIOD          - Generate interrupt after timer counts till TMR_PERIOD register value. This is valid for the Continuous PWMOUT mode and EXTCLK mode.
*/
#define ADI_TIMER0_IRQ_MODE			ADI_TMR_IRQMODE_PERIOD

/*!Selects the pulse polarity of the timer pin, true=active high, false=active low*/
#define ADI_TIMER0_PULSE_POLARITY  	false

/*!Selects the clock input source of the Timer0.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_CLKIN_SYSCLK        - Use system clock as clock input.
    
	ADI_TMR_CLKIN_ALTCLK0       - Use alternative clock input source 0.
    
	ADI_TMR_CLKIN_ALTCLK1       - Use alternative clock input source 1.
*/
#define ADI_TIMER0_CLKIN_SOURCE		ADI_TMR_CLKIN_SYSCLK

/*!Selects the input of the timer, true=Auxiliary input pin, false=timer pin input*/
#define ADI_TIMER0_INPUT_AUXIN		false

/*!The period value for timer operation in SCLK cycles. This field is valid only for PWM Out modes, Windowed WDOG modes and External clock mode. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER0_PERIOD			2000u

/*!The width value for timer operation in SCLK cycles. This field is valid only for PWM Out modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER0_WIDTH			500u

/*!The delay value for timer operation in SCLK cycles. This field is valid only for PWM Out modes and windowed WDOG modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER0_DELAY			0u

/*!Choose the stop configuration of the timer as abrupt stop or graceful stop. This option is valid only in the PWM OUT modes*/
#define ADI_TIMER0_ENABLE_GRACEFUL_STOP    true

/*!If true, then the timer runs during emulation.*/
#define ADI_TIMER0_EMULATION_RUN          false

#endif /*(ADI_TMR_TMR0==1)*/


/*------------------TIMER 1------------------------------------*/
#if (ADI_TMR_TMR1==1)
/* TIMER1 Instance */
/*! Select TIMER1 Instance */
#define ADI_TIMER1_INSTANCE                     1u

/*!Selects the mode of operation for the Timer1.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_MODE_IDLE     		 	- Idle mode          
    
	ADI_TMR_MODE_PERIOD_WD  		- Period watchdog mode 
    
	ADI_TMR_MODE_WIDTH_WD 			- Width watchdog mode 
    
	ADI_TMR_MODE_CAPTURE_ASSERT 	- Width capture mode at asserting edge 
    
	ADI_TMR_MODE_CAPTURE_DEASSERT 	- Width capture mode at de-asserting edge
    
	ADI_TMR_MODE_CONTINUOUS_PWMOUT 	- Continuous PWM out mode 
    
	ADI_TMR_MODE_SINGLE_PWMOUT   	- Single PWM out mode     
    
	ADI_TMR_MODE_EXTCLK             - External clock mode
    
	ADI_TMR_MODE_PINT               - Pin interrupt mode
*/
#define ADI_TIMER1_MODE        		ADI_TMR_MODE_CONTINUOUS_PWMOUT

/*!Selects the IRQ Mode of operation of the Timer1.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_IRQMODE_PULSE           - Generate interrupt every active edge.This IRQ Mode is valid for the PIN Interrupt mode only.
									  
    ADI_TMR_IRQMODE_DELAY           - Generate interrupt after timer counts till TMR_DELAY register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.
									  
    ADI_TMR_IRQMODE_WIDTH_DELAY     - Generate interrupt after timer counts till TMR_DELAY + TMR_WIDTH register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.

    ADI_TMR_IRQMODE_INSIDE_WINDOW   - Generate interrupt if de-asserting edge of timer input falls within width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_OUTSIDE_WINDOW  - Generate interrupt if de-asserting edge of timer input falls outside width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_PERIOD          - Generate interrupt after timer counts till TMR_PERIOD register value. This is valid for the Continuous PWMOUT mode and EXTCLK mode.
*/
#define ADI_TIMER1_IRQ_MODE			ADI_TMR_IRQMODE_PERIOD

/*!Selects the pulse polarity of the timer pin, true=active high*/
#define ADI_TIMER1_PULSE_POLARITY  	false

/*!Selects the clock input source of the Timer1.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_CLKIN_SYSCLK        - Use system clock as clock input.
    
	ADI_TMR_CLKIN_ALTCLK0       - Use alternative clock input source 0.
    
	ADI_TMR_CLKIN_ALTCLK1       - Use alternative clock input source 1.
*/
#define ADI_TIMER1_CLKIN_SOURCE		ADI_TMR_CLKIN_SYSCLK

/*!Selects the input of the timer, true=Auxiliary input pin, false=timer pin input*/
#define ADI_TIMER1_INPUT_AUXIN		false

/*!The period value for timer operation in SCLK cycles. This field is valid only for PWM Out modes, Windowed WDOG modes and External clock mode. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER1_PERIOD			2000u

/*!The width value for timer operation in SCLK cycles. This field is valid only for PWM Out modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER1_WIDTH			500u

/*!The delay value for timer operation in SCLK cycles. This field is valid only for PWM Out modes and windowed WDOG modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER1_DELAY			0u

/*!Choose the stop configuration of the timer as abrupt stop or graceful stop*/
/*!This option is valid only in the PWM OUT modes*/
#define ADI_TIMER1_ENABLE_GRACEFUL_STOP    true

/*!If true, then the timer runs during emulation.*/
#define ADI_TIMER1_EMULATION_RUN          false

#endif /*(ADI_TMR_TMR1==1)*/

/*------------------TIMER 2------------------------------------*/
#if (ADI_TMR_TMR2==1)
/* TIMER2 Instance */
/*! Select TIMER2 Instance */
#define ADI_TIMER2_INSTANCE                     1u

/*!Selects the mode of operation for the Timer2.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_MODE_IDLE     		 	- Idle mode          
    
	ADI_TMR_MODE_PERIOD_WD  		- Period watchdog mode 
    
	ADI_TMR_MODE_WIDTH_WD 			- Width watchdog mode 
    
	ADI_TMR_MODE_CAPTURE_ASSERT 	- Width capture mode at asserting edge 
    
	ADI_TMR_MODE_CAPTURE_DEASSERT 	- Width capture mode at de-asserting edge
    
	ADI_TMR_MODE_CONTINUOUS_PWMOUT 	- Continuous PWM out mode 
    
	ADI_TMR_MODE_SINGLE_PWMOUT   	- Single PWM out mode     
    
	ADI_TMR_MODE_EXTCLK             - External clock mode
    
	ADI_TMR_MODE_PINT               - Pin interrupt mode
*/
#define ADI_TIMER2_MODE        		ADI_TMR_MODE_CONTINUOUS_PWMOUT

/*!Selects the IRQ Mode of operation of the Timer2.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_IRQMODE_PULSE           - Generate interrupt every active edge.This IRQ Mode is valid for the PIN Interrupt mode only.
									  
    ADI_TMR_IRQMODE_DELAY           - Generate interrupt after timer counts till TMR_DELAY register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.
									  
    ADI_TMR_IRQMODE_WIDTH_DELAY     - Generate interrupt after timer counts till TMR_DELAY + TMR_WIDTH register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.

    ADI_TMR_IRQMODE_INSIDE_WINDOW   - Generate interrupt if de-asserting edge of timer input falls within width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_OUTSIDE_WINDOW  - Generate interrupt if de-asserting edge of timer input falls outside width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_PERIOD          - Generate interrupt after timer counts till TMR_PERIOD register value. This is valid for the Continuous PWMOUT mode and EXTCLK mode.
*/
#define ADI_TIMER2_IRQ_MODE			ADI_TMR_IRQMODE_PERIOD

/*!Selects the pulse polarity of the timer pin, true=active high*/
#define ADI_TIMER2_PULSE_POLARITY  	false

/*!Selects the clock input source of the Timer2.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_CLKIN_SYSCLK        - Use system clock as clock input.
    
	ADI_TMR_CLKIN_ALTCLK0       - Use alternative clock input source 0.
    
	ADI_TMR_CLKIN_ALTCLK1       - Use alternative clock input source 1.
*/
#define ADI_TIMER2_CLKIN_SOURCE		ADI_TMR_CLKIN_SYSCLK

/*!Selects the input of the timer, true=Auxiliary input pin, false=timer pin input*/
#define ADI_TIMER2_INPUT_AUXIN		false

/*!The period value for timer operation in SCLK cycles. This field is valid only for PWM Out modes, Windowed WDOG modes and External clock mode. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER2_PERIOD			2000u

/*!The width value for timer operation in SCLK cycles. This field is valid only for PWM Out modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER2_WIDTH			500u

/*!The delay value for timer operation in SCLK cycles. This field is valid only for PWM Out modes and windowed WDOG modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER2_DELAY			0u

/*!Choose the stop configuration of the timer as abrupt stop or graceful stop*/
/*!This option is valid only in the PWM OUT modes*/
#define ADI_TIMER2_ENABLE_GRACEFUL_STOP    true

/*!If true, then the timer runs during emulation.*/
#define ADI_TIMER2_EMULATION_RUN          false

#endif /*(ADI_TMR_TMR2==1)*/

/*------------------TIMER 3------------------------------------*/
#if (ADI_TMR_TMR3==1)
/* TIMER3 Instance */
/*! Select TIMER3 Instance */
#define ADI_TIMER3_INSTANCE                     1u

/*!Selects the mode of operation for the Timer3.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_MODE_IDLE     		 	- Idle mode          
    
	ADI_TMR_MODE_PERIOD_WD  		- Period watchdog mode 
    
	ADI_TMR_MODE_WIDTH_WD 			- Width watchdog mode 
    
	ADI_TMR_MODE_CAPTURE_ASSERT 	- Width capture mode at asserting edge 
    
	ADI_TMR_MODE_CAPTURE_DEASSERT 	- Width capture mode at de-asserting edge
    
	ADI_TMR_MODE_CONTINUOUS_PWMOUT 	- Continuous PWM out mode 
    
	ADI_TMR_MODE_SINGLE_PWMOUT   	- Single PWM out mode     
    
	ADI_TMR_MODE_EXTCLK             - External clock mode
    
	ADI_TMR_MODE_PINT               - Pin interrupt mode
*/
#define ADI_TIMER3_MODE        		ADI_TMR_MODE_CONTINUOUS_PWMOUT

/*!Selects the IRQ Mode of operation of the Timer3.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_IRQMODE_PULSE           - Generate interrupt every active edge.This IRQ Mode is valid for the PIN Interrupt mode only.
									  
    ADI_TMR_IRQMODE_DELAY           - Generate interrupt after timer counts till TMR_DELAY register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.
									  
    ADI_TMR_IRQMODE_WIDTH_DELAY     - Generate interrupt after timer counts till TMR_DELAY + TMR_WIDTH register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.

    ADI_TMR_IRQMODE_INSIDE_WINDOW   - Generate interrupt if de-asserting edge of timer input falls within width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_OUTSIDE_WINDOW  - Generate interrupt if de-asserting edge of timer input falls outside width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_PERIOD          - Generate interrupt after timer counts till TMR_PERIOD register value. This is valid for the Continuous PWMOUT mode and EXTCLK mode.
*/
#define ADI_TIMER3_IRQ_MODE			ADI_TMR_IRQMODE_PERIOD

/*!Selects the pulse polarity of the timer pin, true=active high*/
#define ADI_TIMER3_PULSE_POLARITY  	false

/*!Selects the clock input source of the Timer3.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_CLKIN_SYSCLK        - Use system clock as clock input.
    
	ADI_TMR_CLKIN_ALTCLK0       - Use alternative clock input source 0.
    
	ADI_TMR_CLKIN_ALTCLK1       - Use alternative clock input source 1.
*/
#define ADI_TIMER3_CLKIN_SOURCE		ADI_TMR_CLKIN_SYSCLK

/*!Selects the input of the timer, true=Auxiliary input pin, false=timer pin input*/
#define ADI_TIMER3_INPUT_AUXIN		false

/*!The period value for timer operation in SCLK cycles. This field is valid only for PWM Out modes, Windowed WDOG modes and External clock mode. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER3_PERIOD			2000u

/*!The width value for timer operation in SCLK cycles. This field is valid only for PWM Out modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER3_WIDTH			500u

/*!The delay value for timer operation in SCLK cycles. This field is valid only for PWM Out modes and windowed WDOG modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER3_DELAY			0u

/*!Choose the stop configuration of the timer as abrupt stop or graceful stop*/
/*!This option is valid only in the PWM OUT modes*/
#define ADI_TIMER3_ENABLE_GRACEFUL_STOP    true

/*!If true, then the timer runs during emulation.*/
#define ADI_TIMER3_EMULATION_RUN          false

#endif /*(ADI_TMR_TMR3==1)*/

/*------------------TIMER 4------------------------------------*/
#if (ADI_TMR_TMR4==1)
/* TIMER4 Instance */
/*! Select TIMER4 Instance */
#define ADI_TIMER4_INSTANCE                     1u

/*!Selects the mode of operation for the Timer4.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_MODE_IDLE     		 	- Idle mode          
    
	ADI_TMR_MODE_PERIOD_WD  		- Period watchdog mode 
    
	ADI_TMR_MODE_WIDTH_WD 			- Width watchdog mode 
    
	ADI_TMR_MODE_CAPTURE_ASSERT 	- Width capture mode at asserting edge 
    
	ADI_TMR_MODE_CAPTURE_DEASSERT 	- Width capture mode at de-asserting edge
    
	ADI_TMR_MODE_CONTINUOUS_PWMOUT 	- Continuous PWM out mode 
    
	ADI_TMR_MODE_SINGLE_PWMOUT   	- Single PWM out mode     
    
	ADI_TMR_MODE_EXTCLK             - External clock mode
    
	ADI_TMR_MODE_PINT               - Pin interrupt mode
*/
#define ADI_TIMER4_MODE        		ADI_TMR_MODE_CONTINUOUS_PWMOUT

/*!Selects the IRQ Mode of operation of the Timer4.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_IRQMODE_PULSE           - Generate interrupt every active edge.This IRQ Mode is valid for the PIN Interrupt mode only.
									  
    ADI_TMR_IRQMODE_DELAY           - Generate interrupt after timer counts till TMR_DELAY register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.
									  
    ADI_TMR_IRQMODE_WIDTH_DELAY     - Generate interrupt after timer counts till TMR_DELAY + TMR_WIDTH register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.

    ADI_TMR_IRQMODE_INSIDE_WINDOW   - Generate interrupt if de-asserting edge of timer input falls within width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_OUTSIDE_WINDOW  - Generate interrupt if de-asserting edge of timer input falls outside width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_PERIOD          - Generate interrupt after timer counts till TMR_PERIOD register value. This is valid for the Continuous PWMOUT mode and EXTCLK mode.
*/
#define ADI_TIMER4_IRQ_MODE			ADI_TMR_IRQMODE_PERIOD

/*!Selects the pulse polarity of the timer pin, true=active high*/
#define ADI_TIMER4_PULSE_POLARITY  	false

/*!Selects the clock input source of the Timer4.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_CLKIN_SYSCLK        - Use system clock as clock input.
    
	ADI_TMR_CLKIN_ALTCLK0       - Use alternative clock input source 0.
    
	ADI_TMR_CLKIN_ALTCLK1       - Use alternative clock input source 1.
*/
#define ADI_TIMER4_CLKIN_SOURCE		ADI_TMR_CLKIN_SYSCLK

/*!Selects the input of the timer, true=Auxiliary input pin, false=timer pin input*/
#define ADI_TIMER4_INPUT_AUXIN		false

/*!The period value for timer operation in SCLK cycles. This field is valid only for PWM Out modes, Windowed WDOG modes and External clock mode. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER4_PERIOD			2000u

/*!The width value for timer operation in SCLK cycles. This field is valid only for PWM Out modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER4_WIDTH			500u

/*!The delay value for timer operation in SCLK cycles. This field is valid only for PWM Out modes and windowed WDOG modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER4_DELAY			0u

/*!Choose the stop configuration of the timer as abrupt stop or graceful stop*/
/*!This option is valid only in the PWM OUT modes*/
#define ADI_TIMER4_ENABLE_GRACEFUL_STOP    true

/*!If true, then the timer runs during emulation.*/
#define ADI_TIMER4_EMULATION_RUN          false

#endif /*(ADI_TMR_TMR4==1)*/

/*------------------TIMER 5------------------------------------*/
#if (ADI_TMR_TMR5==1)
/* TIMER5 Instance */
/*! Select TIMER5 Instance */
#define ADI_TIMER5_INSTANCE                     1u

/*!Selects the mode of operation for the Timer5.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_MODE_IDLE     		 	- Idle mode          
    
	ADI_TMR_MODE_PERIOD_WD  		- Period watchdog mode 
    
	ADI_TMR_MODE_WIDTH_WD 			- Width watchdog mode 
    
	ADI_TMR_MODE_CAPTURE_ASSERT 	- Width capture mode at asserting edge 
    
	ADI_TMR_MODE_CAPTURE_DEASSERT 	- Width capture mode at de-asserting edge
    
	ADI_TMR_MODE_CONTINUOUS_PWMOUT 	- Continuous PWM out mode 
    
	ADI_TMR_MODE_SINGLE_PWMOUT   	- Single PWM out mode     
    
	ADI_TMR_MODE_EXTCLK             - External clock mode
    
	ADI_TMR_MODE_PINT               - Pin interrupt mode
*/
#define ADI_TIMER5_MODE        		ADI_TMR_MODE_CONTINUOUS_PWMOUT

/*!Selects the IRQ Mode of operation of the Timer5.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_IRQMODE_PULSE           - Generate interrupt every active edge.This IRQ Mode is valid for the PIN Interrupt mode only.
									  
    ADI_TMR_IRQMODE_DELAY           - Generate interrupt after timer counts till TMR_DELAY register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.
									  
    ADI_TMR_IRQMODE_WIDTH_DELAY     - Generate interrupt after timer counts till TMR_DELAY + TMR_WIDTH register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.

    ADI_TMR_IRQMODE_INSIDE_WINDOW   - Generate interrupt if de-asserting edge of timer input falls within width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_OUTSIDE_WINDOW  - Generate interrupt if de-asserting edge of timer input falls outside width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_PERIOD          - Generate interrupt after timer counts till TMR_PERIOD register value. This is valid for the Continuous PWMOUT mode and EXTCLK mode.
*/
#define ADI_TIMER5_IRQ_MODE			ADI_TMR_IRQMODE_PERIOD

/*!Selects the pulse polarity of the timer pin, true=active high*/
#define ADI_TIMER5_PULSE_POLARITY  	false

/*!Selects the clock input source of the Timer5.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_CLKIN_SYSCLK        - Use system clock as clock input.
    
	ADI_TMR_CLKIN_ALTCLK0       - Use alternative clock input source 0.
    
	ADI_TMR_CLKIN_ALTCLK1       - Use alternative clock input source 1.
*/
#define ADI_TIMER5_CLKIN_SOURCE		ADI_TMR_CLKIN_SYSCLK

/*!Selects the input of the timer, true=Auxiliary input pin, false=timer pin input*/
#define ADI_TIMER5_INPUT_AUXIN		false

/*!The period value for timer operation in SCLK cycles. This field is valid only for PWM Out modes, Windowed WDOG modes and External clock mode. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER5_PERIOD			2000u

/*!The width value for timer operation in SCLK cycles. This field is valid only for PWM Out modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER5_WIDTH			500u

/*!The delay value for timer operation in SCLK cycles. This field is valid only for PWM Out modes and windowed WDOG modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER5_DELAY			0u

/*!Choose the stop configuration of the timer as abrupt stop or graceful stop*/
/*!This option is valid only in the PWM OUT modes*/
#define ADI_TIMER5_ENABLE_GRACEFUL_STOP    true

/*!If true, then the timer runs during emulation.*/
#define ADI_TIMER5_EMULATION_RUN          false

#endif /*(ADI_TMR_TMR5==1)*/

/*------------------TIMER 6------------------------------------*/
#if (ADI_TMR_TMR6==1)
/* TIMER6 Instance */
/*! Select TIMER6 Instance */
#define ADI_TIMER6_INSTANCE                     1u

/*!Selects the mode of operation for the Timer6.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_MODE_IDLE     		 	- Idle mode          
    
	ADI_TMR_MODE_PERIOD_WD  		- Period watchdog mode 
    
	ADI_TMR_MODE_WIDTH_WD 			- Width watchdog mode 
    
	ADI_TMR_MODE_CAPTURE_ASSERT 	- Width capture mode at asserting edge 
    
	ADI_TMR_MODE_CAPTURE_DEASSERT 	- Width capture mode at de-asserting edge
    
	ADI_TMR_MODE_CONTINUOUS_PWMOUT 	- Continuous PWM out mode 
    
	ADI_TMR_MODE_SINGLE_PWMOUT   	- Single PWM out mode     
    
	ADI_TMR_MODE_EXTCLK             - External clock mode
    
	ADI_TMR_MODE_PINT               - Pin interrupt mode
*/
#define ADI_TIMER6_MODE        		ADI_TMR_MODE_CONTINUOUS_PWMOUT

/*!Selects the IRQ Mode of operation of the Timer6.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_IRQMODE_PULSE           - Generate interrupt every active edge.This IRQ Mode is valid for the PIN Interrupt mode only.
									  
    ADI_TMR_IRQMODE_DELAY           - Generate interrupt after timer counts till TMR_DELAY register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.
									  
    ADI_TMR_IRQMODE_WIDTH_DELAY     - Generate interrupt after timer counts till TMR_DELAY + TMR_WIDTH register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.

    ADI_TMR_IRQMODE_INSIDE_WINDOW   - Generate interrupt if de-asserting edge of timer input falls within width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_OUTSIDE_WINDOW  - Generate interrupt if de-asserting edge of timer input falls outside width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_PERIOD          - Generate interrupt after timer counts till TMR_PERIOD register value. This is valid for the Continuous PWMOUT mode and EXTCLK mode.
*/
#define ADI_TIMER6_IRQ_MODE			ADI_TMR_IRQMODE_PERIOD

/*!Selects the pulse polarity of the timer pin, true=active high*/
#define ADI_TIMER6_PULSE_POLARITY  	false

/*!Selects the clock input source of the Timer6.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_CLKIN_SYSCLK        - Use system clock as clock input.
    
	ADI_TMR_CLKIN_ALTCLK0       - Use alternative clock input source 0.
    
	ADI_TMR_CLKIN_ALTCLK1       - Use alternative clock input source 1.
*/
#define ADI_TIMER6_CLKIN_SOURCE		ADI_TMR_CLKIN_SYSCLK

/*!Selects the input of the timer, true=Auxiliary input pin, false=timer input pin*/
#define ADI_TIMER6_INPUT_AUXIN		false

/*!The period value for timer operation in SCLK cycles. This field is valid only for PWM Out modes, Windowed WDOG modes and External clock mode. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER6_PERIOD			2000u

/*!The width value for timer operation in SCLK cycles. This field is valid only for PWM Out modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER6_WIDTH			500u

/*!The delay value for timer operation in SCLK cycles. This field is valid only for PWM Out modes and windowed WDOG modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER6_DELAY			0u

/*!Choose the stop configuration of the timer as abrupt stop or graceful stop*/
/*!This option is valid only in the PWM OUT modes*/
#define ADI_TIMER6_ENABLE_GRACEFUL_STOP    true

/*!If true, then the timer runs during emulation.*/
#define ADI_TIMER6_EMULATION_RUN          false

#endif /*(ADI_TMR_TMR6==1)*/

/*------------------TIMER 7------------------------------------*/
#if (ADI_TMR_TMR7==1)
/* TIMER7 Instance */
/*! Select TIMER7 Instance */
#define ADI_TIMER7_INSTANCE                     1u

/*!Selects the mode of operation for the Timer7.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_MODE_IDLE     		 	- Idle mode          
    
	ADI_TMR_MODE_PERIOD_WD  		- Period watchdog mode 
    
	ADI_TMR_MODE_WIDTH_WD 			- Width watchdog mode 
    
	ADI_TMR_MODE_CAPTURE_ASSERT 	- Width capture mode at asserting edge 
    
	ADI_TMR_MODE_CAPTURE_DEASSERT 	- Width capture mode at de-asserting edge
    
	ADI_TMR_MODE_CONTINUOUS_PWMOUT 	- Continuous PWM out mode 
    
	ADI_TMR_MODE_SINGLE_PWMOUT   	- Single PWM out mode     
    
	ADI_TMR_MODE_EXTCLK             - External clock mode
    
	ADI_TMR_MODE_PINT               - Pin interrupt mode
*/
#define ADI_TIMER7_MODE        		ADI_TMR_MODE_CONTINUOUS_PWMOUT

/*!Selects the IRQ Mode of operation of the Timer7.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_IRQMODE_PULSE           - Generate interrupt every active edge.This IRQ Mode is valid for the PIN Interrupt mode only.
									  
    ADI_TMR_IRQMODE_DELAY           - Generate interrupt after timer counts till TMR_DELAY register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.
									  
    ADI_TMR_IRQMODE_WIDTH_DELAY     - Generate interrupt after timer counts till TMR_DELAY + TMR_WIDTH register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.

    ADI_TMR_IRQMODE_INSIDE_WINDOW   - Generate interrupt if de-asserting edge of timer input falls within width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_OUTSIDE_WINDOW  - Generate interrupt if de-asserting edge of timer input falls outside width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_PERIOD          - Generate interrupt after timer counts till TMR_PERIOD register value. This is valid for the Continuous PWMOUT mode and EXTCLK mode.
*/
#define ADI_TIMER7_IRQ_MODE			ADI_TMR_IRQMODE_PERIOD

/*!Selects the pulse polarity of the timer pin, true=active high*/
#define ADI_TIMER7_PULSE_POLARITY  	false

/*!Selects the clock input source of the Timer7.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_CLKIN_SYSCLK        - Use system clock as clock input.
    
	ADI_TMR_CLKIN_ALTCLK0       - Use alternative clock input source 0.
    
	ADI_TMR_CLKIN_ALTCLK1       - Use alternative clock input source 1.
*/
#define ADI_TIMER7_CLKIN_SOURCE		ADI_TMR_CLKIN_SYSCLK

/*!Selects the input of the timer, true=Auxiliary input pin, false=timer pin input*/
#define ADI_TIMER7_INPUT_AUXIN		false

/*!The period value for timer operation in SCLK cycles. This field is valid only for PWM Out modes, Windowed WDOG modes and External clock mode. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER7_PERIOD			2000u

/*!The width value for timer operation in SCLK cycles. This field is valid only for PWM Out modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER7_WIDTH			500u

/*!The delay value for timer operation in SCLK cycles. This field is valid only for PWM Out modes and windowed WDOG modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER7_DELAY			0u

/*!Choose the stop configuration of the timer as abrupt stop or graceful stop*/
/*!This option is valid only in the PWM OUT modes*/
#define ADI_TIMER7_ENABLE_GRACEFUL_STOP    true

/*!If true, then the timer runs during emulation.*/
#define ADI_TIMER7_EMULATION_RUN          false

#endif /*(ADI_TMR_TMR7==1)*/

/*------------------TIMER 8------------------------------------*/
#if (ADI_TMR_TMR8==1)
/* TIMER8 Instance */
/*! Select TIMER8 Instance */
#define ADI_TIMER8_INSTANCE                     1u

/*!Selects the mode of operation for the Timer8.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_MODE_IDLE     		 	- Idle mode          
    
	ADI_TMR_MODE_PERIOD_WD  		- Period watchdog mode 
    
	ADI_TMR_MODE_WIDTH_WD 			- Width watchdog mode 
    
	ADI_TMR_MODE_CAPTURE_ASSERT 	- Width capture mode at asserting edge 
    
	ADI_TMR_MODE_CAPTURE_DEASSERT 	- Width capture mode at de-asserting edge
    
	ADI_TMR_MODE_CONTINUOUS_PWMOUT 	- Continuous PWM out mode 
    
	ADI_TMR_MODE_SINGLE_PWMOUT   	- Single PWM out mode     
    
	ADI_TMR_MODE_EXTCLK             - External clock mode
    
	ADI_TMR_MODE_PINT               - Pin interrupt mode
*/
#define ADI_TIMER8_MODE        		ADI_TMR_MODE_CONTINUOUS_PWMOUT

/*!Selects the IRQ Mode of operation of the Timer8.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_IRQMODE_PULSE           - Generate interrupt every active edge.This IRQ Mode is valid for the PIN Interrupt mode only.
									  
    ADI_TMR_IRQMODE_DELAY           - Generate interrupt after timer counts till TMR_DELAY register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.
									  
    ADI_TMR_IRQMODE_WIDTH_DELAY     - Generate interrupt after timer counts till TMR_DELAY + TMR_WIDTH register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.

    ADI_TMR_IRQMODE_INSIDE_WINDOW   - Generate interrupt if de-asserting edge of timer input falls within width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_OUTSIDE_WINDOW  - Generate interrupt if de-asserting edge of timer input falls outside width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_PERIOD          - Generate interrupt after timer counts till TMR_PERIOD register value. This is valid for the Continuous PWMOUT mode and EXTCLK mode.
*/
#define ADI_TIMER8_IRQ_MODE			ADI_TMR_IRQMODE_PERIOD

/*!Selects the pulse polarity of the timer pin, true=active high*/
#define ADI_TIMER8_PULSE_POLARITY  	false

/*!Selects the clock input source of the Timer8.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_CLKIN_SYSCLK        - Use system clock as clock input.
    
	ADI_TMR_CLKIN_ALTCLK0       - Use alternative clock input source 0.
    
	ADI_TMR_CLKIN_ALTCLK1       - Use alternative clock input source 1.
*/
#define ADI_TIMER8_CLKIN_SOURCE		ADI_TMR_CLKIN_SYSCLK

/*!Selects the input of the timer, true=Auxiliary input pin, false=timer pin input*/
#define ADI_TIMER8_INPUT_AUXIN		false

/*!The period value for timer operation in SCLK cycles. This field is valid only for PWM Out modes, Windowed WDOG modes and External clock mode. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER8_PERIOD			2000u

/*!The width value for timer operation in SCLK cycles. This field is valid only for PWM Out modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER8_WIDTH			500u

/*!The delay value for timer operation in SCLK cycles. This field is valid only for PWM Out modes and windowed WDOG modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER8_DELAY			0u

/*!Choose the stop configuration of the timer as abrupt stop or graceful stop*/
/*!This option is valid only in the PWM OUT modes*/
#define ADI_TIMER8_ENABLE_GRACEFUL_STOP    true

/*!If true, then the timer runs during emulation.*/
#define ADI_TIMER8_EMULATION_RUN          false

#endif /*(ADI_TMR_TMR8==1)*/


/*------------------TIMER 9------------------------------------*/
#if (ADI_TMR_TMR9==1)
/* TIMER9 Instance */
/*! Select TIMER9 Instance */
#define ADI_TIMER9_INSTANCE                     1u

/*!Selects the mode of operation for the Timer9.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_MODE_IDLE     		 	- Idle mode          
    
	ADI_TMR_MODE_PERIOD_WD  		- Period watchdog mode 
    
	ADI_TMR_MODE_WIDTH_WD 			- Width watchdog mode 
    
	ADI_TMR_MODE_CAPTURE_ASSERT 	- Width capture mode at asserting edge 
    
	ADI_TMR_MODE_CAPTURE_DEASSERT 	- Width capture mode at de-asserting edge
    
	ADI_TMR_MODE_CONTINUOUS_PWMOUT 	- Continuous PWM out mode 
    
	ADI_TMR_MODE_SINGLE_PWMOUT   	- Single PWM out mode     
    
	ADI_TMR_MODE_EXTCLK             - External clock mode
    
	ADI_TMR_MODE_PINT               - Pin interrupt mode
*/
#define ADI_TIMER9_MODE        		ADI_TMR_MODE_CONTINUOUS_PWMOUT

/*!Selects the IRQ Mode of operation of the Timer9.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_IRQMODE_PULSE           - Generate interrupt every active edge.This IRQ Mode is valid for the PIN Interrupt mode only.
									  
    ADI_TMR_IRQMODE_DELAY           - Generate interrupt after timer counts till TMR_DELAY register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.
									  
    ADI_TMR_IRQMODE_WIDTH_DELAY     - Generate interrupt after timer counts till TMR_DELAY + TMR_WIDTH register value. This is valid for Continuous PWMOUT mode and Single Pulse PWMOUT mode.

    ADI_TMR_IRQMODE_INSIDE_WINDOW   - Generate interrupt if de-asserting edge of timer input falls within width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_OUTSIDE_WINDOW  - Generate interrupt if de-asserting edge of timer input falls outside width/period watchdog window. This is valid for the period and width WATCHDOG modes.

    ADI_TMR_IRQMODE_PERIOD          - Generate interrupt after timer counts till TMR_PERIOD register value. This is valid for the Continuous PWMOUT mode and EXTCLK mode.
*/
#define ADI_TIMER9_IRQ_MODE			ADI_TMR_IRQMODE_PERIOD

/*!Selects the pulse polarity of the timer pin, true=active high, false = active low*/
#define ADI_TIMER9_PULSE_POLARITY  	false

/*!Selects the clock input source of the Timer9.
Following are the valid values that can be assigned to the macro
    
	ADI_TMR_CLKIN_SYSCLK        - Use system clock as clock input.
    
	ADI_TMR_CLKIN_ALTCLK0       - Use alternative clock input source 0.
    
	ADI_TMR_CLKIN_ALTCLK1       - Use alternative clock input source 1.
*/
#define ADI_TIMER9_CLKIN_SOURCE		ADI_TMR_CLKIN_SYSCLK

/*!Selects the input of the timer, true=Auxiliary input pin, false=timer pin input*/
#define ADI_TIMER9_INPUT_AUXIN		false

/*!The period value for timer operation in SCLK cycles. This field is valid only for PWM Out modes, Windowed WDOG modes and External clock mode. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER9_PERIOD			2000u

/*!The width value for timer operation in SCLK cycles. This field is valid only for PWM Out modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER9_WIDTH			500u

/*!The delay value for timer operation in SCLK cycles. This field is valid only for PWM Out modes and windowed WDOG modes. The valid range is 0 to (2^32)-1*/
#define ADI_TIMER9_DELAY			0u

/*!Choose the stop configuration of the timer as abrupt stop or graceful stop*/
/*!This option is valid only in the PWM OUT modes*/
#define ADI_TIMER9_ENABLE_GRACEFUL_STOP    true

/*!If true, then the timer runs during emulation.*/
#define ADI_TIMER9_EMULATION_RUN          false

#endif /*(ADI_TMR_TMR9==1)*/



#endif /*__ADI_TMR_CONFIG_SC8XX_H__*/

/** @}*/
/** @}*/
