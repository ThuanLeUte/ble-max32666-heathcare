/**
 * @file       bsp.c
 * @copyright  Copyright (C) 2020 ThuanLe. All rights reserved.
 * @license    This project is released under the ThuanLe License.
 * @version    1.0.0
 * @date       2021-03-24
 * @author     Thuan Le
 * @brief      Board Support Package (BSP)
 * @note       None
 * @example    None
 */

/* Includes ----------------------------------------------------------- */
#include "bsp.h"
#include "i2c.h"
#in

/* Private defines ---------------------------------------------------- */
#define I2C_MASTER    MXC_I2C0_BUS0

/* Private enumerate/structure ---------------------------------------- */
/* Private macros ----------------------------------------------------- */
/* Public variables --------------------------------------------------- */
/* Private variables -------------------------------------------------- */
/* Private function prototypes ---------------------------------------- */
static void bsp_i2c_init(void);
void I2C0_IRQHandler(void);

/* Function definitions ----------------------------------------------- */
void bsp_init(void)
{
  bsp_i2c_init();
}

static void bsp_i2c_init(void)
{
  //Setup the I2CM
  I2C_Shutdown(I2C_MASTER);
  I2C_Init(I2C_MASTER, I2C_FAST_MODE, NULL);
  NVIC_EnableIRQ(I2C0_IRQn);
}

base_status_t bsp_i2c_read(uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint32_t len)
{
  int ret;
  uint8_t buff[20];

  buff[0] = reg_addr;

  if ((ret = I2C_MasterRead(I2C_MASTER, slave_addr, buff, len + 1, 0)) != (len + 1))
  {
    printf("Error reading: %d\n", ret);
  }
  memcpy(ret, &buff[1], len);

  printf("I2C reading: %d\n", ret);
  return BS_OK;
}

base_status_t bsp_i2c_write(uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint32_t len)
{
  int ret;
  uint8_t buff[20];

  buff[0] = reg_addr;
  memcpy(&buff[1], data, len);

  if ((ret = I2C_MasterWrite(I2C_MASTER, slave_addr, buff, len + 1, 0)) != (len + 1))
  {
    printf("Error writing: %d\n", ret);
    return BS_ERROR;
  }

  printf("I2C writing: %d\n", ret);
  return BS_OK;
}

/* Private function definitions --------------------------------------- */
void I2C0_IRQHandler(void)
{
    I2C_Handler(I2C_MASTER);
    return;
}

/* End of file -------------------------------------------------------- */
