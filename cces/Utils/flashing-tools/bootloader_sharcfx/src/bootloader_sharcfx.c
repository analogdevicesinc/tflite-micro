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
#include "Push_Button.h"

#define Single
//#ifdef Dual
//#ifdef Quad

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

int volatile pb1_enable = 0, pb2_enable=0;
void gpioCallback(ADI_GPIO_PIN_INTERRUPT ePinInt, uint32_t Data, void *pCBParam)
{
	if(ePinInt == PUSH_BUTTON1_PINT)
	{
		if(Data & PUSH_BUTTON1_PINT_PIN)
		{
			pb1_enable = 1;

		}
	}
	if(ePinInt == PUSH_BUTTON2_PINT)
	{
		if(Data & PUSH_BUTTON2_PINT_PIN)
		{
			pb2_enable = 1;
		}
	}
}

/* Disable Push buttons so that they can be used by the primary applications */
void CleanupPushButtonGPIO(void)
{
    ADI_GPIO_RESULT result;
    uint32_t dummy;

    // Unregister callbacks
    result = adi_gpio_UnRegisterCallback(PUSH_BUTTON1_PINT, PUSH_BUTTON1_PINT_PIN, gpioCallback);
    if (result != ADI_GPIO_SUCCESS) {
        PRINT_INFO("Failed to unregister callback for PB1\n");
    }

    result = adi_gpio_UnRegisterCallback(PUSH_BUTTON2_PINT, PUSH_BUTTON2_PINT_PIN, gpioCallback);
    if (result != ADI_GPIO_SUCCESS) {
        PRINT_INFO("Failed to unregister callback for PB2\n");
    }

    // Disable pin interrupt masks
    result = adi_gpio_EnablePinInterruptMask(PUSH_BUTTON1_PINT, PUSH_BUTTON1_PINT_PIN, false);
    if (result != ADI_GPIO_SUCCESS) {
        PRINT_INFO("Failed to disable interrupt mask for PB1\n");
    }

    result = adi_gpio_EnablePinInterruptMask(PUSH_BUTTON2_PINT, PUSH_BUTTON2_PINT_PIN, false);
    if (result != ADI_GPIO_SUCCESS) {
        PRINT_INFO("Failed to disable interrupt mask for PB2\n");
    }

    // Clear interrupt state
    result = adi_gpio_GetPinInterruptState(PUSH_BUTTON1_PINT, &dummy);
    if (result != ADI_GPIO_SUCCESS) {
        PRINT_INFO("Failed to clear interrupt state for PB1\n");
    }

    result = adi_gpio_GetPinInterruptState(PUSH_BUTTON2_PINT, &dummy);
    if (result != ADI_GPIO_SUCCESS) {
        PRINT_INFO("Failed to clear interrupt state for PB2\n");
    }

    // Reset pin direction to input
    result = adi_gpio_SetDirection(PUSH_BUTTON1_PORT, PUSH_BUTTON1_PIN, ADI_GPIO_DIRECTION_INPUT);
    if (result != ADI_GPIO_SUCCESS) {
        PRINT_INFO("Failed to set direction for PB1\n");
    }

    result = adi_gpio_SetDirection(PUSH_BUTTON2_PORT, PUSH_BUTTON2_PIN, ADI_GPIO_DIRECTION_INPUT);
    if (result != ADI_GPIO_SUCCESS) {
        PRINT_INFO("Failed to set direction for PB2\n");
    }

//    PRINT_INFO("Push button GPIO cleanup complete.\n");
}

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

	ADI_GPIO_RESULT Result;

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
	PRINT_INFO("\n Hi its the Bootloader");
	ConfigSoftSwitches_EV_SOMCRR_EZKIT();

	if(PUSH_BUTTON1_PORT == PUSH_BUTTON2_PORT)
	{
		/*Configure the GPIO as output for PB1 and PB2*/
		Result = adi_gpio_PortInit(PUSH_BUTTON1_PORT,PUSH_BUTTON1_PIN | PUSH_BUTTON2_PIN,ADI_GPIO_DIRECTION_INPUT,true);
		if(Result != ADI_GPIO_SUCCESS)
		{
			PRINT_INFO("GPIO Initialization failed \n");
		}
	}

	else
	{
		/*Configure the GPIO as output for PB1*/
		Result = adi_gpio_PortInit(PUSH_BUTTON1_PORT,PUSH_BUTTON1_PIN,ADI_GPIO_DIRECTION_INPUT,true);
		if(Result != ADI_GPIO_SUCCESS)
		{
			PRINT_INFO("GPIO Initialization failed \n");
		}

		/*Configure the GPIO as output for PB2*/
		Result = adi_gpio_PortInit(PUSH_BUTTON2_PORT,PUSH_BUTTON2_PIN,ADI_GPIO_DIRECTION_INPUT,true);
		if(Result != ADI_GPIO_SUCCESS)
		{
			PRINT_INFO("GPIO Initialization failed \n");
		}

	}

	uint32_t gpiocallbacks;
	Result = adi_gpio_Init((void*)gpioMemory,GPIO_MEMORY_SIZE,&gpiocallbacks);
	if(Result != ADI_GPIO_SUCCESS)
	{
		PRINT_INFO("GPIO Initialization failed \n");
	}

	/*Register Callback for PB1 GPIO pin*/
	Result = adi_gpio_RegisterCallback(PUSH_BUTTON1_PINT, PUSH_BUTTON1_PINT_PIN, gpioCallback,(void*)0);
	if(Result != ADI_GPIO_SUCCESS)
	{
		PRINT_INFO("GPIO Initialization failed \n");
	}

	/*Register Callback for PB2 GPIO pin*/
	Result = adi_gpio_RegisterCallback(PUSH_BUTTON2_PINT, PUSH_BUTTON2_PINT_PIN, gpioCallback,(void*)0);
	if(Result != ADI_GPIO_SUCCESS)
	{
		PRINT_INFO("GPIO Initialization failed \n");
	}

	/* Configure the PINT interrupt for level */
	Result = adi_gpio_PinInt(PUSH_BUTTON1_PIN_ASSIGN, PUSH_BUTTON1_PINT_PIN, PUSH_BUTTON1_PINT, PUSH_BUTTON1_PIN_ASSIGN_BYTE,true,ADI_GPIO_SENSE_LEVEL_HIGH);
	if(Result != ADI_GPIO_SUCCESS)
	{
		PRINT_INFO("GPIO Initialization failed \n");
	}

	/* Configure the PINT interrupt for level */
	Result = adi_gpio_PinInt(PUSH_BUTTON2_PIN_ASSIGN, PUSH_BUTTON2_PINT_PIN, PUSH_BUTTON2_PINT, PUSH_BUTTON2_PIN_ASSIGN_BYTE,true,ADI_GPIO_SENSE_LEVEL_HIGH);
	if(Result != ADI_GPIO_SUCCESS)
	{
		PRINT_INFO("GPIO Initialization failed \n");
	}


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

	PRINT_INFO("\n ***************************************** \n");
	PRINT_INFO("\n Press Push Button 1 for Genre ID application ");
	PRINT_INFO("\n Press Push Button 2 for UrbanSound application ");
	PRINT_INFO("\n Press Push Button 1 and 2 both for KWS application ");

	PRINT_INFO("\n Incase within 2 seconds no button is pressed Denoiser application will start running ");
	PRINT_INFO("\n Reboot to come to the selection menu again");
	PRINT_INFO("\n ***************************************** \n");

	for(int volatile i=0;i<0x2000000;i++);

	if(pb1_enable == 0 && pb2_enable == 0){
		PRINT_INFO("\n********************** DTLN Denoiser ************************\n");
		/* Disable Push buttons so that they can be used by the primary applications */
		CleanupPushButtonGPIO();
		START_ADDRESS = 0x70080000;
		adi_rom_Boot((void *)START_ADDRESS, 0, 0, 0,BootCmd,0);
	}
	else if(pb1_enable == 1 && pb2_enable == 1){
		PRINT_INFO("\n********************** Keyword Spotter ************************\n");
		/* Disable Push buttons so that they can be used by the primary applications */
		CleanupPushButtonGPIO();
		START_ADDRESS = 0x70E3BA00;
		adi_rom_Boot((void *)START_ADDRESS, 0, 0, 0,BootCmd,0);
	}
	else if(pb1_enable == 1){
		PRINT_INFO("\n****************** Genre Identification ********************\n");
		START_ADDRESS = 0x70513e00;
		adi_rom_Boot((void *)START_ADDRESS, 0, 0, 0,BootCmd,0);
	}
	else if(pb2_enable == 1){
		PRINT_INFO("\n ********************* UrbanSound Identification **********************\n");
		START_ADDRESS = 0x709A7C00;
		adi_rom_Boot((void *)START_ADDRESS, 0, 0, 0,BootCmd,0);
	}

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
