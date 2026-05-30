# STM32-EnvMonitor CMakeLists.txt
# 构建工具: arm-none-eabi-gcc + CMake + Ninja (或 Make)
#
# 使用方法:
#   mkdir build && cd build
#   cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake ..
#   make
#
# 注意: 需要自行准备 STM32F4 HAL库 和 FreeRTOS 源码
#       并设置 STM32_HAL_PATH 和 FREERTOS_PATH 变量

cmake_minimum_required(VERSION 3.20)

# ==================== 项目名称 ====================
project(STM32_EnvMonitor C ASM)

# ==================== 编译器设置 ====================
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# ARM GCC 工具链 (可通过环境变量或 -D 参数覆盖)
if(NOT DEFINED ARM_TOOLCHAIN_DIR)
    set(ARM_TOOLCHAIN_DIR "C:/Program Files/GNU Arm Embedded Toolchain/10 2021.10/bin")
endif()

set(CMAKE_C_COMPILER   ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-gcc.exe)
set(CMAKE_ASM_COMPILER ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-gcc.exe)
set(CMAKE_AR           ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-ar.exe)
set(CMAKE_OBJCOPY      ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-objcopy.exe)
set(CMAKE_OBJDUMP      ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-objdump.exe)
set(CMAKE_SIZE         ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-size.exe)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# ==================== 目标芯片 ====================
set(MCU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(COMMON_FLAGS "${MCU_FLAGS} -Wall -Wextra -fdata-sections -ffunction-sections")

# ==================== HAL库路径 (需根据实际安装修改) ====================
if(NOT DEFINED STM32_HAL_PATH)
    set(STM32_HAL_PATH "${CMAKE_SOURCE_DIR}/Libraries/STM32F4xx_HAL_Driver")
endif()

if(NOT DEFINED CMSIS_PATH)
    set(CMSIS_PATH "${CMAKE_SOURCE_DIR}/Libraries/CMSIS")
endif()

if(NOT DEFINED FREERTOS_PATH)
    set(FREERTOS_PATH "${CMAKE_SOURCE_DIR}/Libraries/FreeRTOS")
endif()

# ==================== 源文件 ====================
set(SOURCES
    # Core
    Core/Src/main.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_gpio.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_i2c.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_uart.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_cortex.c
    ${STM32_HAL_PATH}/Src/stm32f4xx_hal_rcc.c
    # 启动文件
    ${CMSIS_PATH}/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f407xx.s
    # BSP驱动
    Drivers/BSP/bh1750.c
    Drivers/BSP/bmp280.c
    Drivers/BSP/aht20.c
    Drivers/BSP/oled.c
    # 中间件
    Drivers/Middleware/sensor_manager.c
    # 用户任务
    User/Src/sensor_task.c
    User/Src/display_task.c
    User/Src/report_task.c
    User/Src/led_task.c
    # FreeRTOS源码
    ${FREERTOS_PATH}/Source/tasks.c
    ${FREERTOS_PATH}/Source/queue.c
    ${FREERTOS_PATH}/Source/list.c
    ${FREERTOS_PATH}/Source/timers.c
    ${FREERTOS_PATH}/Source/croutine.c
    ${FREERTOS_PATH}/Source/event_groups.c
    ${FREERTOS_PATH}/Source/stream_buffer.c
    ${FREERTOS_PATH}/Source/portable/MemMang/heap_4.c
    ${FREERTOS_PATH}/Source/portable/GCC/ARM_CM4F/port.c
)

# ==================== 头文件路径 ====================
set(INCLUDE_DIRS
    Core/Inc
    Drivers/BSP
    Drivers/Middleware
    User/Inc
    ${STM32_HAL_PATH}/Inc
    ${CMSIS_PATH}/Device/ST/STM32F4xx/Include
    ${CMSIS_PATH}/Include
    ${FREERTOS_PATH}/Source/include
    ${FREERTOS_PATH}/Source/portable/GCC/ARM_CM4F
)

# ==================== 编译定义 ====================
set(DEFINITIONS
    USE_HAL_DRIVER
    STM32F407xx
    HSE_VALUE=8000000
)

# ==================== 链接脚本 ====================
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32F407VGTx.ld)

# ==================== 创建可执行目标 ====================
add_executable(${PROJECT_NAME}.elf ${SOURCES})

target_include_directories(${PROJECT_NAME}.elf PRIVATE ${INCLUDE_DIRS})
target_compile_definitions(${PROJECT_NAME}.elf PRIVATE ${DEFINITIONS})
target_compile_options(${PROJECT_NAME}.elf PRIVATE ${COMMON_FLAGS} -O2 -g3)

# 链接选项
target_link_options(${PROJECT_NAME}.elf PRIVATE
    ${MCU_FLAGS}
    -T${LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,-Map=${PROJECT_NAME}.map
    -specs=nano.specs
    -specs=nosys.specs
    -u _printf_float
)

# ==================== 生成二进制文件 ====================
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "生成 .bin 和 .hex 文件"
)

# ==================== 清理扩展 ====================
set_property(DIRECTORY PROPERTY ADDITIONAL_CLEAN_FILES
    "${PROJECT_NAME}.bin"
    "${PROJECT_NAME}.hex"
    "${PROJECT_NAME}.map"
)
