/**
 * @file       ble_bts.h
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-04-04
 * @author     Thuan Le
 * @brief      BTS (BLE Body Temperature Service)
 * @note       None
 * @example    None
 */

/* Define to prevent recursive inclusion ------------------------------ */
#ifndef __BLE_BTS_H
#define __BLE_BTS_H

/* Includes ----------------------------------------------------------- */
#include "att_api.h"

/* Public defines ----------------------------------------------------- */
#define BTS_START_HDL   0x20                // Service start handle
#define BTS_END_HDL     (BTS_MAX_HDL - 1)   // Service end handle

/* Public enumerate/structure ----------------------------------------- */
/**
 * @brief Body temperature service event type
 */
enum
{
  BTS_SVC_HDL = BTS_START_HDL,           // BTS service declaration
  BTS_VALUE_CH_HDL,                      // BTS value characteristic
  BTS_VALUE_HDL,                         // BTS value
  BTS_VALUE_CH_CCC_HDL,                  // BTS value CCCD
  BTS_MAX_HDL                            // Maximum handle.
};

/* Public macros ------------------------------------------------------ */
/* Public variables --------------------------------------------------- */
/* Public function prototypes ----------------------------------------- */
/**
 * @brief         Function for initializing the Body temperature service.
 *
 * @param[in]     None
 * 
 * @attention     None
 *
 * @return        None
 */
void ble_bts_init();

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
void ble_bts_callback_register(attsReadCback_t read_cb, attsWriteCback_t write_cb);

#endif // __BLE_BTS_H

/* End of file -------------------------------------------------------- */
