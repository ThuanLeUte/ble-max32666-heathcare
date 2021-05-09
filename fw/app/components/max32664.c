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
#define MAXFAST_ARRAY_SIZE        (6)
#define READ_DELAY                (10)

/* Private enumerate/structure ---------------------------------------- */
/* Private macros ----------------------------------------------------- */

/* Public variables --------------------------------------------------- */
/* Private variables -------------------------------------------------- */
/* Private function prototypes ---------------------------------------- */
static base_status_t m_max32664_read(max32664_t *me,
                                     uint8_t cmd_family,
                                     uint8_t cmd_index,
                                     uint8_t *p_data,
                                     uint8_t len);

static base_status_t m_max32664_read_byte(max32664_t *me,
                                          uint8_t cmd_family,
                                          uint8_t cmd_index,
                                          uint8_t *p_data);

static base_status_t m_max32664_write(max32664_t *me,
                                      uint8_t cmd_family,
                                      uint8_t cmd_index,
                                      uint8_t *write_byte,
                                      uint8_t len);

static base_status_t m_max32664_write_byte(max32664_t *me,
                                           uint8_t cmd_family,
                                           uint8_t cmd_index,
                                           uint8_t write_byte);

/* Function definitions ----------------------------------------------- */
base_status_t max32664_init(max32664_t *me)
{
  uint8_t status;

  if ((me == NULL) || (me->i2c_read == NULL) || (me->i2c_write == NULL))
    return BS_ERROR_PARAMS;

  // Check device mode is application operating mode
  m_max32664_read_byte(me, READ_DEVICE_MODE, 0x00, &status);

  printf("max32664_read_status: 0x%X\n", status);

  CHECK_STATUS(status);

  max32664_read_status(me, &status);

  return BS_OK;
}

base_status_t max32664_read_status(max32664_t *me, uint8_t *status)
{
  // Check device mode is application operating mode
  m_max32664_read_byte(me, HUB_STATUS, 0x00, status);

  printf("max32664_read_status: 0x%X\n", *status);

  return BS_OK;
}

base_status_t max32664_read_bpm(max32664_t *me)
{
  uint8_t data[MAXFAST_ARRAY_SIZE + 1];

  m_max32664_read(me, READ_DATA_OUTPUT, READ_DATA, data, MAXFAST_ARRAY_SIZE + 1);

  // Heart Rate formatting
  me->bio_data.heart_rate = ((uint16_t)(data[1]) << 8);
  me->bio_data.heart_rate |= (data[2]);
  me->bio_data.heart_rate /= 10;

  // Confidence formatting
  me->bio_data.confidence = data[3];

  // Blood oxygen formatting
  me->bio_data.oxygen = ((uint16_t)(data[4]) << 8);
  me->bio_data.oxygen |= (data[5]);
  me->bio_data.oxygen /= 10;

  // Machine state - Has a finger been detected ?
  me->bio_data.status = data[6];

  return BS_OK;
}

base_status_t max32664_config_bpm(max32664_t *me, max32664_mode_t mode)
{
  // Set output mode is just algorithm data
  CHECK_STATUS(max32664_set_output_mode(me, SENSOR_AND_ALGORITHM));

  // One sample before interrupt is fired
  CHECK_STATUS(max32664_set_fifo_threshold(me, 0x05));

  // Enable MAX86140 sensor
  CHECK_STATUS(max32664_enable_max86140(me, ENABLE));

  // Enable AGC algorithm
  CHECK_STATUS(max32664_agc_algo_control(me, ENABLE));

  // Enable fast 
  CHECK_STATUS(max32664_fast_algo_control(me, mode));

  return BS_OK;
}

base_status_t max32664_enable_max86140(max32664_t *me, bool sen_switch)
{
  uint8_t buffer[2];

  buffer[0] = sen_switch;
  buffer[1] = 0x00;

  CHECK_STATUS(m_max32664_write(me, ENABLE_SENSOR, ENABLE_MAX86140, buffer, 1));

  return BS_OK;
}

base_status_t max32664_fast_algo_control(max32664_t *me, max32664_mode_t mode)
{
  CHECK_STATUS(m_max32664_write_byte(me, ENABLE_ALGORITHM, ENABLE_WHRM_ALGO, mode));

  return BS_OK;
}

base_status_t max32664_set_output_mode(max32664_t *me, max32664_output_mode_t output_type)
{
  if (output_type > SENSOR_ALGO_COUNTER)
    return BS_ERROR_PARAMS;

  CHECK_STATUS(m_max32664_write_byte(me, OUTPUT_MODE, SET_FORMAT, output_type));

  return BS_OK;
}

base_status_t max32664_set_fifo_threshold(max32664_t *me, uint8_t threshold)
{
  CHECK_STATUS(m_max32664_write_byte(me, OUTPUT_MODE, WRITE_SET_THRESHOLD, threshold));

  return BS_OK;
}

base_status_t max32664_agc_algo_control(max32664_t *me, bool enable)
{
  CHECK_STATUS(m_max32664_write_byte(me, ENABLE_ALGORITHM, ENABLE_AGC_ALGO, enable));

  return BS_OK;
}

/* Private function definitions ---------------------------------------- */
/**
 * @brief         MAX32664 read
 *
 * @param[in]     me          Pointer to handle of MAX32664 module.
 * @param[in]     cmd_falily  Command family
 * @param[in]     cmd_index   Command index
 * @param[in]     p_data      Pointer to handle of data
 * @param[in]     len         Data length
 *
 * @attention     None
 *
 * @return
 * - BS_OK
 * - BS_ERROR
 */
static base_status_t m_max32664_read(max32664_t *me,
                                     uint8_t cmd_family,
                                     uint8_t cmd_index,
                                     uint8_t *p_data,
                                     uint8_t len)
{
  CHECK(0 == me->i2c_write(me->device_address, cmd_family, &cmd_index, 1), BS_ERROR);

  me->delay(READ_DELAY);

  CHECK(0 == me->i2c_read(me->device_address, p_data, len + 1), BS_ERROR);

  printf("m_max32664_read Error: %d\n", p_data[0]);

  CHECK_STATUS(p_data[0]);

  return BS_OK;
}

/**
 * @brief         MAX32664 read byte
 *
 * @param[in]     me          Pointer to handle of MAX32664 module.
 * @param[in]     cmd_falily  Command family
 * @param[in]     cmd_index   Command index
 * 
 * @attention     None
 *
 * @return
 * - BS_OK
 * - BS_ERROR
 */
static base_status_t m_max32664_read_byte(max32664_t *me,
                                     uint8_t cmd_family,
                                     uint8_t cmd_index,
                                     uint8_t *p_data)
{
  uint8_t buffer[2];

  CHECK(0 == me->i2c_write(me->device_address, cmd_family, &cmd_index, 1), BS_ERROR);

  me->delay(READ_DELAY);

  CHECK(0 == me->i2c_read(me->device_address, buffer, 2), BS_ERROR);

  printf("m_max32664_read_byte Error: %d\n", buffer[0]);

  CHECK_STATUS(buffer[0]);

  *p_data = buffer[1];

  return BS_OK;
}

/**
 * @brief         MAX32664 write byte
 *
 * @param[in]     me          Pointer to handle of MAX32664 module.
 * @param[in]     cmd_falily  Command family
 * @param[in]     cmd_index   Command index
 * @param[in]     write_byte  Write byte
 *
 * @attention     None
 *
 * @return
 * - BS_OK
 * - BS_ERROR
 */
static base_status_t m_max32664_write_byte(max32664_t *me,
                                           uint8_t cmd_family,
                                           uint8_t cmd_index,
                                           uint8_t write_byte)
{
  uint8_t buffer[2];

  buffer[0] = cmd_index;
  buffer[1] = write_byte;

  CHECK(0 == me->i2c_write(me->device_address, cmd_family, buffer, 2), BS_ERROR);

  me->delay(READ_DELAY);

  CHECK(0 == me->i2c_read(me->device_address, buffer, 1), BS_ERROR);

  printf("m_max32664_write_byte Error: %d\n", buffer[0]);

  // CHECK_STATUS(buffer[0]);

  return BS_OK;
}

/**
 * @brief         MAX32664 write
 *
 * @param[in]     me          Pointer to handle of MAX32664 module.
 * @param[in]     cmd_falily  Command family
 * @param[in]     cmd_index   Command index
 * @param[in]     write_byte  Write byte
 *
 * @attention     None
 *
 * @return
 * - BS_OK
 * - BS_ERROR
 */
static base_status_t m_max32664_write(max32664_t *me,
                                      uint8_t cmd_family,
                                      uint8_t cmd_index,
                                      uint8_t *write_byte,
                                      uint8_t len)
{
  uint8_t buffer[10];

  buffer[0] = cmd_index;
  memcpy(&buffer[1], write_byte, len);

  printf("m_max32664_write Error: 0x%2X, 0x%2X, 0x%2X, 0x%2X, 0x%2X\n", me->device_address, cmd_family, 
                                          cmd_index, write_byte[0], write_byte[1]);

  CHECK(0 == me->i2c_write(me->device_address, cmd_family, buffer, len + 1), BS_ERROR);

  me->delay(READ_DELAY);

  CHECK(0 == me->i2c_read(me->device_address, buffer, 1), BS_ERROR);

  printf("m_max32664_write Error: %d\n", buffer[0]);

  CHECK_STATUS(buffer[0]);

  return BS_OK;
}

/* End of file -------------------------------------------------------- */
