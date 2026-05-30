/**
 * @file    led_task.c
 * @brief   LED状态指示任务
 * @details 通过LED闪烁模式指示系统运行状态
 *          LED引脚: PC13 (板载LED)
 *          闪烁模式:
 *            - 正常运行: 每500ms闪烁 (快闪)
 *            - 传感器异常: 每2000ms闪烁 (慢闪)
 *            - 初始化阶段: 常亮
 * @version 1.0.0
 * @date    2026-05-30
 */

#include "task_define.h"
#include "../Drivers/Middleware/sensor_manager.h"

/* ==================== LED配置 ==================== */
#define LED_GPIO_PORT       GPIOC       /* LED端口 */
#define LED_GPIO_PIN        GPIO_PIN_13  /* LED引脚 (板载LED) */
#define LED_BLINK_NORMAL_MS 500         /* 正常闪烁间隔 (毫秒) */
#define LED_BLINK_ERROR_MS  2000        /* 异常闪烁间隔 (毫秒) */

/**
 * @brief  LED状态指示任务
 * @param  pvParameters: 未使用
 * @note   检查传感器状态，根据状态选择不同闪烁频率
 */
void LedTask(void *pvParameters)
{
    (void)pvParameters;
    uint8_t error_count;
    TickType_t blink_interval;

    /* 等待系统初始化完成 */
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* 初始化阶段: LED常亮1秒 */
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(1000));

    for (;;) {
        /* 检查传感器状态 */
        error_count = SensorManager_GetStatusSummary();

        /* 根据状态选择闪烁频率 */
        if (error_count == 0) {
            /* 所有传感器正常: 快闪 */
            blink_interval = pdMS_TO_TICKS(LED_BLINK_NORMAL_MS);
        } else {
            /* 有传感器异常: 慢闪 */
            blink_interval = pdMS_TO_TICKS(LED_BLINK_ERROR_MS);
        }

        /* LED翻转 */
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);

        /* 等待 */
        vTaskDelay(blink_interval);
    }
}
