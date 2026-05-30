/**
 * @file    main.c
 * @brief   主函数 - 系统初始化和FreeRTOS任务创建
 * @details STM32F4环境监测系统主入口
 *          1. HAL库初始化
 *          2. 外设初始化 (I2C1, USART1, GPIO)
 *          3. 传感器管理器初始化
 *          4. 创建FreeRTOS任务和队列
 *          5. 启动调度器
 * @version 1.0.0
 * @date    2026-05-30
 */

/* ==================== 头文件包含 ==================== */
#include "main.h"
#include "task_define.h"
#include "../Drivers/BSP/oled.h"
#include "../Drivers/Middleware/sensor_manager.h"

/* ==================== 外设句柄定义 ==================== */
I2C_HandleTypeDef hi2c1;     /* I2C1句柄 */
UART_HandleTypeDef huart1;  /* USART1句柄 */

/* ==================== 私有函数声明 ==================== */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);

/* ==================== 任务句柄定义 ==================== */
TaskHandle_t xSensorTaskHandle   = NULL;
TaskHandle_t xDisplayTaskHandle = NULL;
TaskHandle_t xReportTaskHandle   = NULL;
TaskHandle_t xLedTaskHandle      = NULL;

/* ==================== 队列和信号量定义 ==================== */
QueueHandle_t xSensorDataQueue = NULL;
SemaphoreHandle_t xI2CMutex = NULL;

/**
 * @brief  主函数
 * @retval int: 不应返回 (FreeRTOS运行后进入任务调度)
 */
int main(void)
{
    /* 1. HAL库初始化 (配置SysTick为1ms) */
    HAL_Init();

    /* 2. 配置系统时钟 (HSE 8MHz -> PLL -> 168MHz) */
    SystemClock_Config();

    /* 3. 外设初始化 */
    MX_GPIO_Init();     /* GPIO初始化 (OLED_RST, LED) */
    MX_I2C1_Init();     /* I2C1初始化 (传感器和OLED总线) */
    MX_USART1_UART_Init(); /* USART1初始化 (数据上报) */

    /* 4. OLED初始化 */
    OLED_Init();

    /* 5. 传感器管理器初始化 (注册并初始化所有传感器) */
    SensorManager_Init();

    /* 6. 创建队列 */
    xSensorDataQueue = xQueueCreate(QUEUE_DEPTH_SENSOR, sizeof(SensorMsg_t));
    if (xSensorDataQueue == NULL) {
        Error_Handler();
    }

    /* 7. 创建互斥信号量 (I2C总线保护) */
    xI2CMutex = xSemaphoreCreateMutex();
    if (xI2CMutex == NULL) {
        Error_Handler();
    }

    /* 8. 创建所有FreeRTOS任务 */
    CreateAllTasks();

    /* 9. 启动FreeRTOS调度器 (正常不会返回) */
    vTaskStartScheduler();

    /* 如果调度器启动失败，进入错误处理 */
    while (1) {
        Error_Handler();
    }
}

/**
 * @brief  创建所有FreeRTOS任务
 */
void CreateAllTasks(void)
{
    /* 创建传感器采集任务 */
    xTaskCreate(SensorTask,
                "SensorTask",
                TASK_STACK_SIZE_SENSOR,
                NULL,
                TASK_PRIORITY_SENSOR,
                &xSensorTaskHandle);

    /* 创建OLED显示任务 */
    xTaskCreate(DisplayTask,
                "DisplayTask",
                TASK_STACK_SIZE_DISPLAY,
                NULL,
                TASK_PRIORITY_DISPLAY,
                &xDisplayTaskHandle);

    /* 创建UART上报任务 */
    xTaskCreate(ReportTask,
                "ReportTask",
                TASK_STACK_SIZE_REPORT,
                NULL,
                TASK_PRIORITY_REPORT,
                &xReportTaskHandle);

    /* 创建LED指示任务 */
    xTaskCreate(LedTask,
                "LedTask",
                TASK_STACK_SIZE_LED,
                NULL,
                TASK_PRIORITY_LED,
                &xLedTaskHandle);
}

/**
 * @brief  系统时钟配置
 * @note   HSE(8MHz) -> PLL -> SYSCLK(168MHz)
 *         AHB=168MHz, APB1=42MHz, APB2=84MHz
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* 配置电压调节器输出 */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* 配置HSE和PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;    /* 8MHz / 8 = 1MHz */
    RCC_OscInitStruct.PLL.PLLN = 336;  /* 1MHz * 336 = 336MHz */
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;  /* 336MHz / 2 = 168MHz */
    RCC_OscInitStruct.PLL.PLLQ = 7;    /* 48MHz (USB) */
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /* 配置系统时钟源和分频 */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;   /* AHB: 168MHz */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;     /* APB1: 42MHz */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;     /* APB2: 84MHz */
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief  GPIO初始化
 * @note   初始化OLED复位引脚和板载LED引脚
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能GPIO时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();  /* OLED_RST (PA8) */
    __HAL_RCC_GPIOC_CLK_ENABLE();  /* 板载LED (PC13) */
    __HAL_RCC_GPIOB_CLK_ENABLE();  /* I2C1 (PB6/PB7) */

    /* 配置OLED复位引脚 (PA8): 推挽输出 */
    GPIO_InitStruct.Pin = OLED_RST_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(OLED_RST_PORT, &GPIO_InitStruct);

    /* 初始状态: OLED复位引脚拉高 (非复位状态) */
    HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_SET);

    /* 配置板载LED (PC13): 推挽输出 */
    GPIO_InitStruct.Pin = BOARD_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BOARD_LED_PORT, &GPIO_InitStruct);

    /* 初始状态: LED熄灭 (PC13高电平=熄灭) */
    HAL_GPIO_WritePin(BOARD_LED_PORT, BOARD_LED_PIN, GPIO_PIN_SET);
}

/**
 * @brief  I2C1初始化
 * @note   I2C1用于连接所有传感器和OLED
 *         SCL: PB6, SDA: PB7
 *         速率: 400kHz (快速模式)
 */
static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = I2C1_CLOCK_SPEED;     /* 400kHz */
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;       /* 快速模式占空比 */
    hi2c1.Init.OwnAddress1 = 0;                    /* 主机模式 */
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief  USART1初始化
 * @note   USART1用于传感器数据上报
 *         TX: PA9, RX: PA10
 *         波特率: 115200, 8N1
 */
static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = USART1_BAUDRATE;
    huart1.Init.WordLength = USART1_WORDLEN;
    huart1.Init.StopBits = USART1_STOPBITS;
    huart1.Init.Parity = USART1_PARITY;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief  错误处理函数
 * @note   死循环，可在此添加调试输出
 */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        /* LED快闪指示错误 */
        HAL_GPIO_TogglePin(BOARD_LED_PORT, BOARD_LED_PIN);
        for (volatile uint32_t i = 0; i < 0x100000; i++) {
            /* 简单延时 */
        }
    }
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  断言失败回调
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif
