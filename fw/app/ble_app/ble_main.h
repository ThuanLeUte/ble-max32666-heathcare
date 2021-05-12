/**
 * @file       ble_main.h
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-05-11
 * @author     Thuan Le
 * @brief      BLE application interface
 * @note       None
 * @example    None
 */

/* Define to prevent recursive inclusion ------------------------------ */
#ifndef __BLE_MAIN_H
#define __BLE_MAIN_H

/* Includes ----------------------------------------------------------- */
#include "wsf_os.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Public defines ----------------------------------------------------- */
#ifndef FIT_CONN_MAX
#define FIT_CONN_MAX                  1
#endif

/* Public enumerate/structure ----------------------------------------- */
/* Public macros ------------------------------------------------------ */
/* Public variables --------------------------------------------------- */
/* Public function prototypes ----------------------------------------- */
/**
 * @brief  Start the application
 *
 * @param[in]     None
 *
 * @attention     None
 *
 * @return        None
 */
void ble_start(void);

/**
 * @brief  Application handler init function called during system initialization.
 *
 * @param[in]     handler_id  WSF handler ID for App.
 *
 * @attention     None
 *
 * @return        None
 */
void ble_handler_init(wsfHandlerId_t handler_id);

/**
 * @brief  WSF event handler for the application.
 *
 * @param[in]     event     WSF event mask
 * @param[in]     p_msg     WSF message
 *
 * @attention     None
 *
 * @return        None
 */
void ble_handler(wsfEventMask_t event, wsfMsgHdr_t *p_msg);

#endif // __BLE_MAIN_H

#ifdef __cplusplus
};
#endif

/* End of file -------------------------------------------------------- */