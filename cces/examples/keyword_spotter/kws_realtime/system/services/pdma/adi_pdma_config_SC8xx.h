/******************************************************************************
 * @file:    adi_pdma_config_SC8xx.h
 * @brief:   PDMA descriptor configuration file.
 *****************************************************************************

 Copyright(c) 2023 Analog Devices, Inc. All Rights Reserved.

 This software is proprietary.  By using this software you agree
 to the terms of the associated Analog Devices License Agreement.

*********************************************************************************/

/*!
* @file      adi_pdma_config_SC8xx.h
*
* @brief     PDMA Config file
*
* @details   This is the file which has macros related to PDMA configuration.
*/

/** @addtogroup PDMA_Service PDMA Service
 *  @{
 */
 
/** @addtogroup PDMA_Service_Static_Configuration PDMA Service Static Configuration
 *  @{
 */

#ifndef __ADI_PDMA_CONFIG_SC8XX_H__
#define __ADI_PDMA_CONFIG_SC8XX_H__

/*! Below mentioned are the type of ADI_PDMA_MODE based on which ADI_PDMA_LISTDESCRIPTOR can be enabled/disabled.\n
    ADI_PDMA_DESCRIPTOR_LIST                      Enable the macro ADI_PDMA_LISTDESCRIPTOR\n
    ADI_PDMA_DESCRIPTOR_LIST_ONDEMAND             Enable the macro ADI_PDMA_LISTDESCRIPTOR\n
    ADI_PDMA_DESCRIPTOR_ARRAY                     Disable the macro ADI_PDMA_LISTDESCRIPTOR\n
    ADI_PDMA_DESCRIPTOR_ARRAY_ONDEMAND            Disable the macro ADI_PDMA_LISTDESCRIPTOR\n
*/
#define ADI_PDMA_LISTDESCRIPTOR

#if defined(__ADSPSHARCFX__)
#define ADI_PDMA_CACHE_MANGEMENT   1u
#endif

#endif /*__ADI_PDMA_CONFIG_SC8XX_H__*/

/*@}*/
/*@}*/
