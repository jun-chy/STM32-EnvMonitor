/**
 * @file    report_task.c
 * @brief   UART数据上报任务
 * @details 通过USART1将传感器数据以JSON格式上报
 *          上报协议: 自定义JSON格式
 *          上报周期: 2000ms
 *          波特率: 115200
 *          JSON格式: {"T":25.5,"H":60.2,"L":450,"P":1013.2,"ts":12345}
 * @version 1.0.0
 * @date    2026-05-30
 */

#include "task_define.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* ==================== 上报参数 ==================== */
#define REPORT_INTERVAL_MS         2000    /* 上报周期 (毫秒) */
#define REPORT_UART_TIMEOUT        100     /* UART发送超时 (毫秒) */
#define REPORT_JSON_BUF_SIZE       128     /* JSON缓冲区大小 */
#define REPORT_UART_HANDLE         huart1  /* 使用的UART外设 */

/* ==================== UART extern声明 (在main.c中定义) ==================== */
extern UART_HandleTypeDef huart1;

/* ==================== 私有函数声明 ==================== */
static int Report_Snprintf(char *buf, uint16_t size, const char *fmt, ...);

/**
 * @brief  UART数据上报任务
 * @param  pvParameters: 未使用
 * @note   主循环:
 *         1. 从队列获取最新传感器数据
 *         2. 格式化为JSON字符串
 *         3. 通过USART1发送
 *         4. 等待下一上报周期
 */
void ReportTask(void *pvParameters)
{
    (void)pvParameters;
    SensorMsg_t msg;
    char json_buf[REPORT_JSON_BUF_SIZE];
    uint16_t json_len;

    /* 等待系统初始化完成 */
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* 发送启动消息 */
    const char *startup_msg = "{\"type\":\"boot\",\"device\":\"STM32-EnvMonitor\",\"ver\":\"1.0\"}\r\n";
    HAL_UART_Transmit(&REPORT_UART_HANDLE, (uint8_t *)startup_msg,
                      strlen(startup_msg), REPORT_UART_TIMEOUT);

    for (;;) {
        /* 从队列获取最新数据 (阻塞等待, 超时5秒) */
        if (xQueueReceive(xSensorDataQueue, &msg, pdMS_TO_TICKS(5000)) == pdTRUE) {

            /* 格式化为JSON字符串 */
            json_len = (uint16_t)Report_Snprintf(json_buf, sizeof(json_buf),
                "{\"T\":%.1f,\"H\":%.1f,\"L\":%.0f,\"P\":%.1f,\"ts\":%lu}\r\n",
                msg.temperature,
                msg.humidity,
                msg.light,
                msg.pressure,
                (unsigned long)msg.timestamp);

            /* 通过UART发送JSON数据 */
            HAL_UART_Transmit(&REPORT_UART_HANDLE, (uint8_t *)json_buf,
                              json_len, REPORT_UART_TIMEOUT);
        }

        /* 等待下一上报周期 */
        vTaskDelay(pdMS_TO_TICKS(REPORT_INTERVAL_MS));
    }
}

/**
 * @brief  安全的sprintf包装 (防止缓冲区溢出)
 * @param  buf:  目标缓冲区
 * @param  size: 缓冲区大小
 * @param  fmt:  格式化字符串
 * @retval 实际写入的字符数
 */
static int Report_Snprintf(char *buf, uint16_t size, const char *fmt, ...)
{
    va_list args;
    int ret;

    va_start(args, fmt);
    ret = vsnprintf(buf, size, fmt, args);
    va_end(args);

    /* 确保不超过缓冲区大小 */
    if (ret < 0 || (uint16_t)ret >= size) {
        buf[size - 1] = '\0';
        return size - 1;
    }

    return (uint16_t)ret;
}
