/**
 * @file    sensor_manager.c
 * @brief   传感器管理中间件实现
 * @details 统一管理所有传感器: 注册、初始化、数据更新
 *          初始化流程: 注册传感器 -> 依次初始化硬件 -> 准备就绪
 *          数据更新: 遍历所有传感器，调用对应驱动读取数据
 */

#include "sensor_manager.h"
#include "../BSP/aht20.h"
#include "../BSP/bh1750.h"
#include "../BSP/bmp280.h"
#include <string.h>

/* ==================== 全局实例 ==================== */
SensorManager_t g_sensor_manager;

/**
 * @brief  传感器管理器初始化
 * @note   1. 清零管理器 -> 2. 注册所有传感器 -> 3. 初始化硬件
 */
HAL_StatusTypeDef SensorManager_Init(void)
{
    HAL_StatusTypeDef status;

    /* 清零管理器 */
    memset(&g_sensor_manager, 0, sizeof(SensorManager_t));
    g_sensor_manager.last_update_tick = HAL_GetTick();

    /* 注册AHT20温度传感器 */
    status = SensorManager_Register(SENSOR_ID_AHT20_TEMP, SENSOR_TYPE_TEMPERATURE,
                                     "AHT20-T", "C");
    if (status != HAL_OK) return status;

    /* 注册AHT20湿度传感器 */
    status = SensorManager_Register(SENSOR_ID_AHT20_HUMI, SENSOR_TYPE_HUMIDITY,
                                     "AHT20-H", "%RH");
    if (status != HAL_OK) return status;

    /* 注册BH1750光照传感器 */
    status = SensorManager_Register(SENSOR_ID_BH1750_LIGHT, SENSOR_TYPE_LIGHT,
                                     "BH1750", "lux");
    if (status != HAL_OK) return status;

    /* 注册BMP280温度传感器 */
    status = SensorManager_Register(SENSOR_ID_BMP280_TEMP, SENSOR_TYPE_TEMPERATURE,
                                     "BMP280-T", "C");
    if (status != HAL_OK) return status;

    /* 注册BMP280气压传感器 */
    status = SensorManager_Register(SENSOR_ID_BMP280_PRESSURE, SENSOR_TYPE_PRESSURE,
                                     "BMP280-P", "hPa");
    if (status != HAL_OK) return status;

    /* 初始化各传感器硬件 */
    /* AHT20 */
    status = AHT20_Init();
    g_sensor_manager.devices[SENSOR_ID_AHT20_TEMP].status =
        (status == HAL_OK) ? SENSOR_STATUS_OK : SENSOR_STATUS_ERROR;
    g_sensor_manager.devices[SENSOR_ID_AHT20_HUMI].status =
        (status == HAL_OK) ? SENSOR_STATUS_OK : SENSOR_STATUS_ERROR;

    /* BH1750 */
    status = BH1750_Init();
    g_sensor_manager.devices[SENSOR_ID_BH1750_LIGHT].status =
        (status == HAL_OK) ? SENSOR_STATUS_OK : SENSOR_STATUS_ERROR;

    /* BMP280 */
    status = BMP280_Init();
    g_sensor_manager.devices[SENSOR_ID_BMP280_TEMP].status =
        (status == HAL_OK) ? SENSOR_STATUS_OK : SENSOR_STATUS_ERROR;
    g_sensor_manager.devices[SENSOR_ID_BMP280_PRESSURE].status =
        (status == HAL_OK) ? SENSOR_STATUS_OK : SENSOR_STATUS_ERROR;

    return HAL_OK;
}

/**
 * @brief  注册传感器
 */
HAL_StatusTypeDef SensorManager_Register(uint8_t id, SensorType_t type,
                                         const char *name, const char *unit)
{
    if (g_sensor_manager.count >= SENSOR_MAX_COUNT) {
        return HAL_ERROR;
    }

    /* 检查ID是否已存在 */
    for (uint8_t i = 0; i < g_sensor_manager.count; i++) {
        if (g_sensor_manager.devices[i].id == id) {
            return HAL_ERROR;  /* ID重复 */
        }
    }

    /* 填充设备描述符 */
    SensorDevice_t *dev = &g_sensor_manager.devices[g_sensor_manager.count];
    dev->id = id;
    dev->type = type;
    dev->status = SENSOR_STATUS_UNINIT;
    strncpy(dev->name, name, sizeof(dev->name) - 1);
    strncpy(dev->unit, unit, sizeof(dev->unit) - 1);
    dev->data.valid = 0;
    dev->data.value = 0.0f;
    dev->data.timestamp = 0;

    g_sensor_manager.count++;
    return HAL_OK;
}

/**
 * @brief  更新所有传感器数据
 * @note   依次读取各传感器最新值，更新内部缓存
 * @retval HAL_OK: 至少成功读取一个传感器
 */
HAL_StatusTypeDef SensorManager_UpdateAll(void)
{
    HAL_StatusTypeDef ret = HAL_ERROR;
    HAL_StatusTypeDef status;
    float temp, humi, lux, bmp_temp, pressure;

    /* 读取AHT20温湿度 */
    AHT20_Data_t aht_data;
    status = AHT20_ReadData(&aht_data);
    if (status == HAL_OK) {
        g_sensor_manager.devices[SENSOR_ID_AHT20_TEMP].data.value = aht_data.temperature;
        g_sensor_manager.devices[SENSOR_ID_AHT20_TEMP].data.valid = 1;
        g_sensor_manager.devices[SENSOR_ID_AHT20_TEMP].data.timestamp = HAL_GetTick();
        g_sensor_manager.devices[SENSOR_ID_AHT20_TEMP].status = SENSOR_STATUS_OK;

        g_sensor_manager.devices[SENSOR_ID_AHT20_HUMI].data.value = aht_data.humidity;
        g_sensor_manager.devices[SENSOR_ID_AHT20_HUMI].data.valid = 1;
        g_sensor_manager.devices[SENSOR_ID_AHT20_HUMI].data.timestamp = HAL_GetTick();
        g_sensor_manager.devices[SENSOR_ID_AHT20_HUMI].status = SENSOR_STATUS_OK;

        ret = HAL_OK;
    } else {
        g_sensor_manager.devices[SENSOR_ID_AHT20_TEMP].status = SENSOR_STATUS_ERROR;
        g_sensor_manager.devices[SENSOR_ID_AHT20_HUMI].status = SENSOR_STATUS_ERROR;
    }

    /* 读取BH1750光照 */
    status = BH1750_ReadLux(&lux);
    if (status == HAL_OK) {
        g_sensor_manager.devices[SENSOR_ID_BH1750_LIGHT].data.value = lux;
        g_sensor_manager.devices[SENSOR_ID_BH1750_LIGHT].data.valid = 1;
        g_sensor_manager.devices[SENSOR_ID_BH1750_LIGHT].data.timestamp = HAL_GetTick();
        g_sensor_manager.devices[SENSOR_ID_BH1750_LIGHT].status = SENSOR_STATUS_OK;
        ret = HAL_OK;
    } else {
        g_sensor_manager.devices[SENSOR_ID_BH1750_LIGHT].status = SENSOR_STATUS_ERROR;
    }

    /* 读取BMP280温度和气压 */
    status = BMP280_ReadData(&bmp_temp, &pressure);
    if (status == HAL_OK) {
        g_sensor_manager.devices[SENSOR_ID_BMP280_TEMP].data.value = bmp_temp;
        g_sensor_manager.devices[SENSOR_ID_BMP280_TEMP].data.valid = 1;
        g_sensor_manager.devices[SENSOR_ID_BMP280_TEMP].data.timestamp = HAL_GetTick();
        g_sensor_manager.devices[SENSOR_ID_BMP280_TEMP].status = SENSOR_STATUS_OK;

        g_sensor_manager.devices[SENSOR_ID_BMP280_PRESSURE].data.value = pressure;
        g_sensor_manager.devices[SENSOR_ID_BMP280_PRESSURE].data.valid = 1;
        g_sensor_manager.devices[SENSOR_ID_BMP280_PRESSURE].data.timestamp = HAL_GetTick();
        g_sensor_manager.devices[SENSOR_ID_BMP280_PRESSURE].status = SENSOR_STATUS_OK;

        ret = HAL_OK;
    } else {
        g_sensor_manager.devices[SENSOR_ID_BMP280_TEMP].status = SENSOR_STATUS_ERROR;
        g_sensor_manager.devices[SENSOR_ID_BMP280_PRESSURE].status = SENSOR_STATUS_ERROR;
    }

    g_sensor_manager.last_update_tick = HAL_GetTick();
    return ret;
}

/**
 * @brief  通过ID获取传感器设备指针
 */
SensorDevice_t *SensorManager_GetDevice(uint8_t id)
{
    for (uint8_t i = 0; i < g_sensor_manager.count; i++) {
        if (g_sensor_manager.devices[i].id == id) {
            return &g_sensor_manager.devices[i];
        }
    }
    return NULL;
}

/**
 * @brief  通过ID读取传感器值
 */
HAL_StatusTypeDef SensorManager_ReadValue(uint8_t id, float *value)
{
    SensorDevice_t *dev = SensorManager_GetDevice(id);
    if (dev == NULL || !dev->data.valid) {
        return HAL_ERROR;
    }
    *value = dev->data.value;
    return HAL_OK;
}

/**
 * @brief  获取已注册传感器数量
 */
uint8_t SensorManager_GetCount(void)
{
    return g_sensor_manager.count;
}

/**
 * @brief  获取传感器状态摘要
 * @retval 0: 全部正常, 非零: 异常数量
 */
uint8_t SensorManager_GetStatusSummary(void)
{
    uint8_t error_count = 0;
    for (uint8_t i = 0; i < g_sensor_manager.count; i++) {
        if (g_sensor_manager.devices[i].status != SENSOR_STATUS_OK) {
            error_count++;
        }
    }
    return error_count;
}
