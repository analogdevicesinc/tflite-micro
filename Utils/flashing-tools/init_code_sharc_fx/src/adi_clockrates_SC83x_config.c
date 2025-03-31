/*********************************************************************************

Copyright(c) 2018 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

 *********************************************************************************/

#include "adi_clockrates_SC83x_config.h"

#include "stdio.h"
#include <sys/platform.h>
#include <services/pwr/adi_pwr.h>
#include <services/tmr/adi_tmr.h>

#ifdef _MISRA_RULES
#pragma diag(suppress:misra_rule_14_7:"Allow functions to have multiple exits for better readability and optimized code")
#pragma diag(suppress:misra_rule_5_6:"The fsysclk name etc used is the same as a fields of a type in adi_pwr.h.")
#endif


uint32_t adi_uart_byte_delay()
{
	uint8_t gTimerIntflag=0;
    ADI_TMR_HANDLE phTMR = NULL;
    ADI_TMR_CFG_PARAMS ConfigParams;
    uint8_t TimerMemory[ADI_TMR_MEMORY];
	ADI_TMR_RESULT Result;

	/*read clock div*/
	uint32_t clockdiv = *pREG_UART0_CLK;

   	*pREG_UART0_CTL &= ~BITM_UART_CTL_ARTS; /*Disable Automatic RTS */
    *pREG_UART0_CTL &= ~BITM_UART_CTL_MRTS; /* De-assert RTS manually */

	/* Open the Device */
	Result =   adi_tmr_Open (
				TimerNum,
				TimerMemory, //pMemory,
				ADI_TMR_MEMORY,// nMemSize
				NULL,
				NULL, //pCBParam,
				&phTMR	);
	  if (Result != ADI_TMR_SUCCESS)
	  {
		  return 1;
	  }

	#ifdef OVERRIDE_CONFIG_FILE
		/*Configure the device in Single PWM Out Mode*/
		ConfigParams.Mode= ADI_TMR_MODE_SINGLE_PWMOUT;
		ConfigParams.PulsePolarity= true;
		ConfigParams.ClkInSource= ADI_TMR_CLKIN_SYSCLK;
		ConfigParams.InputAuxIn= false;
		ConfigParams.Width= (2*10*16*clockdiv);
		ConfigParams.Delay= 0u;

		Result =  adi_tmr_Config(phTMR,	&ConfigParams);
		if (Result != ADI_TMR_SUCCESS)
		{
		  return 1;
		}
	#endif

	/*Enable the timer*/
	Result =   adi_tmr_Enable(phTMR,true);
	if (Result != ADI_TMR_SUCCESS)
	{
	  return 1;
	}
	/*Wait for TIMER_RUN bit to be cleared*/
	 while((*pREG_TIMER0_RUN & BITM_TIMER_RUN_TMR05)!=0);

	/*Close timer*/
	Result =   adi_tmr_Close(phTMR);
	if (Result != ADI_TMR_SUCCESS)
	{
	  return 1;
	}

	return 0;

}
/**
 * @brief   This API returns the Baud Rate of UART
 * @details This API returns the Baud Rate of UART by reading the current SCLK0 value
 *

 * @return  baud if boot mode is UART boot mode
 *          1    if there is an error determining the system clock frequency
 *          0    if boot mode is not UART boot mode
 *
 * @note
 **/
uint32_t adi_uart_baud_read()
{
    uint32_t fsysclk;
    uint32_t fsclk0;
    uint32_t fsclk1;
    uint32_t clockdiv;
    uint32_t baud;

    if ( (*pREG_RCU0_STAT & BITM_RCU_STAT_BMODE) == (3uL << BITP_RCU_STAT_BMODE))
    {
    	/*read clock div*/
        clockdiv = *pREG_UART0_CLK;

        /*get system clock frequency*/
        if(adi_pwr_GetSystemFreq(0u,&fsysclk,&fsclk0,&fsclk1) != ADI_PWR_SUCCESS)
        {
            return 1u;
        }

        /*calculate the baud rate*/
        baud = fsclk0 / (16u * clockdiv);
        return baud;
    }

    return 0u;
}

/**
 * @brief    This API programs the UART Clock register with new baud rate.
 * @details  This API programs the UART Clock register with new baud rate, by reading the new SCLK0 value
 *
 * @return      0    if boot mode is UART boot mode
 *              1    if boot mode is UART boot mode
 * @note
 **/
uint32_t adi_uart_baud_init(uint32_t UART_Baud_Rate_Val)
{
    uint32_t   fsysclk;
    uint32_t   fsclk0;
    uint32_t   fsclk1;
    uint32_t   clockdiv;



    if ( (*pREG_RCU0_STAT & BITM_RCU_STAT_BMODE) == (3uL << BITP_RCU_STAT_BMODE))
    {

        /*get system clock frequency*/
        if(adi_pwr_GetSystemFreq(0u,&fsysclk,&fsclk0,&fsclk1) != ADI_PWR_SUCCESS)
        {
            return 1u;
        }

        /*calculate the clock div*/
        clockdiv = fsclk0 / (16u * UART_Baud_Rate_Val);

        /*write clock div*/
        *pREG_UART0_CLK = clockdiv;

        *pREG_UART0_CTL |= BITM_UART_CTL_ARTS; /*Re enable the ARTS*/

        return 0u;
    }

    return 1u;
}



/**
 * @brief       Programs SPI Clock Rate
 * @details     This API can be used to program the clock rate of SPI during SPI master boot mode.
 *              It verifies first whether the boot mode is indeed SPI master boot.
 *
 * @param[in]   SPI_Clock_Rate_Val Clock Rate value
 *
 * @return      0    if boot mode is SPI Master boot mode
 *              1    if boot mode is not SPI Master boot mode
 *
 * @note
 **/
uint32_t adi_spi_clock_rate(uint32_t SPI_Clock_Rate_Val)
{
    if ( (*pREG_RCU0_STAT & BITM_RCU_STAT_BMODE) == (1uL << BITP_RCU_STAT_BMODE))
    {
        *pREG_SPI2_CLK = (uint32_t) SPI_Clock_Rate_Val;

        return 0u;
    }

    return 1u;
}



