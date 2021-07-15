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
#define MAXFAST_ARRAY_SIZE        (44)
#define READ_DELAY                (2)
#define ENABLE_DELAY              (2)

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

  me->gpio_write(MAX32644_PIN_MIFO, 1);
  me->gpio_write(MAX32644_PIN_RESET, 0);
  me->delay(10);
  me->gpio_write(MAX32644_PIN_RESET, 1);

  // Check device mode is application operating mode
  m_max32664_read_byte(me, READ_DEVICE_MODE, 0x00, &status);

  printf("READ_DEVICE_MODE: 0x%2X\n", status);

  CHECK_STATUS(status);

  max32664_read_status(me, &status);

  return BS_OK;
}

base_status_t max32664_read_status(max32664_t *me, uint8_t *status)
{
  m_max32664_read_byte(me, HUB_STATUS, 0x00, status);

  printf("HUB_STATUS: 0x%2X\n", *status);

  return BS_OK;
}

base_status_t max32664_read_bpm(max32664_t *me)
{
  uint8_t data[MAXFAST_ARRAY_SIZE + 1];

  m_max32664_read(me, READ_DATA_OUTPUT, READ_DATA, data, MAXFAST_ARRAY_SIZE + 1);

  for (uint8_t i = 0; i < MAXFAST_ARRAY_SIZE + 1; i++)
  {
    printf("Value: 0x%2X\n", data[i]);
  }

  // Heart Rate formatting
  me->bio_data.heart_rate = ((uint16_t)(data[26]) << 8);
  me->bio_data.heart_rate |= (data[27]);
  me->bio_data.heart_rate /= 10;

  // Blood oxygen formatting
  me->bio_data.oxygen = ((uint16_t)(data[36]) << 8);
  me->bio_data.oxygen |= (data[37]);
  me->bio_data.oxygen /= 10;

  return BS_OK;
}

base_status_t max32664_config_bpm(max32664_t *me, max32664_mode_t mode)
{
  printf("max32664_config_bpm\n");

  // Set the output mode to sensor + algorithm data
  CHECK_STATUS(max32664_set_output_mode(me, SENSOR_AND_ALGORITHM));

  // Set the sensor hub interrupt threshold
  CHECK_STATUS(max32664_set_fifo_threshold(me, 0x01));

  // Set the report rate to be one report per every sensor sample
  CHECK_STATUS(max32664_set_report_rate(me, 0x01));

  // Set the algorithm operation mode to Continuous HRM and Continuous SpO2
  CHECK_STATUS(max32664_algo_config(me));

  // Enable WHRM and SpO2 algorithm for the normal algorithm report
  CHECK_STATUS(max32664_enable_algo(me));

  return BS_OK;
}

base_status_t max32664_set_output_mode(max32664_t *me, max32664_output_mode_t output_type)
{
  printf("max32664_set_output_mode\n");

  if (output_type > SENSOR_ALGO_COUNTER)
    return BS_ERROR_PARAMS;

  CHECK_STATUS(m_max32664_write_byte(me, OUTPUT_MODE, SET_FORMAT, output_type));

  return BS_OK;
}

base_status_t max32664_set_fifo_threshold(max32664_t *me, uint8_t threshold)
{
  printf("max32664_set_fifo_threshold\n");

  CHECK_STATUS(m_max32664_write_byte(me, OUTPUT_MODE, WRITE_SET_THRESHOLD, threshold));

  return BS_OK;
}

base_status_t max32664_set_report_rate(max32664_t *me, uint8_t report_rate)
{
  printf("max32664_set_report_rate\n");

  CHECK_STATUS(m_max32664_write_byte(me, OUTPUT_MODE, SET_SAMPLE_REPORT_RATE, report_rate));

  return BS_OK;
}

base_status_t max32664_algo_config(max32664_t *me)
{
  printf("max32664_algo_config\n");

  uint8_t buffer[2];

  buffer[0] = 0x0A;
  buffer[1] = 0x00;

  CHECK_STATUS(m_max32664_write(me, CHANGE_ALGORITHM_CONFIG, 0x07, buffer, 2));

  return BS_OK;
}

base_status_t max32664_enable_algo(max32664_t *me)
{
  printf("max32664_enable_algo\n");

  CHECK_STATUS(m_max32664_write_byte(me, ENABLE_ALGORITHM, 0x07, 0x01));

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
  printf("m_max32664_write_byte: 0x%2X, 0x%2X, 0x%2X\n", me->device_address, cmd_family,
                                                                cmd_index);

  CHECK(0 == me->i2c_write(me->device_address, cmd_family, &cmd_index, 1), BS_ERROR);

  me->delay(READ_DELAY);

  CHECK(0 == me->i2c_read(me->device_address, p_data, len + 1), BS_ERROR);

  printf("m_max32664_read Error: 0x%2X\n", p_data[0]);

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
  static uint8_t buffer[2];

  CHECK(0 == me->i2c_write(me->device_address, cmd_family, &cmd_index, 1), BS_ERROR);

  me->delay(READ_DELAY);

  CHECK(0 == me->i2c_read(me->device_address, buffer, 2), BS_ERROR);

  printf("m_max32664_read_byte Error: 0x%2X\n", buffer[0]);

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
  static uint8_t buffer[2];

  buffer[0] = cmd_index;
  buffer[1] = write_byte;

  printf("m_max32664_write_byte: 0x%2X, 0x%2X, 0x%2X, 0x%2X\n", me->device_address, cmd_family,
                                                                cmd_index, write_byte);

  CHECK(0 == me->i2c_write(me->device_address, cmd_family, buffer, 2), BS_ERROR);

  me->delay(READ_DELAY);

  CHECK(0 == me->i2c_read(me->device_address, buffer, 1), BS_ERROR);

  printf("m_max32664_write_byte Error: %2X\n", buffer[0]);

  CHECK_STATUS(buffer[0]);

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

  printf("m_max32664_write: 0x%2X, 0x%2X, 0x%2X, 0x%2X, 0x%2X\n", me->device_address, cmd_family, 
                                          cmd_index, write_byte[0], write_byte[1]);

  CHECK(0 == me->i2c_write(me->device_address, cmd_family, buffer, len + 1), BS_ERROR);

  me->delay(ENABLE_DELAY);

  CHECK(0 == me->i2c_read(me->device_address, buffer, 1), BS_ERROR);

  printf("m_max32664_write Error: 0x%2X\n", buffer[0]);

  CHECK_STATUS(buffer[0]);

  return BS_OK;
}

/* End of file -------------------------------------------------------- */
