/* Copyright (C) 2016 Analog Devices Inc., All Rights Reserved. */
/*
 * ddr_sweep.h
 *
 *  Created on: Sep 22, 2016
 *      Author: AAgarwal2
 */

#ifndef DDR_SWEEP_H_
#define DDR_SWEEP_H_

#include <stdio.h>
#include <stdbool.h>
#include <sys/platform.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>

#define DMC0_START_ADDRESS 0x80000000

/* DMA Memory transfer word size, ref CCES-15127 */
#if !defined(ENUM_DMA_CFG_PSIZE01)
  #define ENUM_DMA_CFG_PSIZE01  (_ADI_MSK_3(0x00000000,0x00000000UL, uint32_t )) /* PSIZE: 1 Byte */
#endif
#if !defined(ENUM_DMA_CFG_PSIZE02)
  #define ENUM_DMA_CFG_PSIZE02 (_ADI_MSK_3(0x00000010,0x00000010UL, uint32_t ))  /* PSIZE: 2 Bytes */
#endif
#if !defined(ENUM_DMA_CFG_MSIZE01)
  #define ENUM_DMA_CFG_MSIZE01 (_ADI_MSK_3(0x00000000,0x00000000UL, uint32_t  )) /* MSIZE: 1 Byte */
#endif
#if !defined(ENUM_DMA_CFG_MSIZE02)
  #define ENUM_DMA_CFG_MSIZE02 (_ADI_MSK_3(0x00000100,0x00000100UL, uint32_t  )) /* MSIZE: 2 Bytes */
#endif

/*************************************************************************************************************************
									Enum definitions
**************************************************************************************************************************/
//Use these to specifiy whether core/DMA access is required (e.g. for sweep test)
typedef enum ACCESS_TYPE
{
	CORE_ACCESS,
	DMA_ACCESS
} ATYPE;

//Use these to specify the word size (8/16/32/64 bits) requied e.g. for sweep test
typedef enum WORD_SIZE
{
	WORD_SIZE_8BITS  =1,
	WORD_SIZE_16BITS =2,
	WORD_SIZE_32BITS =4,
	WORD_SIZE_64BITS =8,
	WORD_SIZE_128BITS =16,
	WORD_SIZE_256BYTES = 32
} WSIZE;

//Use these to specify the data pattern in the sweep test
typedef enum PATTERN_TPYE
{
	ALL_ZEROS,
	ALL_ONES,
	ALL_A,
	ALL_5,
	INCREMENTAL_VALUES,
	RANDOM_VALUES,
	ALL_BITS_TOGGLING,
} PATTERN;

//Use these to specify the memory type in the sweep test
enum MEM_TEST_TYPE
{
	MEMTEST_L1,
	MEMTEST_L2,
	MEMTEST_DDR2,
	MEMTEST_DDR3,
	MEMTEST_LPDDR,
};

#define BUFF_SIZE 256

/*************************************************************************************************************************
									MACROs
************************************************************************************************************************/
//This macro can be used to configure an MDMA stream by providing the source and destination DMA channel details
#define MDMA_CONFIG(S_MDMA_CHANNEL,S_CFG, S_ADDRSTART, S_MOD, S_COUNT,D_MDMA_CHANNEL,D_CFG, D_ADDRSTART, D_MOD, D_COUNT,BWMLCNT)\
		*pREG_DMA##S_MDMA_CHANNEL##_CFG = S_CFG;\
		*pREG_DMA##S_MDMA_CHANNEL##_ADDRSTART = S_ADDRSTART;\
		*pREG_DMA##S_MDMA_CHANNEL##_XCNT = S_COUNT;\
		*pREG_DMA##S_MDMA_CHANNEL##_XMOD = S_MOD;\
		*pREG_DMA##D_MDMA_CHANNEL##_CFG = D_CFG;\
		*pREG_DMA##D_MDMA_CHANNEL##_ADDRSTART = D_ADDRSTART;\
		*pREG_DMA##D_MDMA_CHANNEL##_XCNT = D_COUNT;\
		*pREG_DMA##D_MDMA_CHANNEL##_XMOD = D_MOD;\
		*pREG_DMA##D_MDMA_CHANNEL##_BWMCNT=0x7FFFFFFF;\
		*pREG_DMA##D_MDMA_CHANNEL##_BWLCNT=BWMLCNT;\
		*pREG_DMA##D_MDMA_CHANNEL##_CFG |= 0x1;\
		*pREG_DMA##S_MDMA_CHANNEL##_CFG |= 0x1;

//Macro to wait for DMA done
#define WAIT_FOR_DMADONE(DMA_CHANNEL)\
		while(!(*pREG_DMA##DMA_CHANNEL##_STAT & ENUM_DMA_STAT_IRQDONE));

//Macro to disable MDMA channels
#define DISABLE_MDMA(S_MDMA_CHANNEL, D_MDMA_CHANNEL)\
		*pREG_DMA##S_MDMA_CHANNEL##_CFG = 0;\
		*pREG_DMA##D_MDMA_CHANNEL##_CFG = 0;




/*************************************************************************************************************************
									Function Declarations
**************************************************************************************************************************/
uint32_t Memory_Sweep_Test_8Bits(uint8_t* uiMUTStartAdd, uint32_t uiCount, uint32_t uiAccessType, uint32_t uiDataPattern, uint8_t* uiSrcBuffAdd, uint8_t* uiDestBuffAdd, uint32_t uiBuffSize);
uint32_t Memory_Sweep_Test_16Bits(uint16_t* uiMUTStartAdd, uint32_t uiCount, uint32_t uiAccessType, uint32_t uiDataPattern, uint16_t* uiSrcBuffAdd, uint16_t* uiDestBuffAdd, uint32_t uiBuffSize);
uint32_t Memory_Sweep_Test_32Bits(uint32_t* uiMUTStartAdd, uint32_t uiCount, uint32_t uiAccessType, uint32_t uiDataPattern, uint32_t* uiSrcBuffAdd, uint32_t* uiDestBuffAdd, uint32_t uiBuffSize);
uint32_t Memory_Sweep_Test_64Bits(uint64_t* uiMUTStartAdd, uint32_t uiCount, uint32_t uiAccessType, uint32_t uiDataPattern, uint64_t* uiSrcBuffAdd, uint64_t* uiDestBuffAdd, uint32_t uiBuffSize);


#endif /* DDR_SWEEP_H_ */
