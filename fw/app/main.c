/**
 * @file       main.c
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-01-07
 * @author     Thuan Le
 * @brief      Main file
 * @note       None
 * @example    None
 */

/* Includes ----------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "mxc_config.h"
#include "wsf_types.h"
#include "wsf_os.h"
#include "wsf_buf.h"
#include "wsf_timer.h"
#include "wsf_trace.h"
#include "app_ui.h"
#include "app_ui.h"
#include "hci_vs.h"
#include "hci_core.h"
#include "hci_drv_sdma.h"
#include "bb_drv.h"
#include "board.h"
#include "ipc_defs.h"
#include "tmr_utils.h"

#include "bsp.h"
#include "bsp_temp.h"
#include "bsp_sh.h"
#include "ble_main.h"

/* Private defines ---------------------------------------------------- */
#define WSF_BUF_SIZE      (0x1048)
#define WSF_BUF_POOLS 	  (6)

/* Private enumerate/structure ---------------------------------------- */
/* Private macros ----------------------------------------------------- */
/* Private variables -------------------------------------------------- */
uint32_t SystemHeapSize = WSF_BUF_SIZE;
uint32_t SystemHeap[WSF_BUF_SIZE / 4];
uint32_t SystemHeapStart;

// Default pool descriptor
static wsfBufPoolDesc_t mainPoolDesc[WSF_BUF_POOLS] =
{
  { 16,  8 },
  { 32,  4 },
  { 64,  4 },
  { 128, 4 },
  { 256, 4 },
  { 512, 4 }
};

/* Private function prototypes ---------------------------------------- */
/* Function definitions ----------------------------------------------- */

// Stack initialization for app
extern void ble_stack_init(void);

/*************************************************************************************************/
void SysTick_Handler(void)
{
  WsfTimerUpdate(WSF_MS_PER_TICK);
}

/*************************************************************************************************/
static bool_t m_my_trace(const uint8_t *pBuf, uint32_t len)
{
  extern uint8_t wsfCsNesting;

  if (wsfCsNesting == 0)
  {
    fwrite(pBuf, len, 1, stdout);
    return TRUE;
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief  Initialize WSF.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void m_wsf_init(void)
{
  uint32_t bytesUsed;
  /* setup the systick for 1MS timer*/
  SysTick->LOAD = (SystemCoreClock / 1000) * WSF_MS_PER_TICK;
  SysTick->VAL = 0;
  SysTick->CTRL |= (SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk);

  WsfTimerInit();

  SystemHeapStart = (uint32_t)&SystemHeap;
  memset(SystemHeap, 0, sizeof(SystemHeap));
  printf("SystemHeapStart = 0x%x\n", SystemHeapStart);
  printf("SystemHeapSize = 0x%x\n", SystemHeapSize);
  bytesUsed = WsfBufInit(WSF_BUF_POOLS, mainPoolDesc);
  printf("bytesUsed = 0x%x\n", bytesUsed);

  WsfTraceRegisterHandler(m_my_trace);
  WsfTraceEnable(TRUE);
}

/*
 * In two-chip solutions, setting the address must wait until the HCI interface is initialized.
 * This handler can also catch other Application events, but none are currently implemented.
 * See ble-profiles/sources/apps/app/common/app_ui.c for further details.
 *
 */
void m_set_address(uint8_t event)
{
  uint8_t bdAddr[6] = {0x02, 0x00, 0x44, 0x8B, 0x05, 0x00};

  switch (event)
  {
  case APP_UI_RESET_CMPL:
    printf("Setting address -- MAC %02X:%02X:%02X:%02X:%02X:%02X\n", bdAddr[5], bdAddr[4], bdAddr[3], bdAddr[2], bdAddr[1], bdAddr[0]);
    HciVsSetBdAddr(bdAddr);
    break;
  default:
    break;
  }
}

/*************************************************************************************************/
/*!
 *  \fn     main
 *
 *  \brief  Entry point for demo software.
 *
 *  \param  None.
 *
 *  \return None.
 */
/*************************************************************************************************/
int main(void)
{
  printf("\n\n***** MAX32665 BLE Data Server *****\n");

  // Initialize Radio
  m_wsf_init();

  ble_stack_init();
  ble_start();

  // Register a handler for Application events
  AppUiActionRegister(m_set_address);

  printf("Setup Complete\n");

  bsp_init();
  bsp_temp_init();

  // bsp_sh_init();
  // uint8_t spo2 = 0;
  // uint8_t heart_rate = 0;

  // while (1)
  // {
  //     float temp = 0;
  //     bsp_temp_get(&temp);
  //     printf("Temmperature: %f \n", (double)temp);

  //     // bsp_sh_get_sensor_value(&spo2, &heart_rate);
  //     // printf("Spo2: %d \n", (uint8_t)spo2);
  //     // printf("Heart rate: %d \n", (uint8_t)heart_rate);

  //     bsp_delay(1000);
  // }

  while (1)
  {
    wsfOsDispatcher();
  }
}

/*****************************************************************/
void HardFault_Handler(void)
{
  printf("\nFaultISR: CFSR %08X, BFAR %08x\n", (unsigned int)SCB->CFSR, (unsigned int)SCB->BFAR);

  // Loop forever
  while (1)
    ;
}
