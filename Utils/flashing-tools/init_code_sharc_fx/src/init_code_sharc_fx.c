/*****************************************************************************
 * init_code_sharc_fx.c
 *****************************************************************************/

#include <sys/platform.h>
#include "adi_clockrates_SC83x_config.h"
#include "cdefSC83x_rom.h"
#include "init_code_sharc_fx.h"




__attribute__((section(".L2.text")))
uint32_t initcode(ADI_ROM_BOOT_CONFIG* pBootStruct);
__attribute__((section(".L2.text")))
int callback(void);

void adi_spi_byteaddressing()
{

///Clear memory mapped mode and disable CTL and TXCTL
*pREG_SPI1_RXCTL=0;
*pREG_SPI1_TXCTL=0;

//Use SPI in non-memory mapped mode
*pREG_SPI1_CTL = ENUM_SPI_CTL_MASTER ;
*pREG_SPI1_TXCTL = ENUM_SPI_TXCTL_TTI_EN | ENUM_SPI_TXCTL_TWC_EN | ENUM_SPI_TXCTL_ZERO ;
*pREG_SPI1_RXCTL = ENUM_SPI_RXCTL_OVERWRITE ;

*pREG_SPI1_TXCTL |= ENUM_SPI_TXCTL_TX_EN;
*pREG_SPI1_RXCTL |= ENUM_SPI_RXCTL_RX_EN;
*pREG_SPI1_CTL |= ENUM_SPI_CTL_EN;

*pREG_SPI1_TXCTL|= ENUM_SPI_TXCTL_TWC_EN ;
*pREG_SPI1_TXCTL |= ENUM_SPI_TXCTL_TX_EN;
*pREG_SPI1_CTL |= ENUM_SPI_CTL_EN;

//select FLASH
*pREG_SPI1_SLVSEL &= ~ENUM_SPI_SLVSEL_SSEL1_HI ;
*pREG_SPI1_SLVSEL &= ~ENUM_SPI_SLVSEL_SSEL1_HI ;

//Enable 4 byte addressing mode
*pREG_SPI1_TWC = 1;
*pREG_SPI1_TFIFO = 0xB7;
while (! (*pREG_SPI1_STAT & BITM_SPI_STAT_TF)) ;
*pREG_SPI1_STAT = BITM_SPI_STAT_TF;

//Deselect FLASH
*pREG_SPI1_SLVSEL |= ENUM_SPI_SLVSEL_SSEL1_HI ;
*pREG_SPI1_SLVSEL |= ENUM_SPI_SLVSEL_SSEL1_HI ;

*pREG_SPI1_CTL  &= ~(ENUM_SPI_CTL_EN) ;
*pREG_SPI1_TXCTL &= ~(ENUM_SPI_TXCTL_TX_EN|ENUM_SPI_TXCTL_TWC_EN) ;

//set the FLASH back to memory mapped mode with 4 byte address size
*pREG_SPI1_MMRDH&=~BITM_SPI_MMRDH_ADRSIZE;
*pREG_SPI1_MMRDH |= (4<<BITP_SPI_MMRDH_ADRSIZE) ;

*pREG_SPI1_RXCTL= 0x5;
*pREG_SPI1_TXCTL = 0x5;
*pREG_SPI1_CTL  = 0x80000043;
}

uint32_t initcode(ADI_ROM_BOOT_CONFIG* pBootStruct)
{
	/*Variables*/
    uint32_t UART_Baud_Rate_Val = 0UL;

#ifdef DMC_INIT
	/*Initialize Power Service*/
	int result = (uint32_t)adi_pwr_Init((uint8_t)0, (uint32_t)25000000);
	if(result != 0)
	{
		//printf("PWR Init Failed\n");
	}

	/* Set DMC Lane Reset */
	adi_dmc_lane_reset(true);

	/* Initialize CGU and CDU */
	result = (uint32_t)adi_pwr_cfg0_init();
	if(result != 0)
	{
		//printf("PWR Config Failed\n");
	}

	/* Clear DMC Lane Reset */
	adi_dmc_lane_reset(false);

	/* Initialize DMC */
	result = adi_dmc_cfg0_init();
	if(result != 0)
	{
		//printf("DMC Config Failed\n");
	}
#endif

#ifdef PWR_INIT
	/*Initialize Power Service*/
	int result = (uint32_t)adi_pwr_Init((uint8_t)0, (uint32_t)25000000);
#endif

#ifdef  CONFIG_BOOT_UART_BAUD_RATE
    /* Read UART Baud Rate*/
    UART_Baud_Rate_Val = adi_uart_baud_read();

   	*pREG_UART0_CTL &= ~BITM_UART_CTL_ARTS; /*Disable Automatic RTS */
   *pREG_UART0_CTL &= ~BITM_UART_CTL_MRTS; /* De-assert RTS manually */

    /*This delay is needed*/
    for(volatile int i =0; i<0xFFFFFF; i++);

#endif

#ifdef PWR_INIT
	/* Initialize CGU and CDU */
	result = (uint32_t)adi_pwr_cfg0_init();
#endif



#ifdef Callback
	/*Call_back_Function*/
	pBootStruct->pCallBackFunction = (ROM_BOOT_CALLBACK_FUNC *)&callback;
#endif


#ifdef CONFIG_BOOT_UART_BAUD_RATE
    /* Reinitialize UART Baud Rate*/
    result = adi_uart_baud_init(UART_Baud_Rate_Val);
#endif

#ifdef ENABLE_4_BYTE_ADDRESSING
    adi_spi_byteaddressing();
#endif
	/*RCU MSG to know init code is executed*/
	*pREG_RCU0_MSG = 0xEE;

    return 0u;
}

/*Call_Back*/
int callback()
{
	*pREG_RCU0_MSG |=0x10;
	return 0u;
}
