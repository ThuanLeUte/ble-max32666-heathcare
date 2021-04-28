/**
 * @file       max32664.c
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-03-22
 * @author     Thuan Le
 * @brief      Driver support MAX32664
 * @note       None
 * @example    None
 */

/* Define to prevent recursive inclusion ------------------------------ */
#ifndef __MAX32664_H
#define __MAX32664_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------- */
#include "bsp.h"

/* Public defines ----------------------------------------------------- */
#define MAX32664_I2C_ADDR                (0xD0)

#define DISABLE                0x00
#define ENABLE                 0x01
#define MODE_ONE               0x01
#define MODE_TWO               0x02
#define SET_FORMAT             0x00
#define READ_FORMAT            0x01 // Index Byte under Family Byte: READ_OUTPUT_MODE (0x11)
#define WRITE_SET_THRESHOLD    0x01 //Index Byte for WRITE_INPUT(0x14)
#define WRITE_EXTERNAL_TO_FIFO 0x00

/* Public enumerate/structure ----------------------------------------- */
/**
 * @brief MAX32664 sensor struct
 */
typedef struct
{
  uint32_t ir_led;
  uint32_t red_led;
  uint16_t heart_rate; // LSB = 0.1bpm
  uint8_t  confidence; // 0-100% LSB = 1%
  uint16_t oxygen; // 0-100% LSB = 1%
  uint8_t  status; // 0: Success, 1: Not Ready, 2: Object Detectected, 3: Finger Detected
  float    r_value;      // -- Algorithm Mode 2 vv
  int8_t   ext_status;   // --
  uint8_t  reserve_one;  // --
  uint8_t  resserve_two; // -- Algorithm Mode 2 ^^
}
max32664_bio_data_t;

/**
 * @brief MAX32664 sensor struct
 */
typedef struct 
{
  uint8_t  device_address;  // I2C device address

  max32664_bio_data_t bio_data;

  // Read n-bytes from device's internal address <reg_addr> via I2C bus
  base_status_t (*i2c_read) (uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint32_t len);

  // Write n-bytes from device's internal address <reg_addr> via I2C bus
  base_status_t (*i2c_write) (uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint32_t len);

  void (*delay) (uint16_t ms);
}
max32664_t;

/**
 * @brief MAX32664 status byte
 */
enum READ_STATUS_BYTE_VALUE
{
  SUCCESS                  = 0x00,
  ERR_UNAVAIL_CMD,
  ERR_UNAVAIL_FUNC,
  ERR_DATA_FORMAT,
  ERR_INPUT_VALUE,
  ERR_TRY_AGAIN,
  ERR_BTLDR_GENERAL        = 0x80,
  ERR_BTLDR_CHECKSUM,
  ERR_BTLDR_AUTH,
  ERR_BTLDR_INVALID_APP,
  ERR_UNKNOWN              = 0xFF
};

/**
 * @brief MAX32664 family register bytes
 */
enum FAMILY_REGISTER_BYTES 
{
  HUB_STATUS               = 0x00,
  SET_DEVICE_MODE,
  READ_DEVICE_MODE,
  OUTPUT_MODE              = 0x10,
  READ_OUTPUT_MODE,
  READ_DATA_OUTPUT,
  READ_DATA_INPUT,
  WRITE_INPUT,
  WRITE_REGISTER           = 0x40,
  READ_REGISTER,
  READ_ATTRIBUTES_AFE,
  DUMP_REGISTERS,
  ENABLE_SENSOR,
  READ_SENSOR_MODE,
  CHANGE_ALGORITHM_CONFIG  = 0x50,
  READ_ALGORITHM_CONFIG,
  ENABLE_ALGORITHM,
  BOOTLOADER_FLASH         = 0x80,
  BOOTLOADER_INFO,
  IDENTITY                 = 0xFF
};

/**
 * @brief Algorithm mode enable enum
 */
enum ALGORITHM_MODE_ENABLE_INDEX_BYTE
{
  ENABLE_AGC_ALGO   = 0x00,
  ENABLE_AFC_ALGO   = 0x01,
  ENABLE_WHRM_ALGO  = 0x02,
  ENABLE_ECG_ALGO   = 0x03,
  ENABLE_BPT_ALGO   = 0x04,
  ENABLE_WSPO2_ALGO = 0x05
};

enum SENSOR_ENABLE_INDEX_BYTE 
{
  ENABLE_MAX86140      = 0x00,
  ENABLE_MAX30205      = 0x01,
  ENABLE_MAX30001      = 0x02,
  ENABLE_MAX30101      = 0x03,
  ENABLE_ACCELEROMETER = 0x04
};

enum FIFO_OUTPUT_INDEX_BYTE
{
  NUM_SAMPLES,
  READ_DATA
};

/**
 * @brief MAX32664 mode enum
 */
typedef enum
{
  MAX32664_MODE_1 = 0x00,
  MAX32664_MODE_2 = 0x01
}
max32664_mode_t;

 /**
 * @brief MAX32664 output mode write byte enum
 */
typedef enum
{
  PAUSE = 0x00,
  SENSOR_DATA,
  ALGO_DATA,
  SENSOR_AND_ALGORITHM,
  PAUSE_TWO,
  SENSOR_COUNTER_BYTE,
  ALGO_COUNTER_BYTE,
  SENSOR_ALGO_COUNTER
}
max32664_output_mode_t;

/* Public macros ------------------------------------------------------ */
/* Public variables --------------------------------------------------- */
/* Public function prototypes ----------------------------------------- */
/**
 * @brief         Initialize MAX32664
 *
 * @param[in]     me            Pointer to handle of MAX32664 module.
 *
 * @attention     None
 *
 * @return
 * - BS_OK
 * - BS_ERROR
 */
base_status_t max32664_init(max32664_t *me);
base_status_t max32664_read_bpm(max32664_t *me);
base_status_t max32664_config_bpm(max32664_t *me, max32664_mode_t mode);
base_status_t max32664_fast_algo_control(max32664_t *me, max32664_mode_t mode);
base_status_t max32664_control(max32664_t *me, bool sen_switch);
base_status_t max32664_set_output_mode(max32664_t *me, max32664_output_mode_t output_type);
base_status_t max32664_set_fifo_threshold(max32664_t *me, uint8_t threshold);
base_status_t max32664_agc_algo_control(max32664_t *me, bool enable);

/* -------------------------------------------------------------------------- */
#ifdef __cplusplus
} // extern "C"
#endif
#endif // __MAX32664_H

/* End of file -------------------------------------------------------- */
