/**
 * @file       max32664.h
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-03-22
 * @author     Thuan Le
 * @brief      Driver support MAX32664
 * @note       None
 * @example    None
 */

/* Includes ----------------------------------------------------------- */
#include "max32664.h"

/* Private defines ---------------------------------------------------- */
// Registers

/* Private enumerate/structure ---------------------------------------- */
/* Private macros ----------------------------------------------------- */

/* Public variables --------------------------------------------------- */
/* Private variables -------------------------------------------------- */
/* Private function prototypes ---------------------------------------- */
static base_status_t m_max32664_read_reg(max32664_t *me, uint8_t reg, uint8_t *p_data, uint32_t len);
static base_status_t m_max32664_write_reg(max32664_t *me, uint8_t reg, uint8_t *p_data, uint32_t len);

/* Function definitions ----------------------------------------------- */
base_status_t max32664_init(max32664_t *me)
{
  uint8_t identifier = 0x00;

  if ((me == NULL) || (me->i2c_read == NULL) || (me->i2c_write == NULL))
    return BS_ERROR;

  return BS_OK;
}

/* Private function definitions ---------------------------------------- */
/**
 * @brief         MAX32664 read register
 *
 * @param[in]     me      Pointer to handle of MAX32664 module.
 * @param[in]     reg     Register
 * @param[in]     p_data  Pointer to handle of data
 * @param[in]     len     Data length
 *
 * @attention     None
 *
 * @return
 * - BS_OK
 * - BS_ERROR
 */
static base_status_t m_max32664_read_reg(max32664_t *me, uint8_t reg, uint8_t *p_data, uint32_t len)
{
  CHECK(0 == me->i2c_read(me->device_address, reg, p_data, len), BS_ERROR);

  return BS_OK;
}


/* End of file -------------------------------------------------------- */
