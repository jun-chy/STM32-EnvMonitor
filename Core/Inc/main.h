/**
 * @file    main.h
 * @brief   主头文件 - 引脚定义和外设声明
 * @details 包含所有GPIO引脚定义、外设句柄extern声明
 *          以及系统级配置宏
 * @version 1.0.0
 * @date    2026-05-30
 */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 芯片头文件 ==================== */
#include "stm32f4xx_hal.h"

/* ==================== 引脚定义 ==================== */

/* I2C1 引脚 (传感器总线) */
#define I2C1_SCL_PIN         GPIO_PIN_6     /* PB6: I2C1 时钟线 */
#define I2C1_SCL_PORT        GPIOB
#define I2C1_SDA_PIN         GPIO_PIN_7     /* PB7: I2C1 数据线 */
#define I2C1_SDA_PORT        GPIOB

/* USART1 引脚 (数据上报) */
#define USART1_TX_PIN        GPIO_PIN_9     /* PA9: USART1 发送 */
#define USART1_TX_PORT       GPIOA
#define USART1_RX_PIN        GPIO_PIN_10    /* PA10: USART1 接收 */
#define USART1_RX_PORT       GPIOA

/* OLED 复位引脚 */
#define OLED_RST_PIN         GPIO_PIN_8     /* PA8: OLED 复位 */
#define OLED_RST_PORT        GPIOA

/* 板载LED */
#define BOARD_LED_PIN        GPIO_PIN_13    /* PC13: 板载LED */
#define BOARD_LED_PORT       GPIOC

/* ==================== 外设句柄 (extern声明) ==================== */
extern I2C_HandleTypeDef hi2c1;       /* I2C1句柄 */
extern UART_HandleTypeDef huart1;     /* USART1句柄 */

/* ==================== UART配置 ==================== */
#define USART1_BAUDRATE      115200   /* UART波特率 */
#define USART1_WORDLEN       UART_WORDLENGTH_8B
#define USART1_STOPBITS      UART_STOPBITS_1
#define USART1_PARITY        UART_PARITY_NONE

/* ==================== I2C配置 ==================== */
#define I2C1_CLOCK_SPEED     400000   /* I2C时钟频率 (400kHz) */
#define I2C1_OWN_ADDR        0       /* 主机模式无需地址 */

/* ==================== 系统时钟 ==================== */
extern uint32_t SystemCoreClock;     /* 系统核心时钟 (Hz) */

/* ==================== 错误处理函数 ==================== */
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
