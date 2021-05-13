/**
 * @file       bsp_temp.c
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-03-24
 * @author     Thuan Le
 * @brief      Board support package for digital M_temperature sensor
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
float m_max30208_calculate_temp(uint8_t msb, uint8_t lsb);

/* Function definitions ----------------------------------------------- */
base_status_t bsp_temp_init(void)
{
  m_max30208.device_address = MAX30208_I2C_ADDR;
  m_max30208.i2c_read       = bsp_i2c_read_mem;
  m_max30208.i2c_write      = bsp_i2c_write;

  max30208_init(&m_max30208);

  return max30208_start_convert(&m_max30208);
}

base_status_t bsp_temp_get(float *temp)
{
  uint8_t status;

  max30208_get_interrupt_status(&m_max30208, &status);

  if (status & MAX30208_INT_ENA_TEMP_RDY)
  {
    printf("Data ready: 0x%x \n", status);

    max30208_get_fifo_available(&m_max30208);

    if ((m_max30208.fifo_len % 2) != 0)
    {
      m_max30208.fifo_len -= 1;
    }

    max30208_get_fifo(&m_max30208);

    for (uint8_t i = 0; i  < m_max30208.fifo_len / 2; i = i + 2)
    {
      m_max30208.head++;
      m_max30208.head %= 16;
      m_max30208.temperature[m_max30208.head] = m_max30208_calculate_temp(m_max30208.fifo[i], m_max30208.fifo[i + 1]);
      printf("Buffer position: %d\n", m_max30208.head);
    }

    printf("FIFO Available: %d\n", m_max30208.fifo_len);
    printf("TEMP Available: %d\n", m_max30208.fifo_len / 2);

    max30208_get_temperature(&m_max30208, temp);

    return max30208_start_convert(&m_max30208);
  }
  else
  {
    printf("Temperature is not READY\n");
  }

  return BS_ERROR;
}

/* Private function definitions --------------------------------------- */
/**
 * @brief         Calculate tenp
 *
 * @param[in]     msb    MSB
 * @param[in]     lsb    LSB
 *
 * @attention     None
 *
 * @return        Temperature
 */
float m_max30208_calculate_temp(uint8_t msb, uint8_t lsb)
{
  if (msb & 0x80)
  {
    return 0 - (((msb << 8) | (lsb)) * 0.005);
  }
  return ((msb << 8) | (lsb)) * 0.005;
}

/* End of file -------------------------------------------------------- */
