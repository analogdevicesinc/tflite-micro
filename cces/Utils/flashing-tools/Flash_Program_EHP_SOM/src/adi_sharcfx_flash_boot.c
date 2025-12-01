/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/
/*****************************************************************************
 * adi_sharcfx_flash_boot.c
 *****************************************************************************/


#include "adi_sharcfx_flash_boot.h"


/*****************************************************************************************************************************************************************/

#define 	WR_Block_ID		0
#define 	RD_Block_ID		WR_Block_ID

__attribute__((section(".L3.data")))
unsigned char Write_BUFF[] = {
// Replace your flash-able built ldr files here.
#include "dtln_denoiser_realtime.ldr"
//#include "kws_realtime.ldr"
};



//section ("seg_sdram")
unsigned char Read_BUFF[10];/*[sizeof((Write_BUFF)/2)];*/
bool result;
/*****************************************************************************************************************************************************************/

void Init_PLL(void);
void Init_pinmux(void);
//void Init_DataBUFF(void);
void Init_SPI1(void);
void Verify_flash_codes(void);
void Erase_flash(unsigned char ERS_BLK_ID, unsigned long long data_count);
void Write_flash(unsigned char Block_ID, unsigned char* flash_page_data_buff, unsigned long long data_count);
void Read_flash(unsigned char Block_ID, unsigned char* flash_page_data_buff, unsigned long long data_count);
bool check_result(void);

unsigned char flash_byte_access(unsigned char Databyte);
void wait_for_flash_status(char flag, bool state);
void Erase_flash_block(unsigned char Block_ID);
void Write_flash_page(unsigned int PAGE_ADDR, unsigned char* flash_page_data_buff, unsigned short page_data_count);
/*****************************************************************************************************************************************************************/
int itemp=0;

int main(void)
{
	/* Initialize managed drivers and/or services */
	//adi_initComponents();


	//Init_PLL();

	//Switch_Enable(SPI1_FLASH_CS_EN,0);
	//Switch_Enable(SPI1_D2_D3_EN,0);
	itemp=sizeof(Write_BUFF);
    Init_pinmux();
	Init_SPI1();
	Verify_flash_codes();

    //Erase_flash(0, sizeof(Write_BUFF));
	Erase_flash(WR_Block_ID, sizeof(Write_BUFF));


	Write_flash(WR_Block_ID, Write_BUFF, sizeof(Write_BUFF));
	Read_flash (RD_Block_ID, Read_BUFF,  sizeof(Read_BUFF));

	result = check_result();
	if(result)
	{
		printf("\n SUCCESS: DATA written and verified to flash \n");


	}

        else
		printf("\n ERROR: DATA writing or verification unsuccessful \n");

	printf("\nDATA written to flash \n");
	return (0);


}
/*****************************************************************************************************************************************************************/

void Init_PLL(void)
{

	  /* Check if the new MSEL is same as current MSEL Value. If so, just update the CGU_DIV register */

	   uint32_t current_MSEL = ((*pREG_CGU0_CTL & BITM_CGU_CTL_MSEL) >> BITP_CGU_CTL_MSEL);

	   if(current_MSEL == MSEL)
	   {

		   *pREG_CGU0_DIV =  (UPDT | OSEL | DSEL | S1SEL | S0SEL | SYSSEL | CSEL);
	   }

	   else
	   {
		   *pREG_CGU0_CTL = (MSEL | DF);
		   /* Check if PLL is enabled , PLL is not locking and Clocks are aligned */
		   while( !((*pREG_CGU0_STAT & BITM_CGU_STAT_CLKSALGN ) ==(~BITM_CGU_STAT_CLKSALGN)));
		   *pREG_CGU0_DIV =  (OSEL | DSEL | S1SEL | S0SEL | SYSSEL | CSEL);
		   *pREG_CGU0_CTL = MSEL ;
		   /* Wait till PLL is locked and clocks are aligned */
		   while( !((*pREG_CGU0_STAT & (BITM_CGU_STAT_PLOCK | BITM_CGU_STAT_CLKSALGN) ) == (BITM_CGU_STAT_PLOCK & (~BITM_CGU_STAT_CLKSALGN))));

	   }


}

/*****************************************************************************************************************************************************************/


bool check_result(void)
{
	unsigned long long count,	BUFF_size = sizeof(Read_BUFF);
	for(count=0; count<BUFF_size; count++)
	{
		if(Read_BUFF[count] != Write_BUFF[count])
		{
			return false;
		}
	}
	return true;
}
/*****************************************************************************************************************************************************************/


void Init_pinmux(void)
{

//	*pREG_PORTB_MUX = 0;
//	*pREG_PORTB_FER = (BITM_PORT_FER_PX10 |BITM_PORT_FER_PX7| BITM_PORT_FER_PX12 | BITM_PORT_FER_PX13 | BITM_PORT_FER_PX14| BITM_PORT_FER_PX11 | BITM_PORT_FER_PX15);

//	*pREG_PORTC_MUX = 0;
//	*pREG_PORTC_FER = (BITM_PORT_FER_PX0 );
//
//	*pREG_PORTD_MUX = 0;
//	*pREG_PORTD_FER = (BITM_PORT_FER_PX8 );
//
//	*pREG_PORTF_MUX = 0;
//	*pREG_PORTF_FER = (BITM_PORT_FER_PX10 );
//
	*pREG_PORTA_MUX = (1<<20)|(1<<22)|(1<<24)|(1<<26)|(1<<28)|(1<<30);
	*pREG_PORTA_FER = (BITM_PORT_FER_PX10 |BITM_PORT_FER_PX11|BITM_PORT_FER_PX12|BITM_PORT_FER_PX13|BITM_PORT_FER_PX14|BITM_PORT_FER_PX15);
}
/*****************************************************************************************************************************************************************/


void Init_SPI1(void)
{
	*pREG_SPI1_CLK	 = 9;
	*pREG_SPI1_DLY	 = 0;	//((1 << BITP_SPI_DLY_STOP) & BITM_SPI_DLY_STOP) ;
	*pREG_SPI1_SLVSEL= ENUM_SPI_SLVSEL_SSEL1_HI | ENUM_SPI_SLVSEL_SSEL1_EN;
	*pREG_SPI1_SLVSEL= ENUM_SPI_SLVSEL_SSEL1_HI | ENUM_SPI_SLVSEL_SSEL1_EN;

	*pREG_SPI1_CTL 	 = ENUM_SPI_CTL_MASTER ;
	*pREG_SPI1_TXCTL = ENUM_SPI_TXCTL_TTI_EN | ENUM_SPI_TXCTL_TWC_EN | ENUM_SPI_TXCTL_ZERO ;
	*pREG_SPI1_RXCTL = ENUM_SPI_RXCTL_OVERWRITE ;

	*pREG_SPI1_TXCTL |= ENUM_SPI_TXCTL_TX_EN;	//	enable Tx SPI
	*pREG_SPI1_RXCTL |= ENUM_SPI_RXCTL_RX_EN;	//	enable Rx SPI
	*pREG_SPI1_CTL 	 |= ENUM_SPI_CTL_EN;
}
/*****************************************************************************************************************************************************************/


void Verify_flash_codes(void)
{
	unsigned char FLASH_MAN_ID;
	unsigned short FLASH_DEV_ID;

	*pREG_SPI1_SLVSEL &= ~ENUM_SPI_SLVSEL_SSEL1_HI ;			//	select flash
	*pREG_SPI1_SLVSEL &= ~ENUM_SPI_SLVSEL_SSEL1_HI ;

	flash_byte_access(FLASH_RD_JDCID);							//	send command for reading flash Identification codes
	FLASH_MAN_ID  = (flash_byte_access(DUMMY_BYTE));			//	Manufacturer ID
	FLASH_DEV_ID  = (flash_byte_access(DUMMY_BYTE)<<8);			//	Memory type ID15-8
	FLASH_DEV_ID |= (flash_byte_access(DUMMY_BYTE));			//	capacity ID7-0

	*pREG_SPI1_SLVSEL |= ENUM_SPI_SLVSEL_SSEL1_HI ;				//	deselct_flash;
	*pREG_SPI1_SLVSEL |= ENUM_SPI_SLVSEL_SSEL1_HI ;
/*
	if((FLASH_MAN_ID == 0x20) && (FLASH_DEV_ID ==0x16))
		printf("\n Flash Identification passed:\n\tManufactarer ID = 0xEF\tDevice ID = 0x4016\n");
	else	{
		printf("\nERROR: Flash Identification failed\n");
		while(1) asm("nop;");	}*/

}
/*****************************************************************************************************************************************************************/


void Erase_flash(unsigned char ERS_BLK_ID, unsigned long long data_count)
{
	unsigned char ERS_NUM_BLK = ( data_count / FLASH_BLOCK_SIZE ) + 1 ;
	
	if((ERS_BLK_ID+ERS_NUM_BLK) > FLASH_BLOCK_COUNT)	{
		printf("\nERROR: Flash Block ID may exceed maximum supported blocks\n");
		while(1) asm("nop;");				}

	printf("\nErasing the Blocks:\n");
	do{
		Erase_flash_block(ERS_BLK_ID);
		printf("\tSUCCESS: Block %d Erased\n", ERS_BLK_ID);
		ERS_NUM_BLK--; ERS_BLK_ID++;
	}while(ERS_NUM_BLK);
	printf("Erasing Completed\n ");
}
/*****************************************************************************************************************************************************************/


void Write_flash(unsigned char Block_ID, unsigned char* flash_page_data_buff, unsigned long long data_count)
{
	if(Block_ID > FLASH_BLOCK_COUNT-1)	{
		printf("\nERROR: Invalid Flash Block ID\n");
		while(1) asm("nop;");		}

	unsigned int PRGM_ADDR = Block_ID * FLASH_BLOCK_SIZE;
	printf("\nPogramming the Flash from Block %d:\n", Block_ID);
	while(data_count >= FLASH_PAGE_SIZE)
	{
		Write_flash_page(PRGM_ADDR, flash_page_data_buff, FLASH_PAGE_SIZE);
	//	printf("\tSUCCESS: Flash Page %d programmed\n", PRGM_ADDR>>8);
		data_count		-= FLASH_PAGE_SIZE;
		flash_page_data_buff	+= FLASH_PAGE_SIZE;
		PRGM_ADDR		+= FLASH_PAGE_SIZE;
	}

	if(data_count != 0x00)
	{
		Write_flash_page(PRGM_ADDR, flash_page_data_buff, data_count);
	//	printf("\tSUCCESS: Flash Page %d programmed\n", PRGM_ADDR>>8);
		flash_page_data_buff	+= data_count;
		PRGM_ADDR		+= data_count;
	}
	printf("Flash Programming Completed\n ");
}
/*****************************************************************************************************************************************************************/


void Read_flash(unsigned char Block_ID, unsigned char* flash_page_data_buff, unsigned long long data_count)
{
	if(Block_ID > FLASH_BLOCK_COUNT-1)		{
		printf("\nERROR: Invalid Flash Block ID\n");
		while(1) asm("nop;");			}

	unsigned int READ_ADDR = Block_ID * FLASH_BLOCK_SIZE;
	printf("\nReading the Flash from Block %d:\n", Block_ID);
	*pREG_SPI1_SLVSEL &= ~ENUM_SPI_SLVSEL_SSEL1_HI ;			//	selct_flash;
	*pREG_SPI1_SLVSEL &= ~ENUM_SPI_SLVSEL_SSEL1_HI ;

	flash_byte_access(FLASH_RD_DAT);					//	send sector erase command (0x64KB)
	flash_byte_access(READ_ADDR >> 16);					//	send block address
	flash_byte_access(READ_ADDR >> 8);
	flash_byte_access(READ_ADDR);

	unsigned long long RD_count;
	for(RD_count=0; RD_count<data_count; RD_count++)
		*flash_page_data_buff++ = (flash_byte_access(DUMMY_BYTE));

	*pREG_SPI1_SLVSEL |= ENUM_SPI_SLVSEL_SSEL1_HI ;				//	deselct_flash;
	*pREG_SPI1_SLVSEL |= ENUM_SPI_SLVSEL_SSEL1_HI ;
	printf("Flash Reading Completed\n");
}
/*****************************************************************************************************************************************************************/


unsigned char flash_byte_access(unsigned char Databyte)
{
	//	read RFIFO till it is empty
	while(!(*pREG_SPI1_STAT & BITM_SPI_STAT_RFE))
		*pREG_SPI1_RFIFO;

	*pREG_SPI1_TWC	= 	1;								//	single byte instruction, no addr/data
	*pREG_SPI1_TFIFO = Databyte;						//	command ID
	while(!(*pREG_SPI1_STAT & BITM_SPI_STAT_TF));		//	wait till completion
	*pREG_SPI1_STAT = BITM_SPI_STAT_TF;					//	clear latch
	return *pREG_SPI1_RFIFO;
}
/*****************************************************************************************************************************************************************/


void wait_for_flash_status(char flag, bool state)
{
	unsigned char flash_status;
	*pREG_SPI1_SLVSEL &= ~ENUM_SPI_SLVSEL_SSEL1_HI ;			//	selct_flash;
	*pREG_SPI1_SLVSEL &= ~ENUM_SPI_SLVSEL_SSEL1_HI ;
	flash_byte_access(FLASH_RD_STAT1);					//	send read status register command

	if(state)  {
		do {
			flash_status = flash_byte_access(FLASH_RD_STAT1);
		   } while(!(flash_status & flag));		}
	else	   {
		do {
			flash_status = flash_byte_access(FLASH_RD_STAT1);
		   } while(flash_status & flag);		}

	*pREG_SPI1_SLVSEL |= ENUM_SPI_SLVSEL_SSEL1_HI ;				//	deselct_flash;
	*pREG_SPI1_SLVSEL |= ENUM_SPI_SLVSEL_SSEL1_HI ;
}
/*****************************************************************************************************************************************************************/


void Erase_flash_block(unsigned char Block_ID)
{
	unsigned int Erase_ADDR = Block_ID * FLASH_BLOCK_SIZE;
	if(Block_ID > FLASH_BLOCK_COUNT-1)		{
		printf("\nERROR: Invalid Flash Block ID\n");
		while(1) asm("nop;");	}

	*pREG_SPI1_SLVSEL &= ~ENUM_SPI_SLVSEL_SSEL1_HI ;			//	selct_flash;
	*pREG_SPI1_SLVSEL &= ~ENUM_SPI_SLVSEL_SSEL1_HI ;
	flash_byte_access(FLASH_WR_EN);						//	send write enable command
	*pREG_SPI1_SLVSEL |= ENUM_SPI_SLVSEL_SSEL1_HI ;				//	deselct_flash;
	*pREG_SPI1_SLVSEL |= ENUM_SPI_SLVSEL_SSEL1_HI ;
	wait_for_flash_status(WEL, 1);					//	check whether WEN bit set properly


	*pREG_SPI1_SLVSEL &= ~ENUM_SPI_SLVSEL_SSEL1_HI ;			//	selct_flash;
	*pREG_SPI1_SLVSEL &= ~ENUM_SPI_SLVSEL_SSEL1_HI ;

	flash_byte_access(FLASH_BLK_ERS_L);					//	send sector erase command (0x64KB)
	flash_byte_access(Erase_ADDR >> 16);					//	send block address
	flash_byte_access(Erase_ADDR >> 8);
	flash_byte_access(Erase_ADDR);

	*pREG_SPI1_SLVSEL |= ENUM_SPI_SLVSEL_SSEL1_HI ;				//	deselct_flash;
	*pREG_SPI1_SLVSEL |= ENUM_SPI_SLVSEL_SSEL1_HI ;

	wait_for_flash_status(WIP, 0);
}
/*****************************************************************************************************************************************************************/


void Write_flash_page(unsigned int PAGE_ADDR, unsigned char* flash_page_data_buff, unsigned short page_data_count)
{
	*pREG_SPI1_SLVSEL &= ~ENUM_SPI_SLVSEL_SSEL1_HI ;			//	selct_flash;
	*pREG_SPI1_SLVSEL &= ~ENUM_SPI_SLVSEL_SSEL1_HI ;
	flash_byte_access(FLASH_WR_EN);						//	send write enable command
	*pREG_SPI1_SLVSEL |= ENUM_SPI_SLVSEL_SSEL1_HI ;				//	deselct_flash;
	*pREG_SPI1_SLVSEL |= ENUM_SPI_SLVSEL_SSEL1_HI ;
	wait_for_flash_status(WEL, 1);


	*pREG_SPI1_SLVSEL &= ~ENUM_SPI_SLVSEL_SSEL1_HI ;			//	selct_flash;
	*pREG_SPI1_SLVSEL &= ~ENUM_SPI_SLVSEL_SSEL1_HI ;
	flash_byte_access(FLASH_PG_PRM);					//	send page program command
	flash_byte_access(PAGE_ADDR >> 16);
	flash_byte_access(PAGE_ADDR >> 8);
	flash_byte_access(PAGE_ADDR);

	int i;
	for(i=0; i<page_data_count; i++)
		flash_byte_access(*flash_page_data_buff++);

	*pREG_SPI1_SLVSEL |= ENUM_SPI_SLVSEL_SSEL1_HI ;				//	deselct_flash;
	*pREG_SPI1_SLVSEL |= ENUM_SPI_SLVSEL_SSEL1_HI ;

	wait_for_flash_status(WIP, 0);
}
/*****************************************************************************************************************************************************************/

