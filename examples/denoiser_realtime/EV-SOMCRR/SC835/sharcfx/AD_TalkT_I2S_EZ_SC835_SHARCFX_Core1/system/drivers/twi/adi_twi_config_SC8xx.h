/******************************************************************************
 * @file:    adi_twi_config_SC8xx.h
 * @brief:   TWI driver static configuration
 * @version: $Revision:  $
 * @date:    $Date: $
 *****************************************************************************

 Copyright (c) 2022-2023 Analog Devices.  All Rights Reserved.

 This software is proprietary.  By using this software you agree
 to the terms of the associated Analog Devices License Agreement.

*******************************************************************************/

/** @addtogroup TWI_Driver TWI Device Driver
 *  @{
 */
 
  /** @addtogroup TWI_Driver_Static_Configuration TWI Device Driver Static Configuration
 *  @{

  Static Configuration: Using this configuration applications can configure the control registers
  parameters upfront which can be set to default before TWI data transfer starts. This configuration
  has macros which can be configured through adi_twi_config_SC83xx.h file which by default is available
  with TWI driver. Example project can maintain a copy of it locally for custom configuration.
  In the default static configuration, each TWI instance is enabled and PRESCALE is configured
  according to preload SYSCLOCK and set in fast mode.
*/

/*!
* @file      adi_twi_config_SC83xx.h
*
* @brief     TWI driver static configuration Header file
*
* @details
*            TWI driver static configuration Header file
*/
#ifndef __ADI_TWI_CONFIG_SC8XX_H__
#define __ADI_TWI_CONFIG_SC8XX_H__

#if defined(__ADSPSHARCFX__)
/*! Specifies if the SEC interrupts should be used for all TWI instances
 * If set as 1, SEC interrupt path is enabled for all TWI instances
 * If set as 0, direct interrupt path is enabled for all TWI instances*/ 
#define ADI_TWI_CONFIG_SEC_INT       0u
#endif

/*!TWI0 Instance*/
/*!Select TWI0 Instance */
#define ADI_TWI0_INSTANCE                     		1u

/*! Specifies the Prescale value for internal time reference*/
#define ADI_TWI0_CTL_PRESCALE              	 	  	(12u)
/*! Specifies the TWI clock frequency in kHz*/
#define ADI_TWI0_CLK_FREQ              	 	 	  	(400u)
/*! Specifies the TWI clock duty cycle */
#define ADI_TWI0_DUTY_CYCLE              	 	  	(50u)
/*! Specifies the TWI Master Address register Configuration */
#define ADI_TWI0_MSTRADDR_ADDR 				 	 	(255u)
/*! Specifies the TWI Slave address register Configuration */
#define ADI_TWI0_SLVADDR_ADDR				     	(255u)



/*!TWI1 Instance*/
/*!Select TWI1 Instance */
#define ADI_TWI1_INSTANCE                     		1u

/*! Specifies the Prescale value for internal time reference*/
#define ADI_TWI1_CTL_PRESCALE              	 	  	(12u)
/*! Specifies the TWI clock frequency in kHz*/
#define ADI_TWI1_CLK_FREQ              	 	 	  	(400u)
/*! Specifies the TWI clock duty cycle */
#define ADI_TWI1_DUTY_CYCLE              	 	  	(50u)
/*! Specifies the TWI Master Address register Configuration */
#define ADI_TWI1_MSTRADDR_ADDR 				 	 	(255u)
/*! Specifies the TWI Slave address register Configuration */
#define ADI_TWI1_SLVADDR_ADDR				     	(255u)


/*!TWI2 Instance*/
/*!Select TWI2 Instance */
#define ADI_TWI2_INSTANCE                    		 1u

/*! Specifies the Prescale value for internal time reference*/
#define ADI_TWI2_CTL_PRESCALE              	 	  	(12u)
/*! Specifies the TWI clock frequency in kHz*/
#define ADI_TWI2_CLK_FREQ              	 	 	  	(400u)
/*! Specifies the TWI clock duty cycle */
#define ADI_TWI2_DUTY_CYCLE              	 	  	(50u)
/*! Specifies the TWI Master Address register Configuration */
#define ADI_TWI2_MSTRADDR_ADDR 				 	 	(255u)
/*! Specifies the TWI Slave address register Configuration */
#define ADI_TWI2_SLVADDR_ADDR				     	(255u)

/*!TWI3 Instance*/
/*!Select TWI3 Instance */
#define ADI_TWI3_INSTANCE                    		 1u

/*! Specifies the Prescale value for internal time reference*/
#define ADI_TWI3_CTL_PRESCALE              	 	  	(12u)
/*! Specifies the TWI clock frequency in kHz*/
#define ADI_TWI3_CLK_FREQ              	 	 	  	(400u)
/*! Specifies the TWI clock duty cycle */
#define ADI_TWI3_DUTY_CYCLE              	 	  	(50u)
/*! Specifies the TWI Master Address register Configuration */
#define ADI_TWI3_MSTRADDR_ADDR 				 	 	(255u)
/*! Specifies the TWI Slave address register Configuration */
#define ADI_TWI3_SLVADDR_ADDR				     	(255u)


/*!TWI4 Instance*/
/*!Select TWI4 Instance */
#define ADI_TWI4_INSTANCE                    		 1u

/*! Specifies the Prescale value for internal time reference*/
#define ADI_TWI4_CTL_PRESCALE              	 	  	(12u)
/*! Specifies the TWI clock frequency in kHz*/
#define ADI_TWI4_CLK_FREQ              	 	 	  	(400u)
/*! Specifies the TWI clock duty cycle */
#define ADI_TWI4_DUTY_CYCLE              	 	  	(50u)
/*! Specifies the TWI Master Address register Configuration */
#define ADI_TWI4_MSTRADDR_ADDR 				 	 	(255u)
/*! Specifies the TWI Slave address register Configuration */
#define ADI_TWI4_SLVADDR_ADDR				     	(255u)


/*!TWI5 Instance*/
/*!Select TWI5 Instance */
#define ADI_TWI5_INSTANCE                    		 1u

/*! Specifies the Prescale value for internal time reference*/
#define ADI_TWI5_CTL_PRESCALE              	 	  	(12u)
/*! Specifies the TWI clock frequency in kHz*/
#define ADI_TWI5_CLK_FREQ              	 	 	  	(400u)
/*! Specifies the TWI clock duty cycle */
#define ADI_TWI5_DUTY_CYCLE              	 	  	(50u)
/*! Specifies the TWI Master Address register Configuration */
#define ADI_TWI5_MSTRADDR_ADDR 				 	 	(255u)
/*! Specifies the TWI Slave address register Configuration */
#define ADI_TWI5_SLVADDR_ADDR				     	(255u)


#endif /*__ADI_TWI_CONFIG_SC8XX_H__*/

/**@}*/

/**@}*/

