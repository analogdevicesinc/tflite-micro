/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/
/*****************************************************************************
 * adi_sharcfx_flash_boot.h
 *****************************************************************************/

#ifndef __ADI_SHARCFX_FLASH_BOOT_H__
#define __ADI_SHARCFX_FLASH_BOOT_H__

/* Add your custom header content here */
#include <sys/platform.h>
#include <stdio.h>
#include "stdlib.h"
#include <stdint.h>
#include <stdbool.h>

#ifndef SPI_FLASH_DRIVER_BD_H
#define SPI_FLASH_DRIVER_BD_H

/* flash commands */
#define	FLASH_WR_EN			(0x06)  //	Set Write Enable Latch
#define FLASH_WR_DI			(0x04)  //	Reset Write Enable Latch
#define SPI_RDID            (0x9F)  //	Read Identification
#define FLASH_RD_STAT1		(0x05)  //	Read Status Register1
#define FLASH_RD_STAT2		(0x35)  //	Read Status Register2
#define FLASH_WR_STAT		(0x01)	//	Write status register

#define	FLASH_SECT_ERS		(0x20)	//	sector Erase (4KB)
#define FLASH_BLK_ERS_S		(0x52)	//	Block Erase (32KB)
#define FLASH_BLK_ERS_L		(0xD8)	//	Block Erase (64KB)
#define FLASH_CHIP_ERS		(0x60)	//	Chip Erase

#define FLASH_PG_PRM		(0x02)	//	Page Program

#define	FLASH_RD_DAT		(0x03)	//	Read Data
#define	FLASH_FST_RD		(0xB)	//	Fast Read mode

#define	FLASH_RD_DEVID		(0xAB)	//	Read Device ID
#define FLASH_RD_MANID		(0x90)	//	Read Manufacturer ID + Device ID
#define FLASH_RD_JDCID		(0x9F)	//	Read JEDEC ID



#define DUMMY_BYTE			(0x00)	//dummy word, used in read operations
#define WIP                  (0x1)	//Check the write in progress bit of the SPI status register
#define WEL                  (0x2)	//Check the write enable bit of the SPI status register

#define FLASH_PAGE_SIZE		256
#define FLASH_BLOCK_SIZE	0x10000
#define	FLASH_BLOCK_COUNT	255

/*****CGU Settings ****/

/* PLL Multiplier and Divisor Selections (Required Value, Bit Position) */

#define MSEL   ((16 << BITP_CGU_CTL_MSEL)   & BITM_CGU_CTL_MSEL)   /* PLL Multiplier Select [1-127]: PLLCLK = ((CLKIN x MSEL/DF+1)) = 800MHz(max) */
#define DF   	((0   << BITP_CGU_CTL_DF) 	& BITM_CGU_CTL_DF)    /* Set this if DF is required */


#define CSEL   ((1  << BITP_CGU_DIV_CSEL)   & BITM_CGU_DIV_CSEL)   /* Core Clock Divisor Select [1-31]: (CLKIN x MSEL/DF+1)/CSEL = 400MHz(max)     */
#define SYSSEL ((2  << BITP_CGU_DIV_SYSSEL) & BITM_CGU_DIV_SYSSEL) /* System Clock Divisor Select [1-31]: (CLKIN x MSEL/DF+1)/SYSSEL = 200MHz(max) */
#define S0SEL  ((2  << BITP_CGU_DIV_S0SEL)  & BITM_CGU_DIV_S0SEL)  /* SCLK0 Divisor Select [1-7]: SYSCLK/S0SEL = 100MHz(max)                       */
#define S1SEL  ((2  << BITP_CGU_DIV_S1SEL)  & BITM_CGU_DIV_S1SEL)  /* SCLK1 Divisor Select [1-7]: SYSLCK/S1SEL = 200MHz(max)                       */
#define DSEL   ((2  << BITP_CGU_DIV_DSEL)   & BITM_CGU_DIV_DSEL)   /* DDR Clock Divisor Select [1-31]: (CLKIN x MSEL/DF+1)/DSEL = 200MHz(max)      */
#define OSEL   ((8  << BITP_CGU_DIV_OSEL)   & BITM_CGU_DIV_OSEL)   /* OUTCLK Divisor Select [1-127]: (CLKIN x MSEL/DF+1)/OSEL = 50 MHz(max)        */

#define UPDT   	((1   << BITP_CGU_DIV_UPDT) 	& BITM_CGU_DIV_UPDT)

#endif

#endif /* __ADI_SHARCFX_FLASH_BOOT_H__ */
