/*
 ** Copyright (C) 2021 Analog Devices Inc., All Rights Reserved.
 **
 ** This file was originally generated based upon the options selected in
 ** the Basic Configuration of CGU Initialization configuration dialog.
 ** Subsequently it has been manually edited.
 */
/** @addtogroup Init_Preload_SC59x Processor Initialization Code
 *  @{
 *
 */

/*!
* @file      adi_pwr_SC598_config.c
*
* @brief     power Service configuration file
*
* @details
*            power Service configuration file
*/

#include <sys/platform.h>
#include <stdint.h>
#include <stdlib.h>
#include <services/pwr/adi_pwr.h>


uint32_t adi_pwr_cfg0_init();


/**********************************************************************************************
 *                     CGU Configuration Number 0
 **********************************************************************************************/
/*
Configuration Number    : 0
SDRAM Mode              : DDR3
SYS_CLKIN0 (MHz)        : 25
Use CGU1 ?              : Yes

CDU Initialization Options
--------------------------
SHARCFX & its Accelerators (CLKO0) : CCLK0_0          : 1000 MHz
ARM M33                    (CLKO2) : SCLK1_EXEN_0     :  200 MHz
DDR                        (CLKO3) : DCLK_1			  :  800 MHz
CANFD                      (CLKO4) : OCLK_1           :  100 MHz
SPDIF                      (CLKO5) : SCLK1_EXEN_0     :  200MHz to source M33. With this 200MHz, SPDIF freq is reduced initially, (it'll be be changed later to 333.3 MHz & M33 - 400MHz)
SPI                        (CLKO6) : SCLK0_0          :  125 MHz
GigE                       (CLKO7) : SCLK0_0          :  125 MHz
LP                         (CLKO8) : SCLK0_0          :  125 MHz
LP_DDR                     (CLKO9) : OCLK_0           :  250 MHz
xSPI	                   (CLKO10): SYSCLK_0         :  125 MHz //to be checked
TRACE                      (CLKO12): SCLK0_0          :  125 MHz
ePWM                       (CLKO13): SCLK0_EXEN_1     :  125 MHz


CGU0 Initialization Options
---------------------------
fVCO                   :   4000.00 MHz
fPLL                   :   2000.00 MHz
CCLK                   :   1000.00 MHz
SYSCLK                 :    500.00 MHz
SCLK0                  :    125.00 MHz
SCLK1                  :    250.00 MHz
SCLK1_EXEN             :    200.00 MHz
DCLK                   :    667.67 MHz
OCLK                   :    250.00 MHz

MSEL                   :   80
Use DF?                :   No
DF                     :   0
CSEL                   :   2
CCLK to SYSCLK Ratio   :   2:1
SYSCLK to SCLK0 Ratio  :   4:1
SYSSEL				   :   4
S0SEL                  :   4
S1SEL                  :   2
DSEL                   :   3
OSEL                   :   8
Use S1SELEX?           :   Yes
S1SELEX                :   10

CGU1 Initialization Options
---------------------------
fVCO                   :   3200.00 MHz
fPLL                   :   1600.00 MHz
CCLK                   :    100.00 MHz
SYSCLK                 :    400.00 MHz
SCLK0                  :    100.00 MHz
SCLK0_EXEN             :     50.00 MHz
SCLK1                  :    100.00 MHz
SCLK1_EXEN             :    100.00 MHz
DCLK                   :    800.00 MHz
OCLK                   :    100.00 MHz

MSEL                   :   64
Use DF?                :   No
DF                     :   0
CSEL                   :   16
SYSSEL				   :   4
S0SEL                  :   4
S1SEL                  :   4
DSEL                   :   2
OSEL                   :   16
Use S0SELEX?           :   Yes
S0SELEX                :   32
Use S1SELEX?           :   Yes
S1SELEX                :   16
*/

/**********************************************************************************************
 *                      CGU Configuration Number 0
 **********************************************************************************************/

#define CFG0_BIT_CGU0_CLKIN                                25000000 /*!< Macro for SYS_CLKIN */
#define CFG0_BIT_CGU1_CLKIN                                25000000 /*!< Macro for SYS_CLKIN */

/*****************************************CGU1_CLKINSELV**********************************************/
#define CFG0_BIT_CDU0_CLKINSEL                             (ADI_PWR_CDU_CLK_SELECT_CLKIN0) /*!< Macro for CDU CFG0 Selection */

#define CFG0_BIT_CDU0_CFG0_SEL_VALUE                       (ENUM_CDU_CFG_IN0)        /*!< CLKO0 : SHARC 0 and its accelerators : CCLK0_0 */

#define CFG0_BIT_CDU0_CFG2_SEL_VALUE                       (ENUM_CDU_CFG_IN0)        /*!< CLKO2 : ARM : CCLK2_0 CCLK2_1 */

#define CFG0_BIT_CDU0_CFG3_SEL_VALUE                       (ENUM_CDU_CFG_IN1)        /*!< CLKO3 : DDR CLOCK : DCLK_0 DCLK_1 */

#define CFG0_BIT_CDU0_CFG4_SEL_VALUE                       (ENUM_CDU_CFG_IN1)        /*!< CLKO4 : CAN : OCLK_1 */

#define CFG0_BIT_CDU0_CFG5_SEL_VALUE                       (ENUM_CDU_CFG_IN0)        /*!< CLKO5 : SPDIF : SCLK1_0 */

#define CFG0_BIT_CDU0_CFG6_SEL_VALUE                       (ENUM_CDU_CFG_IN0)        /*!< CLKO6 : SPI : SCLK0_0 OCLK_0 */

#define CFG0_BIT_CDU0_CFG7_SEL_VALUE                       (ENUM_CDU_CFG_IN0)        /*!< CLKO7 : GigE : SCLK0_0 SCLK0_1 */

#define CFG0_BIT_CDU0_CFG8_SEL_VALUE                       (ENUM_CDU_CFG_IN1)        /*!< CLKO8 : LP  : OCLK_0 SCLK0_0 CCLK0_1 */

#define CFG0_BIT_CDU0_CFG9_SEL_VALUE                       (ENUM_CDU_CFG_IN0)        /*!< CLKO9 : LP_DDR : OCLK_0 DCLK_0 SYSCLKO_1 */

#define CFG0_BIT_CDU0_CFG10_SEL_VALUE                      (ENUM_CDU_CFG_IN2)        /*!< CLKO10 : OSPI_REFCLK : SYSCLK0_0 SCLK0_0 SCLK1_1 */

#define CFG0_BIT_CDU0_CFG12_SEL_VALUE                      (ENUM_CDU_CFG_IN0)        /*!< CLKO12 : TRACE : SCLK0_0 */

#define CFG0_BIT_CDU0_CFG13_SEL_VALUE                      (ENUM_CDU_CFG_IN1)        /*!< CLKO13 : EMMC : OCLK_0 SCLK0_1 DCLK_0/2 DCLK_1/2  */

/**********************************************************************************************
 *                     CGU Configuration Number 0 Register Values
 **********************************************************************************************/
/*****************************************CGU0_CTL**********************************************/
#define CFG0_BIT_CGU0_CTL_DF                               0        /*!< Macro for CGU0 DF bit */
#define CFG0_BIT_CGU0_CTL_MSEL                             80       /*!< Macro for CGU0 MSEL field */
/*****************************************CGU0_DIV**********************************************/
#define CFG0_BIT_CGU0_DIV_CSEL                             2        /*!< Macro for CGU0 CSEL field */
#define CFG0_BIT_CGU0_DIV_SYSSEL                           4        /*!< Macro for CGU0 SYSSEL field */
#define CFG0_BIT_CGU0_DIV_S0SEL                            4        /*!< Macro for CGU0 S0SEL field */
#define CFG0_BIT_CGU0_DIV_S1SEL                            2        /*!< Macro for CGU0 S1SEL field */
#define CFG0_BIT_CGU0_DIV_DSEL                             3        /*!< Macro for CGU0 DSEL field */
#define CFG0_BIT_CGU0_DIV_OSEL                             8        /*!< Macro for CGU0 OSEL field */
/*****************************************CGU0_DIVEX**********************************************/
#define CFG0_BIT_CGU0_DIV_S1SELEX                          10        /*!< Macro for CGU0 S1SELEX field */

/*****************************************CGU1_CTL**********************************************/
#define CFG0_BIT_CGU1_CTL_DF                               0        /*!< Macro for CGU1 DF bit */
#define CFG0_BIT_CGU1_CTL_MSEL                             64       /*!< Macro for CGU1 MSEL field */
/*****************************************CGU1_DIV**********************************************/
#define CFG0_BIT_CGU1_DIV_CSEL                             16       /*!< Macro for CGU1 CSEL field */
#define CFG0_BIT_CGU1_DIV_SYSSEL                           4        /*!< Macro for CGU1 SYSSEL field */
#define CFG0_BIT_CGU1_DIV_S0SEL                            4        /*!< Macro for CGU1 S0SEL field */
#define CFG0_BIT_CGU1_DIV_S1SEL                            4        /*!< Macro for CGU1 S1SEL field */
#define CFG0_BIT_CGU1_DIV_DSEL                             2        /*!< Macro for CGU1 DSEL field */
#define CFG0_BIT_CGU1_DIV_OSEL                             16       /*!< Macro for CGU1 OSEL field */
/*****************************************CGU1_DIVEX**********************************************/
#define CFG0_BIT_CGU1_DIV_S0SELEX                          32       /*!< Macro for CGU1 S1SELEX field */
#define CFG0_BIT_CGU1_DIV_S1SELEX                          16       /*!< Macro for CGU1 S1SELEX field */

/**
 * @brief    Initializes clocks, including CGU and CDU modules.
 *
 * @return   Status
 *           - 0: Successful in all the initializations.
 *           - 1: Error.

 */
uint32_t adi_pwr_cfg0_init()
{
    uint32_t status = 0u; /*Return zero if there are no errors*/

    /* Structure pointer for CGU0 and CGU1 parameters*/
    ADI_PWR_CGU_PARAM_LIST pADI_CGU_Param_List;

    /* Structure pointer for CDU parameters*/
    ADI_PWR_CDU_PARAM_LIST pADI_CDU_Param_List;

    /* CDU Configuration*/
    pADI_CDU_Param_List.cdu_settings[0].cfg_SEL                     =       (ADI_PWR_CDU_CLKIN)CFG0_BIT_CDU0_CFG0_SEL_VALUE;
    pADI_CDU_Param_List.cdu_settings[0].cfg_EN                      =       true;

    pADI_CDU_Param_List.cdu_settings[2].cfg_SEL                     =       (ADI_PWR_CDU_CLKIN)CFG0_BIT_CDU0_CFG2_SEL_VALUE;
    pADI_CDU_Param_List.cdu_settings[2].cfg_EN                      =       true;

    pADI_CDU_Param_List.cdu_settings[3].cfg_SEL                     =       (ADI_PWR_CDU_CLKIN)CFG0_BIT_CDU0_CFG3_SEL_VALUE;
    pADI_CDU_Param_List.cdu_settings[3].cfg_EN                      =       true;

    pADI_CDU_Param_List.cdu_settings[4].cfg_SEL                     =       (ADI_PWR_CDU_CLKIN)CFG0_BIT_CDU0_CFG4_SEL_VALUE;
    pADI_CDU_Param_List.cdu_settings[4].cfg_EN                      =       true;

    pADI_CDU_Param_List.cdu_settings[5].cfg_SEL                     =       (ADI_PWR_CDU_CLKIN)CFG0_BIT_CDU0_CFG5_SEL_VALUE;
    pADI_CDU_Param_List.cdu_settings[5].cfg_EN                      =       true;

    pADI_CDU_Param_List.cdu_settings[6].cfg_SEL                     =       (ADI_PWR_CDU_CLKIN)CFG0_BIT_CDU0_CFG6_SEL_VALUE;
    pADI_CDU_Param_List.cdu_settings[6].cfg_EN                      =       true;

    pADI_CDU_Param_List.cdu_settings[7].cfg_SEL                     =       (ADI_PWR_CDU_CLKIN)CFG0_BIT_CDU0_CFG7_SEL_VALUE;
    pADI_CDU_Param_List.cdu_settings[7].cfg_EN                      =       true;

    pADI_CDU_Param_List.cdu_settings[8].cfg_SEL                     =       (ADI_PWR_CDU_CLKIN)CFG0_BIT_CDU0_CFG8_SEL_VALUE;
    pADI_CDU_Param_List.cdu_settings[8].cfg_EN                      =       true;

    pADI_CDU_Param_List.cdu_settings[9].cfg_SEL                     =       (ADI_PWR_CDU_CLKIN)CFG0_BIT_CDU0_CFG9_SEL_VALUE;
    pADI_CDU_Param_List.cdu_settings[9].cfg_EN                      =       true;

    pADI_CDU_Param_List.cdu_settings[10].cfg_SEL                    =       (ADI_PWR_CDU_CLKIN)CFG0_BIT_CDU0_CFG10_SEL_VALUE;
    pADI_CDU_Param_List.cdu_settings[10].cfg_EN                     =       true;


    pADI_CDU_Param_List.cdu_settings[12].cfg_SEL                    =       (ADI_PWR_CDU_CLKIN)CFG0_BIT_CDU0_CFG12_SEL_VALUE;
    pADI_CDU_Param_List.cdu_settings[12].cfg_EN                     =       true;

    pADI_CDU_Param_List.cdu_settings[13].cfg_SEL                    =       (ADI_PWR_CDU_CLKIN)CFG0_BIT_CDU0_CFG13_SEL_VALUE;
    pADI_CDU_Param_List.cdu_settings[13].cfg_EN                     =       true;


    /* CGU0 Configuration*/
    pADI_CGU_Param_List.cgu0_settings.clocksettings.ctl_MSEL        =       (uint32_t)CFG0_BIT_CGU0_CTL_MSEL;
    pADI_CGU_Param_List.cgu0_settings.clocksettings.ctl_DF          =       (uint32_t)CFG0_BIT_CGU0_CTL_DF;
    pADI_CGU_Param_List.cgu0_settings.clocksettings.div_CSEL        =       (uint32_t)CFG0_BIT_CGU0_DIV_CSEL;
    pADI_CGU_Param_List.cgu0_settings.clocksettings.div_SYSSEL      =       (uint32_t)CFG0_BIT_CGU0_DIV_SYSSEL;
    pADI_CGU_Param_List.cgu0_settings.clocksettings.div_S0SEL       =       (uint32_t)CFG0_BIT_CGU0_DIV_S0SEL;
    pADI_CGU_Param_List.cgu0_settings.clocksettings.div_S1SEL       =       (uint32_t)CFG0_BIT_CGU0_DIV_S1SEL;
    pADI_CGU_Param_List.cgu0_settings.clocksettings.divex_S1SELEX   =       (uint32_t)CFG0_BIT_CGU0_DIV_S1SELEX;
    pADI_CGU_Param_List.cgu0_settings.clocksettings.div_DSEL        =       (uint32_t)CFG0_BIT_CGU0_DIV_DSEL;
    pADI_CGU_Param_List.cgu0_settings.clocksettings.div_OSEL        =       (uint32_t)CFG0_BIT_CGU0_DIV_OSEL;
    pADI_CGU_Param_List.cgu0_settings.clkin                         =       (uint32_t)CFG0_BIT_CGU0_CLKIN;
    pADI_CGU_Param_List.cgu0_settings.enable_SCLK1ExDiv             =       true;

    /* CGU1 Configuration*/
    pADI_CGU_Param_List.cgu1_settings.clocksettings.ctl_MSEL        =       (uint32_t)CFG0_BIT_CGU1_CTL_MSEL;
    pADI_CGU_Param_List.cgu1_settings.clocksettings.ctl_DF          =       (uint32_t)CFG0_BIT_CGU1_CTL_DF;
    pADI_CGU_Param_List.cgu1_settings.clocksettings.div_CSEL        =       (uint32_t)CFG0_BIT_CGU1_DIV_CSEL;
    pADI_CGU_Param_List.cgu1_settings.clocksettings.div_SYSSEL      =       (uint32_t)CFG0_BIT_CGU1_DIV_SYSSEL;
    pADI_CGU_Param_List.cgu1_settings.clocksettings.div_S0SEL       =       (uint32_t)CFG0_BIT_CGU1_DIV_S0SEL;
    pADI_CGU_Param_List.cgu1_settings.clocksettings.div_S1SEL       =       (uint32_t)CFG0_BIT_CGU1_DIV_S1SEL;
    pADI_CGU_Param_List.cgu1_settings.clocksettings.divex_S0SELEX   =       (uint32_t)CFG0_BIT_CGU1_DIV_S0SELEX;
    pADI_CGU_Param_List.cgu1_settings.clocksettings.divex_S1SELEX   =       (uint32_t)CFG0_BIT_CGU1_DIV_S1SELEX;
    pADI_CGU_Param_List.cgu1_settings.clocksettings.div_DSEL        =       (uint32_t)CFG0_BIT_CGU1_DIV_DSEL;
    pADI_CGU_Param_List.cgu1_settings.clocksettings.div_OSEL        =       (uint32_t)CFG0_BIT_CGU1_DIV_OSEL;
    pADI_CGU_Param_List.cgu1_settings.clkin                         =       (uint32_t)CFG0_BIT_CGU1_CLKIN;
    pADI_CGU_Param_List.cgu1_settings.enable_SCLK0ExDiv             =       true;
    pADI_CGU_Param_List.cgu1_settings.enable_SCLK1ExDiv             =       true;

    pADI_CGU_Param_List.cgu1_settings.cgu1_clkinsel                 =       (ADI_PWR_CDU_CLK_SELECT)ADI_PWR_CDU_CLK_SELECT_CLKIN0;


    /* Initialize all the clocks */
    if (adi_pwr_ClockInit(&pADI_CGU_Param_List, &pADI_CDU_Param_List) != ADI_PWR_SUCCESS)
    {
       /* Return non-zero */
       status = 1u;
    }

    return status;
   }

/*@}*/
