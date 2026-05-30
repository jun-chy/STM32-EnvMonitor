/**
 * @file    stm32f4xx_hal_conf.h
 * @brief   HAL库配置文件 (裁剪版)
 * @details 只启用项目需要的HAL模块，减少Flash占用
 *          已启用模块: GPIO, I2C, UART, CORTEX, RCC
 *          未启用模块: SPI, USB, ETH, SDIO, CAN, DAC, ADC等
 * @version 1.0.0
 * @date    2026-05-30
 */

#ifndef __STM32F4xx_HAL_CONF_H
#define __STM32F4xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 模块使能宏 ==================== */

/* 必须启用的核心模块 */
#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED     /* Cortex-M内核中断和延迟函数 */
#define HAL_RCC_MODULE_ENABLED        /* 时钟复位控制 */

/* 项目使用的模块 */
#define HAL_GPIO_MODULE_ENABLED       /* GPIO操作 */
#define HAL_I2C_MODULE_ENABLED       /* I2C通信 (传感器+OLED) */
#define HAL_UART_MODULE_ENABLED      /* UART通信 (数据上报) */

/* 未使用的模块 (注释掉以节省Flash)
 * 如需启用，取消对应注释即可
 */
/* #define HAL_ADC_MODULE_ENABLED */
/* #define HAL_CAN_MODULE_ENABLED */
/* #define HAL_CRC_MODULE_ENABLED */
/* #define HAL_CRYP_MODULE_ENABLED */
/* #define HAL_DAC_MODULE_ENABLED */
/* #define HAL_DCMI_MODULE_ENABLED */
/* #define HAL_DMA_MODULE_ENABLED */
/* #define HAL_DMA2D_MODULE_ENABLED */
/* #define HAL_ETH_MODULE_ENABLED */
/* #define HAL_FLASH_MODULE_ENABLED */
/* #define HAL_NAND_MODULE_ENABLED */
/* #define HAL_NOR_MODULE_ENABLED */
/* #define HAL_PCCARD_MODULE_ENABLED */
/* #define HAL_SRAM_MODULE_ENABLED */
/* #define HAL_SDRAM_MODULE_ENABLED */
/* #define HAL_HASH_MODULE_ENABLED */
/* #define HAL_I2S_MODULE_ENABLED */
/* #define HAL_IRDA_MODULE_ENABLED */
/* #define HAL_SMARTCARD_MODULE_ENABLED */
/* #define HAL_SPI_MODULE_ENABLED */
/* #define HAL_TIM_MODULE_ENABLED */
/* #define HAL_WWDG_MODULE_ENABLED */
/* #define HAL_PCD_MODULE_ENABLED */
/* #define HAL_HCD_MODULE_ENABLED */
/* #define HAL_QSPI_MODULE_ENABLED */
/* #define HAL_CEC_MODULE_ENABLED */
/* #define HAL_FMPI2C_MODULE_ENABLED */
/* #define HAL_SPDIFRX_MODULE_ENABLED */
/* #define HAL_DFSDM_MODULE_ENABLED */
/* #define HAL_LPTIM_MODULE_ENABLED */
/* #define HAL_LTDC_MODULE_ENABLED */
/* #define HAL_DSI_MODULE_ENABLED */

/* 外部振荡器配置 */
#define HSE_VALUE    8000000U     /* 外部高速晶振: 8MHz */
#define HSE_STARTUP_TIMEOUT  100U /* HSE启动超时: 100ms */
#define LSE_VALUE    32768U       /* 外部低速晶振: 32.768KHz */
#define LSE_STARTUP_TIMEOUT  5000U

/* ==================== 系统配置 ==================== */
#define VDD_VALUE                    3300U  /* 供电电压: 3.3V */
#define TICK_INT_PRIORITY            0x0FU /* SysTick中断优先级 */
#define USE_RTOS                     0      /* 不在HAL中使用RTOS */
#define PREFETCH_ENABLE              1      /* 指令预取使能 */
#define INSTRUCTION_CACHE_ENABLE    1      /* 指令缓存使能 */
#define DATA_CACHE_ENABLE           1      /* 数据缓存使能 */

/* ==================== HAL包含 (根据使能宏自动包含) ==================== */
#include "stm32f4xx_hal.h"

#ifdef HAL_CORTEX_MODULE_ENABLED
#include "stm32f4xx_hal_cortex.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm32f4xx_hal_gpio.h"
#endif

#ifdef HAL_I2C_MODULE_ENABLED
#include "stm32f4xx_hal_i2c.h"
#endif

#ifdef HAL_UART_MODULE_ENABLED
#include "stm32f4xx_hal_uart.h"
#endif

#ifdef HAL_RCC_MODULE_ENABLED
#include "stm32f4xx_hal_rcc.h"
#endif

/* ==================== 断言宏 ==================== */
#ifdef USE_FULL_ASSERT
  #define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
  void assert_failed(uint8_t *file, uint32_t line);
#else
  #define assert_param(expr) ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_HAL_CONF_H */
