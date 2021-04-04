/**
 * @file       ble_bas.h
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-01-07
 * @author     Thuan Le
 * @brief      BAS (BLE Battery Service)
 * @note       None
 * @example    None
 */

/* Define to prevent recursive inclusion ------------------------------ */
#ifndef __BLE_BAS_H
#define __BLE_BAS_H

/* Includes ----------------------------------------------------------- */
#include "att_api.h"

/* Public defines ----------------------------------------------------- */
#define BAS_START_HDL   0x60               /*!< Service start handle. */
#define BAS_END_HDL     (BAS_MAX_HDL - 1)  /*!< Service end handle. */

/* Public enumerate/structure ----------------------------------------- */
/**
 * @brief Battery Service event type
 */
enum
{
  BAS_SVC_HDL = BAS_START_HDL,         /*!< BAS service declaration */
  BAS_LVL_CH_HDL,                      /*!< BAS level characteristic */
  BAS_LVL_HDL,                         /*!< BAS level */
  BAS_LVL_CH_CCC_HDL,                  /*!< BAS level CCCD */
  BAS_MAX_HDL                          /*!< Maximum handle. */
};

/* Public macros ------------------------------------------------------ */
/* Public variables --------------------------------------------------- */
/* Public function prototypes ----------------------------------------- */
/**
 * @brief         Function for initializing the Battery Service.
 *
 * @param[in]     None
 * 
 * @attention     None
 *
 * @return        None
 */
void ble_bas_init();

/**
 * @brief         Register callbacks for the service
 *
 * @param[in]     read_cb    Read callback function
 * @param[in]     write_cb   Write callback function
 * 
 * @attention     None
 *
 * @return        None
 */
void ble_bas_callback_register(attsReadCback_t read_cb, attsWriteCback_t write_cb);

/**
 * @brief         Function for updating the battery level.
 *
 * @param[in]     value   New battery measurement value
 * 
 * @attention     None
 *
 * @return        None
 */
void ble_bas_battery_level_update(uint8_t value);

#endif // __BLE_BAS_H

/* End of file -------------------------------------------------------- */
