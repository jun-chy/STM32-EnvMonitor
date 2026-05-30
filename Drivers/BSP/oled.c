/**
 * @file    oled.c
 * @brief   OLED SSD1306 显示驱动实现
 * @details 基于HAL库I2C接口，支持文字、图形绘制和专用页面显示
 *          使用显存缓冲模式，所有绘制先写入RAM，调用Refresh刷新到屏幕
 */

#include "oled.h"
#include "oledfont.h"
#include <string.h>
#include <stdio.h>

/* ==================== SSD1306 命令定义 ==================== */
#define SSD1306_CMD_SET_MUX_RATIO         0xA8    /* 设置MUX复用率 */
#define SSD1306_CMD_SET_DISPLAY_OFFSET     0xD3    /* 设置显示偏移 */
#define SSD1306_CMD_SET_START_LINE         0x40    /* 设置起始行 */
#define SSD1306_CMD_SET_SEGMENT_REMAP      0xA1    /* 列地址映射 (左右翻转) */
#define SSD1306_CMD_SET_COM_SCAN_DIR       0xC8    /* 行扫描方向 (上下翻转) */
#define SSD1306_CMD_SET_COM_PINS           0xDA    /* 设置COM引脚配置 */
#define SSD1306_CMD_SET_CONTRAST           0x81    /* 设置对比度 */
#define SSD1306_CMD_ENABLE_CHARGE_PUMP     0x8D    /* 使能内部电荷泵 */
#define SSD1306_CMD_SET_PRECHARGE          0xD9    /* 设置预充电周期 */
#define SSD1306_CMD_SET_VCOM_DESELECT      0xDB    /* 设置VCOMH电压 */
#define SSD1306_CMD_ENTIRE_DISPLAY_ON      0xA4    /* 恢复显示内容到RAM */
#define SSD1306_CMD_SET_NORMAL_DISPLAY     0xA6    /* 正常显示模式 */
#define SSD1306_CMD_SET_INVERT_DISPLAY     0xA7    /* 反色显示模式 */
#define SSD1306_CMD_DISPLAY_ON            0xAF    /* 开启显示 */
#define SSD1306_CMD_DISPLAY_OFF           0xAE     /* 关闭显示 */
#define SSD1306_CMD_SET_COLUMN_ADDR       0x21    /* 设置列地址范围 */
#define SSD1306_CMD_SET_PAGE_ADDR          0x22    /* 设置页地址范围 */

/* ==================== 显存缓冲区 ==================== */
static uint8_t OLED_Buffer[OLED_PAGES][OLED_WIDTH];  /* 8页 x 128列 */

/* ==================== 私有函数声明 ==================== */
static void OLED_WriteCmd(uint8_t cmd);
static void OLED_WriteData(uint8_t *data, uint16_t len);
static void OLED_SetCursor(uint8_t page, uint8_t col);

/* ==================== 基础操作 ==================== */

/**
 * @brief  OLED初始化
 * @note   完整的SSD1306初始化序列
 *         1. 硬件复位 -> 2. 发送初始化命令序列 -> 3. 清屏
 */
void OLED_Init(void)
{
    /* 硬件复位: RST拉低 -> 延时 -> RST拉高 */
    HAL_GPIO_WritePin(OLED_RST_GPIO_PORT, OLED_RST_GPIO_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(OLED_RST_GPIO_PORT, OLED_RST_GPIO_PIN, GPIO_PIN_SET);
    HAL_Delay(10);

    /* 发送SSD1306初始化命令序列 */
    OLED_WriteCmd(SSD1306_CMD_DISPLAY_OFF);           /* 关闭显示 */
    OLED_WriteCmd(SSD1306_CMD_SET_MUX_RATIO);        /* 设置MUX复用率 */
    OLED_WriteCmd(OLED_HEIGHT - 1);                    /* 64行 */
    OLED_WriteCmd(SSD1306_CMD_SET_DISPLAY_OFFSET);    /* 设置显示偏移 */
    OLED_WriteCmd(0x00);                               /* 无偏移 */
    OLED_WriteCmd(SSD1306_CMD_SET_START_LINE);        /* 设置起始行为0 */
    OLED_WriteCmd(SSD1306_CMD_SET_SEGMENT_REMAP);     /* 列地址映射: 左右翻转 */
    OLED_WriteCmd(SSD1306_CMD_SET_COM_SCAN_DIR);      /* 行扫描方向: 上到下 */
    OLED_WriteCmd(SSD1306_CMD_SET_COM_PINS);          /* 设置COM引脚 */
    OLED_WriteCmd(0x12);                               /* Sequential COM配置 */
    OLED_WriteCmd(SSD1306_CMD_SET_CONTRAST);          /* 设置对比度 */
    OLED_WriteCmd(0xCF);                               /* 对比度值 (外部VCC) */
    OLED_WriteCmd(SSD1306_CMD_ENABLE_CHARGE_PUMP);    /* 使能电荷泵 */
    OLED_WriteCmd(0x14);                               /* 内部DC-DC开启 */
    OLED_WriteCmd(SSD1306_CMD_SET_PRECHARGE);         /* 设置预充电周期 */
    OLED_WriteCmd(0xF1);                               /* 预充电值 (外部VCC) */
    OLED_WriteCmd(SSD1306_CMD_SET_VCOM_DESELECT);     /* 设置VCOMH电压 */
    OLED_WriteCmd(0x40);                               /* VCOMH = 0.77Vcc */
    OLED_WriteCmd(SSD1306_CMD_ENTIRE_DISPLAY_ON);       /* 恢复RAM内容 */
    OLED_WriteCmd(SSD1306_CMD_SET_NORMAL_DISPLAY);     /* 正常显示模式 */
    OLED_WriteCmd(SSD1306_CMD_DISPLAY_ON);              /* 开启显示 */

    /* 清屏 */
    OLED_Clear();
}

/**
 * @brief  清屏 (全黑)
 */
void OLED_Clear(void)
{
    memset(OLED_Buffer, 0x00, sizeof(OLED_Buffer));
    OLED_Refresh();
}

/**
 * @brief  全屏填充
 * @param  data: 填充值 (0x00=全黑, 0xFF=全白)
 */
void OLED_Fill(uint8_t data)
{
    memset(OLED_Buffer, data, sizeof(OLED_Buffer));
    OLED_Refresh();
}

/**
 * @brief  开启显示
 */
void OLED_DisplayOn(void)
{
    OLED_WriteCmd(SSD1306_CMD_DISPLAY_ON);
}

/**
 * @brief  关闭显示 (进入低功耗)
 */
void OLED_DisplayOff(void)
{
    OLED_WriteCmd(SSD1306_CMD_DISPLAY_OFF);
}

/**
 * @brief  刷新显存到屏幕
 * @note   将显存缓冲区的数据一次性写入SSD1306
 *         使用连续写入模式提高效率
 */
void OLED_Refresh(void)
{
    uint8_t page;

    for (page = 0; page < OLED_PAGES; page++) {
        /* 设置写入区域: 页地址和列地址范围 */
        OLED_WriteCmd(SSD1306_CMD_SET_PAGE_ADDR);
        OLED_WriteCmd(page);                /* 起始页 */
        OLED_WriteCmd(page);                /* 结束页 (同页) */
        OLED_WriteCmd(SSD1306_CMD_SET_COLUMN_ADDR);
        OLED_WriteCmd(0);                    /* 起始列: 0 */
        OLED_WriteCmd(OLED_WIDTH - 1);       /* 结束列: 127 */

        /* 写入该页的128字节数据 */
        OLED_WriteData(OLED_Buffer[page], OLED_WIDTH);
    }
}

/* ==================== 绘图函数 ==================== */

/**
 * @brief  设置像素点
 * @param  x: 列坐标 (0~127)
 * @param  y: 行坐标 (0~63)
 */
void OLED_SetPixel(uint8_t x, uint8_t y)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    OLED_Buffer[y / 8][x] |= (1 << (y % 8));
}

/**
 * @brief  画线 (Bresenham算法)
 */
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    int16_t dx = (x2 > x1) ? x2 - x1 : x1 - x2;
    int16_t dy = (y2 > y1) ? y2 - y1 : y1 - y2;
    int16_t sx = (x1 < x2) ? 1 : -1;
    int16_t sy = (y1 < y2) ? 1 : -1;
    int16_t err = dx - dy;

    while (1) {
        OLED_SetPixel(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        int16_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

/**
 * @brief  画矩形边框
 */
void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t i;
    for (i = 0; i < w; i++) {
        OLED_SetPixel(x + i, y);         /* 顶边 */
        OLED_SetPixel(x + i, y + h - 1); /* 底边 */
    }
    for (i = 0; i < h; i++) {
        OLED_SetPixel(x, y + i);         /* 左边 */
        OLED_SetPixel(x + w - 1, y + i); /* 右边 */
    }
}

/**
 * @brief  填充矩形
 */
void OLED_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t i, j;
    for (i = 0; i < w; i++) {
        for (j = 0; j < h; j++) {
            OLED_SetPixel(x + i, y + j);
        }
    }
}

/**
 * @brief  画圆 (中点圆算法)
 */
void OLED_DrawCircle(uint8_t x, uint8_t y, uint8_t r)
{
    int16_t a = 0, b = r;
    int16_t d = 3 - 2 * r;

    while (a <= b) {
        OLED_SetPixel(x + a, y + b);
        OLED_SetPixel(x - a, y + b);
        OLED_SetPixel(x + a, y - b);
        OLED_SetPixel(x - a, y - b);
        OLED_SetPixel(x + b, y + a);
        OLED_SetPixel(x - b, y + a);
        OLED_SetPixel(x + b, y - a);
        OLED_SetPixel(x - b, y - a);

        if (d < 0) {
            d += 4 * a + 6;
        } else {
            d += 4 * (a - b) + 10;
            b--;
        }
        a++;
    }
}

/**
 * @brief  画三角形
 */
void OLED_DrawTriangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                        uint8_t x2, uint8_t y2)
{
    OLED_DrawLine(x0, y0, x1, y1);
    OLED_DrawLine(x1, y1, x2, y2);
    OLED_DrawLine(x2, y2, x0, y0);
}

/* ==================== 字符显示函数 ==================== */

/**
 * @brief  显示ASCII字符
 * @param  x: 起始列坐标
 * @param  y: 起始行坐标 (页对齐, 即y必须是8的倍数)
 * @param  chr: ASCII字符
 * @param  size: 字号 (1=6x8, 2=8x16)
 */
void OLED_DrawChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t size)
{
    uint8_t c = chr - ' ';  /* 字模索引 */
    uint8_t i, j;

    if (size == 1) {
        /* 6x8字体 */
        for (i = 0; i < 6; i++) {
            if (x + i >= OLED_WIDTH) break;
            OLED_Buffer[y / 8][x + i] = OLED_Font6x8[c][i];
        }
    } else if (size == 2) {
        /* 8x16字体 (上下两半) */
        for (i = 0; i < 8; i++) {
            if (x + i >= OLED_WIDTH) break;
            /* 上半部分 */
            OLED_Buffer[y / 8][x + i] = OLED_Font8x16[c][i];
            /* 下半部分 */
            OLED_Buffer[y / 8 + 1][x + i] = OLED_Font8x16[c][i + 8];
        }
    }
}

/**
 * @brief  显示字符串
 * @param  x: 起始列坐标
 * @param  y: 起始页坐标 (0~7)
 * @param  str: 字符串指针
 * @param  size: 字号 (1=6x8, 2=8x16)
 */
void OLED_DrawString(uint8_t x, uint8_t y, const char *str, uint8_t size)
{
    uint8_t char_width = (size == 1) ? 6 : 8;

    while (*str != '\0') {
        if (x > OLED_WIDTH - char_width) {
            /* 超出屏幕宽度，换行 */
            x = 0;
            y += (size == 1) ? 1 : 2;
        }
        OLED_DrawChar(x, y, *str, size);
        x += char_width;
        str++;
    }
}

/**
 * @brief  显示整数
 * @param  x: 起始列坐标
 * @param  y: 起始页坐标
 * @param  num: 数值
 * @param  len: 显示位数 (不足补前导空格)
 * @param  size: 字号
 */
void OLED_DrawNumber(uint8_t x, uint8_t y, int32_t num, uint8_t len, uint8_t size)
{
    uint8_t char_width = (size == 1) ? 6 : 8;
    char buf[12];
    int32_t temp = num;
    uint8_t i, digit_count = 0;

    /* 处理负数 */
    if (temp < 0 && len > 0) {
        OLED_DrawChar(x, y, '-', size);
        x += char_width;
        len--;
        temp = -temp;
    }

    /* 计算位数 */
    if (temp == 0) {
        digit_count = 1;
    } else {
        while (temp > 0) {
            buf[digit_count++] = temp % 10 + '0';
            temp /= 10;
        }
    }

    /* 前导空格填充 */
    while (digit_count < len) {
        OLED_DrawChar(x, y, ' ', size);
        x += char_width;
        len--;
    }

    /* 倒序输出数字 */
    for (i = 0; i < digit_count; i++) {
        OLED_DrawChar(x, y, buf[digit_count - 1 - i], size);
        x += char_width;
    }
}

/**
 * @brief  显示浮点数
 * @param  x: 起始列坐标
 * @param  y: 起始页坐标
 * @param  num: 浮点数值
 * @param  int_len: 整数部分位数
 * @param  fra_len: 小数部分位数
 * @param  size: 字号
 */
void OLED_DrawFloat(uint8_t x, uint8_t y, float num, uint8_t int_len,
                     uint8_t fra_len, uint8_t size)
{
    int32_t integer_part;
    uint32_t fractional_part;
    uint8_t char_width = (size == 1) ? 6 : 8;
    uint8_t i, multiplier = 1;

    /* 处理负数 */
    if (num < 0) {
        OLED_DrawChar(x, y, '-', size);
        x += char_width;
        num = -num;
    }

    /* 分离整数和小数部分 */
    integer_part = (int32_t)num;
    for (i = 0; i < fra_len; i++) multiplier *= 10;
    fractional_part = (uint32_t)((num - integer_part) * multiplier + 0.5f);

    /* 处理小数进位 */
    if (fractional_part >= multiplier) {
        fractional_part -= multiplier;
        integer_part++;
    }

    /* 显示整数部分 */
    OLED_DrawNumber(x, y, integer_part, int_len, size);
    x += int_len * char_width;

    /* 显示小数点 */
    OLED_DrawChar(x, y, '.', size);
    x += char_width;

    /* 显示小数部分 */
    OLED_DrawNumber(x, y, (int32_t)fractional_part, fra_len, size);
}

/* ==================== 环境监测专用显示页面 ==================== */

/**
 * @brief  显示页面1: 温湿度
 * @param  temp: 温度 (°C)
 * @param  humi: 湿度 (%RH)
 */
void OLED_ShowTempHumidityPage(float temp, float humi)
{
    OLED_Clear();
    /* 标题 */
    OLED_DrawString(0, 0, "Temp/Humidity", 1);
    /* 分隔线 */
    OLED_DrawLine(0, 10, 127, 10);
    /* 温度 */
    OLED_DrawString(0, 16, "T:", 2);
    OLED_DrawFloat(20, 16, temp, 3, 1, 2);
    OLED_DrawString(90, 16, "C", 2);
    /* 湿度 */
    OLED_DrawString(0, 40, "H:", 2);
    OLED_DrawFloat(20, 40, humi, 3, 1, 2);
    OLED_DrawString(90, 40, "%", 2);
    /* 温度条形图 (简易) */
    uint8_t bar_len = (uint8_t)((temp + 10) * 0.8f); /* -10~50°C 映射到 0~48像素 */
    if (bar_len > 48) bar_len = 48;
    OLED_FillRect(70, 16, bar_len, 16);
    OLED_DrawRect(70, 16, 48, 16);
    /* 湿度条形图 */
    bar_len = (uint8_t)(humi * 0.48f); /* 0~100% 映射到 0~48像素 */
    if (bar_len > 48) bar_len = 48;
    OLED_FillRect(70, 40, bar_len, 16);
    OLED_DrawRect(70, 40, 48, 16);

    OLED_Refresh();
}

/**
 * @brief  显示页面2: 光照
 * @param  lux: 光照值 (lux)
 */
void OLED_ShowLightPage(float lux)
{
    OLED_Clear();
    /* 标题 */
    OLED_DrawString(0, 0, "Light (BH1750)", 1);
    /* 分隔线 */
    OLED_DrawLine(0, 10, 127, 10);
    /* 光照图标 (太阳形状) */
    OLED_DrawCircle(32, 32, 8);
    /* 光照值 */
    OLED_DrawString(0, 48, "L:", 2);
    if (lux > 9999) {
        OLED_DrawFloat(20, 48, lux, 2, 0, 2);
    } else {
        OLED_DrawFloat(20, 48, lux, 4, 0, 2);
    }
    OLED_DrawString(100, 48, "lx", 2);

    OLED_Refresh();
}

/**
 * @brief  显示页面3: 气压
 * @param  pressure: 气压值 (hPa)
 */
void OLED_ShowPressurePage(float pressure)
{
    OLED_Clear();
    /* 标题 */
    OLED_DrawString(0, 0, "Pressure (BMP280)", 1);
    /* 分隔线 */
    OLED_DrawLine(0, 10, 127, 10);
    /* 气压值 */
    OLED_DrawString(0, 16, "P:", 2);
    OLED_DrawFloat(20, 16, pressure, 4, 1, 2);
    OLED_DrawString(106, 16, "hPa", 1);
    /* 简易气压仪表 */
    OLED_DrawCircle(64, 44, 16);
    OLED_DrawLine(64, 44, 64, 30);  /* 指针 */
    /* 刻度 */
    OLED_DrawString(0, 48, "950", 1);
    OLED_DrawString(96, 48, "1050", 1);

    OLED_Refresh();
}

/**
 * @brief  显示页面4: 系统状态
 * @param  uptime: 运行时间 (秒)
 * @param  sensor_ok: 传感器状态 (0=异常, 1=正常)
 */
void OLED_ShowStatusPage(uint32_t uptime, uint8_t sensor_ok)
{
    uint8_t hours = (uint8_t)(uptime / 3600);
    uint8_t minutes = (uint8_t)((uptime % 3600) / 60);
    uint8_t seconds = (uint8_t)(uptime % 60);

    OLED_Clear();
    /* 标题 */
    OLED_DrawString(0, 0, "System Status", 1);
    /* 分隔线 */
    OLED_DrawLine(0, 10, 127, 10);
    /* 运行时间 */
    OLED_DrawString(0, 16, "Up:", 2);
    OLED_DrawNumber(24, 16, hours, 2, 2);
    OLED_DrawChar(42, 16, ':', 2);
    OLED_DrawNumber(52, 16, minutes, 2, 2);
    OLED_DrawChar(70, 16, ':', 2);
    OLED_DrawNumber(80, 16, seconds, 2, 2);
    /* 传感器状态 */
    OLED_DrawString(0, 40, "Sensor:", 2);
    if (sensor_ok) {
        OLED_DrawString(70, 40, "OK", 2);
    } else {
        OLED_DrawString(70, 40, "ERR", 2);
    }
    /* FreeRTOS运行指示 (方块闪烁由调用者控制) */
    OLED_DrawString(0, 56, "RTOS:Running", 1);

    OLED_Refresh();
}

/* ==================== 底层I2C通信函数 ==================== */

/**
 * @brief  发送SSD1306命令
 * @param  cmd: 命令字节
 */
static void OLED_WriteCmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};  /* 控制字节0x00: Co=0, D/C#=0 (命令) */
    HAL_I2C_Master_Transmit(&OLED_I2C_HANDLE, OLED_I2C_ADDR, buf, 2, OLED_I2C_TIMEOUT);
}

/**
 * @brief  发送SSD1306数据
 * @param  data: 数据缓冲区指针
 * @param  len: 数据长度
 */
static void OLED_WriteData(uint8_t *data, uint16_t len)
{
    /* SSD1306数据传输使用特殊模式:
     * 先发送控制字节0x40 (Co=0, D/C#=1, 数据模式)
     * 然后连续发送数据字节
     */
    uint8_t *buf = (uint8_t *)malloc(len + 1);
    if (buf == NULL) return;

    buf[0] = 0x40;  /* 控制字节: 数据模式 */
    memcpy(&buf[1], data, len);

    HAL_I2C_Master_Transmit(&OLED_I2C_HANDLE, OLED_I2C_ADDR, buf, len + 1, OLED_I2C_TIMEOUT);

    free(buf);
}

/**
 * @brief  设置光标位置
 * @param  page: 页号 (0~7)
 * @param  col: 列号 (0~127)
 */
static void OLED_SetCursor(uint8_t page, uint8_t col)
{
    OLED_WriteCmd(0xB0 | page);          /* 设置页地址 */
    OLED_WriteCmd(col & 0x0F);            /* 设置列低4位 */
    OLED_WriteCmd(0x10 | (col >> 4));     /* 设置列高4位 */
}
