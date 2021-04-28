/**
 * @file       max30208.c
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-03-22
 * @author     Thuan Le
 * @brief      Driver support MAX30208 (Digital Temperature Sensor)
 * @note       None
 * @example    None
 */

/* Define to prevent recursive inclusion ------------------------------ */
#ifndef __MAX30208_H
#define __MAX30208_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------- */
#include "bsp.h"

/* Public defines ----------------------------------------------------- */
#define MAX30208_I2C_ADDR                  (0x50 << 1)

// Bit setup
#define MAX30208_INT_ENA_AFULL             (1 << 4)
#define MAX30208_INT_ENA_TEMP_LOW          (1 << 4)
#define MAX30208_INT_ENA_TEMP_HIGH         (1 << 2)
#define MAX30208_INT_ENA_TEMP_RDY          (1 << 0)

/* Public enumerate/structure ----------------------------------------- */
/**
 * @brief MAX30208 sensor struct
 */
typedef struct 
{
  uint8_t device_address;  // I2C device address
  uint8_t fifo[32];        // FIFO data
  uint8_t fifo_len;        // FIFO length

  // Ring buffer
  float   temperature[16];
  uint8_t head;
  uint8_t tail;

  // Read n-bytes from device's internal address <reg_addr> via I2C bus
  base_status_t (*i2c_read) (uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint32_t len);

  // Write n-bytes from device's internal address <reg_addr> via I2C bus
  base_status_t (*i2c_write) (uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint32_t len);
}
max30208_t;

/* Public macros ------------------------------------------------------ */
/* Public variables --------------------------------------------------- */
/* Public function prototypes ----------------------------------------- */
/**
 * @brief         Initialize MAX30208
 *
 * @param[in]     me            Pointer to handle of MAX30208 module.
 *
 * @attention     None
 *
 * @return
 * - BS_OK
 * - BS_ERROR
 */
base_status_t max30208_init(max30208_t *me);

/**
 * @brief         MAX30208 start convert data
 *
 * @param[in]     me            Pointer to handle of MAX30208 module.
 *
 * @attention     None
 *
 * @return
 * - BS_OK
 * - BS_ERROR
 */
base_status_t max30208_start_convert(max30208_t *me);

/**
 * @brief         MAX30208 get interrupt status
 *
 * @param[in]     me            Pointer to handle of MAX30208 module.
 * @param[in]     status        Pointer to status
 *
 * @attention     None
 *
 * @return
 * - BS_OK
 * - BS_ERROR
 */
base_status_t max30208_get_interrupt_status(max30208_t *me, uint8_t *status);

/**
 * @brief         MAX30208 get fifo available
 *
 * @param[in]     me            Pointer to handle of MAX30208 module.
 *
 * @attention     None
 *
 * @return
 * - BS_OK
 * - BS_ERROR
 */
base_status_t max30208_get_fifo_available(max30208_t *me);

/**
 * @brief         MAX30208 get FIFO
 *
 * @param[in]     me            Pointer to handle of MAX30208 module.
 *
 * @attention     None
 *
 * @return
 * - BS_OK
 * - BS_ERROR
 */
base_status_t max30208_get_fifo(max30208_t *me);

/**
 * @brief         MAX30208 get temperature
 *
 * @param[in]     me            Pointer to handle of MAX30208 module.
 * @param[in]     temp          Pointer to temperature
 *
 * @attention     None
 *
 * @return
 * - BS_OK
 * - BS_ERROR
 */
base_status_t max30208_get_temperature(max30208_t *me, float *temp);

/* -------------------------------------------------------------------------- */
#ifdef __cplusplus
} // extern "C"
#endif
#endif // __MAX30208_H

/* End of file -------------------------------------------------------- */
