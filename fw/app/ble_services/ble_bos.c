/**
 * @file       ble_bos.c
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-04-04
 * @author     Thuan Le
 * @brief      BOS (BLE Blood Oxygen Service)
 * @note       None
 * @example    None
 */

/* Includes ----------------------------------------------------------- */
#include "wsf_types.h"
#include "util/bstream.h"
#include "svc_cfg.h"
#include "ble_bos.h"

/* Private defines ---------------------------------------------------- */
#define BLE_UUID_BOS_SERVICE              (0x1234) /**< The part UUID of the Blood Oxygen Service. */
#define BLE_UUID_BOS_CHARATERISTIC        (0x1235) /**< The part UUID of the Blood Oxygen Charateristic. */

/*! \brief Macro for building BOS UUIDs */
/*! \brief Base UUID:  E0262760-08C2-11E1-9073-0E8AC72EXXXX */
#define ATT_UUID_BOS_BUILD(part)           0x41, 0xEE, 0x68, 0x3A, 0x99, 0x0F, 0x0E, 0x72, \
                                           0x85, 0x49, 0x8D, 0xB3, UINT16_TO_BYTES(part),0x00, 0x00

/**< The UUID of the Blood Oxygen Service. */
#define ATT_UUID_BOS_SERVICE              ATT_UUID_BOS_BUILD(BLE_UUID_BOS_SERVICE)
#define ATT_UUID_BOS_CHARACTERICSTIC      ATT_UUID_BOS_BUILD(BLE_UUID_BOS_CHARATERISTIC)

/*! Characteristic read permissions */
#ifndef BOS_SEC_PERMIT_READ
#define BOS_SEC_PERMIT_READ SVC_SEC_PERMIT_READ
#endif

/*! Characteristic write permissions */
#ifndef BOS_SEC_PERMIT_WRITE
#define BOS_SEC_PERMIT_WRITE SVC_SEC_PERMIT_WRITE
#endif

/* Private enumerate/structure ---------------------------------------- */
/*!
 * Blood oxygen service
 */
static const uint8_t svcDatUuid[] = {ATT_UUID_BOS_CHARACTERICSTIC};

/* Blood oxygen service declaration */
static const uint8_t m_bos_service[] = {ATT_UUID_BOS_SERVICE};
static const uint16_t m_bos_service_len = sizeof(m_bos_service);

/* Blood oxygen level characteristic */
static const uint8_t m_bos_charac[] = {ATT_PROP_READ | ATT_PROP_NOTIFY, UINT16_TO_BYTES(BOS_LVL_HDL), ATT_UUID_BOS_CHARACTERICSTIC};
static const uint16_t m_bos_charac_len = sizeof(m_bos_charac);

/* Blood oxygen level */
static uint8_t m_spo2[] = {0};
static const uint16_t m_spo2_len = sizeof(m_spo2);

/* Blood oxygen level client characteristic configuration */
static uint8_t m_spo2_cc[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t m_spo2_cc_len = sizeof(m_spo2_cc);

/* Attribute list for group */
static const attsAttr_t m_bos_list[] =
{
  /* Service declaration */
  {
    attPrimSvcUuid,
    (uint8_t *) m_bos_service,
    (uint16_t *) &m_bos_service_len,
    sizeof(m_bos_service),
    0,
    ATTS_PERMIT_READ
  },
  /* Characteristic declaration */
  {
    attChUuid,
    (uint8_t *) m_bos_charac,
    (uint16_t *) &m_bos_charac_len,
    sizeof(m_bos_charac),
    0,
    ATTS_PERMIT_READ
  },
  /* Characteristic value */
  {
    svcDatUuid,
    (uint8_t *) m_spo2,
    (uint16_t *) &m_spo2_len,
    sizeof(m_spo2),
    (ATTS_SET_READ_CBACK | ATTS_SET_UUID_128 | ATTS_SET_VARIABLE_LEN | ATTS_SET_WRITE_CBACK),
    (BOS_SEC_PERMIT_READ | BOS_SEC_PERMIT_WRITE)
  },
  /* Characteristic CCC descriptor */
  {
    attCliChCfgUuid,
    (uint8_t *) m_spo2_cc,
    (uint16_t *) &m_spo2_cc_len,
    sizeof(m_spo2_cc),
    ATTS_SET_CCC,
    (ATTS_PERMIT_READ | BOS_SEC_PERMIT_WRITE)
  }
};

/* Blood oxygen group structure */
static attsGroup_t m_bos_group =
{
  NULL,
  (attsAttr_t *) m_bos_list,
  NULL,
  NULL,
  BOS_START_HDL,
  BOS_END_HDL
};

/* Private macros ----------------------------------------------------- */
/* Public variables --------------------------------------------------- */
/* Private variables -------------------------------------------------- */
/* Private function prototypes ---------------------------------------- */
/* Function definitions ----------------------------------------------- */
void ble_bos_init(void)
{
  AttsAddGroup(&m_bos_group);
}

void ble_bos_callback_register(attsReadCback_t read_cb, attsWriteCback_t write_cb)
{
  m_bos_group.readCback  = read_cb;
  m_bos_group.writeCback = write_cb;
}

/* Private function definitions --------------------------------------- */
/* End of file -------------------------------------------------------- */
