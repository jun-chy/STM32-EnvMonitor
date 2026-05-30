/**
 * @file    aht20.h
 * @brief   AHT20 温湿度传感器驱动 (I2C接口)
 * @details 数字温湿度传感器，I2C接口
 *          温度范围: -40~+85°C, 精度: ±0.3°C
 *          湿度范围: 0~100%RH, 精度: ±2%RH
 *          I2C地址: 0x38 (固定)
 *          连接至 I2C1 (PB6=SCL, PB7=SDA)
 * @version 1.0.0
 * @date    2026-05-30
 */

#ifndef __AHT20_H
#define __AHT20_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 硬件配置 ==================== */
#define AHT20_I2C_HANDLE         hi2c1          /* 使用的I2C外设 */
#define AHT20_I2C_ADDR           (0x38U << 1)   /* I2C写地址 (7位地址0x38, 左移1位) */
#define AHT20_I2C_TIMEOUT        100             /* I2C超时时间(ms) */

/* ==================== AHT20 命令定义 ==================== */
#define AHT20_CMD_INIT           0xBE    /* 初始化命令 */
#define AHT20_CMD_INIT_PARAM     0x08    /* 初始化参数: 正常功耗模式 */
#define AHT20_CMD_MEASURE        0xAC    /* 触发测量命令 */
#define AHT20_CMD_MEASURE_PARAM  0x33    /* 测量参数: 正常模式 */
#define AHT20_CMD_SOFTRESET      0xBA    /* 软件复位命令 */
#define AHT20_CMD_STATUS_REG     0x71    /* 状态寄存器读取命令 */

/* ==================== 状态寄存器位定义 ==================== */
#define AHT20_STATUS_BUSY        0x80    /* Bit7: 忙标志 */
#define AHT20_STATUS_CAL_ENABLE  0x08    /* Bit3: 校准使能标志 */
#define AHT20_STATUS_BUSY_MASK   0x80    /* 忙标志掩码 */
#define AHT20_STATUS_CAL_MASK    0x08    /* 校准使能掩码 */

/* ==================== 数据类型定义 ==================== */

/**
 * @brief  AHT20数据结构体
 */
typedef struct {
    float temperature;    /* 温度 (单位: °C) */
    float humidity;       /* 湿度 (单位: %RH) */
    uint8_t status;       /* 传感器状态 */
} AHT20_Data_t;

/* ==================== 函数声明 ==================== */

/**
 * @brief  AHT20初始化 (含校准检查和初始化)
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef AHT20_Init(void);

/**
 * @brief  读取温湿度数据
 * @param  data: 返回的温湿度数据结构体指针
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef AHT20_ReadData(AHT20_Data_t *data);

/**
 * @brief  仅读取温度
 * @param  temperature: 返回温度值 (单位: °C)
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef AHT20_ReadTemperature(float *temperature);

/**
 * @brief  仅读取湿度
 * @param  humidity: 返回湿度值 (单位: %RH)
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef AHT20_ReadHumidity(float *humidity);

/**
 * @brief  软件复位AHT20
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef AHT20_SoftReset(void);

/**
 * @brief  读取状态寄存器
 * @param  status: 返回状态值
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef AHT20_ReadStatus(uint8_t *status);

#ifdef __cplusplus
}
#endif

#endif /* __AHT20_H */
