/**
 * @file       bsp_sh.c
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-03-24
 * @author     Thuan Le
 * @brief      Board support package for Sensor Hub
 * @note       None
 * @example    None
 */

/* Includes ----------------------------------------------------------- */
#include "bsp_sh.h"
#include "bsp.h"

/* Private defines ---------------------------------------------------- */
/* Private enumerate/structure ---------------------------------------- */
/* Private macros ----------------------------------------------------- */
/* Public variables --------------------------------------------------- */
/* Private variables -------------------------------------------------- */
static max32664_t m_max32664;

/* Private function prototypes ---------------------------------------- */
/* Function definitions ----------------------------------------------- */
base_status_t bsp_sh_init(void)
{
  m_max32664.device_address = MAX32664_I2C_ADDR;
  m_max32664.i2c_read       = bsp_i2c_read;
  m_max32664.i2c_write      = bsp_i2c_write;
  m_max32664.delay          = bsp_delay;
  m_max32664.gpio_write     = bsp_gpio_write;

  max32664_init(&m_max32664);

  return max32664_config_bpm(&m_max32664, MODE_ONE);
}

base_status_t bsp_sh_get_sensor_value(uint8_t *spo2, uint8_t *heart_rate)
{
  max32664_read_bpm(&m_max32664);

  *spo2 = m_max32664.bio_data.oxygen;
  *heart_rate = m_max32664.bio_data.heart_rate;

  return BS_OK;
}

/* Private function definitions ---------------------------------------- */
/* End of file -------------------------------------------------------- */
