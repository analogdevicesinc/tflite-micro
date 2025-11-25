
/* To be added SPU and memory translation */

#include "ddr_sweep.h"


#define DEBUG_PRINT printf
//#define printf UART_PRINT

//#define DEBUG_PRINT debugf
#ifdef UART_VIEW
	#define putchar(c) UART_putc(c)
#endif



#ifdef ADI_DEBUG_PRIVATE
	#include "adi_dmc_def.h"
#endif




void MdmaAccess(uint32_t uiSourceDMACFG,uint8_t* pSrcBuf,uint32_t uiSourceMSIZE,uint32_t uiBuffSize1,
		uint32_t uiDestDMACFG, uint8_t* pDestBuff, uint32_t uiDestMSIZE,uint32_t uiBuffSize2)
{
	MDMA_CONFIG(8,uiSourceDMACFG, (uint8_t*)adi_rtl_internal_to_system_addr((uint32_t)pSrcBuf,0), uiSourceMSIZE, uiBuffSize1,9,uiDestDMACFG, (uint8_t*)adi_rtl_internal_to_system_addr((uint32_t)pDestBuff,0), uiDestMSIZE, uiBuffSize2,0x0);
	WAIT_FOR_DMADONE(9);
	DISABLE_MDMA(8,9);
}




/**************************************************************************************************************************
	Function name	:  Memory_Sweep_Test_8Bits
    Description		:  This function can be used to perform a memory sweep test for 8 bit accesses
	Arguments		:	Parameter					 |	Description		 							| Valid values
						uiMUTStartAdd				 |  Start address of the memory under test 		| 8 bit aligned address
						uiCount						 |  Total number of words to be swept			| unsigned int 32 bit
						uiAccessType				 |  Type of access (core/DMA)					| 0=core, 1= DMA
						uiDataPattern				 |  Type of pattern								| 0=all zeros, 1= all ones,2=all A's,
																									  3= all 5's, 4=incremental, 5 =random
						sBuffAdd			     	 |  Source buffer start address for testing 	| 8 bit aligned address
						uiDestBuffAdd				 |  Destination buffer start address for testing| 8 bit aligned address
						uiBuffSize					 |  Size of the test buffer						| unsigned int 32 bit
	Return value	:   0=for no failure, non-zero= number of failures.
**************************************************************************************************************************/
uint32_t Memory_Sweep_Test_8Bits(uint8_t* uiMUTStartAdd, uint32_t uiCount, uint32_t uiAccessType, uint32_t uiDataPattern, uint8_t* uiSrcBuffAdd, uint8_t* uiDestBuffAdd, uint32_t uiBuffSize)
{
	int i,j;

	uint8_t* pSrcBuff; uint8_t* pDestBuff; uint8_t* pMUTRead; uint8_t* pMUTWrite;

	uint32_t uiFailCount=0;

	pSrcBuff=(uint8_t*)uiSrcBuffAdd;

	uint32_t uiSourceMSIZE  = 	1;
	uint32_t uiDestMSIZE 	=	1;
	uint32_t uiSourceDMACFG = 	ENUM_DMA_CFG_MSIZE01|ENUM_DMA_CFG_PSIZE04;
	uint32_t uiDestDMACFG   =   BITM_DMA_CFG_WNR|ENUM_DMA_CFG_PSIZE04|ENUM_DMA_CFG_MSIZE01|ENUM_DMA_CFG_XCNT_INT;

	switch (uiAccessType){
		case CORE_ACCESS:
			DEBUG_PRINT("\nCORE_ACCESS\n");
			break;
		case DMA_ACCESS:
			DEBUG_PRINT("\nDMA_ACCESS\n");
			break;
	}
	DEBUG_PRINT("Test Word size = 1 byte\n");

	//Fill the source buffer with data based on the pattern type argument
	switch (uiDataPattern)
	{
			case ALL_ZEROS:
						DEBUG_PRINT("Data pattern = All ZEROS\n");
						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=0x0;
							pSrcBuff++;
						}
						break;

			case ALL_ONES:
						DEBUG_PRINT("Data pattern = All ONEs\n");
						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=0xFF;
							pSrcBuff++;
						}
						break;

			case ALL_A:
						DEBUG_PRINT("Data pattern = All A\n");
						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=0xAA;
							pSrcBuff++;
						}
						break;

			case ALL_5:
						DEBUG_PRINT("Data pattern = All 5\n");
						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=0x55;
							pSrcBuff++;
						}
						break;

			case INCREMENTAL_VALUES:
						DEBUG_PRINT("Data pattern = INCREMENTAL\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=(uint8_t)i;
							pSrcBuff++;
						}
						break;

			case RANDOM_VALUES:
						DEBUG_PRINT("Data pattern = RANDOM\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=(uint8_t)(rand()%0xFF);
							 pSrcBuff++;
						}
						break;

			case ALL_BITS_TOGGLING:
						DEBUG_PRINT("Data pattern = ALL_BITS_TOGGLING\n");

						for(i=0;i<uiBuffSize/4;i++)
						{
							*pSrcBuff=0xFF; pSrcBuff++;
							*pSrcBuff=0xFF; pSrcBuff++;
							*pSrcBuff=0; pSrcBuff++;
							*pSrcBuff=0; pSrcBuff++;
						}
						break;
	};

	//Initialize pointers
	pMUTRead=uiMUTStartAdd;
	pMUTWrite=uiMUTStartAdd;

	//Now perform the test
	for(i=0;i<uiCount;i=i+uiBuffSize)
	{
		pSrcBuff=uiSrcBuffAdd;
		pDestBuff=uiDestBuffAdd;


		if(uiAccessType==CORE_ACCESS)
		{
			//Perform core write accesses to transfer data from source buffer to the target memory location under test
			for(j=0;j<uiBuffSize;j++)
			{
				*pMUTWrite=*pSrcBuff;
				pMUTWrite++;
				pSrcBuff++;

			}
		}
		else if(uiAccessType==DMA_ACCESS)
		{
			//Perform DMA write accesses to transfer data from source buffer to the target memory location under test
             MdmaAccess(uiSourceDMACFG,pSrcBuff, uiSourceMSIZE, uiBuffSize,uiDestDMACFG, pMUTWrite, uiDestMSIZE, uiBuffSize);

			//Increment the write pointer
			pMUTWrite+=uiBuffSize;
		}

		if(uiAccessType==CORE_ACCESS)
		{
			//Perform core read accesses to transfer data from the memory location under test and to the dest buffer
			for(j=0;j<uiBuffSize;j++)
			{
				*pDestBuff=*pMUTRead;
				pMUTRead++;
				pDestBuff++;

			}
		}

		else if(uiAccessType==DMA_ACCESS)
		{
			//Perform DMA read accesses to transfer data from the memory location under test and to the dest buffer
			MdmaAccess(uiSourceDMACFG,pMUTRead,uiSourceMSIZE,uiBuffSize,uiDestDMACFG, pDestBuff, uiDestMSIZE, uiBuffSize);
			//Increment the read pointer
			pMUTRead+=uiBuffSize;
		}

		//Re-initialize the source and dest buffer pointers
		pSrcBuff=uiSrcBuffAdd;
		pDestBuff=uiDestBuffAdd;

		//Compare source and destination buffers for verification
		for(j=0;j<uiBuffSize;j+=1)
		{
			//If match does not occur, increment the failure count
			if(*pSrcBuff!=*pDestBuff)
			{
				uiFailCount++;
//				DEBUG_PRINT("\nFailure at DMC address=0x%08x, expected data=0x%x, read data=0x%x\n", (pMUTRead-uiBuffSize+j), *pSrcBuff, *pDestBuff);
//				while(1);

			}
			pSrcBuff++;
			pDestBuff++;
		}
	}
	if(uiFailCount==0)
	{
		DEBUG_PRINT("Test Passed...\n");
	}
	else
	{
		DEBUG_PRINT("Test Failed...Test Failed...Test Failed...Test Failed...Test Failed...\n");
	}
	return uiFailCount;
}
/**************************************************************************************************************************
	Function name	:  Memory_Sweep_Test_16Bits
    Description		:  This function can be used to perform a memory sweep test for 16 bit accesses
	Arguments		:	Parameter					 |	Description		 							| Valid values
						uiMUTStartAdd				 |  Start address of the memory under test 		| 16 bit aligned address
						uiCount						 |  Total number of words to be swept			| unsigned int 32 bit
						uiAccessType				 |  Type of access (core/DMA)					| 0=core, 1= DMA
						uiDataPattern				 |  Type of pattern								| 0=all zeros, 1= all ones,2=all A's,
																									  3= all 5's, 4=incremental, 5 =random
						sBuffAdd			     	 |  Source buffer start address for testing 	| 16 bit aligned address
						uiDestBuffAdd				 |  Destination buffer start address for testing| 16 bit aligned address
						uiBuffSize					 |  Size of the test buffer						| unsigned int 32 bit
	Return value	:   0=for no failure, non-zero= number of failures.
**************************************************************************************************************************/
uint32_t Memory_Sweep_Test_16Bits(uint16_t* uiMUTStartAdd, uint32_t uiCount, uint32_t uiAccessType, uint32_t uiDataPattern, uint16_t* uiSrcBuffAdd, uint16_t* uiDestBuffAdd, uint32_t uiBuffSize)
{
	int i,j;

	uint16_t* pSrcBuff; uint16_t* pDestBuff; uint16_t* pMUTRead; uint16_t* pMUTWrite;

	uint32_t uiFailCount=0;

	pSrcBuff=(uint16_t*)uiSrcBuffAdd;

	uint32_t uiSourceMSIZE  = 	2;
	uint32_t uiDestMSIZE 	=	2;
	uint32_t uiSourceDMACFG = 	ENUM_DMA_CFG_MSIZE02|ENUM_DMA_CFG_PSIZE04;
	uint32_t uiDestDMACFG   =   BITM_DMA_CFG_WNR|ENUM_DMA_CFG_PSIZE04|ENUM_DMA_CFG_MSIZE02|ENUM_DMA_CFG_XCNT_INT;

	switch (uiAccessType){
		case CORE_ACCESS:
			DEBUG_PRINT("\nCORE_ACCESS\n");
			break;
		case DMA_ACCESS:
			DEBUG_PRINT("\nDMA_ACCESS\n");
			break;
	}
	DEBUG_PRINT("Test Word size = 2 bytes\n");

	//Fill the source buffer with data based on the pattern type argument
	switch (uiDataPattern)
	{
			case ALL_ZEROS:
						DEBUG_PRINT("Data pattern = All ZEROS\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=0x0000;
							pSrcBuff++;
						}
						break;

			case ALL_ONES:
						DEBUG_PRINT("Data pattern = All ONEs\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=0xFFFF;
							pSrcBuff++;
						}
						break;

			case ALL_A:
						DEBUG_PRINT("Data pattern = All A\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=0xAAAA;
							pSrcBuff++;
						}
						break;

			case ALL_5:
						DEBUG_PRINT("Data pattern = All 5\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=0x5555;
							pSrcBuff++;
						}
						break;

			case INCREMENTAL_VALUES:

						DEBUG_PRINT("Data pattern = INCREMENTAL\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=(uint16_t)i;
							pSrcBuff++;
						}
						break;

			case RANDOM_VALUES:

						DEBUG_PRINT("Data pattern = RANDOM\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=(uint16_t)(rand()%0xFFFF);
							 pSrcBuff++;
						}
						break;

			case ALL_BITS_TOGGLING:
						DEBUG_PRINT("Data pattern = ALL_BITS_TOGGLING\n");

						for(i=0;i<uiBuffSize/2;i++)
						{
							*pSrcBuff=0xFFFF; pSrcBuff++;
							*pSrcBuff=0x0000; pSrcBuff++;
						}
						break;

	};

	//Initialize pointers
	pMUTRead=uiMUTStartAdd;
	pMUTWrite=uiMUTStartAdd;

	//Now perform the test
	for(i=0;i<uiCount;i=i+uiBuffSize)
	{
		pSrcBuff=uiSrcBuffAdd;
		pDestBuff=uiDestBuffAdd;

		if(uiAccessType==CORE_ACCESS)
		{
			//Perform core write accesses to transfer data from source buffer to the target memory location under test
			for(j=0;j<uiBuffSize;j++)
			{
				*pMUTWrite=*pSrcBuff;
				pMUTWrite++;
				pSrcBuff++;

			}
		}
		else if(uiAccessType==DMA_ACCESS)
		{
			//Perform DMA write accesses to transfer data from source buffer to the target memory location under test
			MdmaAccess(uiSourceDMACFG,pSrcBuff, uiSourceMSIZE, uiBuffSize,uiDestDMACFG, pMUTWrite, uiDestMSIZE, uiBuffSize);
			//Increment the write pointer
			pMUTWrite+=uiBuffSize;
		}

		if(uiAccessType==CORE_ACCESS)
		{
			//Perform core read accesses to transfer data from the memory location under test and to the dest buffer
			for(j=0;j<uiBuffSize;j++)
			{
				*pDestBuff=*pMUTRead;
				pMUTRead++;
				pDestBuff++;

			}
		}

		else if(uiAccessType==DMA_ACCESS)
		{
			//Perform DMA read accesses to transfer data from the memory location under test and to the dest buffer
			//Increment the read pointer
			MdmaAccess(uiSourceDMACFG,pMUTRead, uiSourceMSIZE, uiBuffSize,uiDestDMACFG, pDestBuff, uiDestMSIZE, uiBuffSize);
			pMUTRead+=uiBuffSize;
		}

		//Re-initialize the source and dest buffer pointers
		pSrcBuff=uiSrcBuffAdd;
		pDestBuff=uiDestBuffAdd;

		//Compare source and destination buffers for verification
		for(j=0;j<uiBuffSize;j++)
		{
			//If match does not occur, increment the failure count
			if((*pSrcBuff)!=(*pDestBuff ))
			{
				uiFailCount++;
				DEBUG_PRINT("\nFailure at DMC address=0x%08x, expected data=0x%x, read data=0x%x\n", (pMUTRead-uiBuffSize+j), *pSrcBuff, *pDestBuff);
				while(1);
			}
			pSrcBuff++;
			pDestBuff++;
		}
	}
	if(uiFailCount==0)
	{
		DEBUG_PRINT("Test Passed...\n");
	}
	else
	{
		DEBUG_PRINT("Test Failed...Test Failed...Test Failed...Test Failed...Test Failed...\n");
	}
	return uiFailCount;
}
/**************************************************************************************************************************
	Function name	:  Memory_Sweep_Test_32Bits
    Description		:  This function can be used to perform a memory sweep test for 32 bit accesses
	Arguments		:	Parameter					 |	Description		 							| Valid values
						uiMUTStartAdd				 |  Start address of the memory under test 		| 32 bit aligned address
						uiCount						 |  Total number of words to be swept			| unsigned int 32 bit
						uiAccessType				 |  Type of access (core/DMA)					| 0=core, 1= DMA
						uiDataPattern				 |  Type of pattern								| 0=all zeros, 1= all ones,2=all A's,
																									  3= all 5's, 4=incremental, 5 =random
						sBuffAdd			     	 |  Source buffer start address for testing 	| 32 bit aligned address
						uiDestBuffAdd				 |  Destination buffer start address for testing| 32 bit aligned address
						uiBuffSize					 |  Size of the test buffer						| unsigned int 32 bit
	Return value	:   0=for no failure, non-zero= number of failures.
**************************************************************************************************************************/
uint32_t Memory_Sweep_Test_32Bits(uint32_t* uiMUTStartAdd, uint32_t uiCount, uint32_t uiAccessType, uint32_t uiDataPattern, uint32_t* uiSrcBuffAdd, uint32_t* uiDestBuffAdd, uint32_t uiBuffSize)
{
	int i,j;
	uint32_t count;
	uint32_t* src, dst;

	uint32_t* pSrcBuff; uint32_t* pDestBuff; uint32_t* pMUTRead; uint32_t* pMUTWrite;

	uint32_t uiFailCount=0;

	pSrcBuff=(uint32_t*)uiSrcBuffAdd;

	uint32_t uiSourceMSIZE  = 	4;
	uint32_t uiDestMSIZE 	=	4;
	uint32_t uiSourceDMACFG = 	ENUM_DMA_CFG_MSIZE04|ENUM_DMA_CFG_PSIZE04;
	uint32_t uiDestDMACFG   =   BITM_DMA_CFG_WNR|ENUM_DMA_CFG_PSIZE04|ENUM_DMA_CFG_MSIZE04|ENUM_DMA_CFG_XCNT_INT;

	switch (uiAccessType){
		case CORE_ACCESS:
			DEBUG_PRINT("\nCORE_ACCESS\n");
			break;
		case DMA_ACCESS:
			DEBUG_PRINT("\nDMA_ACCESS\n");
			break;
	}
	DEBUG_PRINT("Test Word size = 4 bytes\n");

	//Fill the source buffer with data based on the pattern type argument
	switch (uiDataPattern)
	{
			case ALL_ZEROS:
						DEBUG_PRINT("Data pattern = All ZEROS\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=0x00000000;
							pSrcBuff++;
						}
						break;

			case ALL_ONES:
						DEBUG_PRINT("Data pattern = All ONEs\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=0xFFFFFFFF;
							pSrcBuff++;
						}
						break;

			case ALL_A:
						DEBUG_PRINT("Data pattern = All A\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=0xAAAAAAAA;
							pSrcBuff++;
						}
						break;

			case ALL_5:
						DEBUG_PRINT("Data pattern = All 5\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=0x55555555;
							pSrcBuff++;
						}
						break;

			case INCREMENTAL_VALUES:
						DEBUG_PRINT("Data pattern = INCREMENTAL\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=(uint32_t)i;
							pSrcBuff++;
						}
						break;

			case RANDOM_VALUES:
						DEBUG_PRINT("Data pattern = RANDOM\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=(uint32_t)(rand()%0xFFFFFFFF);
							 pSrcBuff++;
						}
						break;

			case ALL_BITS_TOGGLING:
						DEBUG_PRINT("Data pattern = ALL_BITS_TOGGLING\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=0xFFFF0000; pSrcBuff++;
						}
						break;

	};

	//Initialize pointers
	pMUTRead=uiMUTStartAdd;
	pMUTWrite=uiMUTStartAdd;

	//Now perform the test
	for(i=0;i<uiCount;i=i+uiBuffSize)
	{
		pSrcBuff=uiSrcBuffAdd;
		pDestBuff=uiDestBuffAdd;

		if(uiAccessType==CORE_ACCESS)
		{
			//Perform core write accesses to transfer data from source buffer to the target memory location under test
			for(j=0;j<uiBuffSize;j++)
			{
				*pMUTWrite=*pSrcBuff;
				pMUTWrite++;
				pSrcBuff++;

			}
		}
		else if(uiAccessType==DMA_ACCESS)
		{
			//Perform DMA write accesses to transfer data from source buffer to the target memory location under test
			MdmaAccess(uiSourceDMACFG,pSrcBuff, uiSourceMSIZE, uiBuffSize,uiDestDMACFG, pMUTWrite, uiDestMSIZE, uiBuffSize);

			//Increment the write pointer
			pMUTWrite+=uiBuffSize;
		}

		if(uiAccessType==CORE_ACCESS)
		{
			//Perform core read accesses to transfer data from the memory location under test and to the dest buffer
			for(j=0;j<uiBuffSize;j++)
			{
				*pDestBuff=*pMUTRead;
				pMUTRead++;
				pDestBuff++;

			}
		}

		else if(uiAccessType==DMA_ACCESS)
		{
			//Perform DMA read accesses to transfer data from the memory location under test and to the dest buffer
			MdmaAccess(uiSourceDMACFG,pMUTRead,uiSourceMSIZE,uiBuffSize,uiDestDMACFG, pDestBuff, uiDestMSIZE, uiBuffSize);
			//Increment the read pointer
			pMUTRead+=uiBuffSize;
		}

		//Re-initialize the source and dest buffer pointers
		pSrcBuff=uiSrcBuffAdd;
		pDestBuff=uiDestBuffAdd;

		//Compare source and destination buffers for verification
		for(j=0;j<uiBuffSize;j++)
		{
			//If match does not occur, increment the failure count
			if((*pSrcBuff )!=(*pDestBuff ))
			{
				uiFailCount++;
			}
			pSrcBuff++;
			pDestBuff++;
		}
	}
	if(uiFailCount==0)
	{
		DEBUG_PRINT("Test Passed...\n");
	}
	else
	{
		DEBUG_PRINT("Test Failed...Test Failed...Test Failed...Test Failed...Test Failed...\n");
	}
	return uiFailCount;
}
/**************************************************************************************************************************
	Function name	:  Memory_Sweep_Test_64Bits
    Description		:  This function can be used to perform a memory sweep test for 64 bit accesses
	Arguments		:	Parameter					 |	Description		 							| Valid values
						uiMUTStartAdd				 |  Start address of the memory under test 		| 64 bit aligned address
						uiCount						 |  Total number of words to be swept			| unsigned int 32 bit
						uiAccessType				 |  Type of access (core/DMA)					| 0=core, 1= DMA
						uiDataPattern				 |  Type of pattern								| 0=all zeros, 1= all ones,2=all A's,
																									  3= all 5's, 4=incremental, 5 =random
						sBuffAdd			     	 |  Source buffer start address for testing 	| 64 bit aligned address
						uiDestBuffAdd				 |  Destination buffer start address for testing| 64 bit aligned address
						uiBuffSize					 |  Size of the test buffer						| unsigned int 32 bit
	Return value	:   0=for no failure, non-zero= number of failures.
**************************************************************************************************************************/
uint32_t Memory_Sweep_Test_64Bits(uint64_t* uiMUTStartAdd, uint32_t uiCount, uint32_t uiAccessType, uint32_t uiDataPattern, uint64_t* uiSrcBuffAdd, uint64_t* uiDestBuffAdd, uint32_t uiBuffSize)
{
	int i,j;

	uint64_t* pSrcBuff; uint64_t* pDestBuff; uint64_t* pMUTRead; uint64_t* pMUTWrite;

	uint32_t uiFailCount=0;

	pSrcBuff=(uint64_t*)uiSrcBuffAdd;

	uint32_t uiSourceMSIZE  = 	8;
	uint32_t uiDestMSIZE 	=	8;
	uint32_t uiSourceDMACFG = 	ENUM_DMA_CFG_MSIZE08|ENUM_DMA_CFG_PSIZE04;
	uint32_t uiDestDMACFG   =   BITM_DMA_CFG_WNR|ENUM_DMA_CFG_PSIZE04|ENUM_DMA_CFG_MSIZE08|ENUM_DMA_CFG_XCNT_INT;

	switch (uiAccessType){
		case CORE_ACCESS:
			DEBUG_PRINT("\nCORE_ACCESS\n");
			break;
		case DMA_ACCESS:
			DEBUG_PRINT("\nDMA_ACCESS\n");
			break;
	}
	DEBUG_PRINT("Test Word size = 8 bytes\n");

	//Fill the source buffer with data based on the pattern type argument
	switch (uiDataPattern)
	{
			case ALL_ZEROS:
						DEBUG_PRINT("Data pattern = All ZEROS\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=0x0000000000000000;
							pSrcBuff++;
						}
						break;

			case ALL_ONES:
						DEBUG_PRINT("Data pattern = All ONEs\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=0xFFFFFFFFFFFFFFFFull;
							pSrcBuff++;
						}
						break;

			case ALL_A:
						DEBUG_PRINT("Data pattern = All A\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=0xAAAAAAAAAAAAAAAAull;
							pSrcBuff++;
						}
						break;

			case ALL_5:
						DEBUG_PRINT("Data pattern = All 5\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=0x5555555555555555ull;
							pSrcBuff++;
						}
						break;

			case INCREMENTAL_VALUES:
						DEBUG_PRINT("Data pattern = INCREMENTAL\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=(uint64_t)i;
							pSrcBuff++;
						}
						break;

			case RANDOM_VALUES:
						DEBUG_PRINT("Data pattern = RANDOM\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=(uint64_t)(rand()%0xFFFFFFFFFFFFFFFFull);
							 pSrcBuff++;
						}
						break;

			case ALL_BITS_TOGGLING:
						DEBUG_PRINT("Data pattern = ALL_BITS_TOGGLING\n");

						for(i=0;i<uiBuffSize;i++)
						{
							*pSrcBuff=0xFFFF0000FFFF0000ull;
							pSrcBuff++;
						}
						break;

	};

	//Initialize pointers
	pMUTRead=uiMUTStartAdd;
	pMUTWrite=uiMUTStartAdd;

	//Now perform the test
	for(i=0;i<uiCount;i=i+uiBuffSize)
	{
		pSrcBuff=uiSrcBuffAdd;
		pDestBuff=uiDestBuffAdd;

		if(uiAccessType==CORE_ACCESS)
		{
			//Perform core write accesses to transfer data from source buffer to the target memory location under test
			for(j=0;j<uiBuffSize;j++)
			{
				*pMUTWrite=*pSrcBuff;
				pMUTWrite++;
				pSrcBuff++;

			}
		}
		else if(uiAccessType==DMA_ACCESS)
		{
			//Perform DMA write accesses to transfer data from source buffer to the target memory location under test
			MdmaAccess(uiSourceDMACFG,pSrcBuff, uiSourceMSIZE, uiBuffSize,uiDestDMACFG, pMUTWrite, uiDestMSIZE, uiBuffSize);
			//Increment the write pointer

			pMUTWrite+=uiBuffSize;
		}

		if(uiAccessType==CORE_ACCESS)
		{
			//Perform core read accesses to transfer data from the memory location under test and to the dest buffer
			for(j=0;j<uiBuffSize;j++)
			{
				*pDestBuff=*pMUTRead;
				pMUTRead++;
				pDestBuff++;

			}
		}

		else if(uiAccessType==DMA_ACCESS)
		{
			//Perform DMA read accesses to transfer data from the memory location under test and to the dest buffer
			MdmaAccess(uiSourceDMACFG,pMUTRead,uiSourceMSIZE,uiBuffSize,uiDestDMACFG, pDestBuff, uiDestMSIZE, uiBuffSize);
			//Increment the read pointer
			pMUTRead+=uiBuffSize;
		}

		//Re-initialize the source and dest buffer pointers
		pSrcBuff=uiSrcBuffAdd;
		pDestBuff=uiDestBuffAdd;
		//Compare source and destination buffers for verification
		for(j=0;j<uiBuffSize;j++)
		{
			//If match does not occur, increment the failure count
			if((*pSrcBuff )!=(*pDestBuff ))
			{
				uiFailCount++;
				DEBUG_PRINT("\nFailure at DMC address=0x%08x, expected data=0x%llx, read data=0x%0llx\n", (pMUTRead-uiBuffSize+j), *pSrcBuff, *pDestBuff);
				   while(1){
					   pDestBuff=(pMUTRead-uiBuffSize+j);
					   if((*pSrcBuff )==(*pDestBuff ))
					   {
						   DEBUG_PRINT("\pass at DMC address=0x%08x, expected data=0x%x, read data=0x%0x\n", (pMUTRead-uiBuffSize+j), *pSrcBuff, *pDestBuff);
						   break;
					   }
					   else
					   {
						   DEBUG_PRINT("\nFailure at DMC address=0x%08x, expected data=0x%x, read data=0x%0x\n", (pMUTRead-uiBuffSize+j), *pSrcBuff, *pDestBuff);
					   }
				   }

			}

			pSrcBuff++;
			pDestBuff++;
		}
	}
	if(uiFailCount==0)
	{
		DEBUG_PRINT("Test Passed...\n");
	}
	else
	{
		DEBUG_PRINT("Test Failed...Test Failed...Test Failed...Test Failed...Test Failed...\n");
	}
	DEBUG_PRINT("Memory Test End Address =%08x, %08x\n",pMUTRead,pMUTWrite);
	return uiFailCount;
}

uint32_t Memory_Sweep_Test_MSIZE16(uint64_t* uiMUTStartAdd, uint32_t uiCount, uint32_t uiAccessType, uint32_t uiDataPattern, uint64_t* uiSrcBuffAdd, uint64_t* uiDestBuffAdd, uint32_t uiBuffSize)
{
	int i,j;

	uint64_t* pSrcBuff; uint64_t* pDestBuff; uint64_t* pMUTRead; uint64_t* pMUTWrite;

	uint32_t uiFailCount=0;

	pSrcBuff=(uint64_t*)uiSrcBuffAdd;

	uint32_t uiSourceMSIZE  = 	16;
	uint32_t uiDestMSIZE 	=	16;
	uint32_t uiSourceDMACFG = 	ENUM_DMA_CFG_MSIZE16|ENUM_DMA_CFG_PSIZE04;
	uint32_t uiDestDMACFG   =   BITM_DMA_CFG_WNR|ENUM_DMA_CFG_PSIZE04|ENUM_DMA_CFG_MSIZE16|ENUM_DMA_CFG_XCNT_INT;

	switch (uiAccessType){
		case CORE_ACCESS:
			DEBUG_PRINT("\nCORE_ACCESS\n");
			break;
		case DMA_ACCESS:
			DEBUG_PRINT("\nDMA_ACCESS\n");
			break;
	}
	DEBUG_PRINT("Test Word size = 16 bytes\n");

	//Fill the source buffer with data based on the pattern type argument
	switch (uiDataPattern)
	{
			case ALL_ZEROS:
					DEBUG_PRINT("Data pattern = All ZEROS\n");

						for(i=0;i<uiBuffSize*4;i++)
						{
							*pSrcBuff=0x0000000000000000;
							pSrcBuff++;
						}
						break;

			case ALL_ONES:
						DEBUG_PRINT("Data pattern = All ONEs\n");

						for(i=0;i<uiBuffSize*4;i++)
						{
							*pSrcBuff=0xFFFFFFFFFFFFFFFFull;
							pSrcBuff++;
						}
						break;

			case ALL_A:
						DEBUG_PRINT("Data pattern = All A\n");

						for(i=0;i<uiBuffSize*4;i++)
						{
							*pSrcBuff=0xAAAAAAAAAAAAAAAAull;
							pSrcBuff++;
						}
						break;

			case ALL_5:
						DEBUG_PRINT("Data pattern = All 5\n");

						for(i=0;i<uiBuffSize*4;i++)
						{
							*pSrcBuff=0x5555555555555555ull;
							pSrcBuff++;
						}
						break;

			case INCREMENTAL_VALUES:
						DEBUG_PRINT("Data pattern = INCREMENTAL\n");

						for(i=0;i<uiBuffSize*4;i++)
						{
							*pSrcBuff=(uint64_t)i;
							pSrcBuff++;
						}
						break;

			case RANDOM_VALUES:
						DEBUG_PRINT("Data pattern = RANDOM\n");

						for(i=0;i<uiBuffSize*4;i++)
						{
							*pSrcBuff=(uint64_t)(rand()%0xFFFFFFFFFFFFFFFFull);
							 pSrcBuff++;
						}
						break;

			case ALL_BITS_TOGGLING:
						DEBUG_PRINT("Data pattern = ALL_BITS_TOGGLING\n");

						for(i=0;i<uiBuffSize*4;i++)
						{
							*pSrcBuff=0xFFFF0000FFFF0000ull;
							pSrcBuff++;
						}
						break;

	};

	//Initialize pointers
	pMUTRead=uiMUTStartAdd;
	pMUTWrite=uiMUTStartAdd;

	//Now perform the test
	for(i=0;i<uiCount;i=i+uiBuffSize)
	{
		pSrcBuff=uiSrcBuffAdd;
		pDestBuff=uiDestBuffAdd;

		//Perform DMA write accesses to transfer data from source buffer to the target memory location under test
		MdmaAccess(uiSourceDMACFG, pSrcBuff, uiSourceMSIZE, uiBuffSize,uiDestDMACFG, pMUTWrite, uiDestMSIZE, uiBuffSize);

		//Increment the write pointer
		pMUTWrite+=uiBuffSize*2;

		//Perform DMA read accesses to transfer data from the memory location under test and to the dest buffer
		MdmaAccess(uiSourceDMACFG, pMUTRead, uiSourceMSIZE, uiBuffSize,uiDestDMACFG, pDestBuff, uiDestMSIZE, uiBuffSize);


		//Increment the read pointer
		pMUTRead+=uiBuffSize*2;

		//Re-initialize the source and dest buffer pointers
		pSrcBuff=uiSrcBuffAdd;
		pDestBuff=uiDestBuffAdd;

		//Compare source and destination buffers for verification
		for(j=0;j<uiBuffSize*2;j++)
		{
			//If match does not occur, increment the failure count
			if((*pSrcBuff )!=(*pDestBuff ))
			{
				uiFailCount++;
				DEBUG_PRINT("\nFailure at DMC address=0x%08x, expected data=0x%llx, read data=0x%0llx\n", (pMUTRead-uiBuffSize+j), *pSrcBuff, *pDestBuff);
			}
			pSrcBuff++;
			pDestBuff++;
		}
	}
	if(uiFailCount==0)
	{
		DEBUG_PRINT("Test Passed...\n");
	}
	else
	{
		DEBUG_PRINT("Test Failed...Test Failed...Test Failed...Test Failed...Test Failed...\n");
	}
	DEBUG_PRINT("Memory Test End Address =%08x, %08x\n",pMUTRead,pMUTWrite);
	return uiFailCount;
}


/**************************************************************************************************************************
	Function name	:  Memory_Sweep_Test_MSIZE32
    Description		:  This function can be used to perform a memory sweep test for DMA accesses with MSIZE=32
	Arguments		:	Parameter					 |	Description		 							| Valid values
						uiMUTStartAdd				 |  Start address of the memory under test 		| 64 bit aligned address
						uiCount						 |  Total number of words to be swept			| unsigned int 32 bit
						uiAccessType				 |  Type of access (core/DMA)					| 0=core, 1= DMA
						uiDataPattern				 |  Type of pattern								| 0=all zeros, 1= all ones,2=all A's,
																									  3= all 5's, 4=incremental, 5 =random
						sBuffAdd			     	 |  Source buffer start address for testing 	| 64 bit aligned address
						uiDestBuffAdd				 |  Destination buffer start address for testing| 64 bit aligned address
						uiBuffSize					 |  Size of the test buffer						| unsigned int 32 bit
	Return value	:   0=for no failure, non-zero= number of failures.
**************************************************************************************************************************/
uint32_t Memory_Sweep_Test_MSIZE32(uint64_t* uiMUTStartAdd, uint32_t uiCount, uint32_t uiAccessType, uint32_t uiDataPattern, uint64_t* uiSrcBuffAdd, uint64_t* uiDestBuffAdd, uint32_t uiBuffSize)
{
	int i,j;

	uint64_t* pSrcBuff; uint64_t* pDestBuff; uint64_t* pMUTRead; uint64_t* pMUTWrite;

	uint32_t uiFailCount=0;

	pSrcBuff=(uint64_t*)uiSrcBuffAdd;

	uint32_t uiSourceMSIZE  = 	32;
	uint32_t uiDestMSIZE 	=	32;
	uint32_t uiSourceDMACFG = 	ENUM_DMA_CFG_MSIZE32|ENUM_DMA_CFG_PSIZE04;
	uint32_t uiDestDMACFG   =   BITM_DMA_CFG_WNR|ENUM_DMA_CFG_PSIZE04|ENUM_DMA_CFG_MSIZE32|ENUM_DMA_CFG_XCNT_INT;


	switch (uiAccessType){
		case CORE_ACCESS:
			DEBUG_PRINT("\nCORE_ACCESS\n");
			break;
		case DMA_ACCESS:
			DEBUG_PRINT("\nDMA_ACCESS\n");
			break;
	}
	DEBUG_PRINT("Test Word size = 32 bytes\n");

	//Fill the source buffer with data based on the pattern type argument
	switch (uiDataPattern)
	{
			case ALL_ZEROS:
					DEBUG_PRINT("Data pattern = All ZEROS\n");

						for(i=0;i<uiBuffSize*4;i++)
						{
							*pSrcBuff=0x0000000000000000;
							pSrcBuff++;
						}
						break;

			case ALL_ONES:
						DEBUG_PRINT("Data pattern = All ONEs\n");

						for(i=0;i<uiBuffSize*4;i++)
						{
							*pSrcBuff=0xFFFFFFFFFFFFFFFFull;
							pSrcBuff++;
						}
						break;

			case ALL_A:
						DEBUG_PRINT("Data pattern = All A\n");

						for(i=0;i<uiBuffSize*4;i++)
						{
							*pSrcBuff=0xAAAAAAAAAAAAAAAAull;
							pSrcBuff++;
						}
						break;

			case ALL_5:
						DEBUG_PRINT("Data pattern = All 5\n");

						for(i=0;i<uiBuffSize*4;i++)
						{
							*pSrcBuff=0x5555555555555555ull;
							pSrcBuff++;
						}
						break;

			case INCREMENTAL_VALUES:
						DEBUG_PRINT("Data pattern = INCREMENTAL\n");

						for(i=0;i<uiBuffSize*4;i++)
						{
							*pSrcBuff=(uint64_t)i;
							pSrcBuff++;
						}
						break;

			case RANDOM_VALUES:
						DEBUG_PRINT("Data pattern = RANDOM\n");

						for(i=0;i<uiBuffSize*4;i++)
						{
							*pSrcBuff=(uint64_t)(rand()%0xFFFFFFFFFFFFFFFFull);
							 pSrcBuff++;
						}
						break;

			case ALL_BITS_TOGGLING:
						DEBUG_PRINT("Data pattern = ALL_BITS_TOGGLING\n");

						for(i=0;i<uiBuffSize*4;i++)
						{
							*pSrcBuff=0xFFFF0000FFFF0000ull;
							pSrcBuff++;
						}
						break;

	};

	//Initialize pointers
	pMUTRead=uiMUTStartAdd;
	pMUTWrite=uiMUTStartAdd;

	//Now perform the test
	for(i=0;i<uiCount;i=i+uiBuffSize)
	{
		pSrcBuff=uiSrcBuffAdd;
		pDestBuff=uiDestBuffAdd;

		//Perform DMA write accesses to transfer data from source buffer to the target memory location under test
		MdmaAccess(uiSourceDMACFG, pSrcBuff, uiSourceMSIZE, uiBuffSize,uiDestDMACFG, pMUTWrite, uiDestMSIZE, uiBuffSize);


		//Increment the write pointer
		pMUTWrite+=256*4;

		//Perform DMA read accesses to transfer data from the memory location under test and to the dest buffer
		MdmaAccess(uiSourceDMACFG, pMUTRead, uiSourceMSIZE, uiBuffSize,uiDestDMACFG, pDestBuff, uiDestMSIZE, uiBuffSize);


		//Increment the read pointer
		pMUTRead+=uiBuffSize*4;

		//Re-initialize the source and dest buffer pointers
		pSrcBuff=uiSrcBuffAdd;
		pDestBuff=uiDestBuffAdd;

		//Compare source and destination buffers for verification
		for(j=0;j<uiBuffSize*4;j++)
		{
			//If match does not occur, increment the failure count
			if((*pSrcBuff )!=(*pDestBuff ))
			{
				uiFailCount++;
				DEBUG_PRINT("\nFailure at DMC address=0x%08x, expected data=0x%llx, read data=0x%0llx\n", (pMUTRead-uiBuffSize+j), *pSrcBuff, *pDestBuff);
			}
			pSrcBuff++;
			pDestBuff++;
		}
	}
	if(uiFailCount==0)
	{
		DEBUG_PRINT("Test Passed...\n");
	}
	else
	{
		DEBUG_PRINT("Test Failed...Test Failed...Test Failed...Test Failed...Test Failed...\n");
	}
	DEBUG_PRINT("Memory Test End Address =0x%08x,0x%08X,%d\n",(pMUTRead),(pMUTWrite),uiCount);
	return uiFailCount;
}

/**************************************************************************************************************************
	Function name	:  Memory_Sweep_Test
    Description		:  This function is a generic memory sweep function which can be used to test core and DMA accesses for different data patterns
	Arguments		:	Parameter					 |	Description		 							| Valid values
						uiMUTStartAdd				 |  Start address of the memory under test 		| unsigned int 32 bit
						uiCount						 |  Total number of words to be swept			| unsigned int 32 bit
	Return value	:   0=for no failure, non-zero= number of failures.


**************************************************************************************************************************/

#pragma align(32)
uint64_t  uiSrcBuff[BUFF_SIZE*4];
#pragma align(32)
uint64_t uiDestBuff[BUFF_SIZE*4];

uint32_t Memory_Sweep_Test(uint32_t uiMUTStartAdd, uint32_t uiCount)
{
	volatile int i; volatile int j;
	uint32_t uiFailCount=0;
	uint32_t uiMemType;


	unsigned int ptrn;
	unsigned int size;
	unsigned int atype;

	*pREG_SPU0_SECUREP147=0x3;
	*pREG_SPU0_SECUREP148=0x3;
	*pREG_SPU0_SECUREP134=0x3;
	*pREG_SPU0_SECUREP135=0x3;

	for (size = WORD_SIZE_32BITS; size <= WORD_SIZE_256BYTES;size+=size)
	{
		switch (size){
			case WORD_SIZE_8BITS:{
				for (atype = CORE_ACCESS; atype <= DMA_ACCESS; atype++)
				{
					for (ptrn = ALL_ZEROS ; ptrn <= ALL_BITS_TOGGLING; ptrn++){
						uiFailCount+=Memory_Sweep_Test_8Bits((uint8_t*)uiMUTStartAdd, uiCount, atype, ptrn, (uint8_t*)uiSrcBuff, (uint8_t*)uiDestBuff, BUFF_SIZE);					}

				}
				break;
			}
			case WORD_SIZE_16BITS:{
				for (atype = CORE_ACCESS; atype <= DMA_ACCESS; atype++)
				{
					for (ptrn = ALL_ZEROS ; ptrn <= ALL_BITS_TOGGLING; ptrn++){
						uiFailCount+=Memory_Sweep_Test_16Bits((uint16_t*)uiMUTStartAdd, uiCount/2, atype, ptrn, (uint16_t*)uiSrcBuff, (uint16_t*)uiDestBuff, BUFF_SIZE);
					}
				}
				break;
			}
			case WORD_SIZE_32BITS:{
				for (atype = CORE_ACCESS; atype <= DMA_ACCESS; atype++)
				{
					for (ptrn = ALL_ZEROS ; ptrn <= ALL_BITS_TOGGLING; ptrn++){
						uiFailCount+=Memory_Sweep_Test_32Bits((uint32_t*)uiMUTStartAdd, uiCount/4, atype, ptrn, (uint32_t*)uiSrcBuff, (uint32_t*)uiDestBuff, BUFF_SIZE);
					}
				}
				break;
			}
			case WORD_SIZE_64BITS:{
				for (atype = CORE_ACCESS; atype <= DMA_ACCESS; atype++)
				{
					for (ptrn = ALL_ZEROS ; ptrn <= ALL_BITS_TOGGLING; ptrn++){
						uiFailCount+=Memory_Sweep_Test_64Bits((uint64_t*)uiMUTStartAdd, uiCount/8, atype, ptrn, (uint64_t*)uiSrcBuff, (uint64_t*)uiDestBuff, BUFF_SIZE);
					}
				}
				break;
			}
			case WORD_SIZE_128BITS:{
				for (atype = CORE_ACCESS; atype <= DMA_ACCESS; atype++)
				{
					for (ptrn = ALL_ZEROS ; ptrn <= ALL_BITS_TOGGLING; ptrn++){
						uiFailCount+=Memory_Sweep_Test_MSIZE16((uint64_t*)uiMUTStartAdd, uiCount/16, atype, ptrn, (uint64_t*)uiSrcBuff, (uint64_t*)uiDestBuff, BUFF_SIZE);
					}
				}
				break;
			}
			case WORD_SIZE_256BYTES:{
				for (atype = CORE_ACCESS; atype <= DMA_ACCESS; atype++)
				{
					for (ptrn = ALL_ZEROS ; ptrn <= ALL_BITS_TOGGLING; ptrn++){
						uiFailCount+=Memory_Sweep_Test_MSIZE32((uint64_t*)uiMUTStartAdd, uiCount/32, atype, ptrn, (uint64_t*)uiSrcBuff, (uint64_t*)uiDestBuff, BUFF_SIZE);
					}
				}
				break;
			}
		}
	}
	return uiFailCount;
}

#ifdef ADI_DEBUG_PRIVATE

void printDdrRegisters(uint32_t pcreg, uint32_t ppreg)
{
	ADI_DMC_PHY_REG *Preg=(ADI_DMC_PHY_REG *)(ppreg);
	ADI_DMC_CTL_REGS *Creg=(ADI_DMC_CTL_REGS *)(pcreg);

	DEBUG_PRINT("\nPrinting PHY registers");
	DEBUG_PRINT("\nCactl %x",Preg->CaControl);
	DEBUG_PRINT("\nLane0ctl0 %x",Preg->Lane0Control0);
	DEBUG_PRINT("\nLane0ctl1 %x",Preg->Lane0Control1);
	DEBUG_PRINT("\nLane0ctl2 %x",Preg->Lane0Control2);
	DEBUG_PRINT("\nLane0stat0 %x",Preg->Lane0Stat0);

	DEBUG_PRINT("\nLane0ctl0 %x",Preg->Lane1Control0);
	DEBUG_PRINT("\nLane0ctl1 %x",Preg->Lane1Control1);
	DEBUG_PRINT("\nLane0ctl2 %x",Preg->Lane1Control2);
	DEBUG_PRINT("\nLane1stat0 %x",Preg->Lane1Stat0);

	DEBUG_PRINT("\nRootCtl %x",Preg->RootControl);

	DEBUG_PRINT("\nScratch0 %x",Preg->Scratch0);
	DEBUG_PRINT("\nScratch1 %x",Preg->Scratch1);
	DEBUG_PRINT("\nScratch2 %x",Preg->Scratch2);
	DEBUG_PRINT("\nScratch3 %x",Preg->Scratch3);
	DEBUG_PRINT("\nScratch4 %x",Preg->Scratch4);
	DEBUG_PRINT("\nScratch5 %x",Preg->Scratch5);
	DEBUG_PRINT("\nScratch6 %x",Preg->Scratch6);
	DEBUG_PRINT("\nScratch7 %x",Preg->Scratch7);

	DEBUG_PRINT("\nZqctl0 %x",Preg->ZqControl0);
	DEBUG_PRINT("\nZqctl1 %x",Preg->ZqControl1);
	DEBUG_PRINT("\nZqctl2 %x",Preg->ZqControl2);



	DEBUG_PRINT("\nPrinting Controller registers");

	DEBUG_PRINT("\nConfig %x",Creg->Config);
	DEBUG_PRINT("\nControl %x",Creg->Control);
	DEBUG_PRINT("\nDllcontrol %x",Creg->DllControl);
	DEBUG_PRINT("\nEfficiencyControl %x",Creg->EfficiencyCcontrol);
	DEBUG_PRINT("\nModeMask %x",Creg->ModeMask);


	DEBUG_PRINT("\nPriority 0 %x",Creg->Priority0);
	DEBUG_PRINT("\nPriority 2 %x",Creg->Priority2);
	DEBUG_PRINT("\nPriorityMask0 %x",Creg->PriorityMask0);
	DEBUG_PRINT("\nPriorityMask2 %x",Creg->PriorityMask2);
	DEBUG_PRINT("\nRdBufid1 %x",Creg->RdDataBufId1);
	DEBUG_PRINT("\nRdBufid2 %x",Creg->RdDataBufId2);
	DEBUG_PRINT("\nRdBufMskid1 %x",Creg->RdDataBufMask1);
	DEBUG_PRINT("\nRdBufMskid1 %x",Creg->RdDataBufMsk2);
	DEBUG_PRINT("\nShadowMR %x",Creg->ShadowMR);
	DEBUG_PRINT("\nShadowMR1 %x",Creg->ShadowMR1);
	DEBUG_PRINT("\nShadowMR2 %x",Creg->ShadowMR2);
	DEBUG_PRINT("\nShadowMR3 %x",Creg->ShadowMR3);
	DEBUG_PRINT("\nTiming0 %x",Creg->Timing0);
	DEBUG_PRINT("\nTiming1 %x",Creg->Timing1);
	DEBUG_PRINT("\nTiming2 %x",Creg->Timing2);
	DEBUG_PRINT("\nDMCstat %x",Creg->Status);

	int *stat0_l0=0x3107101c,*stat0_l1=0x31071024;

	/* to print lane DLL code */
	DEBUG_PRINT("\nDLL CODE LANE0 0x%x\n",(*(stat0_l0)&(0x000001FF)));
	DEBUG_PRINT("\nDLL CODE LANE1 0x%x\n",(*(stat0_l1)&(0x000001FF)));
}

void clearDdrRegisters(uint32_t pcreg, uint32_t ppreg)
{
	ADI_DMC_PHY_REG *Preg=(ADI_DMC_PHY_REG *)(ppreg);
	ADI_DMC_CTL_REGS *Creg=(ADI_DMC_CTL_REGS *)(pcreg);


	Preg->CaControl=0;
	Preg->Lane0Control0=0;
	Preg->Lane0Control1=0;
	Preg->Lane0Control2=0;
	Preg->Lane0Stat0=0;

	Preg->Lane1Control0=0;
	Preg->Lane1Control1=0;
	Preg->Lane1Control2=0;


	Preg->RootControl=0;

	Preg->Scratch0=0;
	Preg->Scratch1=0;
	Preg->Scratch2=0;
	Preg->Scratch3=0;
	Preg->Scratch4=0;
	Preg->Scratch5=0;
	Preg->Scratch6=0;
	Preg->Scratch7=0;

	Preg->ZqControl0=0;
	Preg->ZqControl1=0;
	Preg->ZqControl2=0;


	Creg->Control=0;
	Creg->DllControl=0x54b;
	Creg->EfficiencyCcontrol=0;
	Creg->ModeMask=0;


	Creg->Priority0=0;
	Creg->Priority2=0;
	Creg->PriorityMask0=0;
	Creg->PriorityMask2=0;
	Creg->RdDataBufId1=0;
	Creg->RdDataBufId2=0;
	Creg->RdDataBufMask1=0;
	Creg->RdDataBufMsk2=0;
	Creg->ShadowMR=0;
	Creg->ShadowMR1=0;
	Creg->ShadowMR2=0;
	Creg->ShadowMR3=0;
	Creg->Timing0=0;
	Creg->Timing1=0;
	Creg->Timing2=0;
    Creg->Config=0;
    Creg->DllControl=0x0;


}


void WriteDdrRegisters(uint32_t ppreg)
{
	int i=0;
	int keep=0;
	int *stat0_l0=0x3107101c,*stat0_l1=0x31071024;

	/* to print lane DLL code */
	DEBUG_PRINT("\nDLL CODE LANE0 0x%x\n",(*(stat0_l0)&(0x000001FF)));
	DEBUG_PRINT("\nDLL CODE LANE1 0x%x\n",(*(stat0_l1)&(0x000001FF)));


	uint32_t *addr_scratch5=0x31071080, *addr_scratch4=0x3107107c ;
	uint32_t L0_stat_dq_0,L0_stat_dq_1,L0_stat_dq_2,L0_stat_dq_3,L0_stat_dq_4,L0_stat_dq_5,L0_stat_dq_6,L0_stat_dq_7,L0_stat_dqs;
	uint32_t L1_stat_dq_0,L1_stat_dq_1,L1_stat_dq_2,L1_stat_dq_3,L1_stat_dq_4,L1_stat_dq_5,L1_stat_dq_6,L1_stat_dq_7,L1_stat_dqs;
	ADI_DMC_PHY_REG *Preg=(ADI_DMC_PHY_REG *)(ppreg);
//Reading Lane0 DQ 0
	 //RESETTING TRIGGER

	Preg->Lane0Control1= 0x00000000 ;
	for(i=0;i<10000;i++){ keep++; }
	 Preg->RootControl= 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }


	 //SET FREQ FOR M2_S2 WRITE to clk/8   -> 6:5 bits sets the freq.
	 Preg->Lane0Control0= 0x00000000 ;
	 for(i=0;i<10000;i++)
	 //M2-S2 READ  RD->4, 9:5 -> slave ID for DQ0 is xA, 1_0101_0000
		 // Slave ID 10-> DQ0,  1_0101_0000
	 Preg->Lane0Control1= 0x00000150 ;
	 for(i=0;i<8000;i++){ keep++; }


	 //M1-S1 READ // data of s2 is expected in status of s1 TRIG For READ IN L2 -> 24 & RD_CALIB =1 for having rd data in scratch regs->28
//	 assign  TRIG_RD_XFER_L0              = PADCTL[22];
//	 assign  CALIB_MODE_B                 = PADCTL[26];
	 // For lane0 it becomes 0000_0100_0100_0000_0000_0000_0000_0000
	 Preg->RootControl=0x04400000 ;
	 for(i=0;i<10000;i++){ keep++; }


	 //RESETTING TRIGGERS
	 Preg->Lane0Control1  = 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }
	 Preg->RootControl   = 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }

	 for(i=0;i<10000;i++){ keep++; }

	//READING VALUES from phy_stat & phy_scratch_stat regs

	//val0_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_LANE0_STAT1 & 0x0000FFFF) << 16));
	//val1_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT1  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_0 & 0x0000FFFF) << 16));
//	stat_dq_0 = (((*pREG_DMC0_DDR_SCRATCH_0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_4 & 0x0000FFFF) << 16));

	 L0_stat_dq_0 = (((Preg->Scratch4 & 0xFFFF0000)>> 16)|((*(addr_scratch5+0xC) & 0x0000FFFF) << 16));

/****************************************************/
	 //REading DQ1
	 //RESETTING TRIGGER

	Preg->Lane0Control1= 0x00000000 ;
	for(i=0;i<10000;i++){ keep++; }
	 Preg->RootControl= 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }


	 //SET FREQ FOR M2_S2 WRITE to clk/8   -> 6:5 bits sets the freq.
	 Preg->Lane0Control0= 0x00000000 ;
	 for(i=0;i<10000;i++)
	 //M2-S2 READ  RD->4, 9:5 -> slave ID for DQ1 is 9, 1_0011_0000
		 // Slave ID 10-> DQ0,  1_0101_0000
	 Preg->Lane0Control1= 0x00000130 ;
	 for(i=0;i<8000;i++){ keep++; }


	 //M1-S1 READ // data of s2 is expected in status of s1 TRIG For READ IN L2 -> 24 & RD_CALIB =1 for having rd data in scratch regs->28
//	 assign  TRIG_RD_XFER_L0              = PADCTL[22];
//	 assign  CALIB_MODE_B                 = PADCTL[26];
	 // For lane0 it becomes 0000_0100_0100_0000_0000_0000_0000_0000
	 Preg->RootControl=0x04400000 ;
	 for(i=0;i<10000;i++){ keep++; }


	 //RESETTING TRIGGERS
	 Preg->Lane0Control1  = 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }
	 Preg->RootControl   = 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }

	 for(i=0;i<10000;i++){ keep++; }

	//READING VALUES from phy_stat & phy_scratch_stat regs

	//val0_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_LANE0_STAT1 & 0x0000FFFF) << 16));
	//val1_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT1  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_0 & 0x0000FFFF) << 16));
//	stat_dq_0 = (((*pREG_DMC0_DDR_SCRATCH_0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_4 & 0x0000FFFF) << 16));

	 L0_stat_dq_1 = (((Preg->Scratch4 & 0xFFFF0000)>> 16)|((*(addr_scratch5+0xC) & 0x0000FFFF) << 16));

	 //Read L0 DQ2
	 //RESETTING TRIGGER

	Preg->Lane0Control1= 0x00000000 ;
	for(i=0;i<10000;i++){ keep++; }
	 Preg->RootControl= 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }


	 //SET FREQ FOR M2_S2 WRITE to clk/8   -> 6:5 bits sets the freq.
	 Preg->Lane0Control0= 0x00000000 ;
	 for(i=0;i<10000;i++)
	 //M2-S2 READ  RD->4, 9:5 -> slave ID for DQ2 is 8, 1_0001_0000

	 Preg->Lane0Control1= 0x00000110 ;
	 for(i=0;i<8000;i++){ keep++; }


	 //M1-S1 READ // data of s2 is expected in status of s1 TRIG For READ IN L2 -> 24 & RD_CALIB =1 for having rd data in scratch regs->28
//	 assign  TRIG_RD_XFER_L0              = PADCTL[22];
//	 assign  CALIB_MODE_B                 = PADCTL[26];
	 // For lane0 it becomes 0000_0100_0100_0000_0000_0000_0000_0000
	 Preg->RootControl=0x04400000 ;
	 for(i=0;i<10000;i++){ keep++; }


	 //RESETTING TRIGGERS
	 Preg->Lane0Control1  = 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }
	 Preg->RootControl   = 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }

	 for(i=0;i<10000;i++){ keep++; }

	//READING VALUES from phy_stat & phy_scratch_stat regs

	//val0_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_LANE0_STAT1 & 0x0000FFFF) << 16));
	//val1_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT1  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_0 & 0x0000FFFF) << 16));
//	stat_dq_0 = (((*pREG_DMC0_DDR_SCRATCH_0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_4 & 0x0000FFFF) << 16));

	 L0_stat_dq_2 = (((Preg->Scratch4 & 0xFFFF0000)>> 16)|((*(addr_scratch5+0xC) & 0x0000FFFF) << 16));

	 //Read L0 DQ3
	 //RESETTING TRIGGER

	Preg->Lane0Control1= 0x00000000 ;
	for(i=0;i<10000;i++){ keep++; }
	 Preg->RootControl= 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }


	 //SET FREQ FOR M2_S2 WRITE to clk/8   -> 6:5 bits sets the freq.
	 Preg->Lane0Control0= 0x00000000 ;
	 for(i=0;i<10000;i++)
	 //M2-S2 READ  RD->4, 9:5 -> slave ID for DQ3 is 7, 0_1111_0000
	 Preg->Lane0Control1= 0x000000F0 ;
	 for(i=0;i<8000;i++){ keep++; }


	 //M1-S1 READ // data of s2 is expected in status of s1 TRIG For READ IN L2 -> 24 & RD_CALIB =1 for having rd data in scratch regs->28
//	 assign  TRIG_RD_XFER_L0              = PADCTL[22];
//	 assign  CALIB_MODE_B                 = PADCTL[26];
	 // For lane0 it becomes 0000_0100_0100_0000_0000_0000_0000_0000
	 Preg->RootControl=0x04400000 ;
	 for(i=0;i<10000;i++){ keep++; }


	 //RESETTING TRIGGERS
	 Preg->Lane0Control1  = 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }
	 Preg->RootControl   = 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }

	 for(i=0;i<10000;i++){ keep++; }

	//READING VALUES from phy_stat & phy_scratch_stat regs

	//val0_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_LANE0_STAT1 & 0x0000FFFF) << 16));
	//val1_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT1  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_0 & 0x0000FFFF) << 16));
//	stat_dq_0 = (((*pREG_DMC0_DDR_SCRATCH_0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_4 & 0x0000FFFF) << 16));

	 L0_stat_dq_3 = (((Preg->Scratch4 & 0xFFFF0000)>> 16)|((*(addr_scratch5+0xC) & 0x0000FFFF) << 16));

	 // Read DQ4
	 //RESETTING TRIGGER

	Preg->Lane0Control1= 0x00000000 ;
	for(i=0;i<10000;i++){ keep++; }
	 Preg->RootControl= 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }


	 //SET FREQ FOR M2_S2 WRITE to clk/8   -> 6:5 bits sets the freq.
	 Preg->Lane0Control0= 0x00000000 ;
	 for(i=0;i<10000;i++)
	 //M2-S2 READ  RD->4, 9:5 -> slave ID, DQ4 slave id 5 , 0_1011_0000
		 // Slave ID 10-> DQ0,  1_0101_0000
	 Preg->Lane0Control1= 0x000000B0 ;
	 for(i=0;i<8000;i++){ keep++; }


	 //M1-S1 READ // data of s2 is expected in status of s1 TRIG For READ IN L2 -> 24 & RD_CALIB =1 for having rd data in scratch regs->28
//	 assign  TRIG_RD_XFER_L0              = PADCTL[22];
//	 assign  CALIB_MODE_B                 = PADCTL[26];
	 // For lane0 it becomes 0000_0100_0100_0000_0000_0000_0000_0000
	 Preg->RootControl=0x04400000 ;
	 for(i=0;i<10000;i++){ keep++; }


	 //RESETTING TRIGGERS
	 Preg->Lane0Control1  = 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }
	 Preg->RootControl   = 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }

	 for(i=0;i<10000;i++){ keep++; }

	//READING VALUES from phy_stat & phy_scratch_stat regs

	//val0_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_LANE0_STAT1 & 0x0000FFFF) << 16));
	//val1_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT1  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_0 & 0x0000FFFF) << 16));
//	stat_dq_0 = (((*pREG_DMC0_DDR_SCRATCH_0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_4 & 0x0000FFFF) << 16));

	 L0_stat_dq_4 = (((Preg->Scratch4 & 0xFFFF0000)>> 16)|((*(addr_scratch5+0xC) & 0x0000FFFF) << 16));

	 // REad DQ5
	 //RESETTING TRIGGER

	Preg->Lane0Control1= 0x00000000 ;
	for(i=0;i<10000;i++){ keep++; }
	 Preg->RootControl= 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }


	 //SET FREQ FOR M2_S2 WRITE to clk/8   -> 6:5 bits sets the freq.
	 Preg->Lane0Control0= 0x00000000 ;
	 for(i=0;i<10000;i++)
	 //M2-S2 READ  RD->4, 9:5 -> slave ID, DQ5 slave id 4, 0_1101_0000
		 // Slave ID 10-> DQ0,  0_1001_0000
	 Preg->Lane0Control1= 0x00000090 ;
	 for(i=0;i<8000;i++){ keep++; }


	 //M1-S1 READ // data of s2 is expected in status of s1 TRIG For READ IN L2 -> 24 & RD_CALIB =1 for having rd data in scratch regs->28
//	 assign  TRIG_RD_XFER_L0              = PADCTL[22];
//	 assign  CALIB_MODE_B                 = PADCTL[26];
	 // For lane0 it becomes 0000_0100_0100_0000_0000_0000_0000_0000
	 Preg->RootControl=0x04400000 ;
	 for(i=0;i<10000;i++){ keep++; }


	 //RESETTING TRIGGERS
	 Preg->Lane0Control1  = 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }
	 Preg->RootControl   = 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }

	 for(i=0;i<10000;i++){ keep++; }

	//READING VALUES from phy_stat & phy_scratch_stat regs

	//val0_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_LANE0_STAT1 & 0x0000FFFF) << 16));
	//val1_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT1  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_0 & 0x0000FFFF) << 16));
//	stat_dq_0 = (((*pREG_DMC0_DDR_SCRATCH_0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_4 & 0x0000FFFF) << 16));

	 L0_stat_dq_5 = (((Preg->Scratch4 & 0xFFFF0000)>> 16)|((*(addr_scratch5+0xC) & 0x0000FFFF) << 16));

	 // Reading L0 DQ6

	 //RESETTING TRIGGER

	Preg->Lane0Control1= 0x00000000 ;
	for(i=0;i<10000;i++){ keep++; }
	 Preg->RootControl= 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }


	 //SET FREQ FOR M2_S2 WRITE to clk/8   -> 6:5 bits sets the freq.
	 Preg->Lane0Control0= 0x00000000 ;
	 for(i=0;i<10000;i++)
	 //M2-S2 READ  RD->4, 9:5 -> slave ID, DQ6 -> slave id is 3-> 0_0111_0000
		 // Slave ID 10-> DQ0,  1_0101_0000
	 Preg->Lane0Control1= 0x00000070 ;
	 for(i=0;i<8000;i++){ keep++; }


	 //M1-S1 READ // data of s2 is expected in status of s1 TRIG For READ IN L2 -> 24 & RD_CALIB =1 for having rd data in scratch regs->28
//	 assign  TRIG_RD_XFER_L0              = PADCTL[22];
//	 assign  CALIB_MODE_B                 = PADCTL[26];
	 // For lane0 it becomes 0000_0100_0100_0000_0000_0000_0000_0000
	 Preg->RootControl=0x04400000 ;
	 for(i=0;i<10000;i++){ keep++; }


	 //RESETTING TRIGGERS
	 Preg->Lane0Control1  = 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }
	 Preg->RootControl   = 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }

	 for(i=0;i<10000;i++){ keep++; }

	//READING VALUES from phy_stat & phy_scratch_stat regs

	//val0_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_LANE0_STAT1 & 0x0000FFFF) << 16));
	//val1_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT1  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_0 & 0x0000FFFF) << 16));
//	stat_dq_0 = (((*pREG_DMC0_DDR_SCRATCH_0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_4 & 0x0000FFFF) << 16));

	 L0_stat_dq_6 = (((Preg->Scratch4 & 0xFFFF0000)>> 16)|((*(addr_scratch5+0xC) & 0x0000FFFF) << 16));
// Reading DQ7
	 //RESETTING TRIGGER
	 Preg->Lane0Control1= 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }
	 Preg->RootControl= 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }

	 //SET FREQ FOR M2_S2 WRITE to clk/8   -> 6:5 bits sets the freq.
	 Preg->Lane0Control0= 0x00000000 ;
	 for(i=0;i<10000;i++)
	 //M2-S2 READ  RD->4, 9:5 -> slave ID, slave id 2, 0_0101_0000

	 Preg->Lane0Control1= 0x00000050 ;
	 for(i=0;i<8000;i++){ keep++; }


	 //M1-S1 READ // data of s2 is expected in status of s1 TRIG For READ IN L2 -> 24 & RD_CALIB =1 for having rd data in scratch regs->28
//	 assign  TRIG_RD_XFER_L0              = PADCTL[22];
//	 assign  CALIB_MODE_B                 = PADCTL[26];
	 // For lane0 it becomes 0000_0100_0100_0000_0000_0000_0000_0000
	 Preg->RootControl=0x04400000 ;
	 for(i=0;i<10000;i++){ keep++; }


	 //RESETTING TRIGGERS
	 Preg->Lane0Control1  = 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }
	 Preg->RootControl   = 0x00000000 ;
	 for(i=0;i<10000;i++){ keep++; }

	 for(i=0;i<10000;i++){ keep++; }

	//READING VALUES from phy_stat & phy_scratch_stat regs

	//val0_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_LANE0_STAT1 & 0x0000FFFF) << 16));
	//val1_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT1  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_0 & 0x0000FFFF) << 16));
//	stat_dq_0 = (((*pREG_DMC0_DDR_SCRATCH_0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_4 & 0x0000FFFF) << 16));

	 L0_stat_dq_7 = (((Preg->Scratch4 & 0xFFFF0000)>> 16)|((*(addr_scratch5+0xC) & 0x0000FFFF) << 16));

// DQS read
	 //RESETTING TRIGGER

		Preg->Lane0Control1= 0x00000000 ;
		for(i=0;i<10000;i++){ keep++; }
		 Preg->RootControl= 0x00000000 ;
		 for(i=0;i<10000;i++){ keep++; }

		 //SET FREQ FOR M2_S2 WRITE to clk/8   -> 6:5 bits sets the freq.
		 Preg->Lane0Control0= 0x00000000 ;
		 for(i=0;i<10000;i++)
		 //M2-S2 READ  RD->4, 9:5 -> slave ID, slave id 6, 0_1101_0000
			 // Slave ID 10-> DQ0,  1_0101_0000
		 Preg->Lane0Control1= 0x000000D0 ;
		 for(i=0;i<8000;i++){ keep++; }


		 //M1-S1 READ // data of s2 is expected in status of s1 TRIG For READ IN L2 -> 24 & RD_CALIB =1 for having rd data in scratch regs->28
	//	 assign  TRIG_RD_XFER_L0              = PADCTL[22];
	//	 assign  CALIB_MODE_B                 = PADCTL[26];
		 // For lane0 it becomes 0000_0100_0100_0000_0000_0000_0000_0000
		 Preg->RootControl=0x04400000 ;
		 for(i=0;i<10000;i++){ keep++; }


		 //RESETTING TRIGGERS
		 Preg->Lane0Control1  = 0x00000000 ;
		 for(i=0;i<10000;i++){ keep++; }
		 Preg->RootControl   = 0x00000000 ;
		 for(i=0;i<10000;i++){ keep++; }


   L0_stat_dqs = (((Preg->Scratch4 & 0xFFFF0000)>> 16)|((*(addr_scratch5+0xC) & 0x0000FFFF) << 16));
   /***********************************************************************L1DQ0***************************************/

// Lane 1 DQ0
   //Reading Lane0 DQ 0
   	 //RESETTING TRIGGER

   	Preg->Lane1Control1= 0x00000000 ;
   	for(i=0;i<10000;i++){ keep++; }
   	 Preg->RootControl= 0x00000000 ;
   	 for(i=0;i<10000;i++){ keep++; }


   	 //SET FREQ FOR M2_S2 WRITE to clk/8   -> 6:5 bits sets the freq.
   	 Preg->Lane1Control0= 0x00000000 ;
   	 for(i=0;i<10000;i++)
   	 //M2-S2 READ  RD->4, 9:5 -> slave ID for DQ0 is xA, 1_0101_0000
   		 // Slave ID 10-> DQ0,  1_0101_0000
   	 Preg->Lane1Control1= 0x00000150 ;
   	 for(i=0;i<8000;i++){ keep++; }


   	 //M1-S1 READ // data of s2 is expected in status of s1 TRIG For READ IN L2 -> 24 & RD_CALIB =1 for having rd data in scratch regs->28
   //	 assign  TRIG_RD_XFER_L1              = PADCTL[23];
   //	 assign  CALIB_MODE_B                 = PADCTL[26];
   	 // For lane0 it becomes 0000_0100_1000_0000_0000_0000_0000_0000
   	 Preg->RootControl=0x04800000 ;
   	 for(i=0;i<10000;i++){ keep++; }


   	 //RESETTING TRIGGERS
   	 Preg->Lane1Control1  = 0x00000000 ;
   	 for(i=0;i<10000;i++){ keep++; }
   	 Preg->RootControl   = 0x00000000 ;
   	 for(i=0;i<10000;i++){ keep++; }

   	 for(i=0;i<10000;i++){ keep++; }

   	//READING VALUES from phy_stat & phy_scratch_stat regs

   	//val0_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_LANE0_STAT1 & 0x0000FFFF) << 16));
   	//val1_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT1  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_0 & 0x0000FFFF) << 16));
   //	stat_dq_0 = (((*pREG_DMC0_DDR_SCRATCH_0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_4 & 0x0000FFFF) << 16));

   	 L1_stat_dq_0 = (((Preg->Scratch5 & 0xFFFF0000)>> 16)|((*(addr_scratch5+0xC) & 0xFFFF0000) << 16));

//  REad L1 DQ1
/***************************L1 DQ1*********************************/
   	 //REading DQ1
   		 //RESETTING TRIGGER

   		Preg->Lane1Control1= 0x00000000 ;
   		for(i=0;i<10000;i++){ keep++; }
   		 Preg->RootControl= 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }


   		 //SET FREQ FOR M2_S2 WRITE to clk/8   -> 6:5 bits sets the freq.
   		 Preg->Lane1Control0= 0x00000000 ;
   		 for(i=0;i<10000;i++)
   		 //M2-S2 READ  RD->4, 9:5 -> slave ID for DQ1 is 9, 1_0011_0000
   			 // Slave ID 10-> DQ0,  1_0101_0000
   		 Preg->Lane1Control1= 0x00000130 ;
   		 for(i=0;i<8000;i++){ keep++; }


   		 //M1-S1 READ // data of s2 is expected in status of s1 TRIG For READ IN L2 -> 24 & RD_CALIB =1 for having rd data in scratch regs->28
   	//	 assign  TRIG_RD_XFER_L0              = PADCTL[22];
   	//	 assign  CALIB_MODE_B                 = PADCTL[26];
   		 // For lane0 it becomes 0000_0100_1000_0000_0000_0000_0000_0000
   		 Preg->RootControl=0x04800000 ;
   		 for(i=0;i<10000;i++){ keep++; }


   		 //RESETTING TRIGGERS
   		 Preg->Lane1Control1  = 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }
   		 Preg->RootControl   = 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }

   		 for(i=0;i<10000;i++){ keep++; }

   		//READING VALUES from phy_stat & phy_scratch_stat regs

   		//val0_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_LANE0_STAT1 & 0x0000FFFF) << 16));
   		//val1_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT1  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_0 & 0x0000FFFF) << 16));
   	//	stat_dq_0 = (((*pREG_DMC0_DDR_SCRATCH_0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_4 & 0x0000FFFF) << 16));

   		 L1_stat_dq_1 = (((Preg->Scratch5 & 0xFFFF0000)>> 16)|((*(addr_scratch5+0xC) & 0xFFFF0000) << 16));
   		/***********************************************************************L1DQ2***************************************/

   		 //Read L0 DQ2
   		 //RESETTING TRIGGER

   		Preg->Lane1Control1= 0x00000000 ;
   		for(i=0;i<10000;i++){ keep++; }
   		 Preg->RootControl= 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }


   		 //SET FREQ FOR M2_S2 WRITE to clk/8   -> 6:5 bits sets the freq.
   		 Preg->Lane1Control0= 0x00000000 ;
   		 for(i=0;i<10000;i++)
   		 //M2-S2 READ  RD->4, 9:5 -> slave ID for DQ2 is 8, 1_0001_0000

   		 Preg->Lane1Control1= 0x00000110 ;
   		 for(i=0;i<8000;i++){ keep++; }


   		 //M1-S1 READ // data of s2 is expected in status of s1 TRIG For READ IN L2 -> 24 & RD_CALIB =1 for having rd data in scratch regs->28
   	//	 assign  TRIG_RD_XFER_L0              = PADCTL[22];
   	//	 assign  CALIB_MODE_B                 = PADCTL[26];
   		 // For lane0 it becomes 0000_0100_1000_0000_0000_0000_0000_0000
   		 Preg->RootControl=0x04800000 ;
   		 for(i=0;i<10000;i++){ keep++; }


   		 //RESETTING TRIGGERS
   		 Preg->Lane1Control1  = 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }
   		 Preg->RootControl   = 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }

   		 for(i=0;i<10000;i++){ keep++; }

   		//READING VALUES from phy_stat & phy_scratch_stat regs

   		//val0_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_LANE0_STAT1 & 0x0000FFFF) << 16));
   		//val1_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT1  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_0 & 0x0000FFFF) << 16));
   	//	stat_dq_0 = (((*pREG_DMC0_DDR_SCRATCH_0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_4 & 0x0000FFFF) << 16));

   		 L1_stat_dq_2 = (((Preg->Scratch5 & 0xFFFF0000)>> 16)|((*(addr_scratch5+0xC) & 0xFFFF0000) << 16));
   		/***********************************************************************L1DQ3***************************************/

   		 //Read L0 DQ3
   		 //RESETTING TRIGGER

   		Preg->Lane1Control1= 0x00000000 ;
   		for(i=0;i<10000;i++){ keep++; }
   		 Preg->RootControl= 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }


   		 //SET FREQ FOR M2_S2 WRITE to clk/8   -> 6:5 bits sets the freq.
   		 Preg->Lane1Control0= 0x00000000 ;
   		 for(i=0;i<10000;i++)
   		 //M2-S2 READ  RD->4, 9:5 -> slave ID for DQ3 is 7, 0_1111_0000
   		 Preg->Lane1Control1= 0x000000F0 ;
   		 for(i=0;i<8000;i++){ keep++; }


   		 //M1-S1 READ // data of s2 is expected in status of s1 TRIG For READ IN L2 -> 24 & RD_CALIB =1 for having rd data in scratch regs->28
   	//	 assign  TRIG_RD_XFER_L0              = PADCTL[22];
   	//	 assign  CALIB_MODE_B                 = PADCTL[26];
   		 // For lane0 it becomes 0000_0100_0100_0000_0000_0000_0000_0000
   		 Preg->RootControl=0x04800000 ;
   		 for(i=0;i<10000;i++){ keep++; }


   		 //RESETTING TRIGGERS
   		 Preg->Lane1Control1  = 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }
   		 Preg->RootControl   = 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }

   		 for(i=0;i<10000;i++){ keep++; }

   		//READING VALUES from phy_stat & phy_scratch_stat regs

   		//val0_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_LANE0_STAT1 & 0x0000FFFF) << 16));
   		//val1_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT1  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_0 & 0x0000FFFF) << 16));
   	//	stat_dq_0 = (((*pREG_DMC0_DDR_SCRATCH_0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_4 & 0x0000FFFF) << 16));

   		 L1_stat_dq_3 = (((Preg->Scratch5 & 0xFFFF0000)>> 16)|((*(addr_scratch5+0xC) & 0xFFFF0000) << 16));
   		/***********************************************************************L1DQ4***************************************/

   		 // Read DQ4
   		 //RESETTING TRIGGER

   		Preg->Lane1Control1= 0x00000000 ;
   		for(i=0;i<10000;i++){ keep++; }
   		 Preg->RootControl= 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }


   		 //SET FREQ FOR M2_S2 WRITE to clk/8   -> 6:5 bits sets the freq.
   		 Preg->Lane1Control0= 0x00000000 ;
   		 for(i=0;i<10000;i++)
   		 //M2-S2 READ  RD->4, 9:5 -> slave ID, DQ4 slave id 5 , 0_1011_0000
   			 // Slave ID 10-> DQ0,  1_0101_0000
   		 Preg->Lane1Control1= 0x000000B0 ;
   		 for(i=0;i<8000;i++){ keep++; }


   		 //M1-S1 READ // data of s2 is expected in status of s1 TRIG For READ IN L2 -> 24 & RD_CALIB =1 for having rd data in scratch regs->28
   	//	 assign  TRIG_RD_XFER_L0              = PADCTL[22];
   	//	 assign  CALIB_MODE_B                 = PADCTL[26];
   		 // For lane0 it becomes 0000_0100_0100_0000_0000_0000_0000_0000
   		 Preg->RootControl=0x04800000 ;
   		 for(i=0;i<10000;i++){ keep++; }


   		 //RESETTING TRIGGERS
   		 Preg->Lane1Control1  = 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }
   		 Preg->RootControl   = 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }

   		 for(i=0;i<10000;i++){ keep++; }

   		//READING VALUES from phy_stat & phy_scratch_stat regs

   		//val0_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_LANE0_STAT1 & 0x0000FFFF) << 16));
   		//val1_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT1  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_0 & 0x0000FFFF) << 16));
   	//	stat_dq_0 = (((*pREG_DMC0_DDR_SCRATCH_0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_4 & 0x0000FFFF) << 16));

   		 L1_stat_dq_4 = (((Preg->Scratch5 & 0xFFFF0000)>> 16)|((*(addr_scratch5+0xC) & 0xFFFF0000) << 16));
 		/***********************************************************************L1DQ5***************************************/

   		 // REad DQ5
   		 //RESETTING TRIGGER

   		Preg->Lane1Control1= 0x00000000 ;
   		for(i=0;i<10000;i++){ keep++; }
   		 Preg->RootControl= 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }


   		 //SET FREQ FOR M2_S2 WRITE to clk/8   -> 6:5 bits sets the freq.
   		 Preg->Lane1Control0= 0x00000000 ;
   		 for(i=0;i<10000;i++)
   		 //M2-S2 READ  RD->4, 9:5 -> slave ID, DQ5 slave id 4, 0_1101_0000
   			 // Slave ID 10-> DQ0,  0_1001_0000
   		 Preg->Lane1Control1= 0x00000090 ;
   		 for(i=0;i<8000;i++){ keep++; }


   		 //M1-S1 READ // data of s2 is expected in status of s1 TRIG For READ IN L2 -> 24 & RD_CALIB =1 for having rd data in scratch regs->28
   	//	 assign  TRIG_RD_XFER_L0              = PADCTL[22];
   	//	 assign  CALIB_MODE_B                 = PADCTL[26];
   		 // For lane0 it becomes 0000_0100_1000_0000_0000_0000_0000_0000
   		 Preg->RootControl=0x04800000 ;
   		 for(i=0;i<10000;i++){ keep++; }


   		 //RESETTING TRIGGERS
   		 Preg->Lane1Control1  = 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }
   		 Preg->RootControl   = 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }

   		 for(i=0;i<10000;i++){ keep++; }

   		//READING VALUES from phy_stat & phy_scratch_stat regs

   		//val0_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_LANE0_STAT1 & 0x0000FFFF) << 16));
   		//val1_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT1  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_0 & 0x0000FFFF) << 16));
   	//	stat_dq_0 = (((*pREG_DMC0_DDR_SCRATCH_0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_4 & 0x0000FFFF) << 16));

   		 L1_stat_dq_5 = (((Preg->Scratch5 & 0xFFFF0000)>> 16)|((*(addr_scratch5+0xC) & 0xFFFF0000) << 16));

/***********************************************************************L1DQ6***************************************/
   		 // Reading L1 DQ6

   		 //RESETTING TRIGGER

   		Preg->Lane1Control1= 0x00000000 ;
   		for(i=0;i<10000;i++){ keep++; }
   		 Preg->RootControl= 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }


   		 //SET FREQ FOR M2_S2 WRITE to clk/8   -> 6:5 bits sets the freq.
   		 Preg->Lane1Control0= 0x00000000 ;
   		 for(i=0;i<10000;i++)
   		 //M2-S2 READ  RD->4, 9:5 -> slave ID, DQ6 -> slave id is 3-> 0_0111_0000
   			 // Slave ID 10-> DQ0,  1_0101_0000
   		 Preg->Lane1Control1= 0x00000070 ;
   		 for(i=0;i<8000;i++){ keep++; }


   		 //M1-S1 READ // data of s2 is expected in status of s1 TRIG For READ IN L2 -> 24 & RD_CALIB =1 for having rd data in scratch regs->28
   	//	 assign  TRIG_RD_XFER_L0              = PADCTL[22];
   	//	 assign  CALIB_MODE_B                 = PADCTL[26];
   		 // For lane1 it becomes 0000_0100_1000_0000_0000_0000_0000_0000
   		 Preg->RootControl=0x04800000 ;
   		 for(i=0;i<10000;i++){ keep++; }


   		 //RESETTING TRIGGERS
   		 Preg->Lane1Control1  = 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }
   		 Preg->RootControl   = 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }

   		 for(i=0;i<10000;i++){ keep++; }

   		//READING VALUES from phy_stat & phy_scratch_stat regs

   		//val0_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_LANE0_STAT1 & 0x0000FFFF) << 16));
   		//val1_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT1  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_0 & 0x0000FFFF) << 16));
   	//	stat_dq_0 = (((*pREG_DMC0_DDR_SCRATCH_0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_4 & 0x0000FFFF) << 16));

   		 L1_stat_dq_6 = (((Preg->Scratch5 & 0xFFFF0000)>> 16)|((*(addr_scratch5+0xC) & 0xFFFF0000) << 16));

  /*****************************************************L1DQ7****************/
   	// Reading DQ7
   		 //RESETTING TRIGGER
   		 Preg->Lane1Control1= 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }
   		 Preg->RootControl= 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }

   		 //SET FREQ FOR M2_S2 WRITE to clk/8   -> 6:5 bits sets the freq.
   		 Preg->Lane1Control0= 0x00000000 ;
   		 for(i=0;i<10000;i++)
   		 //M2-S2 READ  RD->4, 9:5 -> slave ID, slave id 2, 0_0101_0000

   		 Preg->Lane1Control1= 0x00000050 ;
   		 for(i=0;i<8000;i++){ keep++; }


   		 //M1-S1 READ // data of s2 is expected in status of s1 TRIG For READ IN L2 -> 24 & RD_CALIB =1 for having rd data in scratch regs->28
   	//	 assign  TRIG_RD_XFER_L0              = PADCTL[22];
   	//	 assign  CALIB_MODE_B                 = PADCTL[26];
   		 // For lane0 it becomes 0000_0100_0100_0000_0000_0000_0000_0000
   		 Preg->RootControl=0x04800000 ;
   		 for(i=0;i<10000;i++){ keep++; }


   		 //RESETTING TRIGGERS
   		 Preg->Lane1Control1  = 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }
   		 Preg->RootControl   = 0x00000000 ;
   		 for(i=0;i<10000;i++){ keep++; }

   		 for(i=0;i<10000;i++){ keep++; }

   		//READING VALUES from phy_stat & phy_scratch_stat regs

   		//val0_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_LANE0_STAT1 & 0x0000FFFF) << 16));
   		//val1_dq_0 = (((*pREG_DMC0_DDR_LANE0_STAT1  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_0 & 0x0000FFFF) << 16));
   	//	stat_dq_0 = (((*pREG_DMC0_DDR_SCRATCH_0  & 0xFFFF0000)>> 16)|((*pREG_DMC0_DDR_SCRATCH_4 & 0x0000FFFF) << 16));

   		 L1_stat_dq_7 = (((Preg->Scratch5 & 0xFFFF0000)>> 16)|((*(addr_scratch5+0xC) & 0xFFFF0000) << 16));
/*********************************************L!DQS*************************/
   	// DQS read
   		 //RESETTING TRIGGER

   			Preg->Lane1Control1= 0x00000000 ;
   			for(i=0;i<10000;i++){ keep++; }
   			 Preg->RootControl= 0x00000000 ;
   			 for(i=0;i<10000;i++){ keep++; }

   			 //SET FREQ FOR M2_S2 WRITE to clk/8   -> 6:5 bits sets the freq.
   			 Preg->Lane1Control0= 0x00000000 ;
   			 for(i=0;i<10000;i++)
   			 //M2-S2 READ  RD->4, 9:5 -> slave ID, slave id 6, 0_1101_0000
   				 // Slave ID 10-> DQ0,  1_0101_0000
   			 Preg->Lane1Control1= 0x000000D0 ;
   			 for(i=0;i<8000;i++){ keep++; }


   			 //M1-S1 READ // data of s2 is expected in status of s1 TRIG For READ IN L2 -> 24 & RD_CALIB =1 for having rd data in scratch regs->28
   		//	 assign  TRIG_RD_XFER_L0              = PADCTL[22];
   		//	 assign  CALIB_MODE_B                 = PADCTL[26];
   			 // For lane0 it becomes 0000_0100_1000_0000_0000_0000_0000_0000
   			 Preg->RootControl=0x04800000 ;
   			 for(i=0;i<10000;i++){ keep++; }


   			 //RESETTING TRIGGERS
   			 Preg->Lane1Control1  = 0x00000000 ;
   			 for(i=0;i<10000;i++){ keep++; }
   			 Preg->RootControl   = 0x00000000 ;
   			 for(i=0;i<10000;i++){ keep++; }


   	   L1_stat_dqs = (((Preg->Scratch5 & 0xFFFF0000)>> 16)|((*(addr_scratch5+0xC) & 0xFFFF0000) << 16));


/********************************End Lane 1************************************************/

   	 DEBUG_PRINT("\nREAD LEVELLING CODE Lane0 DQ0 = 0x%08x\n",L0_stat_dq_0);
	 DEBUG_PRINT("\nREAD LEVELLING CODE Lane0 DQ1 = 0x%08x\n",L0_stat_dq_1);
	 DEBUG_PRINT("\nREAD LEVELLING CODE Lane0 DQ2 = 0x%08x\n",L0_stat_dq_2);
	 DEBUG_PRINT("\nREAD LEVELLING CODE Lane0 DQ3 = 0x%08x\n",L0_stat_dq_3);
	 DEBUG_PRINT("\nREAD LEVELLING CODE Lane0 DQ4 = 0x%08x\n",L0_stat_dq_4);
	 DEBUG_PRINT("\nREAD LEVELLING CODE Lane0 DQ5 = 0x%08x\n",L0_stat_dq_4);
	 DEBUG_PRINT("\nREAD LEVELLING CODE Lane0 DQ6 = 0x%08x\n",L0_stat_dq_6);
	 DEBUG_PRINT("\nREAD LEVELLING CODE Lane0 DQ7 = 0x%08x\n",L0_stat_dq_7);
	 DEBUG_PRINT("\nREAD LEVELLING CODE Lane0 DQS = 0x%08x\n",L0_stat_dqs);
	 DEBUG_PRINT("\nREAD LEVELLING CODE Lane1 DQ0 = 0x%08x\n",L1_stat_dq_0);
	 DEBUG_PRINT("\nREAD LEVELLING CODE Lane1 DQ1 = 0x%08x\n",L1_stat_dq_1);
	 DEBUG_PRINT("\nREAD LEVELLING CODE Lane1 DQ2 = 0x%08x\n",L1_stat_dq_2);
	 DEBUG_PRINT("\nREAD LEVELLING CODE Lane1 DQ3 = 0x%08x\n",L1_stat_dq_3);
	 DEBUG_PRINT("\nREAD LEVELLING CODE Lane1 DQ4 = 0x%08x\n",L1_stat_dq_4);
	 DEBUG_PRINT("\nREAD LEVELLING CODE Lane1 DQ5 = 0x%08x\n",L1_stat_dq_4);
	 DEBUG_PRINT("\nREAD LEVELLING CODE Lane1 DQ6 = 0x%08x\n",L1_stat_dq_6);
	 DEBUG_PRINT("\nREAD LEVELLING CODE Lane1 DQ7 = 0x%08x\n",L1_stat_dq_7);
	 DEBUG_PRINT("\nREAD LEVELLING CODE Lane1 DQS = 0x%08x\n",L1_stat_dqs);


//	DEBUG_PRINT("SCRATCH5 = 0x%08x\n",*addr_scratch5);
//	DEBUG_PRINT("SCRATCH4 = 0x%08x\n",*(addr_scratch4));
//	DEBUG_PRINT("SCRATCH6 = 0x%08x\n",*(addr_scratch5+0x4));
//	DEBUG_PRINT("SCRATCH7 = 0x%08x\n",*(addr_scratch5+0x8));
//	DEBUG_PRINT("SCRATCH8 = 0x%08x\n",*(addr_scratch5+0xC));
//	DEBUG_PRINT("SCRATCH9 = 0x%08x\n",*(addr_scratch5+0x10));
//	DEBUG_PRINT("SCRATCH10 = 0x%08x\n",*(addr_scratch5+0x14));
}
#endif
