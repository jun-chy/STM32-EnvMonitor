/**
 * @file    sensor_manager.h
 * @brief   传感器管理中间件 - 统一接口
 * @details 提供传感器注册/发现/读取的统一接口模式
 *          所有传感器通过统一的 SensorDevice_t 结构体管理
 *          支持运行时注册新传感器和通过ID读取数据
 * @version 1.0.0
 * @date    2026-05-30
 */

#ifndef __SENSOR_MANAGER_H
#define __SENSOR_MANAGER_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 传感器类型定义 ==================== */
typedef enum {
    SENSOR_TYPE_TEMPERATURE = 0x01,   /* 温度传感器 */
    SENSOR_TYPE_HUMIDITY    = 0x02,   /* 湿度传感器 */
    SENSOR_TYPE_LIGHT       = 0x03,   /* 光照传感器 */
    SENSOR_TYPE_PRESSURE    = 0x04,   /* 气压传感器 */
} SensorType_t;

/* ==================== 传感器状态定义 ==================== */
typedef enum {
    SENSOR_STATUS_UNINIT = 0,          /* 未初始化 */
    SENSOR_STATUS_OK     = 1,          /* 正常工作 */
    SENSOR_STATUS_ERROR  = 2,          /* 通信错误 */
    SENSOR_STATUS_TIMEOUT = 3,         /* 超时 */
} SensorStatus_t;

/* ==================== 传感器ID定义 ==================== */
#define SENSOR_ID_AHT20_TEMP       0    /* AHT20温度 */
#define SENSOR_ID_AHT20_HUMI       1    /* AHT20湿度 */
#define SENSOR_ID_BH1750_LIGHT     2    /* BH1750光照 */
#define SENSOR_ID_BMP280_TEMP       3    /* BMP280温度 */
#define SENSOR_ID_BMP280_PRESSURE   4    /* BMP280气压 */
#define SENSOR_MAX_COUNT            5    /* 最大传感器数量 */

/* ==================== 传感器数据结构体 ==================== */

/**
 * @brief  单个传感器的读取数据
 */
typedef struct {
    float value;                  /* 传感器当前值 */
    uint8_t valid;                /* 数据是否有效 (0=无效, 1=有效) */
    uint32_t timestamp;           /* 采集时间戳 (tick count) */
} SensorData_t;

/**
 * @brief  传感器设备描述符
 */
typedef struct {
    uint8_t id;                   /* 传感器唯一ID */
    SensorType_t type;           /* 传感器类型 */
    SensorStatus_t status;       /* 当前状态 */
    char name[16];                /* 传感器名称 */
    char unit[8];                 /* 数值单位 */
    SensorData_t data;            /* 最新数据 */
} SensorDevice_t;

/* ==================== 传感器管理器结构体 ==================== */

/**
 * @brief  传感器管理器 (全局实例)
 */
typedef struct {
    SensorDevice_t devices[SENSOR_MAX_COUNT];  /* 传感器设备数组 */
    uint8_t count;                              /* 已注册传感器数量 */
    uint32_t last_update_tick;                  /* 上次更新时间 */
} SensorManager_t;

/* ==================== 全局变量声明 ==================== */
extern SensorManager_t g_sensor_manager;

/* ==================== 函数声明 ==================== */

/**
 * @brief  传感器管理器初始化
 * @note   初始化所有已注册传感器的硬件
 * @retval HAL_OK: 成功
 */
HAL_StatusTypeDef SensorManager_Init(void);

/**
 * @brief  注册传感器
 * @param  id:   传感器ID
 * @param  type: 传感器类型
 * @param  name: 传感器名称字符串
 * @param  unit: 数值单位字符串
 * @retval HAL_OK: 注册成功, HAL_ERROR: 失败(已满或ID重复)
 */
HAL_StatusTypeDef SensorManager_Register(uint8_t id, SensorType_t type,
                                         const char *name, const char *unit);

/**
 * @brief  更新所有传感器数据 (调用各驱动读取)
 * @retval HAL_OK: 至少一个传感器读取成功
 */
HAL_StatusTypeDef SensorManager_UpdateAll(void);

/**
 * @brief  通过ID获取传感器设备指针
 * @param  id: 传感器ID
 * @retval 传感器设备指针, NULL表示未找到
 */
SensorDevice_t *SensorManager_GetDevice(uint8_t id);

/**
 * @brief  通过ID读取传感器值
 * @param  id:    传感器ID
 * @param  value: 返回的值指针
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef SensorManager_ReadValue(uint8_t id, float *value);

/**
 * @brief  获取传感器数量
 * @retval 已注册传感器数量
 */
uint8_t SensorManager_GetCount(void);

/**
 * @brief  获取所有传感器状态摘要
 * @retval 0: 全部正常, 非零: 有异常传感器
 */
uint8_t SensorManager_GetStatusSummary(void);

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_MANAGER_H */
