/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Wireless Data Exchange profile implementation - File Example.
 *
 *  Copyright (c) 2013-2018 Arm Ltd. All Rights Reserved.
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
#include "max32665.h"
#include "wsf_types.h"
#include "util/wstr.h"
#include "wsf_trace.h"
#include "wsf_assert.h"
#include "wsf_efs.h"
#include "util/bstream.h"
#include "svc_wdxs.h"
#include "wdxs/wdxs_api.h"
#include "wdxs/wdxs_main.h"
#include "wdxs_file.h"
#include "dm_api.h"
#include "att_api.h"
#include "app_api.h"
#include "flc.h"
#include "hash.h"

#define SHA256_BYTES            (256/8)

#define FLASH_START_ADDR        MXC_FLASH1_MEM_BASE
#define FLASH_END_ADDR          (MXC_FLASH1_MEM_BASE + MXC_FLASH_MEM_SIZE)

static uint32_t verifyLen;

/* Prototypes for file functions */
static uint8_t wdxsFileInitMedia(void);
static uint8_t wdxsFileErase(uint32_t address, uint32_t size);
static uint8_t wdxsFileRead(uint8_t *pBuf, uint32_t address, uint32_t len);
static uint8_t wdxsFileWrite(const uint8_t *pBuf, uint32_t address, uint32_t size);
static uint8_t wsfFileHandle(uint8_t cmd, uint32_t param);

/* Use the second half of the flash space for scratch space */
static const wsfEfsMedia_t WDXS_FileMedia =
{
  /*   uint32_t                startAddress;  Start address. */                   FLASH_START_ADDR,
  /*   uint32_t                endAddress;    End address. */                     FLASH_END_ADDR,
  /*   uint32_t                pageSize;      Page size. */                       MXC_FLASH_PAGE_SIZE,
  /*   wsfMediaInitFunc_t      *init;         Media intialization callback. */    wdxsFileInitMedia,
  /*   wsfMediaEraseFunc_t     *erase;        Media erase callback. */            wdxsFileErase,
  /*   wsfMediaReadFunc_t      *read;         Media read callback. */             wdxsFileRead,
  /*   wsfMediaWriteFunc_t     *write;        Media write callback. */            wdxsFileWrite,
  /*   wsfMediaHandleCmdFunc_t *handleCmd;    Media command handler callback. */  wsfFileHandle
};

/*************************************************************************************************/
/*!
 *  \brief  Media Init function, called when media is registered.
 *
 *  \return Status of the operation.
 */
/*************************************************************************************************/
static uint8_t wdxsFileInitMedia(void)
{
  return wdxsFileErase(WDXS_FileMedia.startAddress, WDXS_FileMedia.endAddress - WDXS_FileMedia.startAddress);
}

/*************************************************************************************************/
/*!
 *  \brief  File erase function. Must be page aligned.
 *
 *  \param  address  Address in media to start erasing.
 *  \param  size     Number of bytes to erase.
 *
 *  \return Status of the operation.
 */
/*************************************************************************************************/
static uint8_t wdxsFileErase(uint32_t address, uint32_t size)
{
  /* See if we can mass erase one of the flash arrays */
  if((address == MXC_FLASH1_MEM_BASE) && (size = MXC_FLASH_MEM_SIZE)) {
    if(FLC_MassEraseInst(1) == E_NO_ERROR) {
      return WSF_EFS_SUCCESS;
    }
  }
  /* Page erase the flash sections */
  if(FLC_MultiPageErase(address, address+size) == E_NO_ERROR) {
    return WSF_EFS_SUCCESS;
  }
  return WSF_EFS_FAILURE;
}

/*************************************************************************************************/
/*!
 *  \brief  File Read function.
 *
 *  \param  pBuf     Buffer to hold data.
 *  \param  address  Address in media to read from.
 *  \param  size     Size of pBuf in bytes.
 *
 *  \return Status of the operation.
 */
/*************************************************************************************************/
static uint8_t wdxsFileRead(uint8_t *pBuf, uint32_t address, uint32_t len)
{
  memcpy(pBuf, (uint32_t*)address, len);
  return WSF_EFS_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief  File Write function.
 *
 *  \param  pBuf     Buffer with data to be written.
 *  \param  address  Address in media to write to.
 *  \param  size     Size of pBuf in bytes.
 *
 *  \return Status of the operation.
 */
/*************************************************************************************************/
static uint8_t wdxsFileWrite(const uint8_t *pBuf, uint32_t address, uint32_t size)
{
  if(FLC_Write(address, size, (uint32_t *)pBuf) == E_NO_ERROR) {
    return WSF_EFS_SUCCESS;
  }
  return WSF_EFS_FAILURE;
}

/*************************************************************************************************/
/*!
 *  \brief  Media Specific Command handler.
 *
 *  \param  cmd    Identifier of the media specific command.
 *  \param  param  Optional Parameter to the command.
 *
 *  \return Status of the operation.
 */
/*************************************************************************************************/
static uint8_t wsfFileHandle(uint8_t cmd, uint32_t param)
{
  switch(cmd) {
    case WSF_EFS_WDXS_PUT_COMPLETE_CMD:
    {
      /* Currently unimplemented */
      return WDX_FTC_ST_SUCCESS;
    }
    break;
    case WSF_EFS_VALIDATE_CMD:
    default:
    { 
      /* Validate the image with SHA256, digest is last 256 bits of the file */
      /* param holds the total file length */

      char digest[SHA256_BYTES];

      /* Calculate the digest */
      TPU_Hash_Reset();
      TPU_Hash_Config(TPU_HASH_SHA256);
      TPU_SHA((const char *)WDXS_FileMedia.startAddress, TPU_HASH_SHA256, (param - SHA256_BYTES), digest);
      TPU_Hash_Shutdown();
      
      /* Check the calculated digest against what was received */
      if(memcmp(digest, (const char *)(WDXS_FileMedia.startAddress + param - SHA256_BYTES), SHA256_BYTES) != 0) {
        return WDX_FTC_ST_VERIFICATION;
      }

      /* Verification successful, save the verification length for future update */ 
      verifyLen = param;

      return WDX_FTC_ST_SUCCESS;
    }
    break;
  }
  return WDX_FTC_ST_SUCCESS;
}


/*************************************************************************************************/
/*!
 *  \brief  Example of creating a WDXS stream.
 *
 *  \param  none
 *
 *  \return None.
 */
/*************************************************************************************************/
void WdxsFileInit(void)
{
  wsfEsfAttributes_t attr;
  char versionString[WSF_EFS_VERSION_LEN];

  /* Add major number */
  versionString[0] = FW_VERSION & 0xFF;
  /* Add "." */
  versionString[1] = (FW_VERSION & 0xFF00) >> 8;
  /* Minor number */
  versionString[2] = (FW_VERSION & 0xFF0000) >> 16;
  /* Add termination character */
  versionString[3] = 0;

  /* Register the media for the stream */
  WsfEfsRegisterMedia(&WDXS_FileMedia, WDX_FLASH_MEDIA);

  /* Set the attributes for the stream */
  attr.permissions = (
    WSF_EFS_REMOTE_GET_PERMITTED |
    WSF_EFS_REMOTE_PUT_PERMITTED |
    WSF_EFS_REMOTE_ERASE_PERMITTED |
    WSF_EFS_REMOTE_VERIFY_PERMITTED |
    WSF_EFS_LOCAL_GET_PERMITTED |
    WSF_EFS_LOCAL_PUT_PERMITTED |
    WSF_EFS_LOCAL_ERASE_PERMITTED |
    WSF_EFS_REMOTE_VISIBLE);

  attr.type = WSF_EFS_FILE_TYPE_BULK;

  /* Potential buffer overrun is intentional to zero out fixed length field */
  /* coverity[overrun-buffer-arg] */
  WstrnCpy(attr.name, "File", WSF_EFS_NAME_LEN);
  /* coverity[overrun-buffer-arg] */
  WstrnCpy(attr.version, versionString, WSF_EFS_VERSION_LEN);

  /* Add a file for the stream */
  WsfEfsAddFile(WDXS_FileMedia.endAddress - WDXS_FileMedia.startAddress, WDX_FLASH_MEDIA, &attr, 0);
}


/*************************************************************************************************/
/*!
 *  \brief  Get the base address of the WDXS file.
 *
 *  \return Base address of WDXS file.
 */
/*************************************************************************************************/
uint32_t WdxsFileGetBaseAddr(void)
{
  return WDXS_FileMedia.startAddress;
}

/*************************************************************************************************/
/*!
 *  \brief  Get the length of the last verified WDXS file.
 *
 *  \return Verified length of WDXS file.
 */
/*************************************************************************************************/
uint32_t WdxsFileGetVerifiedLength(void)
{
  return verifyLen;
}
