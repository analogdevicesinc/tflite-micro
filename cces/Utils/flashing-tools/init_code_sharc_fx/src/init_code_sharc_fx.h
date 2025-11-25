/*****************************************************************************
 * init_code_sharc_fx.h
 *****************************************************************************/

#ifndef __INIT_CODE_SHARC_FX_H__
#define __INIT_CODE_SHARC_FX_H__

/* Add your custom header content here */
#define DMC_INIT
//#define PWR_INIT
//#define CONFIG_BOOT_UART_BAUD_RATE
//#define Callback
#define ENABLE_4_BYTE_ADDRESSING

#define REG_SPI1_MMRDH                       0x3102F060            /*  SPI1 Memory Mapped Read Header */
#define REG_SPI1_MMTOP                       0x3102F064            /*  SPI1 SPI Memory Top Address */

#define pREG_SPI1_MMRDH                  ((__IO     uint32_t *)  REG_SPI1_MMRDH)                  /*  Memory Mapped Read Header */
#define pREG_SPI1_MMTOP                  ((__IO     uint32_t *)  REG_SPI1_MMTOP)                  /*  SPI Memory */

#endif /* __INIT_CODE_SHARC_FX_H__ */
