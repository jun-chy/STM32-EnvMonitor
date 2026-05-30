/**
 * @file    bh1750.h
 * @brief   BH1750 光照传感器驱动 (I2C接口)
 * @details 16位分辨率光照传感器，量程 1-65535 lux
 *          I2C地址: 0x23 (ADDR=LOW) 或 0x5C (ADDR=HIGH)
 *          连接至 I2C1 (PB6=SCL, PB7=SDA)
 * @version 1.0.0
 * @date    2026-05-30
 */

#ifndef __BH1750_H
#define __BH1750_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 硬件配置 ==================== */
#define BH1750_I2C_HANDLE        hi2c1          /* 使用的I2C外设 */
#define BH1750_I2C_ADDR          (0x23U << 1)   /* I2C写地址 (7位地址0x23, 左移1位) */
#define BH1750_I2C_TIMEOUT       100             /* I2C超时时间(ms) */

/* ==================== BH1750 命令定义 ==================== */
#define BH1750_CMD_POWER_DOWN    0x00    /* 断电命令 */
#define BH1750_CMD_POWER_ON      0x01    /* 上电命令 */
#define BH1750_CMD_RESET         0x07    /* 复位命令 */
#define BH1750_CMD_CONT_H_RES    0x10    /* 连续高分辨率模式 (1lx, 120ms) */
#define BH1750_CMD_CONT_H_RES2   0x11    /* 连续高分辨率模式2 (0.5lx, 120ms) */
#define BH1750_CMD_CONT_L_RES    0x13    /* 连续低分辨率模式 (4lx, 16ms) */
#define BH1750_CMD_ONE_TIME_H    0x20    /* 单次高分辨率模式 (1lx, 120ms) */
#define BH1750_CMD_ONE_TIME_H2   0x21    /* 单次高分辨率模式2 (0.5lx, 120ms) */
#define BH1750_CMD_ONE_TIME_L    0x23    /* 单次低分辨率模式 (4lx, 16ms) */

/* ==================== 函数声明 ==================== */

/**
 * @brief  BH1750初始化
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef BH1750_Init(void);

/**
 * @brief  读取光照值 (单次测量)
 * @param  lux: 返回的光照值 (单位: lux)
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef BH1750_ReadLux(float *lux);

/**
 * @brief  读取原始光照数据 (2字节)
 * @param  raw_data: 返回的原始数据指针
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef BH1750_ReadRaw(uint16_t *raw_data);

/**
 * @brief  BH1750进入低功耗模式
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef BH1750_PowerDown(void);

/**
 * @brief  BH1750唤醒
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef BH1750_PowerOn(void);

#ifdef __cplusplus
}
#endif

#endif /* __BH1750_H */
