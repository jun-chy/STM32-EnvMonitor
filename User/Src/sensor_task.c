/**
 * @file    sensor_task.c
 * @brief   传感器采集任务
 * @details 周期性采集所有传感器数据，通过队列分发给显示和上报任务
 *          采集周期: 1000ms (通过vTaskDelay实现)
 *          I2C总线访问使用信号量保护，防止多任务冲突
 * @version 1.0.0
 * @date    2026-05-30
 */

#include "task_define.h"
#include "../Drivers/BSP/aht20.h"
#include "../Drivers/BSP/bh1750.h"
#include "../Drivers/BSP/bmp280.h"
#include "../Drivers/Middleware/sensor_manager.h"

/* ==================== 采集参数 ==================== */
#define SENSOR_SAMPLE_INTERVAL_MS    1000    /* 采集周期 (毫秒) */
#define SENSOR_RETRY_COUNT          3       /* 采集失败重试次数 */

/**
 * @brief  传感器采集任务
 * @param  pvParameters: 未使用
 * @note   主循环:
 *         1. 获取I2C互斥信号量
 *         2. 调用SensorManager_UpdateAll()采集所有传感器
 *         3. 打包数据到消息结构体
 *         4. 发送到传感器数据队列
 *         5. 释放信号量
 *         6. 延时等待下一周期
 */
void SensorTask(void *pvParameters)
{
    (void)pvParameters;
    SensorMsg_t msg;
    HAL_StatusTypeDef status;
    uint8_t retry;

    /* 等待系统初始化完成 */
    vTaskDelay(pdMS_TO_TICKS(2000));

    for (;;) {
        /* 获取I2C互斥信号量 (超时100ms) */
        if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            /* 尝试采集数据 (含重试) */
            for (retry = 0; retry < SENSOR_RETRY_COUNT; retry++) {
                status = SensorManager_UpdateAll();
                if (status == HAL_OK) {
                    break;
                }
                /* 重试间隔100ms */
                vTaskDelay(pdMS_TO_TICKS(100));
            }

            /* 释放I2C互斥信号量 */
            xSemaphoreGive(xI2CMutex);

            /* 如果采集成功，发送数据到队列 */
            if (status == HAL_OK) {
                msg.timestamp = HAL_GetTick();

                /* 从传感器管理器读取各传感器值 */
                SensorManager_ReadValue(SENSOR_ID_AHT20_TEMP, &msg.temperature);
                SensorManager_ReadValue(SENSOR_ID_AHT20_HUMI, &msg.humidity);
                SensorManager_ReadValue(SENSOR_ID_BH1750_LIGHT, &msg.light);
                SensorManager_ReadValue(SENSOR_ID_BMP280_PRESSURE, &msg.pressure);

                /* 发送到数据队列 (覆盖最旧数据, 不阻塞) */
                xQueueOverwrite(xSensorDataQueue, &msg);
            }
        }

        /* 等待下一采集周期 */
        vTaskDelay(pdMS_TO_TICKS(SENSOR_SAMPLE_INTERVAL_MS));
    }
}
