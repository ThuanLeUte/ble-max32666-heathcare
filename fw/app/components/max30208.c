/**
 * @file       max30208.h
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-03-22
 * @author     Thuan Le
 * @brief      Driver support MAX30208 (Digital Temperature Sensor)
 * @note       None
 * @example    None
 */

/* Includes ----------------------------------------------------------- */
#include "max30208.h"
#include "bsp.h"

/* Private defines ---------------------------------------------------- */
// Registers
#define MAX30208_REG_STATUS                   (0x00)
#define MAX30208_REG_INTERRUPT_ENABLE         (0x01)
#define MAX30208_REG_FIFO_WRITE_POINTER       (0x04)
#define MAX30208_REG_FIFO_READ_POINTER        (0x05)
#define MAX30208_REG_FIFO_OVERFLOW_COUNTER    (0x06)
#define MAX30208_REG_DATA_COUNTER             (0x07)
#define MAX30208_REG_DATA                     (0x08)
#define MAX30208_REG_FIFO_CONFIG_1            (0x09)
#define MAX30208_REG_FIFO_CONFIG_2            (0x0A)
#define MAX30208_REG_SYSTEM_CONTROL           (0x0C)
#define MAX30208_REG_ALARM_HIGH_MSB           (0x10)
#define MAX30208_REG_ALARM_HIGH_LSB           (0x11)
#define MAX30208_REG_ALARM_LOW_MSB            (0x12)
#define MAX30208_REG_ALARM_LOW_LSB            (0x13)
#define MAX30208_REG_TEMP_SENSOR_SETUP        (0x14)
#define MAX30208_REG_GPIO_SETUP               (0x20)
#define MAX30208_REG_GPIO_CONTROL             (0x21)
#define MAX30208_REG_PART_ID_1                (0x31)
#define MAX30208_REG_PART_ID_2                (0x32)
#define MAX30208_REG_PART_ID_3                (0x33)
#define MAX30208_REG_PART_ID_4                (0x34)
#define MAX30208_REG_PART_ID_5                (0x35)
#define MAX30208_REG_PART_ID_6                (0x36)
#define MAX30208_REG_PART_IDENTIFIER          (0xFF)
#define MAX30208_PART_IDENTIFIER              (0X30)

/* Private enumerate/structure ---------------------------------------- */
/* Private macros ----------------------------------------------------- */
/* Public variables --------------------------------------------------- */
/* Private variables -------------------------------------------------- */
/* Private function prototypes ---------------------------------------- */
static base_status_t m_max30208_read_reg(max30208_t *me, uint8_t reg, uint8_t *p_data, uint32_t len);
static base_status_t m_max30208_write_reg(max30208_t *me, uint8_t reg, uint8_t *p_data, uint32_t len);
static base_status_t m_max30208_interrupt_enable(max30208_t *me, uint8_t reg, uint8_t intr, bool enable);

/* Function definitions ----------------------------------------------- */
base_status_t max30208_init(max30208_t *me)
{
  uint8_t identifier;

  if ((me == NULL) || (me->i2c_read == NULL) || (me->i2c_write == NULL))
    return BS_ERROR;

  CHECK_STATUS(m_max30208_read_reg(me, MAX30208_REG_PART_IDENTIFIER, &identifier, 1));

  if (MAX30208_PART_IDENTIFIER != identifier)
    return BS_ERROR;

  return BS_OK;
}

base_status_t max30208_start_convert(max30208_t *me)
{
  uint8_t data = 0x01;

  // Enable interrupt
  CHECK_STATUS(m_max30208_interrupt_enable(me, MAX30208_REG_INTERRUPT_ENABLE, MAX30208_INT_ENA_TEMP_RDY, true));

  // Start convert temp
  CHECK_STATUS(m_max30208_write_reg(me, MAX30208_REG_TEMP_SENSOR_SETUP, &data, 1));

  return BS_OK;
}

base_status_t max30208_get_interrupt_status(max30208_t *me, uint8_t *status)
{
  // Get interrupt status
  CHECK_STATUS(m_max30208_read_reg(me, MAX30208_REG_STATUS, status, 1));

  return BS_OK;
}

base_status_t max30208_get_fifo_available(max30208_t *me)
{
  // Get FIFO available
  CHECK_STATUS(m_max30208_read_reg(me, MAX30208_REG_FIFO_OVERFLOW_COUNTER, &me->fifo_len, 1));

  if (0 != me->fifo_len)
  {
    me->fifo_len = 32;
    return BS_OK;
  }

  CHECK_STATUS(m_max30208_read_reg(me, MAX30208_REG_DATA_COUNTER, &me->fifo_len, 1));

  return BS_OK;
}

base_status_t max30208_get_fifo(max30208_t *me)
{
  CHECK_STATUS(m_max30208_read_reg(me, MAX30208_REG_DATA, me->fifo, me->fifo_len));

  return BS_OK;
}

base_status_t max30208_get_temperature(max30208_t *me, float *temp)
{
  *temp = me->temperature[me->head];

  return BS_OK;
}


/* Private function definitions ---------------------------------------- */
/**
 * @brief         MAX30208 read register
 *
 * @param[in]     me      Pointer to handle of MAX30208 module.
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
static base_status_t m_max30208_read_reg(max30208_t *me, uint8_t reg, uint8_t *p_data, uint32_t len)
{
  CHECK(0 == me->i2c_read(me->device_address, reg, p_data, len), BS_ERROR);

  return BS_OK;
}

/**
 * @brief         MAX30208 read register
 *
 * @param[in]     me      Pointer to handle of MAX30208 module.
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
static base_status_t m_max30208_write_reg(max30208_t *me, uint8_t reg, uint8_t *p_data, uint32_t len)
{
  CHECK(0 == me->i2c_write(me->device_address, reg, p_data, len), BS_ERROR);

  return BS_OK;
}

/**
 * @brief         MAX30208 interrupt enable
 *
 * @param[in]     me      Pointer to handle of MAX30208 module.
 * @param[in]     reg     Register
 * @param[in]     intr    Interrupts
 * @param[in]     enable  Enable
 *
 * @attention     None
 *
 * @return
 * - BS_OK
 * - BS_ERROR
 */
static base_status_t m_max30208_interrupt_enable(max30208_t *me, uint8_t reg, uint8_t intr, bool enable)
{
  uint8_t data = 0;

  // Read current status
  CHECK_STATUS(m_max30208_read_reg(me, reg, &data, 1));

  if (enable)
    data |= intr;
  else
    data &= ~intr;

  CHECK_STATUS(m_max30208_write_reg(me, reg, &data, 1));

  return BS_OK;
}

/* End of file -------------------------------------------------------- */
