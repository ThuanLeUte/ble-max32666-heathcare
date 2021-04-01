/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Stack initialization for dats.
 *
 *  Copyright (c) 2016-2017 ARM Ltd. All Rights Reserved.
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

#include <stdio.h>
#include <string.h>
#include "mxc_device.h"
#include "wsf_types.h"
#include "wsf_os.h"
#include "util/bstream.h"
#include "dats/dats_api.h"
#include "wdxs/wdxs_api.h"
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
#include "gcr_regs.h"
#include "ll_init_api.h"
#include "hci_drv_sdma.h"

#define LL_IMPL_REV             0x2303

#define LL_MEMORY_FOOTPRINT     0xc152

uint8_t LlMem[LL_MEMORY_FOOTPRINT];

LlRtCfg_t _ll_cfg = {
    /* Device */
    /*compId*/                  LL_COMP_ID_ARM,
    /*implRev*/                 LL_IMPL_REV,
    /*btVer*/                   LL_VER_BT_CORE_SPEC_5_0,
    /*_align32 */               0, // padding for alignment

    /* Advertiser */
#ifdef ENABLE_SDMA
    /*maxAdvSets*/              1, // 1 Extended Advertising Set
    /*maxAdvReports*/           1,
#else /* ENABLE_SDMA */
    /*maxAdvSets*/              4, // 4 Extended Advertising Sets
    /*maxAdvReports*/           8,
#endif /* ENABLE_SDMA */
    /*maxExtAdvDataLen*/        LL_MAX_ADV_DATA_LEN,
    /*defExtAdvDataFrag*/       64,
    /*auxDelayUsec*/            0,

    /* Scanner */
    /*maxScanReqRcvdEvt*/       4,
    /*maxExtScanDataLen*/       LL_MAX_ADV_DATA_LEN,

    /* Connection */
#ifdef ENABLE_SDMA
    /*maxConn*/                 2,
    /*numTxBufs*/               8,
    /*numRxBufs*/               8,
    /*maxAclLen*/               150,
#else /* ENABLE_SDMA */
    /*maxConn*/                 2,
    /*numTxBufs*/               16,
    /*numRxBufs*/               16,
    /*maxAclLen*/               512,
#endif /* ENABLE_SDMA */
    /*defTxPwrLvl*/             0,
    /*ceJitterUsec*/            0,

    /* DTM */
    /*dtmRxSyncMs*/             10000,

    /* PHY */
    /*phy2mSup*/                TRUE,
    /*phyCodedSup*/             FALSE,
    /*stableModIdxTxSup*/       FALSE,
    /*stableModIdxRxSup*/       FALSE
};

const BbRtCfg_t _bb_cfg = {
    /*clkPpm*/                  20,
    /*rfSetupDelayUsec*/        BB_RF_SETUP_DELAY_US,
    /*defaultTxPower*/          -10,
    /*maxScanPeriodMsec*/       BB_MAX_SCAN_PERIOD_MS,
    /*schSetupDelayUsec*/       BB_SCH_SETUP_DELAY_US
};

/*************************************************************************************************/
/*!
 *  \brief      Initialize stack.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void StackInitDats(void)
{
  wsfHandlerId_t handlerId;

#ifndef ENABLE_SDMA
  uint32_t memUsed;

  /* Enable coded PHY */
  if(MXC_GCR->revision != 0xA1) {
    _ll_cfg.phyCodedSup = TRUE;
  }

  /* Initialize link layer. */
  LlInitRtCfg_t ll_init_cfg =
  {
      .pBbRtCfg     = &_bb_cfg,
      .wlSizeCfg    = 4,
      .rlSizeCfg    = 4,
      .plSizeCfg    = 4,
      .pLlRtCfg     = &_ll_cfg,
      .pFreeMem     = LlMem,
      .freeMemAvail = LL_MEMORY_FOOTPRINT
  };

  memUsed = LlInitControllerExtInit(&ll_init_cfg);
  if(memUsed != LL_MEMORY_FOOTPRINT)
  {
    printf("mem_used: 0x%x LL_MEMORY_FOOTPRINT: 0x%x\n", memUsed, 
        LL_MEMORY_FOOTPRINT);
  }
#endif /* ENABLE_SDMA */

#ifdef ENABLE_SDMA
  /* Copy link layer configuration to sdma memory space. */
  SDMASetBBCfg(&_bb_cfg);
  SDMASetLLCfg(&_ll_cfg);
#endif /* ENABLE_SDMA */

  handlerId = WsfOsSetNextHandler(HciHandler);
  HciHandlerInit(handlerId);

  SecInit();
  SecAesInit();
  SecCmacInit();
  SecEccInit();

  handlerId = WsfOsSetNextHandler(DmHandler);
  DmDevVsInit(0);
#ifdef BTLE_APP_USE_LEGACY_API
  DmAdvInit();
  DmConnInit();
  DmConnSlaveInit();
#else /* BTLE_APP_USE_LEGACY_API */
  DmExtAdvInit();
  DmConnInit();
  DmExtConnSlaveInit();
#endif /* BTLE_APP_USE_LEGACY_API */
  DmSecInit();
  DmSecLescInit();
  DmPrivInit();
  DmHandlerInit(handlerId);

  handlerId = WsfOsSetNextHandler(L2cSlaveHandler);
  L2cSlaveHandlerInit(handlerId);
  L2cInit();
  L2cSlaveInit();

  handlerId = WsfOsSetNextHandler(AttHandler);
  AttHandlerInit(handlerId);
  AttsInit();
  AttsIndInit();

  handlerId = WsfOsSetNextHandler(SmpHandler);
  SmpHandlerInit(handlerId);
  SmprInit();
  SmprScInit();
  HciSetMaxRxAclLen(256);

  handlerId = WsfOsSetNextHandler(AppHandler);
  AppHandlerInit(handlerId);

  handlerId = WsfOsSetNextHandler(DatsHandler);
  DatsHandlerInit(handlerId);

#if WDXS_INCLUDED == TRUE
  handlerId = WsfOsSetNextHandler(WdxsHandler);
  WdxsHandlerInit(handlerId);
#endif
  
}
