/**
 * @file       ble_stack.c
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-05-10
 * @author     Thuan Le
 * @brief      BLE Stack initialization
 * @note       None
 * @example    None
 */

/* Includes ----------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include "wsf_types.h"
#include "wsf_os.h"
#include "util/bstream.h"
#include "fit/fit_api.h"
#include "hci_handler.h"
#include "dm_handler.h"
#include "l2c_handler.h"
#include "att_handler.h"
#include "smp_handler.h"
#include "l2c_api.h"
#include "att_api.h"
#include "smp_api.h"
#include "app_api.h"
#include "svc_dis.h"
#include "svc_core.h"
#include "sec_api.h"
#include "ll_init_api.h"
#include "hci_drv_sdma.h"

/* Private defines ---------------------------------------------------- */
#define LL_IMPL_REV             (0x2303)
#define LL_MEMORY_FOOTPRINT     (0xC152)

/* Private enumerate/structure ---------------------------------------- */
/* Private macros ----------------------------------------------------- */
/* Public variables --------------------------------------------------- */
/* Private variables -------------------------------------------------- */
static uint8_t ll_mem[LL_MEMORY_FOOTPRINT];

static const LlRtCfg_t LL_CFG =
{
  // Device
  LL_COMP_ID_ARM,          // CompId
  LL_IMPL_REV,             // ImplRev
  LL_VER_BT_CORE_SPEC_5_0, // BtVer
  0,                       // Padding for alignment

  // Advertiser
  4,                       // MaxAdvSets
  8,                       // MaxAdvReports
  LL_MAX_ADV_DATA_LEN,     // MaxExtAdvDataLen
  64,                      // DefExtAdvDataFrag
  0,                       // AuxDelayUsec

  // Scanner
  4,                       // MaxScanReqRcvdEvt
  LL_MAX_ADV_DATA_LEN,     // MaxExtScanDataLen

  // Connection
  2,                       // MaxConn
  16,                      // NumTxBufs
  16,                      // NumRxBufs
  512,                     // MaxAclLen
  0,                       // DefTxPwrLvl
  0,                       // CeJitterUsec

  // DTM
  10000,                   // DtmRxSyncMs

  // PHY
  TRUE,                    // Phy2mSup
  TRUE,                    // PhyCodedSup
  FALSE,                   // StableModIdxTxSup
  FALSE                    // StableModIdxRxSup
};

static const BbRtCfg_t BB_CFG =
{
  20,                      // ClkPpm
  BB_RF_SETUP_DELAY_US,    // RfSetupDelayUsec
  -10,                     // DefaultTxPower
  BB_MAX_SCAN_PERIOD_MS,   // MaxScanPeriodMsec
  BB_SCH_SETUP_DELAY_US    // SchSetupDelayUsec
};

/* Private function prototypes ---------------------------------------- */
/* Function definitions ----------------------------------------------- */
/**
 * @brief         Initialize stack
 *
 * @param[in]     None
 *
 * @attention     None
 *
 * @return        None
 */
void ble_stack_init(void)
{
  wsfHandlerId_t handler_id;

  uint32_t mem_used;

  // Initialize link layer
  LlInitRtCfg_t ll_init_cfg =
  {
    .pBbRtCfg     = &BB_CFG,
    .wlSizeCfg    = 4,
    .rlSizeCfg    = 4,
    .plSizeCfg    = 4,
    .pLlRtCfg     = &LL_CFG,
    .pFreeMem     = ll_mem,
    .freeMemAvail = LL_MEMORY_FOOTPRINT
  };

  mem_used = LlInitControllerExtInit(&ll_init_cfg);
  if(mem_used != LL_MEMORY_FOOTPRINT)
  {
    printf("Controller memory mismatch 0x%x != 0x%x\n", mem_used, LL_MEMORY_FOOTPRINT);
  }

  handler_id = WsfOsSetNextHandler(HciHandler);
  HciHandlerInit(handler_id);

  SecInit();
  SecAesInit();
  SecCmacInit();
  SecEccInit();

  handler_id = WsfOsSetNextHandler(DmHandler);
  DmDevVsInit(0);
  DmAdvInit();
  DmConnInit();
  DmConnSlaveInit();
  DmSecInit();
  DmSecLescInit();
  DmPrivInit();
  DmPhyInit();
  DmHandlerInit(handler_id);

  handler_id = WsfOsSetNextHandler(L2cSlaveHandler);
  L2cSlaveHandlerInit(handler_id);
  L2cInit();
  L2cSlaveInit();

  handler_id = WsfOsSetNextHandler(AttHandler);
  AttHandlerInit(handler_id);
  AttsInit();
  AttsIndInit();

  handler_id = WsfOsSetNextHandler(SmpHandler);
  SmpHandlerInit(handler_id);
  SmprInit();
  SmprScInit();
  HciSetMaxRxAclLen(100);

  handler_id = WsfOsSetNextHandler(AppHandler);
  AppHandlerInit(handler_id);

  handler_id = WsfOsSetNextHandler(FitHandler);
  FitHandlerInit(handler_id);
}

/* End of file -------------------------------------------------------- */
