/*
** Copyright (C) 2018-2021 Analog Devices Inc., All Rights Reserved.
**
** This file was originally generated automatically based upon options
** selected in the DMC Initialization configuration dialog.
*/

/*!
* @file      adi_dmc.h
*
* @brief     DMC Configuration header file
*
* @details
*            DMC Configuration header file
*/

#ifndef ADI_DMC_H
#define ADI_DMC_H

#include "config.h"


#define cclkdclk_ratio (1.11f)
//#if CONFIG_DMC0

#include <sys/platform.h>
#include <stdbool.h>
#if defined(__ADSPSHARC__) || defined(__ADSPSHARCFX__)
#include <adi_types.h>
#endif

static void execute_nop()
{
    asm volatile("nop;");
}

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_5_1:"Identifiers (internal and external) shall not rely on the significance of more than 31 characters")
#endif

/* Additional required DMC macros */
#define BITP_DMC_TR2_TWR            12 /* Timing Write Recovery */

#define BITP_DMC_EMR1_WL            7
#define BITM_DMC_EMR1_WL            (1ul<<BITP_DMC_EMR1_WL)

#define pREG_DMC0_DDR_SCRATCH_2     ((volatile uint32_t*)0x31071074)
#define pREG_DMC0_DDR_SCRATCH_3     ((volatile uint32_t*)0x31071078)
#define pREG_DMC0_DDR_SCRATCH_6     ((volatile uint32_t*)0x31071084)
#define pREG_DMC0_DDR_SCRATCH_7     ((volatile uint32_t*)0x31071088)
#define pREG_DMC0_DDR_SCRATCH_STAT0     ((volatile uint32_t*)0x3107107C)
#define pREG_DMC0_DDR_SCRATCH_STAT1     ((volatile uint32_t*)0x31071080)
///* ============================================================================================================================
//        DMC0
//   ============================================================================================================================ */
//#define pREG_DMC0_DDR_LANE0_CTL0             (volatile uint32_t*)0x31071000            /*  DMC0 Data Lane 0 Control Register 0 */
//#define pREG_DMC0_DDR_LANE0_CTL1            (volatile uint32_t*)0x31071004            /*  DMC0 Data Lane 0 Control Register 1 */
//#define pREG_DMC0_DDR_LANE1_CTL0            (volatile uint32_t*)0x3107100C            /*  DMC0 Data Lane 1 Control Register 0 */
//#define pREG_DMC0_DDR_LANE1_CTL1            (volatile uint32_t*)0x31071010            /*  DMC0 Data Lane 1 Control Register 1 */
//#define pREG_DMC0_DDR_ROOT_CTL              (volatile uint32_t*)0x31071018            /*  DMC0 DDR ROOT Module Control Register */
//#define pREG_DMC0_DDR_ZQ_CTL0               (volatile uint32_t*)0x31071034            /*  DMC0 DDR Calibration Control Register 0 */
//#define pREG_DMC0_DDR_ZQ_CTL1                (volatile uint32_t*)0x31071038            /*  DMC0 DDR Calibration Control Register 1 */
//#define pREG_DMC0_DDR_ZQ_CTL2                (volatile uint32_t*)0x3107103C            /*  DMC0 DDR Calibration Control Register 2 */
//#define pREG_DMC0_DDR_CA_CTL                 (volatile uint32_t*)0x31071068            /*  DMC0 DDR CA Lane Control Register */
//#define pREG_DMC0_DDR_SCRATCH_4              (volatile uint32_t*)0x3107107C            /*  DMC0 Scratch Register 4 */
//#define pREG_DMC0_DDR_SCRATCH_5              (volatile uint32_t*)0x31071080            /*  DMC0 Scratch Register 5 */

/* ============================================================================================================================
        DMC Register BitMasks, Positions & Enumerations
   ============================================================================================================================ */
/* -------------------------------------------------------------------------------------------------------------------------
          DMC_DDR_LANE0_CTL0                   Pos/Masks         Description
   ------------------------------------------------------------------------------------------------------------------------- */
#define BITP_DMC_DDR_LANE0_CTL0_CB_RSTDAT    27            /*  Reset the Data Pads */
#define BITP_DMC_DDR_LANE0_CTL0_BYPCODE      12            /*  Used to select from four levels of duty cycle adjustment */
#define BITP_DMC_DDR_LANE0_CTL0_BYPSELP      11            /*  Duty Cycle Offset Direction Select */
#define BITP_DMC_DDR_LANE0_CTL0_BYPENB       10            /*  DQS Duty Cycle Offset Adjustment Enable */
#define BITP_DMC_DDR_LANE0_CTL0_CB_RSTDLL     8            /*  Reset the Lane DLL */
#define BITP_DMC_DDR_LANE0_CTL0_MF_SEL        5            /*  SPI Master Frequency Divider Select */
#define BITP_DMC_DDR_LANE0_CTL0_DDR_MODE      1            /*  DDR Mode */
#define BITP_DMC_DDR_LANE0_CTL0_LANE_DIS      0            /*  Clock Gate Data Lane */
#define BITM_DMC_DDR_LANE0_CTL0_CB_RSTDAT    (_ADI_MSK_3(0x08000000,0x08000000UL,uint32_t))    /* Reset the Data Pads */
#define BITM_DMC_DDR_LANE0_CTL0_BYPCODE      (_ADI_MSK_3(0x0000F000,0x0000F000UL,uint32_t))    /* Used to select from four levels of duty cycle adjustment */
#define BITM_DMC_DDR_LANE0_CTL0_BYPSELP      (_ADI_MSK_3(0x00000800,0x00000800UL,uint32_t))    /* Duty Cycle Offset Direction Select */
#define BITM_DMC_DDR_LANE0_CTL0_BYPENB       (_ADI_MSK_3(0x00000400,0x00000400UL,uint32_t))    /* DQS Duty Cycle Offset Adjustment Enable */
#define BITM_DMC_DDR_LANE0_CTL0_CB_RSTDLL    (_ADI_MSK_3(0x00000100,0x00000100UL,uint32_t))    /* Reset the Lane DLL */
#define BITM_DMC_DDR_LANE0_CTL0_MF_SEL       (_ADI_MSK_3(0x00000060,0x00000060UL,uint32_t))    /* SPI Master Frequency Divider Select */
#define BITM_DMC_DDR_LANE0_CTL0_DDR_MODE     (_ADI_MSK_3(0x00000006,0x00000006UL,uint32_t))    /* DDR Mode */
#define BITM_DMC_DDR_LANE0_CTL0_LANE_DIS     (_ADI_MSK_3(0x00000001,0x00000001UL,uint32_t))    /* Clock Gate Data Lane */
#define ENUM_DMC_DDR_LANE0_CTL0_DUTY_ADJ_L1  (_ADI_MSK_3(0x00001000,0x00001000UL,uint32_t))
#define ENUM_DMC_DDR_LANE0_CTL0_DUTY_ADJ_L2  (_ADI_MSK_3(0x00002000,0x00002000UL,uint32_t))
#define ENUM_DMC_DDR_LANE0_CTL0_DUTY_ADJ_L3  (_ADI_MSK_3(0x00004000,0x00004000UL,uint32_t))
#define ENUM_DMC_DDR_LANE0_CTL0_DUTY_ADJ_L4  (_ADI_MSK_3(0x00008000,0x00008000UL,uint32_t))
#define ENUM_DMC_DDR_LANE0_CTL0_DIRECTION_NEG (_ADI_MSK_3(0x00000000,0x00000000UL,uint32_t))    /*  BYPSELP: Negative */
#define ENUM_DMC_DDR_LANE0_CTL0_DIRECTION_POS (_ADI_MSK_3(0x00000800,0x00000800UL,uint32_t))    /*  BYPSELP: Positive */
#define ENUM_DMC_DDR_LANE0_CTL0_DQS_DUTY_DISABLE (_ADI_MSK_3(0x00000000,0x00000000UL,uint32_t))    /*  BYPENB: Disable */
#define ENUM_DMC_DDR_LANE0_CTL0_DQS_DUTY_ENABLE (_ADI_MSK_3(0x00000400,0x00000400UL,uint32_t))    /*  BYPENB: Enable */
#define ENUM_DMC_DDR_LANE0_CTL0_DIVBY2       (_ADI_MSK_3(0x00000000,0x00000000UL,uint32_t))    /*  MF_SEL: Divide by 2 */
#define ENUM_DMC_DDR_LANE0_CTL0_DIVBY4       (_ADI_MSK_3(0x00000020,0x00000020UL,uint32_t))    /*  MF_SEL: Divide by 4 */
#define ENUM_DMC_DDR_LANE0_CTL0_DIVBY8       (_ADI_MSK_3(0x00000040,0x00000040UL,uint32_t))    /*  MF_SEL: Divide by 8 */
#define ENUM_DMC_DDR_LANE0_CTL0_DIVBY16      (_ADI_MSK_3(0x00000060,0x00000060UL,uint32_t))    /*  MF_SEL: Divide by 16 */
#define ENUM_DMC_DDR_LANE0_CTL0_DDR3         (_ADI_MSK_3(0x00000000,0x00000000UL,uint32_t))    /*  DDR_MODE: DDR3/3L Mode */
#define ENUM_DMC_DDR_LANE0_CTL0_ENABLED      (_ADI_MSK_3(0x00000000,0x00000000UL,uint32_t))    /*  LANE_DIS: Lane is Enabled */
#define ENUM_DMC_DDR_LANE0_CTL0_DISABLED     (_ADI_MSK_3(0x00000001,0x00000001UL,uint32_t))    /*  LANE_DIS: Lane is Clock Gated */

/* -------------------------------------------------------------------------------------------------------------------------
          DMC_DDR_LANE0_CTL1                   Pos/Masks         Description
   ------------------------------------------------------------------------------------------------------------------------- */
#define BITP_DMC_DDR_LANE0_CTL1_BYPDELCHAINEN 15            /*  Bypass enable for delay chain controlling extra delay on DATA pins */
#define BITP_DMC_DDR_LANE0_CTL1_BYPCODE      10            /*  Bypass Code for extra delay on DATA pins */
#define BITP_DMC_DDR_LANE0_CTL1_COMP_DCYCLE   1            /*  Compute Datacycle */
#define BITM_DMC_DDR_LANE0_CTL1_BYPDELCHAINEN (_ADI_MSK_3(0x00008000,0x00008000UL,uint32_t))    /* Bypass enable for delay chain controlling extra delay on DATA pins */
#define BITM_DMC_DDR_LANE0_CTL1_BYPCODE      (_ADI_MSK_3(0x00007C00,0x00007C00UL,uint32_t))    /* Bypass Code for extra delay on DATA pins */
#define BITM_DMC_DDR_LANE0_CTL1_COMP_DCYCLE  (_ADI_MSK_3(0x00000002,0x00000002UL,uint32_t))    /* Compute Datacycle */

/* -------------------------------------------------------------------------------------------------------------------------
          DMC_DDR_LANE1_CTL0                   Pos/Masks         Description
   ------------------------------------------------------------------------------------------------------------------------- */
#define BITP_DMC_DDR_LANE1_CTL0_CB_RSTDAT    27            /*  Reset the Data Pads */
#define BITP_DMC_DDR_LANE1_CTL0_BYPCODE      12            /*  Used to select from four levels of duty cycle adjustment */
#define BITP_DMC_DDR_LANE1_CTL0_BYPSELP      11            /*  Duty Cycle Offset Direction Select */
#define BITP_DMC_DDR_LANE1_CTL0_BYPENB       10            /*  DQS Duty Cycle Offset Adjustment Enable */
#define BITP_DMC_DDR_LANE1_CTL0_CB_RSTDLL     8            /*  Reset the Lane DLL */
#define BITP_DMC_DDR_LANE1_CTL0_MF_SEL        5            /*  SPI Master Frequency Divider Select */
#define BITP_DMC_DDR_LANE1_CTL0_DDR_MODE      1            /*  DDR Mode */
#define BITP_DMC_DDR_LANE1_CTL0_LANE_DIS      0            /*  Clock Gate Data Lane */
#define BITM_DMC_DDR_LANE1_CTL0_CB_RSTDAT    (_ADI_MSK_3(0x08000000,0x08000000UL,uint32_t))    /* Reset the Data Pads */
#define BITM_DMC_DDR_LANE1_CTL0_BYPCODE      (_ADI_MSK_3(0x0000F000,0x0000F000UL,uint32_t))    /* Used to select from four levels of duty cycle adjustment */
#define BITM_DMC_DDR_LANE1_CTL0_BYPSELP      (_ADI_MSK_3(0x00000800,0x00000800UL,uint32_t))    /* Duty Cycle Offset Direction Select */
#define BITM_DMC_DDR_LANE1_CTL0_BYPENB       (_ADI_MSK_3(0x00000400,0x00000400UL,uint32_t))    /* DQS Duty Cycle Offset Adjustment Enable */
#define BITM_DMC_DDR_LANE1_CTL0_CB_RSTDLL    (_ADI_MSK_3(0x00000100,0x00000100UL,uint32_t))    /* Reset the Lane DLL */
#define BITM_DMC_DDR_LANE1_CTL0_MF_SEL       (_ADI_MSK_3(0x00000060,0x00000060UL,uint32_t))    /* SPI Master Frequency Divider Select */
#define BITM_DMC_DDR_LANE1_CTL0_DDR_MODE     (_ADI_MSK_3(0x00000006,0x00000006UL,uint32_t))    /* DDR Mode */
#define BITM_DMC_DDR_LANE1_CTL0_LANE_DIS     (_ADI_MSK_3(0x00000001,0x00000001UL,uint32_t))    /* Clock Gate Data Lane */
#define ENUM_DMC_DDR_LANE1_CTL0_DUTY_ADJ_L1  (_ADI_MSK_3(0x00001000,0x00001000UL,uint32_t))
#define ENUM_DMC_DDR_LANE1_CTL0_DUTY_ADJ_L2  (_ADI_MSK_3(0x00002000,0x00002000UL,uint32_t))
#define ENUM_DMC_DDR_LANE1_CTL0_DUTY_ADJ_L3  (_ADI_MSK_3(0x00004000,0x00004000UL,uint32_t))
#define ENUM_DMC_DDR_LANE1_CTL0_DUTY_ADJ_L4  (_ADI_MSK_3(0x00008000,0x00008000UL,uint32_t))
#define ENUM_DMC_DDR_LANE1_CTL0_DIRECTION_NEG (_ADI_MSK_3(0x00000000,0x00000000UL,uint32_t))    /*  BYPSELP: Negative */
#define ENUM_DMC_DDR_LANE1_CTL0_DIRECTION_POS (_ADI_MSK_3(0x00000800,0x00000800UL,uint32_t))    /*  BYPSELP: Positive */
#define ENUM_DMC_DDR_LANE1_CTL0_DQS_DUTY_DISABLE (_ADI_MSK_3(0x00000000,0x00000000UL,uint32_t))    /*  BYPENB: Disable */
#define ENUM_DMC_DDR_LANE1_CTL0_DQS_DUTY_ENABLE (_ADI_MSK_3(0x00000400,0x00000400UL,uint32_t))    /*  BYPENB: Enable */
#define ENUM_DMC_DDR_LANE1_CTL0_DIVBY2       (_ADI_MSK_3(0x00000000,0x00000000UL,uint32_t))    /*  MF_SEL: Divide by 2 */
#define ENUM_DMC_DDR_LANE1_CTL0_DIVBY4       (_ADI_MSK_3(0x00000020,0x00000020UL,uint32_t))    /*  MF_SEL: Divide by 4 */
#define ENUM_DMC_DDR_LANE1_CTL0_DIVBY8       (_ADI_MSK_3(0x00000040,0x00000040UL,uint32_t))    /*  MF_SEL: Divide by 8 */
#define ENUM_DMC_DDR_LANE1_CTL0_DIVBY16      (_ADI_MSK_3(0x00000060,0x00000060UL,uint32_t))    /*  MF_SEL: Divide by 16 */
#define ENUM_DMC_DDR_LANE1_CTL0_DDR3         (_ADI_MSK_3(0x00000000,0x00000000UL,uint32_t))    /*  DDR_MODE: DDR3/3L Mode */
#define ENUM_DMC_DDR_LANE1_CTL0_ENABLED      (_ADI_MSK_3(0x00000000,0x00000000UL,uint32_t))    /*  LANE_DIS: Lane is Enabled */
#define ENUM_DMC_DDR_LANE1_CTL0_DISABLED     (_ADI_MSK_3(0x00000001,0x00000001UL,uint32_t))    /*  LANE_DIS: Lane is Clock Gated */

/* -------------------------------------------------------------------------------------------------------------------------
          DMC_DDR_LANE1_CTL1                   Pos/Masks         Description
   ------------------------------------------------------------------------------------------------------------------------- */
#define BITP_DMC_DDR_LANE1_CTL1_BYPDELCHAINEN 15            /*  Bypass enable for delay chain controlling extra delay on DATA pins */
#define BITP_DMC_DDR_LANE1_CTL1_BYPCODE      10            /*  Bypass Code for extra delay on DATA pins */
#define BITP_DMC_DDR_LANE1_CTL1_COMP_DCYCLE   1            /*  Compute Datacycle */
#define BITM_DMC_DDR_LANE1_CTL1_BYPDELCHAINEN (_ADI_MSK_3(0x00008000,0x00008000UL,uint32_t))    /* Bypass enable for delay chain controlling extra delay on DATA pins */
#define BITM_DMC_DDR_LANE1_CTL1_BYPCODE      (_ADI_MSK_3(0x00007C00,0x00007C00UL,uint32_t))    /* Bypass Code for extra delay on DATA pins */
#define BITM_DMC_DDR_LANE1_CTL1_COMP_DCYCLE  (_ADI_MSK_3(0x00000002,0x00000002UL,uint32_t))    /* Compute Datacycle */

/* -------------------------------------------------------------------------------------------------------------------------
          DMC_DDR_ROOT_CTL                     Pos/Masks         Description
   ------------------------------------------------------------------------------------------------------------------------- */
#define BITP_DMC_DDR_ROOT_CTL_TRIG_RD_XFER_ALL 21            /*  All Lane Read Status */
#define BITP_DMC_DDR_ROOT_CTL_TRIG_WR_XFER_L1 18            /*  Write Transfer from Root to Lane1 */
#define BITP_DMC_DDR_ROOT_CTL_TRIG_WR_XFER_L0 17            /*  Write Transfer from Root to Lane 0 */
#define BITP_DMC_DDR_ROOT_CTL_TRIG_WR_XFER_CA 16            /*  Write Transfer from Root to CA Lane */
#define BITP_DMC_DDR_ROOT_CTL_MF_SEL         14            /*  SPI Master Frequency Divider Select */
#define BITP_DMC_DDR_ROOT_CTL_SW_REFRESH     13            /*  Refresh Lane DLL Code */
#define BITP_DMC_DDR_ROOT_CTL_PIPE_OFSTDCYCLE 10            /*  Pipeline offset for PHYC_DATACYCLE */
#define BITP_DMC_DDR_ROOT_CTL_CPHY_DCYCLE_VAL  6            /*  Bypass Value for CPHY_DATACYCLE */
#define BITP_DMC_DDR_ROOT_CTL_CPHY_DCYCLE_BYP  5            /*  Bypass Enable for CPHY_DATACYCLE */
#define BITP_DMC_DDR_ROOT_CTL_PHYC_DCYCLE_VAL  1            /*  Bypass Value for PHYC_DATACYCLE */
#define BITP_DMC_DDR_ROOT_CTL_PHYC_DCYCLE_BYP  0            /*  Bypass Enable for PHYC_DATACYCLE */
#define BITM_DMC_DDR_ROOT_CTL_TRIG_RD_XFER_ALL (_ADI_MSK_3(0x00200000,0x00200000UL,uint32_t))    /* All Lane Read Status */
#define BITM_DMC_DDR_ROOT_CTL_TRIG_WR_XFER_L1 (_ADI_MSK_3(0x00040000,0x00040000UL,uint32_t))    /* Write Transfer from Root to Lane1 */
#define BITM_DMC_DDR_ROOT_CTL_TRIG_WR_XFER_L0 (_ADI_MSK_3(0x00020000,0x00020000UL,uint32_t))    /* Write Transfer from Root to Lane 0 */
#define BITM_DMC_DDR_ROOT_CTL_TRIG_WR_XFER_CA (_ADI_MSK_3(0x00010000,0x00010000UL,uint32_t))    /* Write Transfer from Root to CA Lane */
#define BITM_DMC_DDR_ROOT_CTL_MF_SEL         (_ADI_MSK_3(0x0000C000,0x0000C000UL,uint32_t))    /* SPI Master Frequency Divider Select */
#define BITM_DMC_DDR_ROOT_CTL_SW_REFRESH     (_ADI_MSK_3(0x00002000,0x00002000UL,uint32_t))    /* Refresh Lane DLL Code */
#define BITM_DMC_DDR_ROOT_CTL_PIPE_OFSTDCYCLE (_ADI_MSK_3(0x00001C00,0x00001C00UL,uint32_t))    /* Pipeline offset for PHYC_DATACYCLE */
#define BITM_DMC_DDR_ROOT_CTL_CPHY_DCYCLE_VAL (_ADI_MSK_3(0x000003C0,0x000003C0UL,uint32_t))    /* Bypass Value for CPHY_DATACYCLE */
#define BITM_DMC_DDR_ROOT_CTL_CPHY_DCYCLE_BYP (_ADI_MSK_3(0x00000020,0x00000020UL,uint32_t))    /* Bypass Enable for CPHY_DATACYCLE */
#define BITM_DMC_DDR_ROOT_CTL_PHYC_DCYCLE_VAL (_ADI_MSK_3(0x0000001E,0x0000001EUL,uint32_t))    /* Bypass Value for PHYC_DATACYCLE */
#define BITM_DMC_DDR_ROOT_CTL_PHYC_DCYCLE_BYP (_ADI_MSK_3(0x00000001,0x00000001UL,uint32_t))    /* Bypass Enable for PHYC_DATACYCLE */
#define ENUM_DMC_DDR_ROOT_CTL_DIVBY2         (_ADI_MSK_3(0x00000000,0x00000000UL,uint32_t))    /*  MF_SEL: Divide by 2 */
#define ENUM_DMC_DDR_ROOT_CTL_DIVBY4         (_ADI_MSK_3(0x00004000,0x00004000UL,uint32_t))    /*  MF_SEL: Divide by 4 */
#define ENUM_DMC_DDR_ROOT_CTL_DIVBY8         (_ADI_MSK_3(0x00008000,0x00008000UL,uint32_t))    /*  MF_SEL: Divide by 8 */
#define ENUM_DMC_DDR_ROOT_CTL_DIVBY16        (_ADI_MSK_3(0x0000C000,0x0000C000UL,uint32_t))    /*  MF_SEL: Divide by 16 */

/* -------------------------------------------------------------------------------------------------------------------------
          DMC_DDR_ZQ_CTL0                      Pos/Masks         Description
   ------------------------------------------------------------------------------------------------------------------------- */
#define BITP_DMC_DDR_ZQ_CTL0_IMPRTT          16            /*  Data/DQS ODT */
#define BITP_DMC_DDR_ZQ_CTL0_IMPWRDQ          8            /*  Data/DQS/DM/CLK Drive Strength */
#define BITP_DMC_DDR_ZQ_CTL0_IMPWRADD         0            /*  Address/Command Drive Strength */
#define BITM_DMC_DDR_ZQ_CTL0_IMPRTT          (_ADI_MSK_3(0x00FF0000,0x00FF0000UL,uint32_t))    /* Data/DQS ODT */
#define BITM_DMC_DDR_ZQ_CTL0_IMPWRDQ         (_ADI_MSK_3(0x0000FF00,0x0000FF00UL,uint32_t))    /* Data/DQS/DM/CLK Drive Strength */
#define BITM_DMC_DDR_ZQ_CTL0_IMPWRADD        (_ADI_MSK_3(0x000000FF,0x000000FFUL,uint32_t))    /* Address/Command Drive Strength */

/* -------------------------------------------------------------------------------------------------------------------------
          DMC_DDR_ZQ_CTL1                      Pos/Masks         Description
   ------------------------------------------------------------------------------------------------------------------------- */
#define BITP_DMC_DDR_ZQ_CTL1_USEFNBUS        22            /*  BSCAN Mode */
#define BITP_DMC_DDR_ZQ_CTL1_PDCAL           12            /*  Calibration Pad Power Down */
#define BITP_DMC_DDR_ZQ_CTL1_BYPRDPD          6            /*  Pull-down Termination Bypass Code */
#define BITP_DMC_DDR_ZQ_CTL1_BYPRDPU          0            /*  Pull-up Termination Bypass Code */
#define BITM_DMC_DDR_ZQ_CTL1_USEFNBUS        (_ADI_MSK_3(0x00400000,0x00400000UL,uint32_t))    /* BSCAN Mode */
#define BITM_DMC_DDR_ZQ_CTL1_PDCAL           (_ADI_MSK_3(0x00001000,0x00001000UL,uint32_t))    /* Calibration Pad Power Down */
#define BITM_DMC_DDR_ZQ_CTL1_BYPRDPD         (_ADI_MSK_3(0x00000FC0,0x00000FC0UL,uint32_t))    /* Pull-down Termination Bypass Code */
#define BITM_DMC_DDR_ZQ_CTL1_BYPRDPU         (_ADI_MSK_3(0x0000003F,0x0000003FUL,uint32_t))    /* Pull-up Termination Bypass Code */

/* -------------------------------------------------------------------------------------------------------------------------
          DMC_DDR_ZQ_CTL2                      Pos/Masks         Description
   ------------------------------------------------------------------------------------------------------------------------- */
#define BITP_DMC_DDR_ZQ_CTL2_RTTCALEN        31            /*  Pad ODT Strength Calibration Enable */
#define BITP_DMC_DDR_ZQ_CTL2_PDCALEN         30            /*  Driver Pull-down Strength Calibration Enable */
#define BITP_DMC_DDR_ZQ_CTL2_PUCALEN         29            /*  Driver Pull-up Strength Calibration Enable */
#define BITP_DMC_DDR_ZQ_CTL2_CALSTRT         28            /*  New Impedance Calibration Enable */
#define BITP_DMC_DDR_ZQ_CTL2_CALTYPE         27            /*  Calibration Type */
#define BITP_DMC_DDR_ZQ_CTL2_BYPPDDQ         21            /*  Bypass Code for DQ,DQS,CLK,DM Pull-down Driver Code */
#define BITP_DMC_DDR_ZQ_CTL2_BYPPUDQ         15            /*  Bypass Code for DQ,DQS,CLK,DM Pull-up Code */
#define BITP_DMC_DDR_ZQ_CTL2_BYPPDADD         9            /*  Address/Command Bypass Pull-down Code */
#define BITP_DMC_DDR_ZQ_CTL2_BYPPUADD         3            /*  Address/Command Bypass Pull-up Codes */
#define BITP_DMC_DDR_ZQ_CTL2_BYPIMRD          2            /*  Bypass Termination Code Enable */
#define BITP_DMC_DDR_ZQ_CTL2_BYPIMDQ          1            /*  DQ/DQS/CLK/DM Bypass Driver Code Enable */
#define BITP_DMC_DDR_ZQ_CTL2_BYPIMAD          0            /*  Address/Command Bypass Code Enable */
#define BITM_DMC_DDR_ZQ_CTL2_RTTCALEN        (_ADI_MSK_3(0x80000000,0x80000000UL,uint32_t))    /* Pad ODT Strength Calibration Enable */
#define BITM_DMC_DDR_ZQ_CTL2_PDCALEN         (_ADI_MSK_3(0x40000000,0x40000000UL,uint32_t))    /* Driver Pull-down Strength Calibration Enable */
#define BITM_DMC_DDR_ZQ_CTL2_PUCALEN         (_ADI_MSK_3(0x20000000,0x20000000UL,uint32_t))    /* Driver Pull-up Strength Calibration Enable */
#define BITM_DMC_DDR_ZQ_CTL2_CALSTRT         (_ADI_MSK_3(0x10000000,0x10000000UL,uint32_t))    /* New Impedance Calibration Enable */
#define BITM_DMC_DDR_ZQ_CTL2_CALTYPE         (_ADI_MSK_3(0x08000000,0x08000000UL,uint32_t))    /* Calibration Type */
#define BITM_DMC_DDR_ZQ_CTL2_BYPPDDQ         (_ADI_MSK_3(0x07E00000,0x07E00000UL,uint32_t))    /* Bypass Code for DQ,DQS,CLK,DM Pull-down Driver Code */
#define BITM_DMC_DDR_ZQ_CTL2_BYPPUDQ         (_ADI_MSK_3(0x001F8000,0x001F8000UL,uint32_t))    /* Bypass Code for DQ,DQS,CLK,DM Pull-up Code */
#define BITM_DMC_DDR_ZQ_CTL2_BYPPDADD        (_ADI_MSK_3(0x00007E00,0x00007E00UL,uint32_t))    /* Address/Command Bypass Pull-down Code */
#define BITM_DMC_DDR_ZQ_CTL2_BYPPUADD        (_ADI_MSK_3(0x000001F8,0x000001F8UL,uint32_t))    /* Address/Command Bypass Pull-up Codes */
#define BITM_DMC_DDR_ZQ_CTL2_BYPIMRD         (_ADI_MSK_3(0x00000004,0x00000004UL,uint32_t))    /* Bypass Termination Code Enable */
#define BITM_DMC_DDR_ZQ_CTL2_BYPIMDQ         (_ADI_MSK_3(0x00000002,0x00000002UL,uint32_t))    /* DQ/DQS/CLK/DM Bypass Driver Code Enable */
#define BITM_DMC_DDR_ZQ_CTL2_BYPIMAD         (_ADI_MSK_3(0x00000001,0x00000001UL,uint32_t))    /* Address/Command Bypass Code Enable */

/* -------------------------------------------------------------------------------------------------------------------------
          DMC_DDR_CA_CTL                       Pos/Masks         Description
   ------------------------------------------------------------------------------------------------------------------------- */
#define BITP_DMC_DDR_CA_CTL_SLAVE_ID         26            /*  SW Completer Read/Write Completer ID */
#define BITP_DMC_DDR_CA_CTL_BYPCODE1         23            /*  Duty Cycle Level Select */
#define BITP_DMC_DDR_CA_CTL_BYPCODE0         19            /*  Duty Cycle Level Select */
#define BITP_DMC_DDR_CA_CTL_BYPSELP          18            /*  Duty Cycle Offset Direction Select */
#define BITP_DMC_DDR_CA_CTL_BYPENB           17            /*  CLK Duty Cycle Offset Adjustment Enable */
#define BITP_DMC_DDR_CA_CTL_MF_SEL           15            /*  SPI Master Frequency Divider Select */
#define BITP_DMC_DDR_CA_CTL_SW_REFRESH       14            /*  Refresh Lane DLL Code */
#define BITM_DMC_DDR_CA_CTL_SLAVE_ID         (_ADI_MSK_3(0x7C000000,0x7C000000UL,uint32_t))    /* SW Completer Read/Write Completer ID */
#define BITM_DMC_DDR_CA_CTL_BYPCODE1         (_ADI_MSK_3(0x01800000,0x01800000UL,uint32_t))    /* Duty Cycle Level Select */
#define BITM_DMC_DDR_CA_CTL_BYPCODE0         (_ADI_MSK_3(0x00180000,0x00180000UL,uint32_t))    /* Duty Cycle Level Select */
#define BITM_DMC_DDR_CA_CTL_BYPSELP          (_ADI_MSK_3(0x00040000,0x00040000UL,uint32_t))    /* Duty Cycle Offset Direction Select */
#define BITM_DMC_DDR_CA_CTL_BYPENB           (_ADI_MSK_3(0x00020000,0x00020000UL,uint32_t))    /* CLK Duty Cycle Offset Adjustment Enable */
#define BITM_DMC_DDR_CA_CTL_MF_SEL           (_ADI_MSK_3(0x00018000,0x00018000UL,uint32_t))    /* SPI Master Frequency Divider Select */
#define BITM_DMC_DDR_CA_CTL_SW_REFRESH       (_ADI_MSK_3(0x00004000,0x00004000UL,uint32_t))    /* Refresh Lane DLL Code */
#define ENUM_DMC_DDR_CA_CTL_DUTY_ADJ_1_L2    (_ADI_MSK_3(0x00000000,0x00000000UL,uint32_t))
#define ENUM_DMC_DDR_CA_CTL_DUTY_ADJ_1_L3    (_ADI_MSK_3(0x00800000,0x00800000UL,uint32_t))
#define ENUM_DMC_DDR_CA_CTL_DUTY_ADJ_1_L4    (_ADI_MSK_3(0x01000000,0x01000000UL,uint32_t))
#define ENUM_DMC_DDR_CA_CTL_DUTY_ADJ_0_L3    (_ADI_MSK_3(0x00000000,0x00000000UL,uint32_t))
#define ENUM_DMC_DDR_CA_CTL_DUTY_ADJ_0_L1    (_ADI_MSK_3(0x00080000,0x00080000UL,uint32_t))
#define ENUM_DMC_DDR_CA_CTL_DUTY_ADJ_0_L2    (_ADI_MSK_3(0x00100000,0x00100000UL,uint32_t))
#define ENUM_DMC_DDR_CA_CTL_DIRECTION_NEG    (_ADI_MSK_3(0x00000000,0x00000000UL,uint32_t))    /*  BYPSELP: Negative */
#define ENUM_DMC_DDR_CA_CTL_DIRECTION_POS    (_ADI_MSK_3(0x00040000,0x00040000UL,uint32_t))    /*  BYPSELP: Positive */
#define ENUM_DMC_DDR_CA_CTL_DISABLE          (_ADI_MSK_3(0x00000000,0x00000000UL,uint32_t))    /*  BYPENB: Disable */
#define ENUM_DMC_DDR_CA_CTL_ENABLE           (_ADI_MSK_3(0x00020000,0x00020000UL,uint32_t))    /*  BYPENB: Enable */
#define ENUM_DMC_DDR_CA_CTL_DIVBY2           (_ADI_MSK_3(0x00000000,0x00000000UL,uint32_t))    /*  MF_SEL: Divide by 2 */
#define ENUM_DMC_DDR_CA_CTL_DIVBY4           (_ADI_MSK_3(0x00008000,0x00008000UL,uint32_t))    /*  MF_SEL: Divide by 4 */
#define ENUM_DMC_DDR_CA_CTL_DIVBY8           (_ADI_MSK_3(0x00010000,0x00010000UL,uint32_t))    /*  MF_SEL: Divide by 8 */
#define ENUM_DMC_DDR_CA_CTL_DIVBY16          (_ADI_MSK_3(0x00018000,0x00018000UL,uint32_t))    /*  MF_SEL: Divide by 16 */

/* -------------------------------------------------------------------------------------------------------------------------
          DMC_DDR_SCRATCH_4                    Pos/Masks         Description
   ------------------------------------------------------------------------------------------------------------------------- */
#define BITP_DMC_DDR_SCRATCH_4_WR_LVL_CODE_L0  0            /*  Write leveling code read back from Lane0 */
#define BITM_DMC_DDR_SCRATCH_4_WR_LVL_CODE_L0 (_ADI_MSK_3(0xFFFFFFFF,0xFFFFFFFFUL,uint32_t))    /* Write leveling code read back from Lane0 */

/* -------------------------------------------------------------------------------------------------------------------------
          DMC_DDR_SCRATCH_5                    Pos/Masks         Description
   ------------------------------------------------------------------------------------------------------------------------- */
#define BITP_DMC_DDR_SCRATCH_5_WR_LVL_CODE_L1  0            /*  Write leveling code read back from Lane1 */
#define BITM_DMC_DDR_SCRATCH_5_WR_LVL_CODE_L1 (_ADI_MSK_3(0xFFFFFFFF,0xFFFFFFFFUL,uint32_t))    /* Write leveling code read back from Lane1 */

typedef enum
{
  ADI_DMC_SUCCESS=0u,
  ADI_DMC_FAILURE
}ADI_DMC_RESULT;

/* structure which holds DMC register values */
typedef struct
{

  uint32_t ulDDR_DLLCTLCFG;               /*!< Content of DDR DLLCTL and DMC_CFG register     */
  uint32_t ulDDR_EMR2EMR3;                /*!< Content of the DDR EMR2 and EMR3 Register      */
  uint32_t ulDDR_CTL;                     /*!< Content of the DDR Control                   */
  uint32_t ulDDR_MREMR1;                  /*!< Content of the DDR MR and EMR1 Register      */
  uint32_t ulDDR_TR0;                     /*!< Content of the DDR Timing Register      */
  uint32_t ulDDR_TR1;                     /*!< Content of the DDR Timing Register      */
  uint32_t ulDDR_TR2;                     /*!< Content of the DDR Timing Register      */
  uint32_t ulDDR_ZQCTL0;                  /*!< Content of ZQCTL0 register */
  uint32_t ulDDR_ZQCTL1;                  /*!< Content of ZQCTL1 register */
  uint32_t ulDDR_ZQCTL2;                  /*!< Content of ZQCTL2 register */

}ADI_DMC_CONFIG;

/* delay function */
#if defined __ADSPARM__
static __inline__ void dmcdelay(uint32_t delay)
{
  /* There is no zero-overhead loop on ARM, so assume each iteration takes
   * 4 processor cycles (based on examination of -O3 and -Ofast output).
   */
  uint32_t i;

  for(i=0; i<delay; i++) {
    __asm__("NOP");
  }
}
#elif defined __ADSPSHARC__ || defined __ADSPSHARCFX__
#pragma inline
static void dmcdelay(uint32_t delay)
{
  uint32_t i;


  /* Convert DDR cycles to core clock cycles */
  float32_t f = (float32_t)delay * cclkdclk_ratio;
  delay = (uint32_t)f;

  for(i=delay; i>0ul; i--){
//   NOP();
   execute_nop();
  }
}

#endif

#ifdef __cplusplus
extern "C" {
#endif

/* reset dmc lanes */
void adi_dmc_lane_reset(bool reset);
/* enter/exit self refresh mode */
void adi_dmc_self_refresh(bool enter);
/* program dmc controller timing registers */
ADI_DMC_RESULT adi_dmc_ctrl_init(ADI_DMC_CONFIG *pConfig);
/* trigger Zq calibration*/
void adi_dmc_phy_calibration(ADI_DMC_CONFIG *pConfig);

#ifdef __cplusplus
}
#endif

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif

//#endif /* CONFIG_DMC0 */



#endif /* ADI_DMC_H */
