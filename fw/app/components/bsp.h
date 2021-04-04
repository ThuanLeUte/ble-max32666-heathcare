/**
 * @file       bsp.h
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-03-24
 * @author     Thuan Le
 * @brief      Board Support Package (BSP)
 * @note       None
 * @example    None
 */

/* Define to prevent recursive inclusion ------------------------------ */
#ifndef __BSP_H
#define __BSP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------- */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Public defines ----------------------------------------------------- */
/* Public enumerate/structure ----------------------------------------- */
/**
 * @brief Base status structure
 */
typedef enum
{
  BS_OK = 0x00,
  BS_ERROR_PARAMS,
  BS_ERROR
}
base_status_t;

/**
 * @brief Bool structure
 */
typedef enum
{
  BS_FALSE = 0x00,
  BS_TRUE  = 0x01
}
bool_t;

/* Public macros ------------------------------------------------------ */
#define CHECK(expr, ret)            \
  do {                              \
    if (!(expr)) {                  \
      NRF_LOG_INFO("%s", #expr);    \
      return (ret);                 \
    }                               \
  } while (0)

#define CHECK_STATUS(expr)          \
  do {                              \
    base_status_t ret = (expr);     \
    if (BS_OK != ret) {             \
      NRF_LOG_INFO("%s", #expr);    \
      return (ret);                 \
    }                               \
  } while (0)

/* Public variables --------------------------------------------------- */
/* Public function prototypes ----------------------------------------- */
/**
 * @brief         I2C write
 *
 * @param[in]     slave_addr   Slave address
 * @param[in]     reg_addr     Register address
 * @param[out]    data         Pointer to data
 * @param[in]     len          Data length
 *
 * @attention     None
 *
 * @return
 * - BS_OK
 * - BS_ERROR
 */
base_status_t bsp_i2c_read(uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint32_t len);

/**
 * @brief         I2C write
 *
 * @param[in]     slave_addr   Slave address
 * @param[in]     reg_addr     Register address
 * @param[in]     data         Pointer to data
 * @param[in]     len          Data length
 *
 * @attention     None
 *
 * @return
 * - BS_OK
 * - BS_ERROR
 */
base_status_t bsp_i2c_write(uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint32_t len);

/* -------------------------------------------------------------------------- */
#ifdef __cplusplus
} // extern "C"
#endif
#endif // __BSP_H

/* Undefine macros ---------------------------------------------------- */
#undef CHECK
#undef CHECK_STATUS
/* End of file -------------------------------------------------------- */
