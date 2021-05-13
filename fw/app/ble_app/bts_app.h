/**
 * @file       bts_app.h
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-05-11
 * @author     Thuan Le
 * @brief      Body Temperature Service application
 * @note       None
 * @example    None
 */

/* Define to prevent recursive inclusion ------------------------------ */
#ifndef __BTS_APP_H
#define __BTS_APP_H

#include "wsf_timer.h"
#include "att_api.h"

/* Includes ----------------------------------------------------------- */
#include "wsf_os.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Public defines ----------------------------------------------------- */
/* Public enumerate/structure ----------------------------------------- */
// Body Temperature service configurable parameters
typedef struct
{
  wsfTimerTicks_t period;     // Body Temperature measurement timer expiration period in seconds
}
bts_app_cfg_t;

/* Public macros ------------------------------------------------------ */
/* Public variables --------------------------------------------------- */
/* Public function prototypes ----------------------------------------- */
/**
 * @brief         Initialize the Body temperature service application
 *
 * @param[in]     handler_id  WSF handler ID for App
 * @param[in]     p_cfg       Body Temperature service configurable parameters
 *
 * @attention     None
 *
 * @return        None
 */

void bts_app_init(wsfHandlerId_t handler_id, bts_app_cfg_t *p_cfg);

/**
 * @brief         Start periodic Body temperature measurement.  This function starts a timer to perform
 *                periodic Body temperature measurements.
 *
 * @param[in]     conn_id       DM connection identifier
 * @param[in]     timer_evt     WSF event designated by the application for the timer
 * @param[in]     temp_ccc_idx  Index of Body temperature CCC descriptor in CCC descriptor handle table
 *
 * @attention     None
 *
 * @return        None
 */
void bts_app_measure_start(dmConnId_t conn_id, uint8_t timer_evt, uint8_t temp_ccc_idx);

/**
 * @brief         Stop periodic Body temperature measurement
 *
 * @param[in]     conn_id      DM connection identifier
 *
 * @attention     None
 *
 * @return        None
 */
void bts_app_measure_stop(dmConnId_t conn_id);

/**
 * @brief         Process received WSF message.
 *
 * @param[in]     p_msg     Event message.
 *
 * @attention     None
 *
 * @return        None
 */
void bts_app_process_msg(wsfMsgHdr_t *p_msg);

/**
 * @brief         ATTS read callback for Body temperature service used to read the Body temperature. Use this
 *                function as a parameter to SvcBattCbackRegister().
 *
 * @param[in]     conn_id    DM connection identifier.
 * @param[in]     handle     ATT handle.
 * @param[in]     operation  ATT operation.
 * @param[in]     offset     read offset.
 * @param[in]     p_attr     pointer to Attribute
 * 
 * @attention     None
 *
 * @return        None
 */
uint8_t bts_app_read_cb(dmConnId_t conn_id, uint16_t handle, uint8_t operation,
                        uint16_t offset, attsAttr_t *p_attr);

#endif // __BTS_APP_H

#ifdef __cplusplus
};
#endif

/* End of file -------------------------------------------------------- */
