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
#include "tmr_utils.h"

/* Private defines ---------------------------------------------------- */
#define I2C_MASTER    MXC_I2C0_BUS0

/* Private enumerate/structure ---------------------------------------- */
/* Private macros ----------------------------------------------------- */
/* Public variables --------------------------------------------------- */
/* Private variables -------------------------------------------------- */
static gpio_cfg_t m_gpio_reset_out = {PORT_1, PIN_12, GPIO_FUNC_OUT, GPIO_PAD_NONE};
static gpio_cfg_t m_gpio_mfio_out = {PORT_1, PIN_7, GPIO_FUNC_OUT, GPIO_PAD_NONE};

/* Private function prototypes ---------------------------------------- */
static void bsp_i2c_init(void);
static void bsp_gpio_init(void);
void I2C0_IRQHandler(void);

/* Function definitions ----------------------------------------------- */
void bsp_init(void)
{
  bsp_i2c_init();
  bsp_gpio_init();
}

void bsp_delay(uint32_t ms)
{
  TMR_Delay(MXC_TMR0, MSEC(ms), 0);
}

base_status_t bsp_i2c_write(uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint32_t len)
{
  int ret;
  uint8_t buff[50];

  printf("I2C writing at: 0x%X\n", reg_addr);

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

base_status_t bsp_i2c_read(uint8_t slave_addr, uint8_t *data, uint32_t len)
{
  int ret;

  printf("I2C reading\n");

  ret = I2C_MasterRead(I2C_MASTER, slave_addr, data, len, 0);
  if (ret != len)
  {
    printf("Error reading: %d\n", ret);
    return BS_ERROR;
  }

  printf("I2C reading: %d\n", ret);
  return BS_OK;
}

base_status_t bsp_i2c_read_mem(uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint32_t len)
{
  int ret;

  printf("I2C reading at: 0x%X\n", reg_addr);

  ret = I2C_MasterWrite(I2C_MASTER, slave_addr, &reg_addr, sizeof(reg_addr), 1);
  if (ret != 1)
  {
    printf("Error reading: %d\n", ret);
    return BS_ERROR;
  }

  ret = I2C_MasterRead(I2C_MASTER, slave_addr, data, len, 0);
  if (ret != len)
  {
    printf("Error reading: %d\n", ret);
    return BS_ERROR;
  }

  printf("I2C reading: %d\n", ret);
  return BS_OK;
}

void bsp_gpio_write(uint8_t pin, uint8_t state)
{
  if (pin == MAX32644_PIN_RESET)
  {
    if (state)
      GPIO_OutSet(&m_gpio_reset_out);
    else
      GPIO_OutClr(&m_gpio_reset_out);
  }
  else if (pin == MAX32644_PIN_MIFO)
  {
    if (state)
      GPIO_OutSet(&m_gpio_mfio_out);
    else
      GPIO_OutClr(&m_gpio_mfio_out);
  }
}

void I2C0_IRQHandler(void)
{
  I2C_Handler(I2C_MASTER);
  return;
}

/* Private function definitions --------------------------------------- */
static void bsp_i2c_init(void)
{
  //Setup the I2CM
  I2C_Shutdown(I2C_MASTER);
  I2C_Init(I2C_MASTER, I2C_FAST_MODE, NULL);
  NVIC_EnableIRQ(I2C0_IRQn);
}

static void bsp_gpio_init(void)
{
  GPIO_Config(&m_gpio_reset_out);
  GPIO_Config(&m_gpio_mfio_out);
}

/* End of file -------------------------------------------------------- */
