/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/
/*****************************************************************************
 * bootloader_sharcfx.cpp
 *****************************************************************************/
#include "adi_initialize.h"
#include "bootloader_sharcfx.h"

#include <sys/platform.h>

#include <sys/platform.h>
#include <sys/adi_core.h>

#include <cdefSC83x_rom.h>
#include <defSC83x_rom.h>

#include "debug.h"

#define Single
//#ifdef Dual
//#ifdef Quad

/* ---------------------------------------------------------------------------
 * uart_wait_for_key()
 *
 * Polls the UART0 RX data-ready bit (BITM_UART_STAT_DR) using the Xtensa
 * core cycle counter (CCOUNT) for timing.  Does NOT rely on OSAL ticks so
 * it works correctly in this bare-metal bootloader which has no timer ISR.
 *
 * CCLK = 1 GHz -> 1,000,000,000 cycles per second.
 * CCOUNT is 32-bit and wraps every ~4.3 s, so we count in 1-second windows.
 *
 * Returns 1 if a key is received within timeout_sec, 0 on timeout.
 * ---------------------------------------------------------------------------*/
#define BOOTLOADER_TIMEOUT_SEC   10u
#define CCLK_CYCLES_PER_SEC      1000000000u

static int uart_wait_for_key(void)
{
    uint32_t start = XT_RSR_CCOUNT();
    uint32_t sec   = 0u;

    while (sec < BOOTLOADER_TIMEOUT_SEC)
    {
        /* Poll UART0 hardware status register � no OSAL dependency */
        if (*pREG_UART0_STAT & BITM_UART_STAT_DR)
            return 1;  /* byte waiting in RX FIFO */

        uint32_t elapsed = XT_RSR_CCOUNT() - start;  /* handles 32-bit wrap */
        if (elapsed >= CCLK_CYCLES_PER_SEC)
        {
            start += CCLK_CYCLES_PER_SEC;
            sec++;
        }
    }
    return 0;  /* timed out */
}

void Init_Twi1PinMux(void)
{
	/* PORTx_MUX registers */
	*pREG_PORTB_MUX |= TWI1_SCL_PORTB_MUX | TWI1_SDA_PORTB_MUX;

	/* PORTx_FER registers */
	*pREG_PORTB_FER |= TWI1_SCL_PORTB_FER | TWI1_SDA_PORTB_FER;
}

void Init_Twi2PinMux(void)
{
	/* PORTx_MUX registers */
	*pREG_PORTA_MUX |= TWI2_SCL_PORTA_MUX | TWI2_SDA_PORTA_MUX;

	/* PORTx_FER registers */
	*pREG_PORTA_FER |= TWI2_SCL_PORTA_FER | TWI2_SDA_PORTA_FER;
}

void callback(void);
void load(void);
int32_t ROM_BOOT_HOOK_FUNC2(ADI_ROM_BOOT_CONFIG * pBootConfig, ROM_HOOK_CALL_CAUSE cause);

void ConfigSoftSwitches_EV_SOMCRR_EZKIT(void);


uint32_t BootCmd=0;
uint32_t START_ADDRESS=0;

extern void SoftConfig_EV_SC835_SOM(void);

int main(int argc, char *argv[])
{
	/**
	 * Initialize managed drivers and/or services that have been added to 
	 * the project.
	 * @return zero on success 
	 */
	*pREG_RCU0_MSG = 0x0;

	adi_initComponents();

	*pREG_SPU0_SECUREP94 = 0x3;
	*pREG_SPU0_SECUREP86 = 0x3;
	*pREG_SPU0_SECUREP87 = 0x3;
	*pREG_SPU0_SECUREP137 = 0x3;
	*pREG_SPU0_SECUREP138 = 0x3;
	*pREG_SPU0_SECUREP93 = 0x3;
	*pREG_SPU0_SECUREP84 = 0x3;
	*pREG_SPU0_SECUREP85 = 0x3;

	START_ADDRESS = 0x70080000;

	/* Initialize the power service to CLKIN=25MHz. This API is called so that we can use other low level APIs to
	 * modify individual clocks. adi_pwr_SetFreq() is not called and the default CGU0 settings done in init_code
	 * are used
	 * (For ADSP-21593 - CCLK: 1000 MHz , SYSCLK: 500 MHz) */
	adi_pwr_Init(CGU_DEV, CLKIN);

	adi_pwr_cfg0_init();

	/* Initialize the TWI pin mux for any of the soft config access through TWI1 (SOM board) and TWI2 (EZLITE board) */
	Init_Twi1PinMux();
	Init_Twi2PinMux();

	/* Initialize UART */
	if (Init_UART() != PASSED)
	{
		PRINT_INFO("Could not Initialize \n");
		return FAILED;
	}

	set_print_func(uart_print);
	ConfigSoftSwitches_EV_SOMCRR_EZKIT();


#ifdef SPI_MASTER
#ifdef Single
	BootCmd =(1<<BITP_ROM_BCMD_SPIM_SPEED)|(2<<BITP_ROM_BCMD_SPIM_ADDR)|(1<<BITP_ROM_BCMD_SPIM_DUMMY)|
			(2<<BITP_ROM_BCMD_SPIM_BCODE)|(1<<BITP_ROM_BCMD_DEVENUM)|
			(1<<BITP_ROM_BCMD_SPIM_NOAUTO)|(7<<BITP_ROM_BCMD_DEVICE);
#endif
#ifdef Dual
	BootCmd =(1<<BITP_ROM_BCMD_SPIM_SPEED)|(2<<BITP_ROM_BCMD_SPIM_ADDR)|(1<<BITP_ROM_BCMD_SPIM_DUMMY)|
			(5<<BITP_ROM_BCMD_SPIM_BCODE)|(1<<BITP_ROM_BCMD_DEVENUM)|
			(1<<BITP_ROM_BCMD_SPIM_NOAUTO)|(7<<BITP_ROM_BCMD_DEVICE);
#endif
#ifdef Quad
	BootCmd =(1<<BITP_ROM_BCMD_SPIM_SPEED)|(2<<BITP_ROM_BCMD_SPIM_ADDR)|(3<<BITP_ROM_BCMD_SPIM_DUMMY)|
			(9<<BITP_ROM_BCMD_SPIM_BCODE)|(1<<BITP_ROM_BCMD_DEVENUM)|
			(1<<BITP_ROM_BCMD_SPIM_NOAUTO)|(7<<BITP_ROM_BCMD_DEVICE);
#endif
#endif

	PRINT_INFO("\n ============================================\n");
	PRINT_INFO("           SHARC-FX Application Bootloader     \n");
	PRINT_INFO(" ============================================\n");
	PRINT_INFO(" Press a key to select an application:\n\n");
	PRINT_INFO("   [1]  Denoiser DTLN\n");
	PRINT_INFO("   [2]  Genre Identification\n");
	PRINT_INFO("   [3]  Urban Sound Classification\n");
	PRINT_INFO("   [4]  Keyword Spotter\n\n");
	PRINT_INFO(" --> Auto-booting DTLN Denoiser in 10 seconds...\n");
	PRINT_INFO(" ============================================\n");

	uint8_t cmd[1];
	/* Wait up to BOOTLOADER_TIMEOUT_SEC for a keypress by polling UART0 STAT.
	 * Avoids the OSAL-tick dependency of adi_uart_CoreRead(timeout) which fires
	 * immediately when no timer ISR is running. */
	if (uart_wait_for_key())
	{
		/* Data is ready � read exactly 1 byte (no timeout needed, byte is present) */
		UART_READ(cmd, 1, ADI_OSAL_TIMEOUT_FOREVER);
	}
	else
	{
		PRINT_INFO("\n No selection received. Defaulting to DTLN Denoiser.\n");
		cmd[0] = '1';
	}

	switch(cmd[0])
	{
		case '1':
			PRINT_INFO("\n********************** DTLN Denoiser ************************\n");
			START_ADDRESS = 0x70080000;
			break;
		case '2':
			PRINT_INFO("\n****************** Genre Identification ********************\n");
			START_ADDRESS = 0x70513e00;
			break;
		case '3':
			PRINT_INFO("\n ********************* UrbanSound Identification **********************\n");
			START_ADDRESS = 0x709A7C00;
			break;
		case '4':
			PRINT_INFO("\n********************** Keyword Spotter ************************\n");
			START_ADDRESS = 0x70E3BA00;
			break;
		default:
			PRINT_INFO("\n Invalid selection '%c'. Defaulting to DTLN Denoiser.\n", cmd[0]);
			START_ADDRESS = 0x70080000;
			break;
	}

	adi_rom_Boot((void *)START_ADDRESS, 0, 0, 0, BootCmd, 0);

	return 0;
}

int32_t ROM_BOOT_HOOK_FUNC2(ADI_ROM_BOOT_CONFIG * pBootConfig, ROM_HOOK_CALL_CAUSE cause)
{
	if (cause == ROM_HOOK_CALL_INIT_COMPLETE)
	{
		asm("NOP;");
	}
	if (cause == ROM_HOOK_CALL_CONFIG_COMPLETE)
	{
		asm("NOP;");
	}
	if (cause == ROM_HOOK_REG_COMPLETE)
	{
		asm("NOP;");
	}

	return 0;
}
