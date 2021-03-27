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
#define MAXFAST_ARRAY_SIZE        6  // Number of bytes....

/* Private enumerate/structure ---------------------------------------- */
/* Private macros ----------------------------------------------------- */

/* Public variables --------------------------------------------------- */
/* Private variables -------------------------------------------------- */
/* Private function prototypes ---------------------------------------- */
static base_status_t m_max32664_read(max32664_t *me,
                                     uint8_t cmd_family,
                                     uint8_t cmd_index,
                                     uint8_t *p_data,
                                     uint32_t len,
                                     uint8_t *status);

static base_status_t m_max32664_read_byte(max32664_t *me,
                                          uint8_t cmd_family,
                                          uint8_t cmd_index,
                                          uint8_t *status);

static base_status_t m_max32664_write(max32664_t *me,
                                      uint8_t cmd_family,
                                      uint8_t cmd_index,
                                      uint8_t write_byte,
                                      uint8_t *p_data,
                                      uint8_t len,
                                      uint8_t status);

static base_status_t m_max32664_write_byte(max32664_t *me,
                                           uint8_t cmd_family,
                                           uint8_t cmd_index,
                                           uint8_t write_byte,
                                           uint8_t *status);

/* Function definitions ----------------------------------------------- */
base_status_t max32664_init(max32664_t *me)
{
  uint8_t status;

  if ((me == NULL) || (me->i2c_read == NULL) || (me->i2c_write == NULL))
    return BS_ERROR_PARAMS;

  m_max32664_read_byte(me, READ_DEVICE_MODE, 0x00, &status);

  if (status != SUCCESS)
    return BS_ERROR;

  return BS_OK;
}

base_status_t max32664_read_bpm(max32664_t *me)
{
  uint8_t status;

  uint8_t data[MAXFAST_ARRAY_SIZE];

  m_max32664_read(me, READ_DATA_OUTPUT, READ_DATA, data, MAXFAST_ARRAY_SIZE, &status);

  CHECK_STATUS(status);

  // Hear Rate formatting
  me->bio_data.heart_rate = ((uint16_t)(data[0]) << 8);
  me->bio_data.heart_rate |= (data[1]);
  me->bio_data.heart_rate /= 10;

  // Confidence formmatting
  me->bio_data.confidence = data[2];

  // Blood oxygen formatting
  me->bio_data.oxygen = ((uint16_t)(data[3]) << 8);
  me->bio_data.oxygen |= (data[4]);
  me->bio_data.oxygen /= 10;

  // Machine state - Has a finger been detected ?
  me->bio_data.status = data[5];
}

base_status_t max32664_config_bpm(max32664_t *me, max32664_mode_t mode)
{
  uint8_t status;

  CHECK_STATUS(max32664_set_output_mode(me, ALGO_DATA), BS_ERROR);

  CHECK_STATUS(max32664_set_fifo_threshold(me, 0x01), BS_ERROR);

  CHECK_STATUS(max32664_agc_algo_control(me, ENABLE), BS_ERROR);

  CHECK_STATUS(max32664_control(me, ENABLE), BS_ERROR); // TODO: Check the register ENABLE_MAX30101

  CHECK_STATUS(max32664_fast_algo_control(me, mode), BS_ERROR);

  return BS_OK;
}

base_status_t max32664_fast_algo_control(max32664_t *me, max32664_mode_t mode)
{
  uint8_t status;

  CHECK_STATUS(m_max32664_write_byte(me, ENABLE_ALGORITHM, ENABLE_WHRM_ALGO, mode, &status), BS_ERROR);

  CHECK_STATUS(status);

  return BS_OK;
}

base_status_t max32664_control(max32664_t *me, bool sen_switch)
{
  uint8_t status;

  CHECK_STATUS(m_max32664_write_byte(me, ENABLE_SENSOR, ENABLE_MAX30101, sen_switch, &status), BS_ERROR);

  CHECK_STATUS(status);

  return BS_OK;
}

base_status_t max32664_set_output_mode(max32664_t *me, max32664_output_mode_t output_type)
{
  uint8_t status;

  if (output_type > SENSOR_ALGO_COUNTER)
    return BS_ERROR_PARAMS;

  CHECK_STATUS(m_max32664_write_byte(me, OUTPUT_MODE, SET_FORMAT, output_type, &status), BS_ERROR);

  CHECK_STATUS(status);

  return BS_OK;
}

base_status_t max32664_set_fifo_threshold(max32664_t *me, uint8_t threshold)
{
  uint8_t status;

  CHECK_STATUS(m_max32664_write_byte(me, OUTPUT_MODE, WRITE_SET_THRESHOLD, threshold, &status), BS_ERROR);

  CHECK_STATUS(status);

  return BS_OK;
}

base_status_t max32664_agc_algo_control(max32664_t *me, bool enable)
{
  uint8_t status;

  CHECK_STATUS(m_max32664_write_byte(me, ENABLE_ALGORITHM, ENABLE_AGC_ALGO, enable, &status), BS_ERROR);

  CHECK_STATUS(status);

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
 * @param[in]     status      Status
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
                                     uint32_t len,
                                     uint8_t *status)
{
  uint8_t buffer[2];

  buffer[0] = cmd_index;
  buffer[1] = write_byte;

  CHECK(0 == me->i2c_write(me->device_address, cmd_family, buffer, 2), BS_ERROR);
  me->delay(100);
  CHECK(0 == me->i2c_read(me->device_address, 0x00, p_data, len + 1), BS_ERROR);

  *status = p_data[0];

  CHECK_STATUS(*status);

  return BS_OK;
}

/**
 * @brief         MAX32664 read byte
 *
 * @param[in]     me          Pointer to handle of MAX32664 module.
 * @param[in]     cmd_falily  Command family
 * @param[in]     cmd_index   Command index
 * @param[in]     status      Status
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
                                     uint8_t *status)
{
  CHECK(0 == me->i2c_write(me->device_address, cmd_family, &cmd_index, 1), BS_ERROR);
  me->delay(100);
  CHECK(0 == me->i2c_read(me->device_address, 0x00, status, 1), BS_ERROR);

  CHECK_STATUS(*status);

  return BS_OK;
}

/**
 * @brief         MAX32664 write
 *
 * @param[in]     me          Pointer to handle of MAX32664 module.
 * @param[in]     cmd_falily  Command family
 * @param[in]     cmd_index   Command index
 * @param[in]     write_byte  Write byte
 * @param[in]     p_data      Pointer to handle of data
 * @param[in]     len         Data length
 * @param[in]     status      Status
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
                                      uint8_t write_byte,
                                      uint8_t *p_data,
                                      uint8_t len,
                                      uint8_t *status)
{
  uint8_t buffer[50];

  buffer[0] = cmd_index;
  buffer[1] = write_byte;
  memcpy(&buffer[2], p_data, len);

  CHECK(0 == me->i2c_write(me->device_address, cmd_family, buffer, len + 2), BS_ERROR);
  me->delay(100);
  CHECK(0 == me->i2c_read(me->device_address, 0x00, status, 1), BS_ERROR);

  CHECK_STATUS(*status);

  return BS_OK;
}

/**
 * @brief         MAX32664 write byte
 *
 * @param[in]     me          Pointer to handle of MAX32664 module.
 * @param[in]     cmd_falily  Command family
 * @param[in]     cmd_index   Command index
 * @param[in]     write_byte  Write byte
 * @param[in]     status      Status
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
                                           uint8_t write_byte,
                                           uint8_t *status)
{
  uint8_t buffer[2];

  buffer[0] = cmd_index;
  buffer[1] = write_byte;

  CHECK(0 == me->i2c_write(me->device_address, cmd_family, buffer, 2), BS_ERROR);
  me->delay(100);
  CHECK(0 == me->i2c_read(me->device_address, 0x00, status, 1), BS_ERROR);

  CHECK_STATUS(*status);

  return BS_OK;
}

/* End of file -------------------------------------------------------- */
