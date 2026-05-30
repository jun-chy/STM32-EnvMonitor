/**
 * @file    aht20.c
 * @brief   AHT20 温湿度传感器驱动实现
 * @details 基于HAL库I2C接口，含状态机式测量流程
 *          上电后需等待至少20ms，首次测量前需发送初始化命令
 */

#include "aht20.h"

/* ==================== 私有函数声明 ==================== */
static HAL_StatusTypeDef AHT20_WriteCmd(uint8_t cmd, uint8_t *data, uint16_t len);
static uint8_t AHT20_CalcCRC(uint8_t *data);

/**
 * @brief  AHT20初始化
 * @note   步骤: 软件复位 -> 等待 -> 检查校准状态 -> 发送初始化命令
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef AHT20_Init(void)
{
    HAL_StatusTypeDef status;
    uint8_t reg_status;

    /* 上电等待 (AHT20上电后需等待20ms) */
    HAL_Delay(40);

    /* 1. 读取状态寄存器，检查校准位 */
    status = AHT20_ReadStatus(&reg_status);
    if (status != HAL_OK) {
        return status;
    }

    /* 2. 如果校准位未使能，发送初始化命令使能校准 */
    if ((reg_status & AHT20_STATUS_CAL_MASK) == 0x00) {
        uint8_t init_data[3] = {AHT20_CMD_INIT, AHT20_CMD_INIT_PARAM, 0x00};
        status = AHT20_WriteCmd(AHT20_CMD_INIT, init_data + 1, 2);
        if (status != HAL_OK) {
            return status;
        }
        HAL_Delay(10);  /* 等待初始化完成 */
    }

    return HAL_OK;
}

/**
 * @brief  读取温湿度数据
 * @param  data: 返回的温湿度数据结构体指针
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 * @note   测量流程: 发送测量命令 -> 等待忙标志清除 -> 读取6字节数据
 *         数据格式: [状态][湿度高][湿度低][湿度CRC][温度高][温度低][温度CRC]
 */
HAL_StatusTypeDef AHT20_ReadData(AHT20_Data_t *data)
{
    HAL_StatusTypeDef status;
    uint8_t status_reg;
    uint8_t buf[6];
    uint32_t raw_hum, raw_temp;
    uint32_t wait_count = 0;

    /* 1. 发送测量命令 (AC + 33 + 00) */
    uint8_t measure_cmd[3] = {AHT20_CMD_MEASURE, AHT20_CMD_MEASURE_PARAM, 0x00};
    status = HAL_I2C_Master_Transmit(&AHT20_I2C_HANDLE, AHT20_I2C_ADDR,
                                     measure_cmd, 3, AHT20_I2C_TIMEOUT);
    if (status != HAL_OK) {
        return status;
    }

    /* 2. 等待测量完成 (忙标志清零)
     *    典型测量时间: 正常模式80ms, 慢速模式如下:
     *    轮询状态寄存器，最多等待100ms
     */
    do {
        status = AHT20_ReadStatus(&status_reg);
        if (status != HAL_OK) {
            return status;
        }
        HAL_Delay(5);
        wait_count += 5;
    } while ((status_reg & AHT20_STATUS_BUSY) && (wait_count < 100));

    /* 超时检查 */
    if (status_reg & AHT20_STATUS_BUSY) {
        return HAL_TIMEOUT;
    }

    /* 3. 读取6字节数据 */
    status = HAL_I2C_Master_Receive(&AHT20_I2C_HANDLE, AHT20_I2C_ADDR,
                                     buf, 6, AHT20_I2C_TIMEOUT);
    if (status != HAL_OK) {
        return status;
    }

    /* 4. CRC校验 (可选，此处跳过以简化代码) */
    data->status = buf[0];

    /* 5. 解析湿度数据 (20位)
     *    [buf[1]: 8位高] [buf[2]: 8位中] [buf[3]: 高4位低]
     */
    raw_hum = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | ((uint32_t)buf[3] >> 4);

    /* 6. 解析温度数据 (20位)
     *    [buf[3]: 低4位高] [buf[4]: 8位中] [buf[5]: 8位低]
     */
    raw_temp = (((uint32_t)buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | buf[5];

    /* 7. 转换为实际值
     *    湿度 = raw_hum / 2^20 * 100%
     *    温度 = raw_temp / 2^20 * 200 - 50
     */
    data->humidity = (float)raw_hum / 1048576.0f * 100.0f;
    data->temperature = (float)raw_temp / 1048576.0f * 200.0f - 50.0f;

    return HAL_OK;
}

/**
 * @brief  仅读取温度
 */
HAL_StatusTypeDef AHT20_ReadTemperature(float *temperature)
{
    AHT20_Data_t data;
    HAL_StatusTypeDef status = AHT20_ReadData(&data);
    if (status == HAL_OK) {
        *temperature = data.temperature;
    }
    return status;
}

/**
 * @brief  仅读取湿度
 */
HAL_StatusTypeDef AHT20_ReadHumidity(float *humidity)
{
    AHT20_Data_t data;
    HAL_StatusTypeDef status = AHT20_ReadData(&data);
    if (status == HAL_OK) {
        *humidity = data.humidity;
    }
    return status;
}

/**
 * @brief  软件复位AHT20
 * @note   复位后需等待至少20ms
 */
HAL_StatusTypeDef AHT20_SoftReset(void)
{
    HAL_StatusTypeDef status;

    uint8_t cmd = AHT20_CMD_SOFTRESET;
    status = HAL_I2C_Master_Transmit(&AHT20_I2C_HANDLE, AHT20_I2C_ADDR,
                                     &cmd, 1, AHT20_I2C_TIMEOUT);
    if (status == HAL_OK) {
        HAL_Delay(20);  /* 等待复位完成 */
    }
    return status;
}

/**
 * @brief  读取状态寄存器
 */
HAL_StatusTypeDef AHT20_ReadStatus(uint8_t *status)
{
    uint8_t buf[1];
    /* AHT20状态寄存器: 发送0x71读命令，器件返回1字节状态 */
    HAL_StatusTypeDef ret = HAL_I2C_Master_Receive(&AHT20_I2C_HANDLE, AHT20_I2C_ADDR,
                                                    buf, 1, AHT20_I2C_TIMEOUT);
    if (ret == HAL_OK) {
        *status = buf[0];
    }
    return ret;
}

/**
 * @brief  通用命令发送函数
 */
static HAL_StatusTypeDef AHT20_WriteCmd(uint8_t cmd, uint8_t *data, uint16_t len)
{
    uint8_t buf[3];
    buf[0] = cmd;
    buf[1] = data[0];
    buf[2] = data[1];
    return HAL_I2C_Master_Transmit(&AHT20_I2C_HANDLE, AHT20_I2C_ADDR,
                                   buf, len + 1, AHT20_I2C_TIMEOUT);
}
