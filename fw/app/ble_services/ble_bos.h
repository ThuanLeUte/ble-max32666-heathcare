/**
 * @file       ble_bos.h
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-04-04
 * @author     Thuan Le
 * @brief      BOS (BLE Blood Oxygen Service)
 * @note       None
 * @example    None
 */

/* Define to prevent recursive inclusion ------------------------------ */
#ifndef __BLE_BOS_H
#define __BLE_BOS_H

/* Includes ----------------------------------------------------------- */
#include "att_api.h"

/* Public defines ----------------------------------------------------- */
#define BOS_START_HDL   0x20               /*!< Service start handle. */
#define BOS_END_HDL     (BOS_MAX_HDL - 1)  /*!< Service end handle. */

/* Public enumerate/structure ----------------------------------------- */
/**
 * @brief Blood Oxygen Service event type
 */
enum
{
  BOS_SVC_HDL = BOS_START_HDL,         /*!< BOS service declaration */
  BOS_LVL_CH_HDL,                      /*!< BOS level characteristic */
  BOS_LVL_HDL,                         /*!< BOS level */
  BOS_LVL_CH_CCC_HDL,                  /*!< BOS level CCCD */
  BOS_MAX_HDL                          /*!< Maximum handle. */
};

/* Public macros ------------------------------------------------------ */
/* Public variables --------------------------------------------------- */
/* Public function prototypes ----------------------------------------- */
/**
 * @brief         Function for initializing the Blood Oxygen Service.
 *
 * @param[in]     None
 * 
 * @attention     None
 *
 * @return        None
 */
void ble_bos_init();

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
void ble_bos_callback_register(attsReadCback_t read_cb, attsWriteCback_t write_cb);

#endif // __BLE_BOS_H

/* End of file -------------------------------------------------------- */
