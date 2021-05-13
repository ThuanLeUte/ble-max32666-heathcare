/**
 * @file       ble_bas.c
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-01-07
 * @author     Thuan Le
 * @brief      BAS (BLE Battery Service)
 * @note       None
 * @example    None
 */

/* Includes ----------------------------------------------------------- */
#include "wsf_types.h"
#include "util/bstream.h"
#include "svc_cfg.h"
#include "ble_bas.h"

/* Private defines ---------------------------------------------------- */

/*! Characteristic read permissions */
#ifndef BATT_SEC_PERMIT_READ
#define BATT_SEC_PERMIT_READ SVC_SEC_PERMIT_READ
#endif

/*! Characteristic write permissions */
#ifndef BATT_SEC_PERMIT_WRITE
#define BATT_SEC_PERMIT_WRITE SVC_SEC_PERMIT_WRITE
#endif

/* Private enumerate/structure ---------------------------------------- */
/*!
 * Battery service
 */

/* Battery service declaration */
static const uint8_t m_bas_service[] = {UINT16_TO_BYTES(ATT_UUID_BATTERY_SERVICE)};
static const uint16_t m_bas_service_len = sizeof(m_bas_service);

/* Battery level characteristic */
static const uint8_t m_bas_charac[] = {ATT_PROP_READ | ATT_PROP_NOTIFY, UINT16_TO_BYTES(BAS_LVL_HDL), UINT16_TO_BYTES(ATT_UUID_BATTERY_LEVEL)};
static const uint16_t m_bas_charac_len = sizeof(m_bas_charac);

/* Battery level */
static uint8_t m_batt_level[] = {0};
static const uint16_t m_batt_level_len = sizeof(m_batt_level);

/* Battery level client characteristic configuration */
static uint8_t m_batt_level_cc[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t m_batt_level_cc_len = sizeof(m_batt_level_cc);

/* Attribute list for group */
static const attsAttr_t m_bas_list[] =
{
  /* Service declaration */
  {
    attPrimSvcUuid,
    (uint8_t *) m_bas_service,
    (uint16_t *) &m_bas_service_len,
    sizeof(m_bas_service),
    0,
    ATTS_PERMIT_READ
  },
  /* Characteristic declaration */
  {
    attChUuid,
    (uint8_t *) m_bas_charac,
    (uint16_t *) &m_bas_charac_len,
    sizeof(m_bas_charac),
    0,
    ATTS_PERMIT_READ
  },
  /* Characteristic value */
  {
    attBlChUuid,
    m_batt_level,
    (uint16_t *) &m_batt_level_len,
    sizeof(m_batt_level),
    ATTS_SET_READ_CBACK,
    BATT_SEC_PERMIT_READ
  },
  /* Characteristic CCC descriptor */
  {
    attCliChCfgUuid,
    m_batt_level_cc,
    (uint16_t *) &m_batt_level_cc_len,
    sizeof(m_batt_level_cc),
    ATTS_SET_CCC,
    (ATTS_PERMIT_READ | BATT_SEC_PERMIT_WRITE)
  }
};

/* Battery group structure */
static attsGroup_t m_bas_group =
{
  NULL,
  (attsAttr_t *) m_bas_list,
  NULL,
  NULL,
  BAS_START_HDL,
  BAS_END_HDL
};

/* Private macros ----------------------------------------------------- */
/* Public variables --------------------------------------------------- */
/* Private variables -------------------------------------------------- */
/* Private function prototypes ---------------------------------------- */
/* Function definitions ----------------------------------------------- */
void ble_bas_init(void)
{
  AttsAddGroup(&m_bas_group);
}

void ble_bas_callback_register(attsReadCback_t read_cb, attsWriteCback_t write_cb)
{
  m_bas_group.readCback  = read_cb;
  m_bas_group.writeCback = write_cb;
}

/* Private function definitions --------------------------------------- */
/* End of file -------------------------------------------------------- */
