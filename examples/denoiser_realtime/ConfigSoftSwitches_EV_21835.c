/*******************************************************************************

Copyright(c) 2023 Analog Devices, Inc. All Rights Reserved.

This software is proprietary.  By using this software you
agree to the terms of the associated Analog Devices License Agreement.

*******************************************************************************/

/*
 *  Software Switch Configuration for the EV-21835-SOM Board
 *
 *  Please see the SoftConfig documentation in CCES Help for detailed
 *  information on SoftConfig.
 *
 *  NOTE : "TWI 1"(which is muxed) is being used to configure the soft-switches.
 *  Application is expected to do the approprite Pin-Muxing for "TWI 1" before doing the switch configuration.
 *
 */

#include <drivers/twi/adi_twi.h>

/* TWI settings */
#define TWI_PRESCALE  (12u)
#define TWI_BITRATE   (100u)
#define TWI_DUTYCYCLE (50u)

#define BUFFER_SIZE (32u)

/* TWI hDevice handle */
static ADI_TWI_HANDLE hDevice;

/* TWI data buffer */
static uint8_t twiBuffer[BUFFER_SIZE];

/* TWI device memory */
static uint8_t deviceMemory[ADI_TWI_MEMORY_SIZE];

/* switch register structure */
typedef struct {
	uint8_t Register;
	uint8_t Value;
} SWITCH_CONFIG;

/* switch parameter structure */
typedef struct {
	uint32_t TWIDevice;
	uint16_t HardwareAddress;
	uint32_t NumConfigSettings;
	SWITCH_CONFIG *ConfigSettings;
} SOFT_SWITCH;

/* prototype */
void SoftConfig_EV_21835_SOM(void);

/* disable misra diagnostics as necessary */
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_8_7:"Objects shall be defined at block scope")
#pragma diag(suppress:misra_rule_17_4:"Array indexing shall be the only allowed form of pointer arithmetic")
#endif /* _MISRA_RULES */

#ifdef __ADSPGCC__
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

/************************SoftConfig Information********************************

    ~ means the signal is active low

    Please see the EV-21835-SOM Board schematic for more information on using
    Software-Controlled Switches(SoftConfig)

********************************************************************************/

/* switch 0 register settings */
static SWITCH_CONFIG SwitchConfig0[] =
{

 /*
  GPIO1 	   	   	   	   	   	   	   	   	   GPIO2                                        GPIO3
  R7---------------  Not Used          |	   C7---------------  DS3          	    |       7---------------  Not Used
  | R6-------------  Not Used     	   |	   | C6-------------  Not Used     	    |       | 6-------------  Not Used
  | | R5-----------  Not Used   	   |	   | | C5-----------  Not Used          |       | | 5-----------  Not Used
  | | | R4---------  Not Used	       |	   | | | C4---------  XSPI_EN*		    |       | | | 4---------  Not Used
  | | | | R3-------  SPI1FLASH_CS_EN*  |	   | | | | C3-------  SOM_XSPI_EN*      |       | | | | 3-------  Not Used
  | | | | | R2-----  SPI1D2_D3_EN*     |	   | | | | | C2-----  TWI1_EN*		    |       | | | | | 2-----  Not Used
  | | | | | | R1---  UART0_EN*     	   |       | | | | | | C1---  SOM_TWI1_EN*      |       | | | | | | C9--- DS1
  | | | | | | | R0-  UART0_FLOW_EN*    |	   | | | | | | | C0-  SOM_TWI2_EN*      |       | | | | | | | C8- DS2
  | | | | | | | |                      |	   | | | | | | | |                      |       | | | | | | | |
  X X X X Y Y Y Y                      |	   Y X X N Y Y Y Y                      |       X X X X X X Y Y     ( Active Y or N )
  0 0 0 0 1 1 0 0                      |	   1 0 0 0 0 1 1 0                      |       0 0 0 0 0 0 1 1     ( value being set )

*/
  { 0x1Du, 0x00u },							   { 0x1Eu, 0x00u },                           { 0x1Fu, 0x00u },   /* Select GPIO/Keypad : 0 -> GPIO, 1->Keypad */
  { 0x17u, 0x0Cu },							   { 0x18u, 0x86u },                           { 0x19u, 0x03u },   /* Output Data */

 /*
  * specify inputs/outputs
  */
  { 0x23u, 0x0Fu },    /* Set IO Direction for GPIO1 */
  { 0x24u, 0x9Fu },    /* Set IO Direction for GPIO2 */
  { 0x25u, 0x03u },    /* Set IO Direction for GPIO3 */
};

/* switch configuration */
static SOFT_SWITCH SoftSwitch[] =
{
	{
		1u,
		0x30u,
		sizeof(SwitchConfig0)/sizeof(SWITCH_CONFIG),
		SwitchConfig0
	}
};

#if defined(ADI_DEBUG)
#include <stdio.h>
#define CHECK_RESULT(result, message) \
	do { \
		if((result) != ADI_TWI_SUCCESS) \
		{ \
			printf((message)); \
			printf("\n"); \
		} \
	} while (0)  /* do-while-zero needed for Misra Rule 19.4 */
#else
#define CHECK_RESULT(result, message)
#endif

void SoftConfig_EV_21835_SOM(void)
{
	uint32_t switchNum;
	uint32_t configNum;
	uint32_t switches;

	SOFT_SWITCH *ss;
	SWITCH_CONFIG *configReg;
	ADI_TWI_RESULT result;

	switches = (uint32_t)(sizeof(SoftSwitch)/sizeof(SOFT_SWITCH));
	for (switchNum=0u; switchNum<switches; switchNum++)
	{
		ss = &SoftSwitch[switchNum];

		result = adi_twi_Open(ss->TWIDevice, ADI_TWI_MASTER,
    		deviceMemory, ADI_TWI_MEMORY_SIZE, &hDevice);
		CHECK_RESULT(result, "adi_twi_Open failed");

		result = adi_twi_SetHardwareAddress(hDevice, ss->HardwareAddress);
		CHECK_RESULT(result, "adi_twi_SetHardwareAddress failed");

		result = adi_twi_SetPrescale(hDevice, TWI_PRESCALE);
		CHECK_RESULT(result, "adi_twi_Prescale failed");

		result = adi_twi_SetBitRate(hDevice, TWI_BITRATE);
		CHECK_RESULT(result, "adi_twi_SetBitRate failed");

		result = adi_twi_SetDutyCycle(hDevice, TWI_DUTYCYCLE);
		CHECK_RESULT(result, "adi_twi_SetDutyCycle failed");

		/* switch register settings */
		for (configNum=0u; configNum<ss->NumConfigSettings; configNum++)
		{
			configReg = &ss->ConfigSettings[configNum];

			/* write register value */
			twiBuffer[0] = configReg->Register;
			twiBuffer[1] = configReg->Value;
			result = adi_twi_Write(hDevice, twiBuffer, (uint32_t)2, false);
			CHECK_RESULT(result, "adi_twi_Write failed");
		}

		result = adi_twi_Close(hDevice);
		CHECK_RESULT(result, "adi_twi_Close failed");
	}
}

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

