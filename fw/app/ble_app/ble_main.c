/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  BLEness sample application for the following profiles:
 *            Heart Rate profile
 *
 *  Copyright (c) 2011-2018 Arm Ltd. All Rights Reserved.
 *  ARM Ltd. confidential and proprietary.
 *
 *  IMPORTANT.  Your use of this file is governed by a Software License Agreement
 *  ("Agreement") that must be accepted in order to download or otherwise receive a
 *  copy of this file.  You may not use or copy this file for any purpose other than
 *  as described in the Agreement.  If you do not agree to all of the terms of the
 *  Agreement do not use this file and delete all copies in your possession or control;
 *  if you do not have a copy of the Agreement, you must contact ARM Ltd. prior
 *  to any use, copying or further distribution of this software.
 */
/*************************************************************************************************/

#include <string.h>
#include "wsf_types.h"
#include "util/bstream.h"
#include "wsf_msg.h"
#include "wsf_trace.h"
#include "wsf_buf.h"
#include "hci_api.h"
#include "dm_api.h"
#include "att_api.h"
#include "smp_api.h"
#include "app_api.h"
#include "app_db.h"
#include "app_ui.h"
#include "app_hw.h"
#include "svc_ch.h"
#include "svc_core.h"
#include "svc_hrs.h"
#include "svc_dis.h"
#include "svc_batt.h"
#include "svc_rscs.h"
#include "bas/bas_api.h"
#include "hrps/hrps_api.h"
#include "rscp/rscp_api.h"
#include "util/calc128.h"

#include "ble_main.h"
#include "ble_bas.h"
#include "ble_bos.h"
#include "bas_app.h"
#include "stdio.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

// WSF message event starting value
#define BLE_MSG_START               0xA0

// WSF message event enumeration
enum
{
  BLE_BATT_TIMER_IND = BLE_MSG_START,   // Battery measurement timer expired
  BLE_TEMPERARUE_TIMER_IND,             // Temperature measurement timer expired
  BLE_SENSOR_HUB_TIMER_IND              // Sensor Hub measurement timer expired
};

/**************************************************************************************************
  Data Types
**************************************************************************************************/

// Application message type
typedef union
{
  wsfMsgHdr_t     hdr;
  dmEvt_t         dm;
  attsCccEvt_t    ccc;
  attEvt_t        att;
}
ble_msg_t;

/**************************************************************************************************
  Configurable Parameters
**************************************************************************************************/

// Configurable parameters for advertising
// These intervals directly impact energy usage during the non-connected/advertising mode
static const appAdvCfg_t m_ble_adv_cfg =
{
  { 1000,     0,     0},                  // Advertising durations in ms
  {  200,   200,     0}                   // Advertising intervals in 0.625 ms units
};

// Configurable parameters for slave
static const appSlaveCfg_t m_ble_slave_cfg =
{
  1,                           // Maximum connections
};

// Configurable parameters for security
static const appSecCfg_t m_ble_sec_cfg =
{
  DM_AUTH_BOND_FLAG | DM_AUTH_SC_FLAG,   // Authentication and bonding flags
  0,                                     // Initiator key distribution flags
  DM_KEY_DIST_LTK,                       // Responder key distribution flags
  FALSE,                                 // TRUE if Out-of-band pairing data is present
  FALSE                                  // TRUE to initiate security upon connection
};

// TRUE if Out-of-band pairing data is to be sent
static const bool_t m_ble_send_oob_data = FALSE;

// Configurable parameters for connection parameter update
static const appUpdateCfg_t m_ble_update_cfg =
{
  6000,                        // Connection idle period in ms before attempting
                               // connection parameter update; set to zero to disable
  640,                         // Minimum connection interval in 1.25ms units
  800,                         // Maximum connection interval in 1.25ms units
  0,                           // Connection latency
  900,                         // Supervision timeout in 10ms units
  5                            // Number of update attempts before giving up
};

// Battery measurement configuration
static const bas_app_cfg_t m_ble_bas_cfg =
{
  3,                          // Battery measurement timer expiration period in seconds
};

// SMP security parameter configuration
static const smpCfg_t m_ble_smp_cfg =
{
  3000,                        // 'Repeated attempts' timeout in msec
  SMP_IO_NO_IN_NO_OUT,         // I/O Capability
  7,                           // Minimum encryption key length
  16,                          // Maximum encryption key length
  3,                           // Attempts to trigger 'repeated attempts' timeout
  0,                           // Device authentication requirements
};

/**************************************************************************************************
  Advertising Data
**************************************************************************************************/

//  Advertising data, discoverable mode
static const uint8_t m_ble_adv_data_disc[] =
{
  /*! flags */
  2,                                      /*! length */
  DM_ADV_TYPE_FLAGS,                      /*! AD type */
  DM_FLAG_LE_GENERAL_DISC |               /*! flags */
  DM_FLAG_LE_BREDR_NOT_SUP,

  /*! tx power */
  2,                                      /*! length */
  DM_ADV_TYPE_TX_POWER,                   /*! AD type */
  0,                                      /*! tx power */

  /*! service UUID list */
  9,                                      /*! length */
  DM_ADV_TYPE_16_UUID,                    /*! AD type */
  UINT16_TO_BYTES(ATT_UUID_HEART_RATE_SERVICE),
  UINT16_TO_BYTES(ATT_UUID_RUNNING_SPEED_SERVICE),
  UINT16_TO_BYTES(ATT_UUID_DEVICE_INFO_SERVICE),
  UINT16_TO_BYTES(ATT_UUID_BATTERY_SERVICE)
};

// Scan data, discoverable mode
static const uint8_t m_ble_scan_data_disc[] =
{
  // Device name
  4,                                      /*! length */
  DM_ADV_TYPE_LOCAL_NAME,                 /*! AD type */
  'F',
  'i',
  't'
};

/**************************************************************************************************
  Client Characteristic Configuration Descriptors
**************************************************************************************************/

/*! enumeration of client characteristic configuration descriptors */
enum
{
  BLE_GATT_SC_CCC_IDX,      // GATT service, service changed characteristic
  BLE_BATT_LVL_CCC_IDX,     // Battery service, battery level characteristic
  BLE_TEMP_CCC_IDX,         // Temperature service, temperature monitor characteristic
  BLE_SENSOR_HUB_CCC_IDX,   // Sensor hub service, spo2 monitor characteristic
  BLE_NUM_CCC_IDX
};

// Client characteristic configuration descriptors settings, indexed by above enumeration
static const attsCccSet_t m_ble_ccc_set[BLE_NUM_CCC_IDX] =
{
  /* cccd handle          value range               security level */
  {GATT_SC_CH_CCC_HDL,    ATT_CLIENT_CFG_INDICATE,  DM_SEC_LEVEL_NONE},   // BLE_GATT_SC_CCC_IDX
  {BATT_LVL_CH_CCC_HDL,   ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE},   // BLE_BATT_LVL_CCC_IDX
  {HRS_HRM_CH_CCC_HDL,    ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE},   // BLE_TEMP_CCC_IDX
  {RSCS_RSM_CH_CCC_HDL,   ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE}    // BLE_SENSOR_HUB_CCC_IDX
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

//  WSF handler ID
static wsfHandlerId_t m_ble_handler_id;

// LESC OOB configuration
static dmSecLescOobCfg_t *BLE_oob_cfg;

/* Private function prototypes ---------------------------------------- */
static void m_ble_dm_cb(dmEvt_t *p_dm_evt);
static void m_ble_att_cb(attEvt_t *p_evt);
static void m_ble_ccc_cb(attsCccEvt_t *p_evt);
static void m_ble_close(ble_msg_t *p_msg);
static void m_ble_setup(ble_msg_t *p_msg);
static void m_ble_process_ccc_state(ble_msg_t *p_msg);
static void m_ble_process_msg(ble_msg_t *p_msg);

/* Function definitions ----------------------------------------------- */
void ble_handler_init(wsfHandlerId_t handler_id)
{
  printf("ble_handler_init \n");

  // Store handler ID
  m_ble_handler_id = handler_id;

  // Set configuration pointers
  pAppAdvCfg    = (appAdvCfg_t *) &m_ble_adv_cfg;
  pAppSlaveCfg  = (appSlaveCfg_t *) &m_ble_slave_cfg;
  pAppSecCfg    = (appSecCfg_t *) &m_ble_sec_cfg;
  pAppUpdateCfg = (appUpdateCfg_t *) &m_ble_update_cfg;

  // Initialize application framework
  AppSlaveInit();

  // Set stack configuration pointers
  pSmpCfg = (smpCfg_t *) &m_ble_smp_cfg;

  // TODO: Add ble init

  // Initialize battery service application
  bas_app_init(handler_id, (bas_app_cfg_t *) &m_ble_bas_cfg);
}

void ble_handler(wsfEventMask_t event, wsfMsgHdr_t *p_msg)
{
  if (p_msg != NULL)
  {
    printf("BLE got evt %d \n", p_msg->event);

    if (p_msg->event >= DM_CBACK_START && p_msg->event <= DM_CBACK_END)
    {
      // Process advertising and connection-related messages
      AppSlaveProcDmMsg((dmEvt_t *) p_msg);

      // Process security-related messages
      AppSlaveSecProcDmMsg((dmEvt_t *) p_msg);
    }

    // Perform profile and user interface-related operations
    m_ble_process_msg((ble_msg_t *)p_msg);
  }
}

void ble_start(void)
{
  // Register for stack callbacks
  DmRegister(m_ble_dm_cb);
  DmConnRegister(DM_CLIENT_ID_APP, m_ble_dm_cb);
  AttRegister(m_ble_att_cb);
  AttConnRegister(AppServerConnCback);
  AttsCccRegister(BLE_NUM_CCC_IDX, (attsCccSet_t *) m_ble_ccc_set, m_ble_ccc_cb);

  // Initialize attribute server database
  SvcCoreAddGroup();

  // User service add
  ble_bos_init();
  // SvcHrsAddGroup();
  // ble_bas_callback_register(bas_app_read_cb, NULL);
  // ble_bas_init();
  SvcBattAddGroup();

  // Reset the device
  DmDevReset();
}

/* Private function definitions --------------------------------------- */
/**
 * @brief         Application DM callback
 *
 * @param[in]     p_dm_evt  DM callback event
 *
 * @attention     None
 *
 * @return        None
 */
static void m_ble_dm_cb(dmEvt_t *p_dm_evt)
{
  dmEvt_t *p_msg;
  uint16_t len;

  if (p_dm_evt->hdr.event == DM_SEC_ECC_KEY_IND)
  {
    DmSecSetEccKey(&p_dm_evt->eccMsg.data.key);

    // If the local device sends OOB data.
    if (m_ble_send_oob_data)
    {
      uint8_t oobLocalRandom[SMP_RAND_LEN];
      SecRand(oobLocalRandom, SMP_RAND_LEN);
      DmSecCalcOobReq(oobLocalRandom, p_dm_evt->eccMsg.data.key.pubKey_x);
    }
  }
  else if (p_dm_evt->hdr.event == DM_SEC_CALC_OOB_IND)
  {
    if (BLE_oob_cfg == NULL)
    {
      BLE_oob_cfg = WsfBufAlloc(sizeof(dmSecLescOobCfg_t));
    }

    if (BLE_oob_cfg)
    {
      Calc128Cpy(BLE_oob_cfg->localConfirm, p_dm_evt->oobCalcInd.confirm);
      Calc128Cpy(BLE_oob_cfg->localRandom, p_dm_evt->oobCalcInd.random);
    }
  }
  else
  {
    len = DmSizeOfEvt(p_dm_evt);

    if ((p_msg = WsfMsgAlloc(len)) != NULL)
    {
      memcpy(p_msg, p_dm_evt, len);
      WsfMsgSend(m_ble_handler_id, p_msg);
    }
  }
}

/**
 * @brief         Application ATT callback
 *
 * @param[in]     p_evt    ATT callback event
 *
 * @attention     None
 *
 * @return        None
 */
static void m_ble_att_cb(attEvt_t *p_evt)
{
  attEvt_t *p_msg;

  if ((p_msg = WsfMsgAlloc(sizeof(attEvt_t) + p_evt->valueLen)) != NULL)
  {
    memcpy(p_msg, p_evt, sizeof(attEvt_t));
    p_msg->pValue = (uint8_t *) (p_msg + 1);
    memcpy(p_msg->pValue, p_evt->pValue, p_evt->valueLen);
    WsfMsgSend(m_ble_handler_id, p_msg);
  }
}

/**
 * @brief         Application ATTS client characteristic configuration callback
 *
 * @param[in]     p_dm_evt  DM callback event
 *
 * @attention     None
 *
 * @return        None
 */
static void m_ble_ccc_cb(attsCccEvt_t *p_evt)
{
  attsCccEvt_t  *p_msg;
  appDbHdl_t    dbHdl;

  // If CCC not set from initialization and there's a device record
  if ((p_evt->handle != ATT_HANDLE_NONE) &&
      ((dbHdl = AppDbGetHdl((dmConnId_t) p_evt->hdr.param)) != APP_DB_HDL_NONE))
  {
    // Store value in device database
    AppDbSetCccTblValue(dbHdl, p_evt->idx, p_evt->value);
  }

  if ((p_msg = WsfMsgAlloc(sizeof(attsCccEvt_t))) != NULL)
  {
    memcpy(p_msg, p_evt, sizeof(attsCccEvt_t));
    WsfMsgSend(m_ble_handler_id, p_msg);
  }
}

/**
 * @brief         Perform UI actions on connection close.
 *
 * @param[in]     p_msg    Pointer to message.
 *
 * @attention     None
 *
 * @return        None
 */
static void m_ble_close(ble_msg_t *p_msg)
{
  // Stop heart rate measurement
  HrpsMeasStop((dmConnId_t) p_msg->hdr.param);

  // Stop battery measurement
  bas_app_measure_stop((dmConnId_t) p_msg->hdr.param);
}

/**
 * @brief         Set up advertising and other procedures that need to be performed after
 *                device reset.
 *
 * @param[in]     p_msg    Pointer to message.
 *
 * @attention     None
 *
 * @return        None
 */
static void m_ble_setup(ble_msg_t *p_msg)
{
  // Set advertising and scan response data for discoverable mode
  AppAdvSetData(APP_ADV_DATA_DISCOVERABLE, sizeof(m_ble_adv_data_disc), (uint8_t *) m_ble_adv_data_disc);
  AppAdvSetData(APP_SCAN_DATA_DISCOVERABLE, sizeof(m_ble_scan_data_disc), (uint8_t *) m_ble_scan_data_disc);

  // Set advertising and scan response data for connectable mode
  AppAdvSetData(APP_ADV_DATA_CONNECTABLE, 0, NULL);
  AppAdvSetData(APP_SCAN_DATA_CONNECTABLE, 0, NULL);

  // Start advertising; automatically set connectable/discoverable mode and bondable mode
  AppAdvStart(APP_MODE_AUTO_INIT);
}

/**
 * @brief         Process CCC state change.
 *
 * @param[in]     p_msg    Pointer to message.
 *
 * @attention     None
 *
 * @return        None
 */
static void m_ble_process_ccc_state(ble_msg_t *p_msg)
{
  printf("ccc state ind value: %d, handle: %d, idx: %d \n", p_msg->ccc.value, p_msg->ccc.handle, p_msg->ccc.idx);

  // Handle battery level CCC
  if (p_msg->ccc.idx == BLE_BATT_LVL_CCC_IDX)
  {
    if (p_msg->ccc.value == ATT_CLIENT_CFG_NOTIFY)
    {
      bas_app_measure_start((dmConnId_t) p_msg->ccc.hdr.param, BLE_BATT_TIMER_IND, BLE_BATT_LVL_CCC_IDX);
      printf("bas_app_measure_start\n");
    }
    else
    {
      bas_app_measure_stop((dmConnId_t) p_msg->ccc.hdr.param);
      printf("bas_app_measure_stop\n");
    }
    return;
  }
}

/**
 * @brief         Process messages from the event handler.
 *
 * @param[in]     p_msg    Pointer to message.
 *
 * @attention     None
 *
 * @return        None
 */
static void m_ble_process_msg(ble_msg_t *p_msg)
{
  uint8_t uiEvent = APP_UI_NONE;

  printf("BLE process message, event: %d \n", p_msg->hdr.event);

  switch(p_msg->hdr.event)
  {
    case BLE_SENSOR_HUB_TIMER_IND:
      printf("BLE_SENSOR_HUB_TIMER_IND\n");
      break;

    case BLE_TEMPERARUE_TIMER_IND:
      printf("BLE_TEMPERARUE_TIMER_IND\n");
      break;

    case BLE_BATT_TIMER_IND:
      bas_app_process_msg(&p_msg->hdr);
      break;

    case ATTS_HANDLE_VALUE_CNF:
      HrpsProcMsg(&p_msg->hdr);
      bas_app_process_msg(&p_msg->hdr);
      break;

    case ATTS_CCC_STATE_IND:
      printf("ATTS_CCC_STATE_IND\n");
      m_ble_process_ccc_state(p_msg);
      break;

    case DM_RESET_CMPL_IND:
      printf("DM_RESET_CMPL_IND\n");
      DmSecGenerateEccKeyReq();
      m_ble_setup(p_msg);
      uiEvent = APP_UI_RESET_CMPL;
      break;

    case DM_ADV_START_IND:
      printf("DM_ADV_START_IND\n");
      uiEvent = APP_UI_ADV_START;
      break;

    case DM_ADV_STOP_IND:
      printf("DM_ADV_STOP_IND\n");
      uiEvent = APP_UI_ADV_STOP;
      break;

    case DM_CONN_OPEN_IND:
      printf("DM_CONN_OPEN_IND\n");
      bas_app_process_msg(&p_msg->hdr);
      uiEvent = APP_UI_CONN_OPEN;
      break;

    case DM_CONN_CLOSE_IND:
      printf("DM_CONN_CLOSE_IND\n");
      m_ble_close(p_msg);
      uiEvent = APP_UI_CONN_CLOSE;
      break;

    case DM_SEC_PAIR_CMPL_IND:
      uiEvent = APP_UI_SEC_PAIR_CMPL;
      break;

    case DM_SEC_PAIR_FAIL_IND:
      uiEvent = APP_UI_SEC_PAIR_FAIL;
      break;

    case DM_SEC_ENCRYPT_IND:
      uiEvent = APP_UI_SEC_ENCRYPT;
      break;

    case DM_SEC_ENCRYPT_FAIL_IND:
      uiEvent = APP_UI_SEC_ENCRYPT_FAIL;
      break;

    case DM_SEC_AUTH_REQ_IND:
      AppHandlePasskey(&p_msg->dm.authReq);
      break;

    case DM_SEC_ECC_KEY_IND:
      DmSecSetEccKey(&p_msg->dm.eccMsg.data.key);
      break;

    case DM_SEC_COMPARE_IND:
      AppHandleNumericComparison(&p_msg->dm.cnfInd);
      break;

    case DM_HW_ERROR_IND:
      uiEvent = APP_UI_HW_ERROR;
      break;

    default:
      break;
  }

  if (uiEvent != APP_UI_NONE)
  {
    AppUiAction(uiEvent);
  }
}

