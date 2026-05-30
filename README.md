# STM32 环境监测系统 (STM32-EnvMonitor)

<p align="center">
  <img src="https://img.shields.io/badge/C-99-A8B9CC?logo=c&logoColor=white" alt="C"/>
  <img src="https://img.shields.io/badge/FreeRTOS-10.x-1C2D41" alt="FreeRTOS"/>
  <img src="https://img.shields.io/badge/STM32-F407-0328FC?logo=arm&logoColor=white" alt="STM32"/>
  <img src="https://img.shields.io/badge/License-MIT-green" alt="License"/>
  <img src="https://img.shields.io/badge/Sensors-4-informational" alt="Sensors"/>
</p>

> 基于 STM32F4 + FreeRTOS 的多传感器数据采集与显示系统，支持 AHT20（温湿度）、BH1750（光照）、BMP280（气压）传感器，OLED 实时显示，UART JSON 数据上报。

## 系统架构

```
+--------------------------------------------------+
|              STM32F407 (主控)                     |
|  +----------+  +-----------+  +-----------+      |
|  | FreeRTOS |  | Sensor    |  | OLED      |      |
|  | 调度器   |  | Manager   |  | Driver    |      |
|  +----+-----+  +-----+-----+  +-----+-----+      |
|       |               |               |            |
|  +----+----+----+-----+------+--------+---+       |
|  |         任务层                     |       |
|  | +------+ +------+ +------+ +----+ |       |
|  | |Sensor| |Display| |Report| |LED | |       |
|  | |Task  | |Task  | |Task  | |Task| |       |
|  | +--+---+ +--+---+ +--+---+ +-+--+ |       |
|  |    |        |        |       |     |       |
|  +----+--------+--------+-------+-----+       |
|       |                           |            |
|  +----+--------+--------+---------+---+       |
|  |         BSP驱动层                 |       |
|  | +------+ +------+ +------+ +----+ |       |
|  | |AHT20 | |BH1750| |BMP280| |SSD | |       |
|  | |(T/H) | |(Lux) | |(P)   | |1306| |       |
|  | +------+ +------+ +------+ +----+ |       |
|  +--------+--------+------+-------+       |
|                                                    |
|  HAL库 + FreeRTOS + CMSIS                          |
+--------------------------------------------------+
       |I2C1      |I2C1    |USART1
   +---+----+  +---+----+  +---+----+
   | AHT20  |  |BH1750 |  | PC/串口|
   | BMP280 |  |OLED   |  | 终端   |
   +--------+  +-------+  +--------+
```

## 功能特性

- **多传感器采集**: AHT20(温湿度)、BH1750(光照)、BMP280(气压)
- **实时显示**: 0.96寸OLED四页面循环显示
- **数据上报**: UART JSON格式协议, 115200波特率
- **RTOS调度**: FreeRTOS多任务, 优先级调度
- **统一接口**: 传感器管理中间件(register/get/read模式)
- **I2C总线保护**: 信号量互斥, 支持多设备共享总线
- **LED状态指示**: 根据系统状态自动切换闪烁频率

## 硬件清单

| 器件 | 型号 | 说明 |
|------|------|------|
| MCU | STM32F407VGT6 | Cortex-M4, 168MHz, 1MB Flash |
| 温湿度传感器 | AHT20 | I2C, ±0.3°C / ±2%RH |
| 光照传感器 | BH1750FVI | I2C, 1-65535 lux |
| 气压传感器 | BMP280 | I2C, 300-1100 hPa |
| OLED显示屏 | 0.96寸 SSD1306 | I2C, 128x64 |
| 晶振 | 8MHz HSE | 外部高速晶振 |

## 接线说明

```
STM32F407        AHT20/BMP280/BH1750    OLED (SSD1306)      USB-TTL
-----------      -------------------    --------------      --------
PB6 (I2C1_SCL) -- SCL                  -- SCL
PB7 (I2C1_SDA) -- SDA                  -- SDA
PA8 (GPIO)     --                      -- RST
PA9  (USART1_TX)                       -- (NC)
PA10 (USART1_RX)                       -- (NC)
3.3V           -- VCC                  -- VCC              -- VCC
GND            -- GND                  -- GND              -- GND
                                         3.3V  -- VCC
                                                        -- RX (PA9)
                                                        -- TX (PA10)

注意:
- AHT20地址: 0x38 | BMP280地址: 0x76 | BH1750地址: 0x23 | OLED地址: 0x3C
- 所有I2C设备共享同一总线 (I2C1: PB6/PB7)
- I2C上拉电阻: 使用4.7K上拉到3.3V (大多数模块已自带)
```

## UART数据协议

JSON格式, 每行一条, 115200波特率:

```json
{"type":"boot","device":"STM32-EnvMonitor","ver":"1.0"}
{"T":25.5,"H":60.2,"L":450,"P":1013.2,"ts":12345}
```

字段说明:
| 字段 | 含义 | 单位 | 范围 |
|------|------|------|------|
| T | 温度 | °C | -40~85 |
| H | 湿度 | %RH | 0~100 |
| L | 光照 | lux | 0~65535 |
| P | 气压 | hPa | 300~1100 |
| ts | 时间戳 | ms | 系统运行毫秒数 |

## 项目结构

```
STM32-EnvMonitor/
|-- CMakeLists.txt              # CMake构建文件
|-- .gitignore                  # Git忽略规则
|-- README.md                   # 项目文档
|
|-- Core/
|   |-- Inc/
|   |   |-- main.h              # 主头文件 (引脚定义/外设声明)
|   |   +-- stm32f4xx_hal_conf.h  # HAL库裁剪配置
|   +-- Src/
|       +-- main.c              # 主函数 (初始化/任务创建)
|
|-- Drivers/
|   |-- BSP/                    # 板级支持包 (硬件驱动)
|   |   |-- aht20.h/c           # AHT20温湿度驱动
|   |   |-- bh1750.h/c          # BH1750光照驱动
|   |   |-- bmp280.h/c          # BMP280气压驱动
|   |   |-- oled.h/c            # OLED SSD1306驱动
|   |   +-- oledfont.h          # OLED ASCII字模 (6x8/8x16)
|   +-- Middleware/              # 中间件
|       +-- sensor_manager.h/c  # 传感器统一管理接口
|
+-- User/
    |-- Inc/
    |   +-- task_define.h       # FreeRTOS任务/队列/信号量定义
    +-- Src/
        |-- sensor_task.c       # 传感器采集任务
        |-- display_task.c      # OLED显示任务
        |-- report_task.c       # UART数据上报任务
        +-- led_task.c          # LED状态指示任务
```

## FreeRTOS任务设计

| 任务 | 优先级 | 栈大小 | 周期 | 功能 |
|------|--------|--------|------|------|
| SensorTask | 高 | 256字 | 1000ms | 采集所有传感器数据 |
| DisplayTask | 中高 | 512字 | 500ms | OLED四页面循环显示 |
| ReportTask | 中 | 512字 | 2000ms | UART JSON数据上报 |
| LedTask | 低 | 128字 | 500ms | LED状态闪烁指示 |

## 编译

### 前提条件
- ARM GCC工具链 (arm-none-eabi-gcc)
- CMake >= 3.20
- STM32F4 HAL库
- FreeRTOS源码

### 编译步骤

1. 克隆项目:
```bash
git clone https://github.com/jun-chy/STM32-EnvMonitor.git
cd STM32-EnvMonitor

# 将HAL库和FreeRTOS放入Libraries目录
# Libraries/
#   |-- STM32F4xx_HAL_Driver/
#   |-- CMSIS/
#   +-- FreeRTOS/
```

2. 使用CMake构建:
```bash
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake ..
make -j4
```

3. 生成 `STM32_EnvMonitor.bin` 和 `STM32_EnvMonitor.hex`

### 烧录

```bash
# OpenOCD + ST-Link
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program STM32_EnvMonitor.elf verify reset exit"

# 或使用STM32CubeProgrammer
```

## 依赖

| 依赖 | 版本 | 说明 |
|------|------|------|
| STM32F4 HAL | 1.27+ | ST官方HAL库 |
| CMSIS | 5.x | ARM Cortex微控制器软件接口标准 |
| FreeRTOS | 10.x | 实时操作系统内核 |

## 开发环境

推荐使用以下任一方式:

- **STM32CubeIDE** - ST官方免费IDE, 集成CubeMX
- **VS Code + CMake** - 轻量级, 配合Cortex-Debug插件
- **Keil MDK** - 商业IDE, 需手动导入文件
- **PlatformIO** - 开源生态, 配置灵活

## 许可证

MIT License
