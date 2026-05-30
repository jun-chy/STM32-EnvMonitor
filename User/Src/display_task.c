/**
 * @file    display_task.c
 * @brief   OLED显示任务
 * @details 从队列接收传感器数据，在OLED上循环显示四个页面
 *          页面1: 温湿度 (AHT20)
 *          页面2: 光照 (BH1750)
 *          页面3: 气压 (BMP280)
 *          页面4: 系统状态
 *          每3秒自动切换页面
 * @version 1.0.0
 * @date    2026-05-30
 */

#include "task_define.h"
#include "../Drivers/BSP/oled.h"
#include "../Drivers/Middleware/sensor_manager.h"

/* ==================== 显示参数 ==================== */
#define DISPLAY_PAGE_SWITCH_MS    3000    /* 页面切换间隔 (毫秒) */
#define DISPLAY_REFRESH_MS        500     /* 同一页面内刷新间隔 (毫秒) */
#define DISPLAY_PAGE_COUNT         4       /* 总页面数 */

/**
 * @brief  OLED显示任务
 * @param  pvParameters: 未使用
 * @note   主循环:
 *         1. 从队列接收最新数据
 *         2. 根据当前页面编号调用对应页面渲染函数
 *         3. 刷新OLED屏幕
 *         4. 等待后切换到下一页面
 */
void DisplayTask(void *pvParameters)
{
    (void)pvParameters;
    SensorMsg_t msg;
    uint8_t current_page = 0;
    uint32_t uptime_seconds = 0;
    uint32_t last_switch_tick = HAL_GetTick();
    uint8_t sensor_ok;

    /* 等待系统初始化完成 */
    vTaskDelay(pdMS_TO_TICKS(2500));

    /* 初始显示 */
    OLED_ShowStatusPage(0, 0);

    for (;;) {
        /* 尝试从队列获取最新数据 (不阻塞, 队列为空时保留上次数据) */
        if (xQueuePeek(xSensorDataQueue, &msg, 0) != pdTRUE) {
            /* 队列无数据，等待一段时间 */
            vTaskDelay(pdMS_TO_TICKS(DISPLAY_REFRESH_MS));
            continue;
        }

        /* 检查是否需要切换页面 */
        if ((HAL_GetTick() - last_switch_tick) >= DISPLAY_PAGE_SWITCH_MS) {
            current_page = (current_page + 1) % DISPLAY_PAGE_COUNT;
            last_switch_tick = HAL_GetTick();
        }

        /* 根据页面编号渲染 */
        switch (current_page) {
        case 0:
            /* 页面1: 温湿度 */
            OLED_ShowTempHumidityPage(msg.temperature, msg.humidity);
            break;

        case 1:
            /* 页面2: 光照 */
            OLED_ShowLightPage(msg.light);
            break;

        case 2:
            /* 页面3: 气压 */
            OLED_ShowPressurePage(msg.pressure);
            break;

        case 3:
            /* 页面4: 系统状态 */
            uptime_seconds = msg.timestamp / 1000;
            sensor_ok = (SensorManager_GetStatusSummary() == 0) ? 1 : 0;
            OLED_ShowStatusPage(uptime_seconds, sensor_ok);
            break;

        default:
            current_page = 0;
            break;
        }

        /* 刷新间隔 */
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_REFRESH_MS));
    }
}
