/**
 * @file    task_define.h
 * @brief   FreeRTOS 任务和队列定义
 * @details 定义所有FreeRTOS任务参数、队列句柄和信号量
 *          包含任务优先级、栈大小、队列深度等配置
 * @version 1.0.0
 * @date    2026-05-30
 */

#ifndef __TASK_DEFINE_H
#define __TASK_DEFINE_H

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 任务优先级定义 ==================== */
#define TASK_PRIORITY_SENSOR     (configMAX_PRIORITIES - 4)  /* 传感器采集: 高优先级 */
#define TASK_PRIORITY_DISPLAY    (configMAX_PRIORITIES - 3)  /* OLED显示: 中高优先级 */
#define TASK_PRIORITY_REPORT     (configMAX_PRIORITIES - 2)  /* UART上报: 中优先级 */
#define TASK_PRIORITY_LED         (configMAX_PRIORITIES - 5)  /* LED指示: 低优先级 */

/* ==================== 任务栈大小定义 (单位: 字, 1字=4字节) ==================== */
#define TASK_STACK_SIZE_SENSOR    256     /* 传感器任务: 1KB */
#define TASK_STACK_SIZE_DISPLAY   512     /* 显示任务: 2KB (字模操作需要更多栈) */
#define TASK_STACK_SIZE_REPORT    512     /* 上报任务: 2KB (JSON格式化需要更多栈) */
#define TASK_STACK_SIZE_LED       128     /* LED任务: 512B */

/* ==================== 队列定义 ==================== */

/**
 * @brief  传感器数据消息结构体 (放入队列传递给显示和上报任务)
 */
typedef struct {
    float temperature;        /* 温度 (°C) */
    float humidity;           /* 湿度 (%RH) */
    float light;              /* 光照 (lux) */
    float pressure;            /* 气压 (hPa) */
    uint32_t timestamp;        /* 采集时间戳 */
} SensorMsg_t;

/* 队列深度 */
#define QUEUE_DEPTH_SENSOR      2       /* 传感器数据队列 (保存最近2次测量) */

/* ==================== 信号量定义 ==================== */
/* 用于I2C总线互斥访问 (多传感器共享同一I2C总线) */
extern SemaphoreHandle_t xI2CMutex;

/* ==================== 队列句柄 (extern声明) ==================== */
extern QueueHandle_t xSensorDataQueue;

/* ==================== 任务句柄 (extern声明) ==================== */
extern TaskHandle_t xSensorTaskHandle;
extern TaskHandle_t xDisplayTaskHandle;
extern TaskHandle_t xReportTaskHandle;
extern TaskHandle_t xLedTaskHandle;

/* ==================== 任务函数声明 ==================== */
void SensorTask(void *pvParameters);    /* 传感器采集任务 */
void DisplayTask(void *pvParameters);    /* OLED显示任务 */
void ReportTask(void *pvParameters);     /* UART数据上报任务 */
void LedTask(void *pvParameters);       /* LED状态指示任务 */

/* ==================== 任务创建函数 ==================== */
void CreateAllTasks(void);

#ifdef __cplusplus
}
#endif

#endif /* __TASK_DEFINE_H */
