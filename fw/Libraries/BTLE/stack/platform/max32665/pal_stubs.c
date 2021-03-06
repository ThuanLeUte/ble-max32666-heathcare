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
 *  \brief PAL stub functions.
 */
/*************************************************************************************************/

#include "stack/platform/include/pal_types.h"
#include "stack/platform/include/pal_led.h"
#include "stack/platform/include/pal_uart.h"

void PalLedOn(uint8_t id){}
void PalLedOff(uint8_t id){}

void PalUartInit(PalUartId_t id, const PalUartConfig_t *pCfg){}
void PalUartDeInit(PalUartId_t id){}
PalUartState_t PalUartGetState(PalUartId_t id){return PAL_UART_STATE_UNINIT;}
void PalUartReadData(PalUartId_t id, uint8_t *pData, uint16_t len){}
void PalUartWriteData(PalUartId_t id, const uint8_t *pData, uint16_t len){}
