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
#define MAX32664_I2C_ADDR                (0x90 >> 1)

/* Public enumerate/structure ----------------------------------------- */
/**
 * @brief MAX32664 sensor struct
 */
typedef struct 
{
  uint8_t  device_address;  // I2C device address

  // Read n-bytes from device's internal address <reg_addr> via I2C bus
  int (*i2c_read) (uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint32_t len);

  // Write n-bytes from device's internal address <reg_addr> via I2C bus
  int (*i2c_write) (uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint32_t len);

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
 * @brief MAX32664 mode enum
 */
typedef enum
{
  MAX32664_MODE_1 = 0x00,
  MAX32664_MODE_2 = 0x01
}
max32664_mode_t;

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

/* -------------------------------------------------------------------------- */
#ifdef __cplusplus
} // extern "C"
#endif
#endif // __MAX32664_H

/* End of file -------------------------------------------------------- */
