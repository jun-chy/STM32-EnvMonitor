/**
 * @file    bmp280.c
 * @brief   BMP280 气压传感器驱动实现
 * @details 基于HAL库I2C接口，包含温度补偿计算
 *          使用正常模式连续测量，过采样率x16，IIR滤波x4
 */

#include "bmp280.h"

/* ==================== 校准参数结构体 ==================== */
typedef struct {
    uint16_t dig_T1;    /* 温度校准参数 T1 */
    int16_t  dig_T2;    /* 温度校准参数 T2 */
    int16_t  dig_T3;    /* 温度校准参数 T3 */
    uint16_t dig_P1;    /* 气压校准参数 P1 */
    int16_t  dig_P2;    /* 气压校准参数 P2 */
    int16_t  dig_P3;    /* 气压校准参数 P3 */
    int16_t  dig_P4;    /* 气压校准参数 P4 */
    int16_t  dig_P5;    /* 气压校准参数 P5 */
    int16_t  dig_P6;    /* 气压校准参数 P6 */
    int16_t  dig_P7;    /* 气压校准参数 P7 */
    int16_t  dig_P8;    /* 气压校准参数 P8 */
    int16_t  dig_P9;    /* 气压校准参数 P9 */
    int32_t  t_fine;    /* 精细温度值 (补偿中间变量) */
} BMP280_CalibData_t;

/* 私有校准参数 */
static BMP280_CalibData_t bmp280_calib;

/* ==================== 私有函数声明 ==================== */
static HAL_StatusTypeDef BMP280_ReadCalibData(void);
static HAL_StatusTypeDef BMP280_WriteReg(uint8_t reg, uint8_t value);
static HAL_StatusTypeDef BMP280_ReadReg(uint8_t reg, uint8_t *value);
static HAL_StatusTypeDef BMP280_ReadRegs(uint8_t reg, uint8_t *buf, uint16_t len);
static int32_t  BMP280_CompensateTemp(int32_t raw_temp);
static uint32_t BMP280_CompensatePressure(int32_t raw_press, int32_t raw_temp);

/**
 * @brief  BMP280初始化
 * @note   步骤: 软件复位 -> 读取校准参数 -> 配置测量控制
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef BMP280_Init(void)
{
    HAL_StatusTypeDef status;

    /* 1. 检查芯片ID */
    status = BMP280_CheckID();
    if (status != HAL_OK) {
        return status;
    }

    /* 2. 软件复位 */
    status = BMP280_Reset();
    if (status != HAL_OK) {
        return status;
    }

    /* 复位后等待一段时间让芯片完成初始化 */
    HAL_Delay(10);

    /* 3. 读取工厂校准参数 (从0x88开始的24字节) */
    status = BMP280_ReadCalibData();
    if (status != HAL_OK) {
        return status;
    }

    /* 4. 配置测量控制寄存器 (0xF4)
     *    osrs_t[2:0] = 101 (温度过采样x16)
     *    osrs_p[4:2] = 101 (气压过采样x16)
     *    mode[1:0]    = 11  (正常模式)
     */
    status = BMP280_WriteReg(BMP280_REG_CTRL_MEAS,
                              (BMP280_OVERSAMP_16X << 5) |
                              (BMP280_OVERSAMP_16X << 2) |
                              BMP280_MODE_NORMAL);
    if (status != HAL_OK) {
        return status;
    }

    /* 5. 配置寄存器 (0xF5)
     *    t_sb[2:0]  = 100 (待机时间500ms)
     *    filter[4:2] = 010 (IIR滤波系数4)
     *    spi3w[0]   = 0   (SPI禁用, I2C模式)
     */
    status = BMP280_WriteReg(BMP280_REG_CONFIG,
                              (BMP280_STANDBY_500MS << 5) |
                              (BMP280_FILTER_4 << 2) | 0x00);

    return status;
}

/**
 * @brief  读取温度和气压
 * @param  temperature: 返回温度值 (单位: °C)
 * @param  pressure: 返回气压值 (单位: hPa)
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef BMP280_ReadData(float *temperature, float *pressure)
{
    uint8_t buf[6];
    HAL_StatusTypeDef status;
    int32_t raw_temp, raw_press;
    int32_t comp_temp;
    uint32_t comp_press;

    /* 一次性读取6字节: 气压(3字节) + 温度(3字节) */
    status = BMP280_ReadRegs(BMP280_REG_PRESS_MSB, buf, 6);
    if (status != HAL_OK) {
        return status;
    }

    /* 拼接20位原始气压数据 */
    raw_press = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | ((int32_t)buf[2] >> 4);

    /* 拼接20位原始温度数据 */
    raw_temp = ((int32_t)buf[3] << 12) | ((int32_t)buf[4] << 4) | ((int32_t)buf[5] >> 4);

    /* 温度补偿计算 */
    comp_temp = BMP280_CompensateTemp(raw_temp);

    /* 气压补偿计算 */
    comp_press = BMP280_CompensatePressure(raw_press, raw_temp);

    /* 转换为浮点数 */
    *temperature = (float)comp_temp / 100.0f;
    *pressure = (float)comp_press / 100.0f;

    return HAL_OK;
}

/**
 * @brief  仅读取温度
 * @param  temperature: 返回温度值 (单位: °C)
 */
HAL_StatusTypeDef BMP280_ReadTemperature(float *temperature)
{
    float pressure;
    return BMP280_ReadData(temperature, &pressure);
}

/**
 * @brief  仅读取气压
 * @param  pressure: 返回气压值 (单位: hPa)
 */
HAL_StatusTypeDef BMP280_ReadPressure(float *pressure)
{
    float temperature;
    return BMP280_ReadData(&temperature, pressure);
}

/**
 * @brief  软件复位BMP280
 * @note   复位后需等待至少2ms
 */
HAL_StatusTypeDef BMP280_Reset(void)
{
    HAL_StatusTypeDef status = BMP280_WriteReg(BMP280_REG_RST, BMP280_RESET_VALUE);
    if (status == HAL_OK) {
        HAL_Delay(10);  /* 等待复位完成 */
    }
    return status;
}

/**
 * @brief  检查芯片ID是否正确
 */
HAL_StatusTypeDef BMP280_CheckID(void)
{
    uint8_t chip_id;
    HAL_StatusTypeDef status = BMP280_ReadReg(BMP280_REG_CHIP_ID, &chip_id);
    if (status != HAL_OK) {
        return status;
    }
    return (chip_id == BMP280_CHIP_ID_VALUE) ? HAL_OK : HAL_ERROR;
}

/**
 * @brief  读取校准参数
 * @note   从寄存器0x88开始读取24字节的校准数据
 */
static HAL_StatusTypeDef BMP280_ReadCalibData(void)
{
    uint8_t buf[24];
    HAL_StatusTypeDef status;

    status = BMP280_ReadRegs(BMP280_REG_DIG_T1_LSB, buf, 24);
    if (status != HAL_OK) {
        return status;
    }

    /* 解析温度校准参数 */
    bmp280_calib.dig_T1 = (uint16_t)(buf[0] | (buf[1] << 8));
    bmp280_calib.dig_T2 = (int16_t)(buf[2] | (buf[3] << 8));
    bmp280_calib.dig_T3 = (int16_t)(buf[4] | (buf[5] << 8));

    /* 解析气压校准参数 */
    bmp280_calib.dig_P1 = (uint16_t)(buf[6] | (buf[7] << 8));
    bmp280_calib.dig_P2 = (int16_t)(buf[8] | (buf[9] << 8));
    bmp280_calib.dig_P3 = (int16_t)(buf[10] | (buf[11] << 8));
    bmp280_calib.dig_P4 = (int16_t)(buf[12] | (buf[13] << 8));
    bmp280_calib.dig_P5 = (int16_t)(buf[14] | (buf[15] << 8));
    bmp280_calib.dig_P6 = (int16_t)(buf[16] | (buf[17] << 8));
    bmp280_calib.dig_P7 = (int16_t)(buf[18] | (buf[19] << 8));
    bmp280_calib.dig_P8 = (int16_t)(buf[20] | (buf[21] << 8));
    bmp280_calib.dig_P9 = (int16_t)(buf[22] | (buf[23] << 8));

    return HAL_OK;
}

/**
 * @brief  温度补偿计算 (BMP280 datasheet公式)
 * @param  raw_temp: 20位原始温度值
 * @retval 补偿后的温度值 (单位: 0.01°C)
 */
static int32_t BMP280_CompensateTemp(int32_t raw_temp)
{
    int32_t var1, var2, T;

    var1 = ((((raw_temp >> 3) - ((int32_t)bmp280_calib.dig_T1 << 1))) *
             ((int32_t)bmp280_calib.dig_T2)) >> 11;

    var2 = (((((raw_temp >> 4) - ((int32_t)bmp280_calib.dig_T1)) *
              ((raw_temp >> 4) - ((int32_t)bmp280_calib.dig_T1))) >>
             12) * ((int32_t)bmp280_calib.dig_T3)) >> 14;

    bmp280_calib.t_fine = var1 + var2;
    T = (bmp280_calib.t_fine * 5 + 128) >> 8;

    return T;
}

/**
 * @brief  气压补偿计算 (BMP280 datasheet公式)
 * @param  raw_press: 20位原始气压值
 * @param  raw_temp: 20位原始温度值 (需调用以更新t_fine)
 * @retval 补偿后的气压值 (单位: 0.01 hPa = Pa)
 */
static uint32_t BMP280_CompensatePressure(int32_t raw_press, int32_t raw_temp)
{
    int64_t var1, var2, p;

    /* 先计算温度补偿 (更新t_fine) */
    BMP280_CompensateTemp(raw_temp);

    var1 = ((int64_t)bmp280_calib.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)bmp280_calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)bmp280_calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)bmp280_calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)bmp280_calib.dig_P3) >> 8) +
           ((var1 * (int64_t)bmp280_calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)bmp280_calib.dig_P1) >> 33;

    if (var1 == 0) {
        return 0;  /* 避免除零 */
    }

    p = 1048576 - (int64_t)raw_press;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)bmp280_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)bmp280_calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)bmp280_calib.dig_P7) << 4);

    return (uint32_t)p;
}

/* ==================== 底层I2C读写函数 ==================== */

static HAL_StatusTypeDef BMP280_WriteReg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return HAL_I2C_Master_Transmit(&BMP280_I2C_HANDLE, BMP280_I2C_ADDR,
                                    buf, 2, BMP280_I2C_TIMEOUT);
}

static HAL_StatusTypeDef BMP280_ReadReg(uint8_t reg, uint8_t *value)
{
    return HAL_I2C_Mem_Read(&BMP280_I2C_HANDLE, BMP280_I2C_ADDR,
                             reg, I2C_MEMADD_SIZE_8BIT, value, 1,
                             BMP280_I2C_TIMEOUT);
}

static HAL_StatusTypeDef BMP280_ReadRegs(uint8_t reg, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(&BMP280_I2C_HANDLE, BMP280_I2C_ADDR,
                              reg, I2C_MEMADD_SIZE_8BIT, buf, len,
                              BMP280_I2C_TIMEOUT);
}
