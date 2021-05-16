/**
 * @file       bts_app.c
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-05-11
 * @author     Thuan Le
 * @brief      Body Temperature Service application
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

#include "ble_bts.h"
#include "bts_app.h"
#include "stdio.h"
#include "bsp_temp.h"

/* Private defines ---------------------------------------------------- */
// Body Temperature value initialization value
#define BTS_TEMP_LEVEL_INIT           (0xFF)

/* Private macros ----------------------------------------------------- */
/* Private enumerate/structure ---------------------------------------- */
// Connection control block
typedef struct
{
  dmConnId_t      conn_id;          // Connection ID
  bool_t          temp_to_send;     // Body Temperature measurement ready to be sent on this channel
  float           sent_temp_value;  // Value of last sent temperature value
}
bts_app_conn_t;

// Control block
static struct
{
  bts_app_conn_t  conn[DM_CONN_MAX];  // Connection control block
  wsfTimer_t      meas_timer;         // Periodic measurement timer
  bts_app_cfg_t   cfg;                // Configurable parameters
  uint16_t        curr_count;         // Current measurement period count
  bool_t          tx_ready;           // True if ready to send notifications
  float           temp_value;         // Value of last measured temperature value
}
bts_cb;

/* Public variables --------------------------------------------------- */
/* Private variables -------------------------------------------------- */
/* Private function prototypes ---------------------------------------- */
static void m_bts_meas_time_exp(wsfMsgHdr_t *p_msg);
static void m_bts_setup_to_send(void);
static void m_bts_send_periodic_temp_value(bts_app_conn_t *p_conn);
static void m_bts_conn_open(dmEvt_t *p_msg);
static void m_bts_handle_value_confirm(attEvt_t *p_msg);
static void bts_app_send_temp_value(dmConnId_t conn_id, uint8_t idx, float value);
static bool_t m_bts_no_conn_active(void);
static bts_app_conn_t *m_bts_find_next_to_send(uint8_t ccc_idx);

/* Function definitions ----------------------------------------------- */
void bts_app_init(wsfHandlerId_t handler_id, bts_app_cfg_t *p_cfg)
{
  bts_cb.meas_timer.handlerId = handler_id;
  bts_cb.cfg = *p_cfg;
}

void bts_app_measure_start(dmConnId_t conn_id, uint8_t timer_evt, uint8_t temp_ccc_idx)
{
  // If this is first connection
  if (m_bts_no_conn_active())
  {
    // Initialize control block
    bts_cb.meas_timer.msg.event  = timer_evt;
    bts_cb.meas_timer.msg.status = temp_ccc_idx;
    bts_cb.temp_value            = BTS_TEMP_LEVEL_INIT;

    // Start timer
    WsfTimerStartSec(&bts_cb.meas_timer, bts_cb.cfg.period);
  }

  // Set conn id and last sent temperature value
  bts_cb.conn[conn_id - 1].conn_id = conn_id;
  bts_cb.conn[conn_id - 1].sent_temp_value = BTS_TEMP_LEVEL_INIT;
}

void bts_app_measure_stop(dmConnId_t conn_id)
{
  // Clear connection
  bts_cb.conn[conn_id - 1].conn_id      = DM_CONN_ID_NONE;
  bts_cb.conn[conn_id - 1].temp_to_send = FALSE;

  // If no remaining connections
  if (m_bts_no_conn_active())
  {
    // Stop timer
    WsfTimerStop(&bts_cb.meas_timer);
  }
}

void bts_app_process_msg(wsfMsgHdr_t *p_msg)
{
  if (p_msg->event == DM_CONN_OPEN_IND)
  {
    m_bts_conn_open((dmEvt_t *) p_msg);
  }
  else if (p_msg->event == ATTS_HANDLE_VALUE_CNF)
  {
    m_bts_handle_value_confirm((attEvt_t *) p_msg);
  }
  else if (p_msg->event == bts_cb.meas_timer.msg.event)
  {
    printf("m_bts_meas_time_exp\n");
    m_bts_meas_time_exp(p_msg);
  }
}

uint8_t bts_app_read_cb(dmConnId_t conn_id, uint16_t handle, uint8_t operation,
                        uint16_t offset, attsAttr_t *p_attr)
{
  // Read the temperature value and set attribute value
  bsp_temp_get((float *)&p_attr->pValue);

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
static void m_bts_meas_time_exp(wsfMsgHdr_t *p_msg)
{
  bts_app_conn_t  *p_conn;

  // If there are active connections
  if (m_bts_no_conn_active() == FALSE)
  {
    printf("m_bts_no_conn_active\n");

    // Set up temperature measurement to be sent on all connections
    m_bts_setup_to_send();

    // Read temperature measurement sensor data
    bsp_temp_get(&bts_cb.temp_value);

    printf("Temmperature: %f \n", bts_cb.temp_value);

    // If ready to send measurements
    if (bts_cb.tx_ready)
    {
      // Find next connection to send (note ccc idx is stored in timer status)
      if ((p_conn = m_bts_find_next_to_send(p_msg->status)) != NULL)
      {
        printf("m_bts_find_next_to_send\n");

        m_bts_send_periodic_temp_value(p_conn);
      }
    }
  }

  // Restart timer
  WsfTimerStartSec(&bts_cb.meas_timer, bts_cb.cfg.period);
}

/**
 * @brief         Send the temperature value to the peer device.
 *
 * @param[in]     conn_id     DM connection identifier.
 * @param[in]     idx         Index of temperature value CCC descriptor in CCC descriptor handle table.
 * @param[in]     value       The temperature value.
 *
 * @attention     None
 *
 * @return        None
 */
static void bts_app_send_temp_value(dmConnId_t conn_id, uint8_t idx, float value)
{
  printf("bts_app_send_temp_value\n", conn_id);

  if (AttsCccEnabled(conn_id, idx))
  {
    printf("conn_id: %d\n", conn_id);
    AttsHandleValueNtf(conn_id, BTS_VALUE_HDL, 4,(uint8_t *) &value);
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
static bool_t m_bts_no_conn_active(void)
{
  bts_app_conn_t *p_conn = bts_cb.conn;
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
static void m_bts_setup_to_send(void)
{
  bts_app_conn_t *p_conn = bts_cb.conn;
  uint8_t i;

  for (i = 0; i < DM_CONN_MAX; i++, p_conn++)
  {
    if (p_conn->conn_id != DM_CONN_ID_NONE)
    {
      p_conn->temp_to_send = TRUE;
    }
  }
}

/**
 * @brief         Find next connection with measurement to send.
 *
 * @param[in]     ccc_idx  Body Temperature measurement CCC descriptor index.
 *
 * @attention     None
 *
 * @return        Connection control block
 */
static bts_app_conn_t *m_bts_find_next_to_send(uint8_t ccc_idx)
{
  bts_app_conn_t *p_conn = bts_cb.conn;
  uint8_t i;

  for (i = 0; i < DM_CONN_MAX; i++, p_conn++)
  {
    if (p_conn->conn_id != DM_CONN_ID_NONE && p_conn->temp_to_send)
        // p_conn->sent_temp_value != bts_cb.temp_value)
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
 * @brief         Send periodic temperature measurement.
 *
 * @param[in]     p_conn   Connection control block.
 *
 * @attention     None
 *
 * @return        None
 */
static void m_bts_send_periodic_temp_value(bts_app_conn_t *p_conn)
{
  bts_app_send_temp_value(p_conn->conn_id, bts_cb.meas_timer.msg.status, bts_cb.temp_value);

  // Set value to default
  p_conn->sent_temp_value = bts_cb.temp_value;
  p_conn->temp_to_send    = FALSE;
  bts_cb.tx_ready         = FALSE;
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
static void m_bts_conn_open(dmEvt_t *p_msg)
{
  bts_cb.tx_ready = TRUE;
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
static void m_bts_handle_value_confirm(attEvt_t *p_msg)
{
  bts_app_conn_t  *p_conn;

  if (p_msg->hdr.status == ATT_SUCCESS && p_msg->handle == BTS_VALUE_HDL)
  {
    bts_cb.tx_ready = TRUE;

    // Find next connection to send (note ccc idx is stored in timer status)
    if ((p_conn = m_bts_find_next_to_send(bts_cb.meas_timer.msg.status)) != NULL)
    {
      m_bts_send_periodic_temp_value(p_conn);
    }
  }
}

/* End of file -------------------------------------------------------- */

