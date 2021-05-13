/**
 * @file       bas_app.c
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-05-11
 * @author     Thuan Le
 * @brief      Battery service application
 * @note       None
 * @example    None
 */

/* Includes ----------------------------------------------------------- */
#include <string.h>
#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "util/bstream.h"
#include "att_api.h"
#include "svc_ch.h"
#include "app_api.h"
#include "app_hw.h"

#include "ble_bas.h"
#include "bas_app.h"
#include "stdio.h"

/* Private defines ---------------------------------------------------- */
// Battery level initialization value
#define BAS_BATT_LEVEL_INIT           (0xFF)

/* Private macros ----------------------------------------------------- */
/* Private enumerate/structure ---------------------------------------- */
// Connection control block
typedef struct
{
  dmConnId_t      conn_id;          // Connection ID
  bool_t          batt_to_send;     // Battery measurement ready to be sent on this channel
  uint8_t         sent_batt_level;  // Value of last sent battery level
}
bas_app_conn_t;

// Control block
static struct
{
  bas_app_conn_t  conn[DM_CONN_MAX];  // Connection control block
  wsfTimer_t      meas_timer;         // Periodic measurement timer
  bas_app_cfg_t   cfg;                // Configurable parameters
  uint16_t        curr_count;         // Current measurement period count
  bool_t          tx_ready;           // True if ready to send notifications
  uint8_t         batt_level;         // Value of last measured battery level
}
bas_cb;

/* Public variables --------------------------------------------------- */
/* Private variables -------------------------------------------------- */
/* Private function prototypes ---------------------------------------- */
static void m_bas_meas_time_exp(wsfMsgHdr_t *p_msg);
static void m_bas_setup_to_send(void);
static void m_bat_send_periodic_batt_level(bas_app_conn_t *p_conn);
static void m_bas_conn_open(dmEvt_t *p_msg);
static void m_bas_handle_value_confirm(attEvt_t *p_msg);
static void bas_app_send_batt_level(dmConnId_t conn_id, uint8_t idx, uint8_t level);
static bool_t m_bas_no_conn_active(void);
static bas_app_conn_t *m_bas_find_next_to_send(uint8_t ccc_idx);

/* Function definitions ----------------------------------------------- */
void bas_app_init(wsfHandlerId_t handler_id, bas_app_cfg_t *p_cfg)
{
  bas_cb.meas_timer.handlerId = handler_id;
  bas_cb.cfg = *p_cfg;
}

void bas_app_measure_start(dmConnId_t conn_id, uint8_t timer_evt, uint8_t batt_ccc_idx)
{
  // If this is first connection
  if (m_bas_no_conn_active())
  {
    // Initialize control block
    bas_cb.meas_timer.msg.event  = timer_evt;
    bas_cb.meas_timer.msg.status = batt_ccc_idx;
    bas_cb.batt_level            = BAS_BATT_LEVEL_INIT;

    // Start timer
    WsfTimerStartSec(&bas_cb.meas_timer, bas_cb.cfg.period);
  }

  // Set conn id and last sent battery level
  bas_cb.conn[conn_id - 1].conn_id = conn_id;
  bas_cb.conn[conn_id - 1].sent_batt_level = BAS_BATT_LEVEL_INIT;
}

void bas_app_measure_stop(dmConnId_t conn_id)
{
  // Clear connection
  bas_cb.conn[conn_id - 1].conn_id      = DM_CONN_ID_NONE;
  bas_cb.conn[conn_id - 1].batt_to_send = FALSE;

  // If no remaining connections
  if (m_bas_no_conn_active())
  {
    // Stop timer
    WsfTimerStop(&bas_cb.meas_timer);
  }
}

void bas_app_process_msg(wsfMsgHdr_t *p_msg)
{
  if (p_msg->event == DM_CONN_OPEN_IND)
  {
    m_bas_conn_open((dmEvt_t *) p_msg);
  }
  else if (p_msg->event == ATTS_HANDLE_VALUE_CNF)
  {
    m_bas_handle_value_confirm((attEvt_t *) p_msg);
  }
  else if (p_msg->event == bas_cb.meas_timer.msg.event)
  {
    printf("m_bas_meas_time_exp\n");
    m_bas_meas_time_exp(p_msg);
  }
}

uint8_t bas_app_read_cb(dmConnId_t conn_id, uint16_t handle, uint8_t operation,
                        uint16_t offset, attsAttr_t *p_attr)
{
  // Read the battery level and set attribute value
  AppHwBattRead(p_attr->pValue);

  return ATT_SUCCESS;
}

/* Private function definitions --------------------------------------- */
/**
 * @brief         This function is called by the application when the periodic measurement
 *                timer expires.
 *
 * @param[in]     p_msg     Event message.
 *
 * @attention     None
 *
 * @return        None
 */
static void m_bas_meas_time_exp(wsfMsgHdr_t *p_msg)
{
  bas_app_conn_t  *p_conn;

  // If there are active connections
  if (m_bas_no_conn_active() == FALSE)
  {
    printf("m_bas_no_conn_active\n");

    // Set up battery measurement to be sent on all connections
    m_bas_setup_to_send();

    // Read battery measurement sensor data
    AppHwBattRead(&bas_cb.batt_level);

    // If ready to send measurements
    if (bas_cb.tx_ready)
    {
      // Find next connection to send (note ccc idx is stored in timer status)
      if ((p_conn = m_bas_find_next_to_send(p_msg->status)) != NULL)
      {
        printf("m_bas_find_next_to_send\n");

        m_bat_send_periodic_batt_level(p_conn);
      }
    }
  }

  // Restart timer
  WsfTimerStartSec(&bas_cb.meas_timer, bas_cb.cfg.period);
}

/**
 * @brief         Send the battery level to the peer device.
 *
 * @param[in]     conn_id     DM connection identifier.
 * @param[in]     idx         Index of battery level CCC descriptor in CCC descriptor handle table.
 * @param[in]     level       The battery level.
 *
 * @attention     None
 *
 * @return        None
 */
static void bas_app_send_batt_level(dmConnId_t conn_id, uint8_t idx, uint8_t level)
{
  printf("bas_app_send_batt_level\n", conn_id);

  if (AttsCccEnabled(conn_id, idx))
  {
    printf("conn_id: %d\n", conn_id);
    AttsHandleValueNtf(conn_id, BAS_LVL_HDL, CH_BATT_LEVEL_LEN, &level);
  }
}

/**
 * @brief         Return TRUE if no connections with active measurements
 *
 * @param[in]     None
 *
 * @attention     None
 *
 * @return        TRUE if no connections active
 */
static bool_t m_bas_no_conn_active(void)
{
  bas_app_conn_t *p_conn = bas_cb.conn;
  uint8_t i;

  for (i = 0; i < DM_CONN_MAX; i++, p_conn++)
  {
    if (p_conn->conn_id != DM_CONN_ID_NONE)
    {
      return FALSE;
    }
  }
  return TRUE;
}

/**
 * @brief          Setup to send measurements on active connections.
 *
 * @param[in]     None
 *
 * @attention     None
 *
 * @return        None
 */
static void m_bas_setup_to_send(void)
{
  bas_app_conn_t *p_conn = bas_cb.conn;
  uint8_t i;

  for (i = 0; i < DM_CONN_MAX; i++, p_conn++)
  {
    if (p_conn->conn_id != DM_CONN_ID_NONE)
    {
      p_conn->batt_to_send = TRUE;
    }
  }
}

/**
 * @brief         Find next connection with measurement to send.
 *
 * @param[in]     ccc_idx  Battery measurement CCC descriptor index.
 *
 * @attention     None
 *
 * @return        Connection control block
 */
static bas_app_conn_t *m_bas_find_next_to_send(uint8_t ccc_idx)
{
  bas_app_conn_t *p_conn = bas_cb.conn;
  uint8_t i;

  for (i = 0; i < DM_CONN_MAX; i++, p_conn++)
  {
    if (p_conn->conn_id != DM_CONN_ID_NONE && p_conn->batt_to_send)
        // p_conn->sent_batt_level != bas_cb.batt_level)
    {
      if (AttsCccEnabled(p_conn->conn_id, ccc_idx))
      {
        return p_conn;
      }
    }
  }
  return NULL;
}

/**
 * @brief         Send periodic battery measurement.
 *
 * @param[in]     p_conn   Connection control block.
 *
 * @attention     None
 *
 * @return        None
 */
static void m_bat_send_periodic_batt_level(bas_app_conn_t *p_conn)
{
  bas_app_send_batt_level(p_conn->conn_id, bas_cb.meas_timer.msg.status, bas_cb.batt_level);

  // Set value to default
  p_conn->sent_batt_level = bas_cb.batt_level;
  p_conn->batt_to_send    = FALSE;
  bas_cb.tx_ready         = FALSE;
}

/**
 * @brief         Handle connection open.
 *
 * @param[in]     p_msg     Event message.
 *
 * @attention     None
 *
 * @return        None
 */
static void m_bas_conn_open(dmEvt_t *p_msg)
{
  bas_cb.tx_ready = TRUE;
}

/**
 * @brief         Handle a received ATT handle value confirm
 *
 * @param[in]     p_msg     Event message.
 *
 * @attention     None
 *
 * @return        None
 */
static void m_bas_handle_value_confirm(attEvt_t *p_msg)
{
  bas_app_conn_t  *p_conn;

  if (p_msg->hdr.status == ATT_SUCCESS && p_msg->handle == BAS_LVL_HDL)
  {
    bas_cb.tx_ready = TRUE;

    // Find next connection to send (note ccc idx is stored in timer status)
    if ((p_conn = m_bas_find_next_to_send(bas_cb.meas_timer.msg.status)) != NULL)
    {
      m_bat_send_periodic_batt_level(p_conn);
    }
  }
}

/* End of file -------------------------------------------------------- */

