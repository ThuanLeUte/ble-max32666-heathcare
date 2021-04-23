/**
 * @file       bsp_temp.c
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-03-24
 * @author     Thuan Le
 * @brief      Board support package for digital temperature sensor
 * @note       None
 * @example    None
 */

/* Includes ----------------------------------------------------------- */
#include "bsp_temp.h"

/* Private defines ---------------------------------------------------- */
/* Private enumerate/structure ---------------------------------------- */
/* Private macros ----------------------------------------------------- */
/* Public variables --------------------------------------------------- */
/* Private variables -------------------------------------------------- */
static max30208_t m_max30208;

/* Private function prototypes ---------------------------------------- */
/* Function definitions ----------------------------------------------- */
base_status_t bsp_temp_init(void)
{
  m_max30208.device_address = MAX30208_I2C_ADDR;
  m_max30208.i2c_read = bsp_i2c_read;
  m_max30208.i2c_write = bsp_i2c_write;

  max30208_init(&m_max30208);

  return max30208_start_convert(&m_max30208);
}

base_status_t bsp_temp_get(float *temp)
{
  uint8_t status = 0;
  
  max30208_get_fifo_available(&m_max30208);

  max30208_get_interrupt_status(&m_max30208, &status);

  if (status & MAX30208_INT_ENA_TEMP_RDY)
  {
    printf("DATA READY %d \n",status);
  }

  return max30208_get_temperature(&m_max30208, temp);
}

/* Private function definitions ---------------------------------------- */
/* End of file -------------------------------------------------------- */
