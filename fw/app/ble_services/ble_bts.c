/**
 * @file       ble_bts.c
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-04-04
 * @author     Thuan Le
 * @brief      BTS (BLE Body Temperature Service)
 * @note       None
 * @example    None
 */

/* Includes ----------------------------------------------------------- */
#include "wsf_types.h"
#include "util/bstream.h"
#include "svc_cfg.h"
#include "ble_bts.h"

/* Private defines ---------------------------------------------------- */
#define BLE_UUID_BTS_SERVICE              (0x1231) /**< The part UUID of the Body Temperature Service. */
#define BLE_UUID_BTS_CHARATERISTIC        (0x1232) /**< The part UUID of the Body Temperature Charateristic. */

/*! \brief Macro for building BTS UUIDs */
/*! \brief Base UUID:  E0262760-08C2-11E1-9073-0E8AC72EXXXX */
#define ATT_UUID_BTS_BUILD(part)           0x41, 0xEE, 0x68, 0x3A, 0x99, 0x0F, 0x0E, 0x72, \
                                           0x85, 0x49, 0x8D, 0xB3, UINT16_TO_BYTES(part),0x00, 0x00

/**< The UUID of the Body Temperature Service. */
#define ATT_UUID_BTS_SERVICE              ATT_UUID_BTS_BUILD(BLE_UUID_BTS_SERVICE)
#define ATT_UUID_BTS_CHARACTERICSTIC      ATT_UUID_BTS_BUILD(BLE_UUID_BTS_CHARATERISTIC)

/*! Characteristic read permissions */
#ifndef BTS_SEC_PERMIT_READ
#define BTS_SEC_PERMIT_READ SVC_SEC_PERMIT_READ
#endif

/*! Characteristic write permissions */
#ifndef BTS_SEC_PERMIT_WRITE
#define BTS_SEC_PERMIT_WRITE SVC_SEC_PERMIT_WRITE
#endif

/* Private enumerate/structure ---------------------------------------- */
/*!
 * Body temperature service
 */
static const uint8_t svcDatUuid[] = {ATT_UUID_BTS_CHARACTERICSTIC};

/* Body temperature service declaration */
static const uint8_t m_bts_service[] = {ATT_UUID_BTS_SERVICE};
static const uint16_t m_bts_service_len = sizeof(m_bts_service);

/* Body temperature characteristic */
static const uint8_t m_bts_charac[] = {ATT_PROP_READ | ATT_PROP_NOTIFY, UINT16_TO_BYTES(BTS_VALUE_HDL), ATT_UUID_BTS_CHARACTERICSTIC};
static const uint16_t m_bts_charac_len = sizeof(m_bts_charac);

/* Body temperature */
static uint8_t m_temp[] = {0};
static const uint16_t m_temp_len = sizeof(m_temp);

/* Body temperature client characteristic configuration */
static uint8_t m_temp_cc[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t m_temp_cc_len = sizeof(m_temp_cc);

/* Attribute list for group */
static const attsAttr_t m_bts_list[] =
{
  /* Service declaration */
  {
    attPrimSvcUuid,
    (uint8_t *) m_bts_service,
    (uint16_t *) &m_bts_service_len,
    sizeof(m_bts_service),
    0,
    ATTS_PERMIT_READ
  },
  /* Characteristic declaration */
  {
    attChUuid,
    (uint8_t *) m_bts_charac,
    (uint16_t *) &m_bts_charac_len,
    sizeof(m_bts_charac),
    0,
    ATTS_PERMIT_READ
  },
  /* Characteristic value */
  {
    svcDatUuid,
    (uint8_t *) m_temp,
    (uint16_t *) &m_temp_len,
    sizeof(m_temp),
    (ATTS_SET_READ_CBACK | ATTS_SET_UUID_128 | ATTS_SET_VARIABLE_LEN | ATTS_SET_WRITE_CBACK),
    (BTS_SEC_PERMIT_READ | BTS_SEC_PERMIT_WRITE)
  },
  /* Characteristic CCC descriptor */
  {
    attCliChCfgUuid,
    (uint8_t *) m_temp_cc,
    (uint16_t *) &m_temp_cc_len,
    sizeof(m_temp_cc),
    ATTS_SET_CCC,
    (ATTS_PERMIT_READ | BTS_SEC_PERMIT_WRITE)
  }
};

/* Blood oxygen group structure */
static attsGroup_t m_bts_group =
{
  NULL,
  (attsAttr_t *) m_bts_list,
  NULL,
  NULL,
  BTS_START_HDL,
  BTS_END_HDL
};

/* Private macros ----------------------------------------------------- */
/* Public variables --------------------------------------------------- */
/* Private variables -------------------------------------------------- */
/* Private function prototypes ---------------------------------------- */
/* Function definitions ----------------------------------------------- */
void ble_bts_init(void)
{
  AttsAddGroup(&m_bts_group);
}

void ble_bts_callback_register(attsReadCback_t read_cb, attsWriteCback_t write_cb)
{
  m_bts_group.readCback  = read_cb;
  m_bts_group.writeCback = write_cb;
}

/* Private function definitions --------------------------------------- */
/* End of file -------------------------------------------------------- */
