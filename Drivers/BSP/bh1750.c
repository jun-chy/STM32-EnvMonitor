/**
 * @file    bh1750.c
 * @brief   BH1750 光照传感器驱动实现
 * @details 基于HAL库I2C接口，支持单次/连续测量模式
 *          测量原理：内置16位ADC，输出光照强度值
 */

#include "bh1750.h"

/* ==================== 私有函数声明 ==================== */
static HAL_StatusTypeDef BH1750_WriteCmd(uint8_t cmd);

/**
 * @brief  BH1750初始化
 * @note   发送上电命令，然后配置为连续高分辨率模式
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef BH1750_Init(void)
{
    HAL_StatusTypeDef status;

    /* 发送上电命令 */
    status = BH1750_WriteCmd(BH1750_CMD_POWER_ON);
    if (status != HAL_OK) {
        return status;
    }

    /* 发送复位命令 (清除数据寄存器) */
    status = BH1750_WriteCmd(BH1750_CMD_RESET);
    if (status != HAL_OK) {
        return status;
    }

    /* 配置为连续高分辨率模式，分辨率1lx，典型测量时间120ms */
    status = BH1750_WriteCmd(BH1750_CMD_CONT_H_RES);

    return status;
}

/**
 * @brief  读取光照值
 * @param  lux: 返回的光照值 (单位: lux)
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 * @note   连续模式下直接读取即可，返回值为实际光照强度
 *         高分辨率模式: 1 count = 1 lux
 */
HAL_StatusTypeDef BH1750_ReadLux(float *lux)
{
    uint16_t raw_data;
    HAL_StatusTypeDef status;

    status = BH1750_ReadRaw(&raw_data);
    if (status != HAL_OK) {
        return status;
    }

    /* 高分辨率模式下，原始值即为lux值 */
    /* 低分辨率模式需乘以系数，连续高分辨率直接使用 */
    *lux = (float)raw_data;

    return HAL_OK;
}

/**
 * @brief  读取原始光照数据
 * @param  raw_data: 返回的16位原始数据
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef BH1750_ReadRaw(uint16_t *raw_data)
{
    uint8_t buf[2];
    HAL_StatusTypeDef status;

    /* 从BH1750读取2字节数据 (高字节在前) */
    status = HAL_I2C_Master_Receive(&BH1750_I2C_HANDLE, BH1750_I2C_ADDR,
                                     buf, 2, BH1750_I2C_TIMEOUT);
    if (status != HAL_OK) {
        return status;
    }

    /* 合并高低字节为16位值 */
    *raw_data = ((uint16_t)buf[0] << 8) | buf[1];

    return HAL_OK;
}

/**
 * @brief  进入低功耗模式
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef BH1750_PowerDown(void)
{
    return BH1750_WriteCmd(BH1750_CMD_POWER_DOWN);
}

/**
 * @brief  唤醒BH1750
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef BH1750_PowerOn(void)
{
    return BH1750_WriteCmd(BH1750_CMD_POWER_ON);
}

/**
 * @brief  向BH1750发送单字节命令
 * @param  cmd: 命令字节
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
static HAL_StatusTypeDef BH1750_WriteCmd(uint8_t cmd)
{
    return HAL_I2C_Master_Transmit(&BH1750_I2C_HANDLE, BH1750_I2C_ADDR,
                                   &cmd, 1, BH1750_I2C_TIMEOUT);
}
