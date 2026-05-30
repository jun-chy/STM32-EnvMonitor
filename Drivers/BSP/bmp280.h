/**
 * @file    bmp280.h
 * @brief   BMP280 气压传感器驱动 (I2C接口)
 * @details 数字气压传感器，支持温度和气压测量
 *          温度范围: -40~+85°C, 精度: ±0.5°C
 *          气压范围: 300~1100 hPa, 精度: ±1 hPa
 *          I2C地址: 0x76 (SDO=LOW) 或 0x77 (SDO=HIGH)
 *          连接至 I2C1 (PB6=SCL, PB7=SDA)
 * @version 1.0.0
 * @date    2026-05-30
 */

#ifndef __BMP280_H
#define __BMP280_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 硬件配置 ==================== */
#define BMP280_I2C_HANDLE         hi2c1          /* 使用的I2C外设 */
#define BMP280_I2C_ADDR           (0x76U << 1)   /* I2C写地址 (7位地址0x76, 左移1位) */
#define BMP280_I2C_TIMEOUT        100             /* I2C超时时间(ms) */

/* ==================== BMP280 寄存器地址 ==================== */
#define BMP280_REG_DIG_T1_LSB    0x88    /* 温度校准参数 T1 LSB */
#define BMP280_REG_DIG_T1_MSB    0x89    /* 温度校准参数 T1 MSB */
#define BMP280_REG_DIG_T2_LSB    0x8A    /* 温度校准参数 T2 LSB */
#define BMP280_REG_DIG_T2_MSB    0x8B    /* 温度校准参数 T2 MSB */
#define BMP280_REG_DIG_T3_LSB    0x8C    /* 温度校准参数 T3 LSB */
#define BMP280_REG_DIG_T3_MSB    0x8D    /* 温度校准参数 T3 MSB */
#define BMP280_REG_DIG_P1_LSB    0x8E    /* 气压校准参数 P1 LSB */
#define BMP280_REG_DIG_P1_MSB    0x8F    /* 气压校准参数 P1 MSB */
#define BMP280_REG_DIG_P2_LSB    0x90    /* 气压校准参数 P2 LSB */
#define BMP280_REG_DIG_P2_MSB    0x91    /* 气压校准参数 P2 MSB */
#define BMP280_REG_DIG_P3_LSB    0x92    /* 气压校准参数 P3 LSB */
#define BMP280_REG_DIG_P3_MSB    0x93    /* 气压校准参数 P3 MSB */
#define BMP280_REG_DIG_P4_LSB    0x94    /* 气压校准参数 P4 LSB */
#define BMP280_REG_DIG_P4_MSB    0x95    /* 气压校准参数 P4 MSB */
#define BMP280_REG_DIG_P5_LSB    0x96    /* 气压校准参数 P5 LSB */
#define BMP280_REG_DIG_P5_MSB    0x97    /* 气压校准参数 P5 MSB */
#define BMP280_REG_DIG_P6_LSB    0x98    /* 气压校准参数 P6 LSB */
#define BMP280_REG_DIG_P6_MSB    0x99    /* 气压校准参数 P6 MSB */
#define BMP280_REG_DIG_P7_LSB    0x9A    /* 气压校准参数 P7 LSB */
#define BMP280_REG_DIG_P7_MSB    0x9B    /* 气压校准参数 P7 MSB */
#define BMP280_REG_DIG_P8_LSB    0x9C    /* 气压校准参数 P8 LSB */
#define BMP280_REG_DIG_P8_MSB    0x9D    /* 气压校准参数 P8 MSB */
#define BMP280_REG_DIG_P9_LSB    0x9E    /* 气压校准参数 P9 LSB */
#define BMP280_REG_DIG_P9_MSB    0x9F    /* 气压校准参数 P9 MSB */

#define BMP280_REG_CHIP_ID       0xD0    /* 芯片ID寄存器 */
#define BMP280_REG_RST           0xE0    /* 软件复位寄存器 */
#define BMP280_REG_STATUS        0xF3    /* 状态寄存器 */
#define BMP280_REG_CTRL_MEAS     0xF4    /* 测量控制寄存器 */
#define BMP280_REG_CONFIG        0xF5    /* 配置寄存器 */
#define BMP280_REG_PRESS_MSB     0xF7    /* 气压数据 MSB */
#define BMP280_REG_PRESS_LSB     0xF8    /* 气压数据 LSB */
#define BMP280_REG_PRESS_XLSB    0xF9    /* 气压数据 XLSB */
#define BMP280_REG_TEMP_MSB      0xFA    /* 温度数据 MSB */
#define BMP280_REG_TEMP_LSB      0xFB    /* 温度数据 LSB */
#define BMP280_REG_TEMP_XLSB     0xFC    /* 温度数据 XLSB */

/* ==================== BMP280 常量定义 ==================== */
#define BMP280_CHIP_ID_VALUE     0x58    /* BMP280芯片ID固定值 */
#define BMP280_RESET_VALUE       0xB6    /* 软件复位命令值 */

/* 过采样率设置 */
#define BMP280_OVERSAMP_SKIP     0x00    /* 跳过 */
#define BMP280_OVERSAMP_1X       0x01    /* 1倍过采样 */
#define BMP280_OVERSAMP_2X       0x02    /* 2倍过采样 */
#define BMP280_OVERSAMP_4X       0x03    /* 4倍过采样 */
#define BMP280_OVERSAMP_8X       0x04    /* 8倍过采样 */
#define BMP280_OVERSAMP_16X      0x05    /* 16倍过采样 */

/* 工作模式 */
#define BMP280_MODE_SLEEP        0x00    /* 休眠模式 */
#define BMP280_MODE_FORCED       0x01    /* 强制模式 (单次测量) */
#define BMP280_MODE_NORMAL       0x03    /* 正常模式 (连续测量) */

/* 待机时间设置 */
#define BMP280_STANDBY_0_5MS     0x00    /* 0.5ms */
#define BMP280_STANDBY_62_5MS    0x01    /* 62.5ms */
#define BMP280_STANDBY_125MS     0x02    /* 125ms */
#define BMP280_STANDBY_250MS     0x03    /* 250ms */
#define BMP280_STANDBY_500MS     0x04    /* 500ms */
#define BMP280_STANDBY_1000MS    0x05    /* 1000ms */
#define BMP280_STANDBY_2000MS    0x06    /* 2000ms */
#define BMP280_STANDBY_4000MS    0x07    /* 4000ms */

/* 滤波器设置 */
#define BMP280_FILTER_OFF        0x00    /* 滤波器关闭 */
#define BMP280_FILTER_2         0x01    /* 滤波器系数2 */
#define BMP280_FILTER_4         0x02    /* 滤波器系数4 */
#define BMP280_FILTER_8         0x03    /* 滤波器系数8 */
#define BMP280_FILTER_16        0x04    /* 滤波器系数16 */

/* ==================== 函数声明 ==================== */

/**
 * @brief  BMP280初始化
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef BMP280_Init(void);

/**
 * @brief  读取温度和气压
 * @param  temperature: 返回温度值 (单位: °C)
 * @param  pressure: 返回气压值 (单位: hPa)
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef BMP280_ReadData(float *temperature, float *pressure);

/**
 * @brief  仅读取温度
 * @param  temperature: 返回温度值 (单位: °C)
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef BMP280_ReadTemperature(float *temperature);

/**
 * @brief  仅读取气压
 * @param  pressure: 返回气压值 (单位: hPa)
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef BMP280_ReadPressure(float *pressure);

/**
 * @brief  软件复位BMP280
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef BMP280_Reset(void);

/**
 * @brief  检查BMP280芯片ID
 * @retval HAL_OK: ID正确, HAL_ERROR: ID错误
 */
HAL_StatusTypeDef BMP280_CheckID(void);

#ifdef __cplusplus
}
#endif

#endif /* __BMP280_H */
