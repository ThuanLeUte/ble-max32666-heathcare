/* Copyright (c) 2009-2019 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*************************************************************************************************/
/*!
 *  \brief Link layer controller master scanning operation builder implementation file.
 */
/*************************************************************************************************/

#include "lctr_int_adv_master_ae.h"
#include "lctr_int_adv_master.h"
#include "lctr_api_adv_master_ae.h"
#include "lmgr_api_adv_master_ae.h"
#include "sch_api.h"
#include "sch_api_ble.h"
#include "bb_ble_api_reslist.h"
#include "wsf_assert.h"
#include "wsf_msg.h"
#include "wsf_trace.h"
#include "wsf_math.h"
#include "util/bstream.h"

// TODO Hide in conn only file
#include "lctr_int_conn_master.h"

#include <string.h>

/**************************************************************************************************
  Globals
**************************************************************************************************/

/*! \brief      Extended scan operational context. */
lctrExtScanCtx_t lctrMstExtScanTbl[LCTR_SCAN_PHY_TOTAL];    // TODO: share memory with legacy lctrMstScan

/*! \brief      Extended scan control block. */
lctrExtScanCtrlBlk_t lctrMstExtScan;

/*! \brief      Periodic scan control block. */
lctrPerCreateSyncCtrlBlk_t lctrPerCreateSync;

/*! \brief      Periodic Advertising message data. */
LctrPerScanMsg_t *pLctrMstPerScanMsg;

/*! \brief      Periodic scan context table. */
lctrPerScanCtx_t lctrMstPerScanTbl[LL_MAX_PER_SCAN];

/*! \brief      Pointer to periodic scan context table. */
lctrPerScanCtx_t *pLctrPerScanTbl;

/*! \brief      Extended scan data buffer table. */
static uint8_t *lctrMstExtScanDataBufTbl[LCTR_SCAN_PHY_TOTAL];

/*! \brief      Periodic scan data buffer table. */
static uint8_t *lctrMstPerScanDataBufTbl[LL_MAX_PER_SCAN];

/*************************************************************************************************/
/*!
 *  \brief      Master create sync message dispatcher.
 *
 *  \param      pMsg    Pointer to message buffer.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void lctrMstCreateSyncDisp(LctrPerScanMsg_t *pMsg)
{
  pLctrMstPerScanMsg = pMsg;

  lctrMstCreateSyncExecuteSm(pMsg->hdr.event);
}

/*************************************************************************************************/
/*!
 *  \brief      Periodic scanning message dispatcher.
 *
 *  \param      pMsg    Pointer to message buffer.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void lctrMstPerScanDisp(LctrPerScanMsg_t *pMsg)
{
  lctrPerScanCtx_t *pPerScanCtx;
  pLctrMstPerScanMsg = pMsg;

  if (pMsg->hdr.dispId != LCTR_DISP_BCST)
  {
    WSF_ASSERT(pMsg->hdr.handle < LL_MAX_PER_SCAN);
    pPerScanCtx = LCTR_GET_PER_SCAN_CTX(pMsg->hdr.handle);

    lctrMstPerScanExecuteSm(pPerScanCtx, pMsg->hdr.event);
  }
  else
  {
    unsigned int i;
    /* Broadcast message to all contexts. */
    for (i = 0; i < LL_MAX_PER_SCAN; i++)
    {
      lctrMstPerScanExecuteSm(LCTR_GET_PER_SCAN_CTX(i), pMsg->hdr.event);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Periodic scanning reset handler.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void lctrMstPerScanResetHandler(void)
{
  BbBlePerScanMasterInit();
}

/*************************************************************************************************/
/*!
 *  \brief      Master extended scan reset handler.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void lctrMstExtScanResetHandler(void)
{
  BbBleScanMasterInit();
  BbBleAuxScanMasterInit();
  LctrMstExtScanDefaults();
}

/*************************************************************************************************/
/*!
 *  \brief      Execute common master scan state machine.
 *
 *  \param      pMsg    Pointer to message buffer.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void lctrMstExtScanExecuteCommonSm(LctrExtScanMsg_t *pMsg)
{
  /* Subsystem event handling. */
  switch (pMsg->hdr.event)
  {
    case LCTR_EXT_SCAN_MSG_DISCOVER_ENABLE:
      LL_TRACE_INFO2("lctrMstExtScanExecuteCommonSm: numScanEnabled=%u, scanMode=%u, event=DISCOVER_ENABLE", lmgrCb.numScanEnabled, lmgrCb.scanMode);

      lctrMstExtScan.scanTermByHost = FALSE;

      /* Enable filter. */
      lctrMstExtScan.filtDup = pMsg->enable.filtDup;
      lctrAdvRptEnable(&lctrMstExtScan.advFilt,
                       (lctrMstExtScan.filtDup == LL_SCAN_FILTER_DUP_DISABLE) ? FALSE: TRUE);

      /* Start/Restart timers. */
      WsfTimerStop(&lctrMstExtScan.tmrScanDur);
      WsfTimerStop(&lctrMstExtScan.tmrScanPer);

      lctrMstExtScan.scanDurMs = pMsg->enable.durMs;
      lctrMstExtScan.scanPerMs = pMsg->enable.perMs;
      if (lctrMstExtScan.scanDurMs)
      {
        WsfTimerStartMs(&lctrMstExtScan.tmrScanDur, lctrMstExtScan.scanDurMs);
        if (lctrMstExtScan.scanPerMs)
        {
          WsfTimerStartMs(&lctrMstExtScan.tmrScanPer, lctrMstExtScan.scanPerMs);
        }
      }
      break;

    case LCTR_EXT_SCAN_MSG_DISCOVER_DISABLE:
      LL_TRACE_INFO2("lctrMstExtScanExecuteCommonSm: numScanEnabled=%u, scanMode=%u, event=DISCOVER_DISABLE", lmgrCb.numScanEnabled, lmgrCb.scanMode);

      /* Disable timers. */
      WsfTimerStop(&lctrMstExtScan.tmrScanDur);
      WsfTimerStop(&lctrMstExtScan.tmrScanPer);
      break;

    case LCTR_EXT_SCAN_MSG_TERMINATE:
      LL_TRACE_INFO2("lctrMstExtScanExecuteCommonSm: numScanEnabled=%u, scanMode=%u, event=TERMINATE", lmgrCb.numScanEnabled, lmgrCb.scanMode);
      if (!lctrMstExtScan.scanTermByHost &&
          (lctrMstExtScan.scanPerMs == 0))
      {
        LmgrSendScanTimeoutInd();
      }
      break;

    case LCTR_EXT_SCAN_MSG_TMR_DUR_EXP:
      LL_TRACE_INFO2("lctrMstExtScanExecuteCommonSm: numScanEnabled=%u, scanMode=%u, event=TMR_DUR_EXP", lmgrCb.numScanEnabled, lmgrCb.scanMode);
      break;

    case LCTR_EXT_SCAN_MSG_TMR_PER_EXP:
      LL_TRACE_INFO2("lctrMstExtScanExecuteCommonSm: numScanEnabled=%u, scanMode=%u, event=TMR_PER_EXP", lmgrCb.numScanEnabled, lmgrCb.scanMode);
      /* Reset filter. */
      if (lctrMstExtScan.filtDup == LL_SCAN_FILTER_DUP_ENABLE_PERIODIC)
      {
        lctrAdvRptEnable(&lctrMstExtScan.advFilt,
                         (lctrMstExtScan.filtDup == LL_SCAN_FILTER_DUP_DISABLE) ? FALSE: TRUE);
      }

      /* Restart timers. */
      WsfTimerStartMs(&lctrMstExtScan.tmrScanDur, lctrMstExtScan.scanDurMs);
      WsfTimerStartMs(&lctrMstExtScan.tmrScanPer, lctrMstExtScan.scanPerMs);
      break;

    case LCTR_EXT_SCAN_MSG_RESET:
      LL_TRACE_INFO2("lctrMstExtScanExecuteCommonSm: numScanEnabled=%u, scanMode=%u, event=RESET", lmgrCb.numScanEnabled, lmgrCb.scanMode);
      break;

    default:
      LL_TRACE_ERR3("lctrMstExtScanExecuteCommonSm: numScanEnabled=%u, scanMode=%u, event=%u -- unknown event", lmgrCb.numScanEnabled, lmgrCb.scanMode, pMsg->hdr.event);
      /* No action required. */
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Master extended scan message dispatcher.
 *
 *  \param      pMsg    Pointer to message buffer.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void lctrMstExtScanDisp(LctrExtScanMsg_t *pMsg)
{
  lctrExtScanCtx_t *pExtScanCtx;
  uint8_t event = pMsg->hdr.event;

  bool_t isBcstMsg = FALSE;

  if (pMsg->hdr.dispId == LCTR_DISP_BCST)
  {
    /* Global broadcast message. */
    isBcstMsg = TRUE;
  }

  /* Set message property. */
  switch (event)
  {
    case LCTR_EXT_SCAN_MSG_DISCOVER_DISABLE:
      lctrMstExtScan.scanTermByHost = TRUE;
      break;
    default:
      break;
  }

  /* Remap subsystem message to context message. */
  switch (event)
  {
    case LCTR_EXT_SCAN_MSG_TMR_DUR_EXP:
      event = LCTR_EXT_SCAN_MSG_DISCOVER_DISABLE;
      break;
    case LCTR_EXT_SCAN_MSG_TMR_PER_EXP:
      event = LCTR_EXT_SCAN_MSG_DISCOVER_ENABLE;
      break;
    default:
      break;
  }

  /* Broadcast message. */
  switch (event)
  {
    case LCTR_EXT_SCAN_MSG_DISCOVER_ENABLE:
    case LCTR_EXT_SCAN_MSG_DISCOVER_DISABLE:
      isBcstMsg = TRUE;
      break;
    default:
      break;
  }
  if (pMsg->hdr.handle == LCTR_SCAN_PHY_ALL)
  {
    isBcstMsg = TRUE;
  }

  /* Route message to SM. */
  if (!isBcstMsg)
  {
    WSF_ASSERT(pMsg->hdr.handle < LCTR_SCAN_PHY_TOTAL);
    if (lctrMstExtScan.enaPhys & (1 << pMsg->hdr.handle))
    {
      pExtScanCtx = &lctrMstExtScanTbl[pMsg->hdr.handle];
      lctrMstExtScanExecuteSm(pExtScanCtx, event);
    }
  }
  else
  {
    unsigned int i;
    for (i = 0; i < LCTR_SCAN_PHY_TOTAL; i++)
    {
      if (lctrMstExtScan.enaPhys & (1 << i))
      {
        pExtScanCtx = &lctrMstExtScanTbl[i];
        lctrMstExtScanExecuteSm(pExtScanCtx, event);
      }
    }
  }

  lctrMstExtScanExecuteCommonSm(pMsg);
}

/*************************************************************************************************/
/*!
 *  \brief      Send pending extended advertising report.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void lctrMstSendPendingAdvRptHandler(void)
{
  if (LmgrIsExtCommandAllowed())
  {
    unsigned int i;
    /* Extended advertising report. */
    for (i = 0; i < LCTR_SCAN_PHY_TOTAL; i++)
    {
      lctrExtScanCtx_t *pExtScanCtx = &lctrMstExtScanTbl[i];

      if (pExtScanCtx->data.scan.auxAdvRptState == LCTR_RPT_STATE_COMP)
      {
        LmgrSendExtAdvRptInd(&pExtScanCtx->data.scan.auxAdvRpt);
        pExtScanCtx->data.scan.auxAdvRptState = LCTR_RPT_STATE_IDLE;
      }

      if (pExtScanCtx->data.scan.advRptState == LCTR_RPT_STATE_COMP)
      {
        LmgrSendExtAdvRptInd(&pExtScanCtx->data.scan.advRpt);
        pExtScanCtx->data.scan.advRptState = LCTR_RPT_STATE_IDLE;
      }
    }

    /* Periodic advertising report. */
    for (i = 0; i < LL_MAX_PER_SCAN; i++)
    {
      lctrPerScanCtx_t *pPerScanCtx = &lctrMstPerScanTbl[i];

      if (pPerScanCtx->advRptState == LCTR_RPT_STATE_COMP)
      {
        LmgrSendPerAdvRptInd(&pPerScanCtx->advRpt);
        pPerScanCtx->advRptState = LCTR_RPT_STATE_IDLE;
      }
    }
  }
  else
  {
    /* Legacy mode. */
    lctrMstRxAdvBPduHandler();
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Build extended scan discovery operation.
 *
 *  \param  pExtScanCtx     Extended scan context.
 *
 *  \return Status error code.
 */
/*************************************************************************************************/
uint8_t lctrMstExtDiscoverBuildOp(lctrExtScanCtx_t *pExtScanCtx)
{
  BbOpDesc_t * const pOp = &pExtScanCtx->scanBod;
  BbBleData_t * const pBle = &pExtScanCtx->scanBleData;
  BbBleMstAdvEvent_t * const pScan = &pBle->op.mstAdv;

  memset(pOp, 0, sizeof(BbOpDesc_t));
  memset(pBle, 0, sizeof(BbBleData_t));

  /*** General Setup ***/

  pOp->reschPolicy = BB_RESCH_MOVEABLE;
  pOp->protId = BB_PROT_BLE;
  pOp->prot.pBle = pBle;
  pOp->endCback = lctrMstExtDiscoverEndOp;
  pOp->abortCback = lctrMstExtDiscoverEndOp;
  pOp->pCtx = pExtScanCtx;

  /*** BLE General Setup ***/

  pBle->chan.opType = BB_BLE_OP_MST_ADV_EVENT;

  pBle->chan.chanIdx = lctrScanChanSelectInit(lmgrMstScanCb.scanChanMap);
  pBle->chan.txPower = lmgrCb.advTxPwr;
  pBle->chan.accAddr = LL_ADV_ACCESS_ADDR;
  pBle->chan.crcInit = LL_ADV_CRC_INIT;
  switch (LCTR_GET_EXT_SCAN_HANDLE(pExtScanCtx))
  {
    case LCTR_SCAN_PHY_1M:
    default:
      pBle->chan.txPhy = pBle->chan.rxPhy = BB_PHY_BLE_1M;
      break;
    case LCTR_SCAN_PHY_CODED:
      pBle->chan.txPhy = pBle->chan.rxPhy = BB_PHY_BLE_CODED;
      break;
  }

#if (LL_ENABLE_TESTER == TRUE)
  pBle->chan.accAddrRx = llTesterCb.advAccessAddrRx ^ pBle->chan.accAddr;
  pBle->chan.accAddrTx = llTesterCb.advAccessAddrTx ^ pBle->chan.accAddr;
  pBle->chan.crcInitRx = llTesterCb.advCrcInitRx ^ pBle->chan.crcInit;
  pBle->chan.crcInitTx = llTesterCb.advCrcInitTx ^ pBle->chan.crcInit;
#endif

  pBle->pduFilt.pduTypeFilt = (1 << LL_PDU_ADV_IND) |
                              (1 << LL_PDU_ADV_DIRECT_IND) |
                              (1 << LL_PDU_ADV_NONCONN_IND) |
                              (1 << LL_PDU_SCAN_RSP) |
                              (1 << LL_PDU_ADV_SCAN_IND) |
                              (1 << LL_PDU_ADV_EXT_IND);
  if (pExtScanCtx->scanParam.scanFiltPolicy & LL_SCAN_FILTER_WL_BIT)
  {
    pBle->pduFilt.wlPduTypeFilt = pBle->pduFilt.pduTypeFilt;
  }
  /* Local addresses that are resolvable and cannot be resolved are optionally allowed. */
  if (pExtScanCtx->scanParam.scanFiltPolicy & LL_SCAN_FILTER_RES_INIT_BIT)
  {
    BB_BLE_PDU_FILT_SET_FLAG(&pBle->pduFilt, LOCAL_ADDR_RES_OPT);
  }

  /*** BLE Scan Setup: Rx advertising packet ***/

  pScan->scanChMap = lmgrMstScanCb.scanChanMap;

  pScan->rxAdvCback = lctrMstDiscoverRxExtAdvPktHandler;
  pScan->rxAdvPostCback = lctrMstDiscoverRxExtAdvPktPostProcessHandler;

  if ((pScan->pRxAdvBuf = WsfMsgAlloc(LL_ADV_HDR_LEN + LL_EXT_ADV_HDR_MAX_LEN)) == NULL)
  {
    LL_TRACE_ERR0("Could not allocate advertising buffer");
    return LL_ERROR_CODE_UNSPECIFIED_ERROR;
  }

  /*** BLE Scan Setup: Tx scan request packet ***/

  pScan->txReqCback = lctrMstDiscoverTxLegacyScanReqHandler;

  /* Always match local address in PDU to initiator's address (in directed advertisements). */
  if (pExtScanCtx->scanParam.ownAddrType & LL_ADDR_RANDOM_BIT)
  {
    WSF_ASSERT(lmgrCb.bdAddrRndValid);    /* No further verification after scan starts. */
    pBle->pduFilt.localAddrMatch = lmgrCb.bdAddrRnd;
    BB_BLE_PDU_FILT_SET_FLAG(&pBle->pduFilt, LOCAL_ADDR_MATCH_RAND);
  }
  else
  {
    pBle->pduFilt.localAddrMatch = lmgrPersistCb.bdAddr;
  }
  BB_BLE_PDU_FILT_SET_FLAG(&pBle->pduFilt, LOCAL_ADDR_MATCH_ENA);

  /* Potentially resolve peer & local addresses. */
  if (lmgrCb.addrResEna)
  {
    BB_BLE_PDU_FILT_SET_FLAG(&pBle->pduFilt, PEER_ADDR_RES_ENA);
    BB_BLE_PDU_FILT_SET_FLAG(&pBle->pduFilt, LOCAL_ADDR_RES_ENA);
  }

  switch (pExtScanCtx->scanParam.scanType)
  {
    case LL_SCAN_ACTIVE:
    {
      uint8_t *pBuf;
      lctrScanReq_t scanReq = { 0 };

      pExtScanCtx->reqPduHdr.pduType = LL_PDU_SCAN_REQ;
      pExtScanCtx->reqPduHdr.len = LL_SCAN_REQ_PDU_LEN;

      pExtScanCtx->reqPduHdr.txAddrRnd = BB_BLE_PDU_FILT_FLAG_IS_SET(&pBle->pduFilt, LOCAL_ADDR_MATCH_RAND);
      scanReq.scanAddr = pBle->pduFilt.localAddrMatch;

      /* Pack only known packet information, advertiser's address resolved in Rx handler. */
      pBuf  = pExtScanCtx->reqBuf;
      pBuf += LL_ADV_HDR_LEN;
      /* pBuf += */ lctrPackScanReqPdu(pBuf, &scanReq);

      pScan->pTxReqBuf = pExtScanCtx->reqBuf;
      pScan->txReqLen = LL_ADV_HDR_LEN + LL_SCAN_REQ_PDU_LEN;

#if (LL_ENABLE_TESTER)
      if (llTesterCb.txScanReqPduLen)
      {
        pScan->pTxReqBuf = llTesterCb.txScanReqPdu;
        pScan->txReqLen = llTesterCb.txScanReqPduLen;
      }
#endif

      break;
    }
    case LL_SCAN_PASSIVE:
    default:
      pScan->pTxReqBuf = NULL;
      break;
  }

  pExtScanCtx->reqPduHdr.chSel = LL_CH_SEL_1;

  /*** BLE Scan Setup: Rx scan response packet ***/

  pScan->rxRspCback = lctrMstDiscoverRxLegacyScanRspHandler;

  switch (pExtScanCtx->scanParam.scanType)
  {
    case LL_SCAN_ACTIVE:
    {
      /* Allow only legacy scan response. */
      if ((pScan->pRxRspBuf = WsfMsgAlloc(LL_ADVB_MAX_LEN)) == NULL)
      {
        WsfMsgFree(pScan->pRxAdvBuf);
        pScan->pRxAdvBuf = NULL;

        LL_TRACE_ERR0("Could not allocate scan response buffer");
        return LL_ERROR_CODE_UNSPECIFIED_ERROR;
      }

      break;
    }
    case LL_SCAN_PASSIVE:
    default:
      /* pScan->pRxRspBuf = NULL; */        /* cleared in alloc */
      break;
  }

  /*** Commit operation ***/

  pOp->minDurUsec = pOp->maxDurUsec = LCTR_BLE_TO_US(pExtScanCtx->scanParam.scanWindow);

  pExtScanCtx->selfTerm = FALSE;
  pExtScanCtx->shutdown = FALSE;

  SchInsertNextAvailable(pOp);
  pExtScanCtx->scanWinStart = pOp->due;

  return LL_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief  Build auxiliary scan discovery operation.
 *
 *  \param  pExtScanCtx     Extended scan context.
 *
 *  \return Status error code.
 */
/*************************************************************************************************/
uint8_t lctrMstAuxDiscoverBuildOp(lctrExtScanCtx_t *pExtScanCtx)
{
  /* Pre-resolve common structures for efficient access. */
  BbOpDesc_t * const pOp = &pExtScanCtx->auxScanBod;
  BbBleData_t * const pBle = &pExtScanCtx->auxBleData;
  BbBleMstAuxAdvEvent_t * const pAuxScan = &pBle->op.mstAuxAdv;

  memset(pOp, 0, sizeof(BbOpDesc_t));
  memset(pBle, 0, sizeof(BbBleData_t));

  /*** General Setup ***/

  pOp->reschPolicy = BB_RESCH_MOVEABLE_PREFERRED;
  pOp->protId = BB_PROT_BLE;
  pOp->prot.pBle = pBle;
  pOp->endCback = lctrMstAuxDiscoverEndOp;
  pOp->abortCback = lctrMstAuxDiscoverEndOp;
  pOp->pCtx = pExtScanCtx;

  /*** BLE General Setup ***/

  pBle->chan.opType = BB_BLE_OP_MST_AUX_ADV_EVENT;

  /* pBle->chan.chanIdx = 0; */     /* write after ADV_EXT_IND is received */
  pBle->chan.txPower = lmgrCb.advTxPwr;
  pBle->chan.accAddr = LL_ADV_ACCESS_ADDR;
  pBle->chan.crcInit = LL_ADV_CRC_INIT;
  /* pBle->chan.txPhy = 0; */       /* write after ADV_EXT_IND is received */
  /* pBle->chan.rxPhy = 0; */       /* write after ADV_EXT_IND is received */
  /* pBle->chan.phyOptions = 0; */  /* write after ADV_EXT_IND is received */

#if (LL_ENABLE_TESTER == TRUE)
  pBle->chan.accAddrRx = llTesterCb.advAccessAddrRx ^ pBle->chan.accAddr;
  pBle->chan.accAddrTx = llTesterCb.advAccessAddrTx ^ pBle->chan.accAddr;
  pBle->chan.crcInitRx = llTesterCb.advCrcInitRx ^ pBle->chan.crcInit;
  pBle->chan.crcInitTx = llTesterCb.advCrcInitTx ^ pBle->chan.crcInit;
#endif

  pBle->pduFilt.pduTypeFilt = (1 << LL_PDU_AUX_ADV_IND) |
                              (1 << LL_PDU_AUX_SCAN_RSP);
  if (pExtScanCtx->scanParam.scanFiltPolicy & LL_SCAN_FILTER_WL_BIT)
  {
    pBle->pduFilt.wlPduTypeFilt = pBle->pduFilt.pduTypeFilt;
  }
  /* Local addresses that are resolvable and cannot be resolved are optionally allowed. */
  if (pExtScanCtx->scanParam.scanFiltPolicy & LL_SCAN_FILTER_RES_INIT_BIT)
  {
    BB_BLE_PDU_FILT_SET_FLAG(&pBle->pduFilt, LOCAL_ADDR_RES_OPT);
  }

  /*** BLE Scan Setup: Rx packets ***/
  pAuxScan->isInit = FALSE;
  pAuxScan->rxAuxAdvCback = lctrMstDiscoverRxAuxAdvPktHandler;

  /*** BLE Scan Setup: Tx scan request packet ***/

  /* Always match local address in PDU to initiator's address (in directed advertisements). */
  if (pExtScanCtx->scanParam.ownAddrType & LL_ADDR_RANDOM_BIT)
  {
    WSF_ASSERT(lmgrCb.bdAddrRndValid);    /* No further verification after scan starts. */
    pBle->pduFilt.localAddrMatch = lmgrCb.bdAddrRnd;
    BB_BLE_PDU_FILT_SET_FLAG(&pBle->pduFilt, LOCAL_ADDR_MATCH_RAND);
  }
  else
  {
    pBle->pduFilt.localAddrMatch = lmgrPersistCb.bdAddr;
  }
  BB_BLE_PDU_FILT_SET_FLAG(&pBle->pduFilt, LOCAL_ADDR_MATCH_ENA);

  /* Potentially resolve peer & local addresses. */
  if (lmgrCb.addrResEna)
  {
    BB_BLE_PDU_FILT_SET_FLAG(&pBle->pduFilt, PEER_ADDR_RES_ENA);
    BB_BLE_PDU_FILT_SET_FLAG(&pBle->pduFilt, LOCAL_ADDR_RES_ENA);
  }

  switch (pExtScanCtx->scanParam.scanType)
  {
    case LL_SCAN_ACTIVE:
    {
      /* Use primary channel's SCAN_REQ PDU. */
      pAuxScan->pTxAuxReqBuf = pExtScanCtx->reqBuf;
      pAuxScan->txAuxReqLen = sizeof(pExtScanCtx->reqBuf);

#if (LL_ENABLE_TESTER)
      if (llTesterCb.txScanReqPduLen)
      {
        pAuxScan->pTxAuxReqBuf = llTesterCb.txScanReqPdu;
        pAuxScan->txAuxReqLen = llTesterCb.txScanReqPduLen;
      }
#endif

      break;
    }
    case LL_SCAN_PASSIVE:
    default:
      pAuxScan->pTxAuxReqBuf = NULL;
      break;
  }

  /*** BLE Scan Setup: Rx scan response packet ***/

  pAuxScan->rxAuxRspCback = lctrMstDiscoverRxAuxScanRspHandler;

  /*** BLE Scan Setup: Rx chain packet ***/

  pAuxScan->rxAuxChainCback = lctrMstDiscoverRxAuxChainHandler;
  pAuxScan->rxAuxChainPostCback = lctrMstDiscoverRxAuxChainPostProcessHandler;

  /*** Commit operation ***/

  /* pOp->minDurUsec = 0; */  /* Defer assignment until AuxPtr is received. */
  /* pOp->maxDurUsec = 0; */  /* Not used for aux scan. */

  pExtScanCtx->selfTerm = FALSE;
  pExtScanCtx->shutdown = FALSE;
  pExtScanCtx->auxOpPending = FALSE;

  /* Defer scheduling until AuxPtr is received. */

  return LL_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief  Commit auxiliary discovery operation.
 *
 *  \param  pExtScanCtx     Extended scan context.
 *  \param  pAuxPtr         Auxiliary Pointer.
 *  \param  startTs         Start of ADV_EXT_IND packet (offset origin).
 *  \param  endTs           End of ADV_EXT_IND packet.
 *
 *  \return None.
 */
/*************************************************************************************************/
void lctrMstAuxDiscoverOpCommit(lctrExtScanCtx_t *pExtScanCtx, lctrAuxPtr_t *pAuxPtr, uint32_t startTs, uint32_t endTs)
{
  uint32_t auxOffsetUsec;
  /* Pre-resolve common structures for efficient access. */
  BbOpDesc_t * const pOp = &pExtScanCtx->auxScanBod;
  BbBleData_t * const pBle = &pExtScanCtx->auxBleData;

  /*** BLE General Setup ***/

  pBle->chan.chanIdx = pAuxPtr->auxChIdx;
  if (pExtScanCtx->extAdvHdr.extHdrFlags & LL_EXT_HDR_TX_PWR_BIT)
  {
    pBle->chan.txPower = pExtScanCtx->extAdvHdr.txPwr;
  }
  pBle->chan.txPhy = pBle->chan.rxPhy = lctrConvertAuxPtrPhyToBbPhy(pAuxPtr->auxPhy);

  /*** Commit operation ***/

  WSF_ASSERT(pExtScanCtx->auxOpPending == FALSE);

  lctrMstComputeAuxOffset(pAuxPtr, &auxOffsetUsec, &pBle->op.mstAuxAdv.rxSyncDelayUsec);

  if (auxOffsetUsec < LL_BLE_MAFS_US)
  {
    LL_TRACE_WARN1("Peer requested AUX offset does not meet T_MAFS, actual afsUsec=%u", BB_US_TO_BB_TICKS(pOp->due - endTs));
  }

  pOp->due = startTs + BB_US_TO_BB_TICKS(auxOffsetUsec);
  SchBleCalcAdvOpDuration(pOp);

  if (SchInsertAtDueTime(pOp, NULL))
  {
    pExtScanCtx->auxOpPending = TRUE;
  }
  else
  {
    LL_TRACE_WARN1("Fail to schedule auxiliary scan, scanHandle=%u", LCTR_GET_EXT_SCAN_HANDLE(pExtScanCtx));
  }
}


/*************************************************************************************************/
/*!
 *  \brief      Initialize link layer controller resources for scanning master.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void LctrMstExtScanInit(void)
{
  /* Add extended scan message dispatchers. */
  lctrResetHdlrTbl[LCTR_DISP_EXT_SCAN] = lctrMstExtScanResetHandler;

  /* Add extended scan message dispatchers. */
  lctrMsgDispTbl[LCTR_DISP_EXT_SCAN] = (LctrMsgDisp_t)lctrMstExtScanDisp;

  /* Add extended scan event dispatchers. */
  lctrEventHdlrTbl[LCTR_EVENT_RX_ADVB] = lctrMstSendPendingAdvRptHandler;

  LctrMstExtScanDefaults();
}

/*************************************************************************************************/
/*!
 *  \brief      Set default values for scanning master.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void LctrMstExtScanDefaults(void)
{
  static const LlScanParam_t defScanParam =
  {
    /*.scanInterval */    0x0010,
    /*.scanWindow */      0x0010,
    /*.scanType */        LL_SCAN_PASSIVE,
    /*.ownAddrType */     LL_ADDR_PUBLIC,
    /*.scanFiltPolicy */  LL_SCAN_FILTER_NONE
  };
  unsigned int i;
  lctrMsgHdr_t *pMsg;

  memset(&lctrMstExtScanTbl, 0, sizeof(lctrMstExtScanTbl));
  memset(&lctrMstExtScan, 0, sizeof(lctrMstExtScan));

  lmgrCb.numExtScanPhys = 1;
  lctrMstExtScanTbl[LCTR_SCAN_PHY_1M].scanParam = defScanParam;
  lctrMstExtScan.enaPhys = 1 << LCTR_SCAN_PHY_1M;

  /* Assign buffers. */
  for (i = 0; i < LCTR_SCAN_PHY_TOTAL; i++)
  {
    lctrMstExtScanTbl[i].pExtAdvData = lctrMstExtScanDataBufTbl[i];
  }

  /* Setup timers. */
  lctrMstExtScan.tmrScanDur.handlerId = lmgrPersistCb.handlerId;
  pMsg = (lctrMsgHdr_t *)&lctrMstExtScan.tmrScanDur.msg;
  /* pMsg->handle = 0; */   /* Unused. */
  pMsg->dispId = LCTR_DISP_EXT_SCAN;
  pMsg->event = LCTR_EXT_SCAN_MSG_TMR_DUR_EXP;
  lctrMstExtScan.tmrScanPer.handlerId = lmgrPersistCb.handlerId;
  pMsg = (lctrMsgHdr_t *)&lctrMstExtScan.tmrScanPer.msg;
  /* pMsg->handle = 0; */   /* Unused. */
  pMsg->dispId = LCTR_DISP_EXT_SCAN;
  pMsg->event = LCTR_EXT_SCAN_MSG_TMR_PER_EXP;
}

/*************************************************************************************************/
/*!
 *  \brief  Initialize extended scanner memory resources.
 *
 *  \param  pFreeMem    Pointer to free memory.
 *  \param  freeMemSize Size of pFreeMem.
 *
 *  \return Amount of free memory consumed.
 */
/*************************************************************************************************/
uint16_t LctrInitExtScanMem(uint8_t *pFreeMem, uint32_t freeMemSize)
{
  uint8_t *pAvailMem = pFreeMem;
  unsigned int i;

  /* Extended Scanning requires receiving at least 251 bytes. */
  WSF_ASSERT(pLctrRtCfg->maxExtScanDataLen >= LL_EXT_ADVBU_MAX_LEN);


  /* Allocate extended scan buffer memory. */
  for (i = 0; i < LCTR_SCAN_PHY_TOTAL; i++)
  {
    if (((uint32_t)pAvailMem) & 3)
    {
      /* Align to next word. */
      pAvailMem = (uint8_t *)(((uint32_t)pAvailMem & ~3) + sizeof(uint32_t));
    }

    lctrMstExtScanDataBufTbl[i] = pAvailMem;
    pAvailMem += pLctrRtCfg->maxExtScanDataLen;
  }

  /* Allocate periodic scan buffer memory. */
  for (i = 0; i < LL_MAX_PER_SCAN; i++)
  {
    if (((uint32_t)pAvailMem) & 3)
    {
      /* Align to next word. */
      pAvailMem = (uint8_t *)(((uint32_t)pAvailMem & ~3) + sizeof(uint32_t));
    }

    lctrMstPerScanDataBufTbl[i] = pAvailMem;
    pAvailMem += pLctrRtCfg->maxExtScanDataLen;
  }

  if (((uint32_t)(pAvailMem - pFreeMem)) > freeMemSize)
  {
    LL_TRACE_ERR2("LctrInitExtScanMem: failed to allocate scan buffers, need=%u available=%u", (pAvailMem - pFreeMem), freeMemSize);
    WSF_ASSERT(FALSE);
    return 0;
  }

  return (pAvailMem - pFreeMem);
}

/*************************************************************************************************/
/*!
 *  \brief      Validate all scan parameters.
 *
 *  \return     TRUE if parameters are valid, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t LctrMstExtScanValidateParam(void)
{
  if (!lmgrCb.bdAddrRndValid)
  {
    unsigned int i;

    /* Check for use of random address. */
    for (i = 0; i < LCTR_SCAN_PHY_TOTAL; i++)
    {
      if (lctrMstExtScan.enaPhys & (1 << i))
      {
        if (!LmgrIsAddressTypeAvailable(lctrMstExtScanTbl[i].scanParam.ownAddrType))
        {
          LL_TRACE_WARN1("Address type invalid or not available, ownAddrType=%u", lctrMstExtScanTbl[i].scanParam.ownAddrType);
          return FALSE;
        }
      }
    }
  }

  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief      Set enabled scanning PHY.
 *
 *  \param      scanPhy     Enabled scanning PHY.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void LctrMstExtScanSetScanPhy(uint8_t scanPhy)
{
  WSF_ASSERT(scanPhy < LCTR_SCAN_PHY_TOTAL);

  lctrMstExtScan.enaPhys |= 1 << scanPhy;
}

/*************************************************************************************************/
/*!
 *  \brief      Clear (disable) scanning PHY.
 *
 *  \param      scanPhy     Disabled scanning PHY.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void LctrMstExtScanClearScanPhy(uint8_t scanPhy)
{
  WSF_ASSERT(scanPhy < LCTR_SCAN_PHY_TOTAL);

  lctrMstExtScan.enaPhys &= ~(1 << scanPhy);
}

/*************************************************************************************************/
/*!
 *  \brief      Set extended scan parameters.
 *
 *  \param      scanPhy         Extended scanning PHY.
 *  \param      ownAddrType     Address type used by this device.
 *  \param      scanFiltPolicy  Scan filter policy.
 *  \param      pParam          Extended scanning parameters.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void LctrMstExtScanSetParam(uint8_t scanPhy, uint8_t ownAddrType, uint8_t scanFiltPolicy, const LlExtScanParam_t *pParam)
{
  WSF_ASSERT(scanPhy < LCTR_SCAN_PHY_TOTAL);

  lctrMstExtScanTbl[scanPhy].scanParam.scanInterval = pParam->scanInterval;
  lctrMstExtScanTbl[scanPhy].scanParam.scanWindow = pParam->scanWindow;
  lctrMstExtScanTbl[scanPhy].scanParam.scanType = pParam->scanType;
  lctrMstExtScanTbl[scanPhy].scanParam.ownAddrType = ownAddrType;
  lctrMstExtScanTbl[scanPhy].scanParam.scanFiltPolicy = scanFiltPolicy;
}

/*************************************************************************************************/
/*!
 *  \brief  Send internal extended scan subsystem message.
 *
 *  \param  pExtScanCtx Extended scan context.
 *  \param  event       Extended scan event.
 *
 *  \return None.
 */
/*************************************************************************************************/
void lctrSendExtScanMsg(lctrExtScanCtx_t *pExtScanCtx, uint8_t event)
{
  lctrMsgHdr_t *pMsg;

  if ((pMsg = WsfMsgAlloc(sizeof(lctrMsgHdr_t))) != NULL)
  {
    pMsg->handle = (pExtScanCtx) ? LCTR_GET_EXT_SCAN_HANDLE(pExtScanCtx) : LCTR_SCAN_PHY_ALL;
    pMsg->dispId = LCTR_DISP_EXT_SCAN;
    pMsg->event = event;

    WsfMsgSend(lmgrPersistCb.handlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Send internal periodic create sync subsystem message.
 *
 *  \param  event       Periodic scan event.
 *
 *  \return None.
 */
/*************************************************************************************************/
void lctrSendCreateSyncMsg(uint8_t event)
{
  lctrMsgHdr_t *pMsg;

  if ((pMsg = WsfMsgAlloc(sizeof(lctrMsgHdr_t))) != NULL)
  {
    pMsg->dispId = LCTR_DISP_PER_CREATE_SYNC;
    pMsg->event = event;

    WsfMsgSend(lmgrPersistCb.handlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Send internal periodic advertising subsystem message.
 *
 *  \param  pCtx        Periodic scanning context.
 *  \param  event       Periodic advertising event.
 *
 *  \return None.
 */
/*************************************************************************************************/
void lctrSendPerScanMsg(lctrPerScanCtx_t *pCtx, uint8_t event)
{
  lctrMsgHdr_t *pMsg;

  if ((pMsg = WsfMsgAlloc(sizeof(lctrMsgHdr_t))) != NULL)
  {
    pMsg->handle = LCTR_GET_PER_SCAN_HANDLE(pCtx);
    pMsg->dispId = LCTR_DISP_PER_SCAN;
    pMsg->event = event;

    WsfMsgSend(lmgrPersistCb.handlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Check if create sync is pending or not.
 *
 *  \return     TRUE if create sync is pending, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t LctrMstPerIsSyncPending(void)
{
  return (lctrPerCreateSync.state == LCTR_CREATE_SYNC_STATE_DISCOVER);
}

/*************************************************************************************************/
/*!
 *  \brief      Check if create sync is disabled.
 *
 *  \return     TRUE if create sync is disabled, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t LctrMstPerIsSyncDisabled(void)
{
  return (lctrPerCreateSync.state == LCTR_CREATE_SYNC_STATE_DISABLED);
}

/*************************************************************************************************/
/*!
 *  \brief      Check if scanner is already sync with the same advertiser with the same advertising
 *              set.
 *
 *  \param      advSID      Advertising SID.
 *  \param      advAddrType Advertiser address type.
 *  \param      advAddr     Advertiser address.
 *
 *  \return     TRUE if already sync, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t LctrMstPerIsSync(uint8_t advSID, uint8_t advAddrType, uint64_t advAddr)
{
  unsigned int index;
  for (index = 0; index < LL_MAX_PER_SCAN; index++)
  {
    if (lctrMstPerScanTbl[index].enabled)
    {
      lctrPerScanCtx_t *pPerScanCtx = LCTR_GET_PER_SCAN_CTX(index);

      if ((pPerScanCtx->advSID == advSID) &&
          (pPerScanCtx->advAddrType == advAddrType) &&
          (pPerScanCtx->advAddr == advAddr))
      {
        return TRUE;
      }
    }
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief  Initialize link layer controller resources for master create sync.
 *
 *  \return None.
 */
/*************************************************************************************************/
void LctrMstPerCreateSyncInit(void)
{
  /* Add create sync reset handler. */
  lctrResetHdlrTbl[LCTR_DISP_PER_CREATE_SYNC] = NULL;   /* Not needed. */

  /* Add create sync task message dispatchers. */
  lctrMsgDispTbl[LCTR_DISP_PER_CREATE_SYNC] = (LctrMsgDisp_t)lctrMstCreateSyncDisp;

  /* Set supported features. */
  if (pLctrRtCfg->btVer >= LL_VER_BT_CORE_SPEC_5_0)
  {
    lmgrPersistCb.featuresDefault |= LL_FEAT_LE_PER_ADV;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Initialize link layer controller resources for master periodic scanning.
 *
 *  \return None.
 */
/*************************************************************************************************/
void LctrMstPerScanInit(void)
{
  /* Add periodic scanning reset handler. */
  lctrResetHdlrTbl[LCTR_DISP_PER_SCAN] = lctrMstPerScanResetHandler;

  /* Add periodic scanning task message dispatchers. */
  lctrMsgDispTbl[LCTR_DISP_PER_SCAN] = (LctrMsgDisp_t)lctrMstPerScanDisp;

  /* Set supported features. */
  if (pLctrRtCfg->btVer >= LL_VER_BT_CORE_SPEC_5_0)
  {
    lmgrPersistCb.featuresDefault |= LL_FEAT_LE_PER_ADV;
  }

  lmgrPersistCb.perScanCtxSize = sizeof(lctrPerScanCtx_t);
}

/*************************************************************************************************/
/*!
 *  \brief  Build periodic scan operation.
 *
 *  \param  pPerScanCtx     Periodic scan context.
 *  \param  pMsg            Periodic create sync message.
 *
 *  \return Status error code.
 */
/*************************************************************************************************/
uint8_t lctrMstPerScanBuildOp(lctrPerScanCtx_t *pPerScanCtx, lctrPerCreateSyncMsg_t *pMsg)
{
  BbOpDesc_t * const pOp = &pPerScanCtx->bod;
  BbBleData_t * const pBle = &pPerScanCtx->bleData;
  BbBleMstPerScanEvent_t * const pPerScan = &pBle->op.mstPerScan;

  memset(pOp, 0, sizeof(BbOpDesc_t));
  memset(pBle, 0, sizeof(BbBleData_t));

  /*** General Setup ***/

  pOp->reschPolicy = BB_RESCH_PERIODIC;
  pOp->protId = BB_PROT_BLE;
  pOp->prot.pBle = pBle;
  pOp->endCback = lctrMstPerScanEndOp;
  pOp->abortCback = lctrMstPerScanAbortOp;
  pOp->pCtx = pPerScanCtx;

  /*** BLE General Setup ***/
  pBle->chan.opType = BB_BLE_OP_MST_PER_SCAN_EVENT;

  /*** BLE Scan Setup: Rx advertising packet ***/
  /*** BLE Scan Setup: Rx chain packet ***/
  pPerScan->rxPerAdvCback = lctrMstPerScanRxPerAdvPktHandler;
  pPerScan->rxPerAdvPostCback = lctrMstPerScanRxPerAdvPktPostHandler;

  pPerScanCtx->shutdown = FALSE;

  /* Defer scheduling until SyncInfo is received. */

  return LL_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief  Commit periodic scan discovery operation.
 *
 *  \param  pExtScanCtx     Extended scan context.
 *  \param  pAuxPtr         Auxiliary Pointer.
 *  \param  pSyncInfo       Sync info.
 *  \param  startTs         Start of AUX_ADV_IND packet (offset origin).
 *  \param  endTs           End of AUX_ADV_IND packet.
 *
 *  \return None.
 */
/*************************************************************************************************/
void lctrMstPerScanOpCommit(lctrExtScanCtx_t *pExtScanCtx, lctrAuxPtr_t *pAuxPtr,
                            lctrSyncInfo_t *pSyncInfo, uint32_t startTs, uint32_t endTs)
{
  uint32_t syncOffsetUsec, offsetUsec, caPpm, wwUsec;
  uint16_t dueOffsetUsec;
  uint16_t numUnsyncIntervals = 0;

  /* Pre-resolve common structures for efficient access. */
  lctrPerScanCtx_t *pPerScanCtx = lctrPerCreateSync.pPerScanCtx;
  BbOpDesc_t * const pOp = &pPerScanCtx->bod;
  BbBleData_t * const pBle = &pPerScanCtx->bleData;

  /*** BLE General Setup for Channel ***/

  pPerScanCtx->chanParam.chanMask = pSyncInfo->chanMap;
  pPerScanCtx->chanParam.usedChSel = LL_CH_SEL_2;

  lctrPeriodicBuildRemapTable(&pPerScanCtx->chanParam);
  pPerScanCtx->chanParam.chIdentifier = (pSyncInfo->accAddr >> 16) ^
                                        (pSyncInfo->accAddr >> 0);

  pBle->chan.chanIdx = lctrPeriodicSelectNextChannel(&pPerScanCtx->chanParam, pPerScanCtx->eventCounter);

  if (pExtScanCtx->extAdvHdr.extHdrFlags & LL_EXT_HDR_TX_PWR_BIT)
  {
    pBle->chan.txPower = pExtScanCtx->extAdvHdr.txPwr;
  }

  pBle->chan.accAddr = pSyncInfo->accAddr;
  pBle->chan.crcInit = pSyncInfo->crcInit;
  /* FIXME: Only if scan enabled */
  pBle->chan.txPhy = pBle->chan.rxPhy = lctrConvertAuxPtrPhyToBbPhy(pAuxPtr->auxPhy);

#if (LL_ENABLE_TESTER == TRUE)
  pBle->chan.accAddrRx = llTesterCb.advAccessAddrRx ^ pBle->chan.accAddr;
  pBle->chan.accAddrTx = llTesterCb.advAccessAddrTx ^ pBle->chan.accAddr;
  pBle->chan.crcInitRx = llTesterCb.advCrcInitRx ^ pBle->chan.crcInit;
  pBle->chan.crcInitTx = llTesterCb.advCrcInitTx ^ pBle->chan.crcInit;
#endif

  /*** Commit operation ***/
  offsetUsec = pSyncInfo->syncOffset * ((pSyncInfo->offsetUnits == LCTR_OFFS_UNITS_30_USEC) ? 30 : 300);
  pPerScanCtx->lastAnchorPoint = startTs + BB_US_TO_BB_TICKS(offsetUsec);
  pPerScanCtx->lastActiveEvent = pPerScanCtx->eventCounter;
  caPpm = lctrCalcTotalAccuracy(pSyncInfo->sca);
  wwUsec = lctrCalcAuxAdvWindowWideningUsec(offsetUsec, caPpm);
  syncOffsetUsec = offsetUsec - wwUsec;
  pPerScanCtx->rxSyncDelayUsec = pBle->op.mstPerScan.rxSyncDelayUsec = (wwUsec << 1) + ((pSyncInfo->offsetUnits == LCTR_OFFS_UNITS_30_USEC) ? 30 : 300);    /* rounding compensation */
  dueOffsetUsec         = (offsetUsec - wwUsec) - BB_TICKS_TO_US(BB_US_TO_BB_TICKS(offsetUsec) - BB_US_TO_BB_TICKS(wwUsec));

  if (syncOffsetUsec < LL_BLE_MAFS_US)
  {
    LL_TRACE_WARN1("Peer requested AuxPtr offset does not meet T_MAFS, actual afsUsec=%u", BB_US_TO_BB_TICKS(pOp->due - endTs));
    return;
  }

  pOp->due = startTs + BB_US_TO_BB_TICKS(syncOffsetUsec);
  pOp->dueOffsetUsec = WSF_MAX(dueOffsetUsec, 0);
  SchBleCalcAdvOpDuration(pOp);
  pPerScanCtx->minDurUsec = pOp->minDurUsec;

  while (TRUE)
  {
    uint32_t unsyncTimeUsec, unsyncTime,wwTotalUsec, wwTotal;
    if (SchInsertAtDueTime(pOp, NULL))
    {
      LL_TRACE_INFO1("    >>> Periodic scan started, handle=%u <<<", LCTR_GET_PER_SCAN_HANDLE(pPerScanCtx));
      LL_TRACE_INFO1("                               pOp=%08x", pOp);
      LL_TRACE_INFO1("                               due=%u", pOp->due);
      LL_TRACE_INFO1("                               eventCounter=%u", pPerScanCtx->eventCounter);
      LL_TRACE_INFO1("                               pBle->chan.chanIdx=%u", pBle->chan.chanIdx);

      break;
    }

    LL_TRACE_WARN0("!!! Start periodic scanning schedule conflict");

    pPerScanCtx->eventCounter++;
    pBle->chan.chanIdx = lctrPeriodicSelectNextChannel(&pPerScanCtx->chanParam, pPerScanCtx->eventCounter);
    numUnsyncIntervals++;

    unsyncTimeUsec = BB_TICKS_TO_US(pPerScanCtx->perInter * numUnsyncIntervals);
    unsyncTime     = BB_US_TO_BB_TICKS(unsyncTimeUsec);
    wwTotalUsec    = lctrCalcAuxAdvWindowWideningUsec(unsyncTimeUsec, caPpm);
    wwTotal        = BB_US_TO_BB_TICKS(wwTotalUsec);
    dueOffsetUsec           = (unsyncTimeUsec - wwTotalUsec) - BB_TICKS_TO_US(unsyncTime - wwTotal);

    /* Advance to next interval. */
    pOp->due = pPerScanCtx->lastAnchorPoint + unsyncTime - wwTotal;
    pOp->dueOffsetUsec = WSF_MAX(dueOffsetUsec, 0);
    pOp->minDurUsec = pPerScanCtx->minDurUsec + wwTotalUsec;
    pBle->op.mstPerScan.rxSyncDelayUsec = pPerScanCtx->rxSyncDelayUsec + (wwTotalUsec << 1);
  }

  lctrPerCreateSync.createSyncPending = TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief  Allocate a periodic scan context.
 *
 *  \return Periodic sync context.
 */
/*************************************************************************************************/
lctrPerScanCtx_t *lctrAllocPerScanCtx(void)
{
  unsigned int index;
  for (index = 0; index < LL_MAX_PER_SCAN; index++)
  {
    if (!lctrMstPerScanTbl[index].enabled)
    {
      lctrMsgHdr_t *pMsg;
      lctrPerScanCtx_t *pCtx = LCTR_GET_PER_SCAN_CTX(index);

      memset(&lctrMstPerScanTbl[index], 0, sizeof(lctrPerScanCtx_t));

      pCtx->enabled = TRUE;

      /* Setup periodic advertising data buffer. */
      pCtx->pPerAdvData = lctrMstPerScanDataBufTbl[index];

      /* Setup supervision timer. */
      pCtx->tmrSupTimeout.handlerId = lmgrPersistCb.handlerId;
      pMsg = (lctrMsgHdr_t *)&pCtx->tmrSupTimeout.msg;
      pMsg->handle = index;
      pMsg->dispId = LCTR_DISP_PER_SCAN;
      pMsg->event = LCTR_PER_SCAN_SUP_TIMEOUT;

      /* Update once PHY is known). */
      pCtx->bleData.chan.txPhy = pCtx->bleData.chan.rxPhy = BB_PHY_BLE_1M;

      /* Default PHY. */
      pCtx->rxPhys = lmgrConnCb.rxPhys;

      return pCtx;
    }
  }

  return NULL;
}

/*************************************************************************************************/
/*!
 *  \brief      Convert PHY value to PHY value in extended advertising report.
 *
 *  \param      auxPtrPhy       Auxiliary Pointer PHY field.
 *
 *  \return     LL PHY value.
 */
/*************************************************************************************************/
uint8_t lctrConvertAuxPtrPhyToAdvRptPhy(uint8_t auxPtrPhy)
{
  switch (auxPtrPhy)
  {
    case 0: /* LE_1M */
    default:
      return LL_PHY_LE_1M;
    case 1: /* LE_2M */
      return LL_PHY_LE_2M;
    case 2: /* LE_Coded */
      return LL_PHY_LE_CODED;
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Convert AuxPtr PHY value to PHY value in extended advertising report.
 *
 *  \param      auxPtrPhy       Auxiliary Pointer PHY field.
 *
 *  \return     BB PHY value.
 */
/*************************************************************************************************/
uint8_t lctrConvertAuxPtrPhyToBbPhy(uint8_t auxPtrPhy)
{
  switch (auxPtrPhy)
  {
    case 0: /* LE_1M */
    default:
      return BB_PHY_BLE_1M;
    case 1: /* LE_2M */
      return BB_PHY_BLE_2M;
    case 2: /* LE_Coded */
      return BB_PHY_BLE_CODED;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Compute the connection interval window widening delay in microseconds.
 *
 *  \param  unsyncTimeUsec  Unsynchronized time in microseconds.
 *  \param  caPpm           Total clock accuracy.
 *
 *  \return Window widening delay in microseconds.
 */
/*************************************************************************************************/
uint32_t lctrCalcAuxAdvWindowWideningUsec(uint32_t unsyncTimeUsec, uint32_t caPpm)
{
  if (lctrGetOpFlag(LL_OP_MODE_FLAG_ENA_WW))
  {
    /* Largest unsynchronized time is 1,996 seconds (interval=4s and latency=499) and
     * largest total accuracy is 1000 ppm. */
    /* coverity[overflow_before_widen] */
    uint64_t wwDlyUsec = LL_MATH_DIV_10E6(((uint64_t)unsyncTimeUsec * caPpm) +
                                          999999);     /* round up */

    /* Reduce to 32-bits and always round up to a sleep clock tick. */
    return wwDlyUsec + pLctrRtCfg->ceJitterUsec;
  }
  else
  {
    return 0;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Compute auxiliary offset.
 *
 *  \param  pAuxPtr         Auxiliary Pointer.
 *  \param  pOffsetUsec     Return auxiliary offset in microseconds.
 *  \param  pSyncDelayUsec  Return synchronization delay in microseconds.
 *
 *  \return None.
 */
/*************************************************************************************************/
void lctrMstComputeAuxOffset(lctrAuxPtr_t *pAuxPtr, uint32_t *pOffsetUsec, uint32_t *pSyncDelayUsec)
{
  uint32_t offsetUsec = pAuxPtr->auxOffset * ((pAuxPtr->offsetUnits == LCTR_OFFS_UNITS_30_USEC) ? 30 : 300);
  uint32_t caPpm = BbGetClockAccuracy() + ((pAuxPtr->ca == LCTR_CLK_ACC_0_50_PPM) ? 50 : 500);
  uint32_t wwUsec = lctrCalcAuxAdvWindowWideningUsec(offsetUsec, caPpm);

  *pOffsetUsec = offsetUsec - wwUsec;
  *pSyncDelayUsec = (wwUsec << 1) + ((pAuxPtr->offsetUnits == LCTR_OFFS_UNITS_30_USEC) ? 30 : 300);    /* rounding compensation */
}
