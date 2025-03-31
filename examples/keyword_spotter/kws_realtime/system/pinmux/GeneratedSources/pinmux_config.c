/*
 **
 ** Source file generated on March 12, 2025 at 14:13:18.	
 **
 ** Copyright (C) 2011-2025 Analog Devices Inc., All Rights Reserved.
 **
 ** This file is generated automatically based upon the options selected in 
 ** the Pin Multiplexing configuration editor. Changes to the Pin Multiplexing
 ** configuration should be made by changing the appropriate options rather
 ** than editing this file.
 **
 ** Selected Peripherals
 ** --------------------
 ** TWI1 (SCL, SDA)
 ** TWI2 (SCL, SDA)
 **
 ** GPIO (unavailable)
 ** ------------------
 ** PA14, PA15, PB00, PB01
 */

#include <sys/platform.h>
#include <stdint.h>

#define TWI1_SCL_PORTB_MUX  ((uint16_t) ((uint16_t) 1<<0))
#define TWI1_SDA_PORTB_MUX  ((uint16_t) ((uint16_t) 1<<2))
#define TWI2_SCL_PORTA_MUX  ((uint32_t) ((uint32_t) 0<<28))
#define TWI2_SDA_PORTA_MUX  ((uint32_t) ((uint32_t) 0<<30))

#define TWI1_SCL_PORTB_FER  ((uint16_t) ((uint16_t) 1<<0))
#define TWI1_SDA_PORTB_FER  ((uint16_t) ((uint16_t) 1<<1))
#define TWI2_SCL_PORTA_FER  ((uint32_t) ((uint32_t) 1<<14))
#define TWI2_SDA_PORTA_FER  ((uint32_t) ((uint32_t) 1<<15))

int32_t adi_initpinmux(void);

/*
 * Initialize the Port Control MUX and FER Registers
 */
int32_t adi_initpinmux(void) {
    /* PORTx_MUX registers */
    *pREG_PORTA_MUX = TWI2_SCL_PORTA_MUX | TWI2_SDA_PORTA_MUX;
    *pREG_PORTB_MUX = TWI1_SCL_PORTB_MUX | TWI1_SDA_PORTB_MUX;

    /* PORTx_FER registers */
    *pREG_PORTA_FER = TWI2_SCL_PORTA_FER | TWI2_SDA_PORTA_FER;
    *pREG_PORTB_FER = TWI1_SCL_PORTB_FER | TWI1_SDA_PORTB_FER;
    return 0;
}

