/**
 * @file    oled.h
 * @brief   OLED SSD1306 显示驱动 (I2C接口)
 * @details 0.96寸 128x64 OLED显示屏驱动
 *          控制芯片: SSD1306
 *          I2C地址: 0x3C (SA0=LOW) 或 0x3D (SA0=HIGH)
 *          复位引脚: PA8 (低电平复位)
 *          连接至 I2C1 (PB6=SCL, PB7=SDA)
 * @version 1.0.0
 * @date    2026-05-30
 */

#ifndef __OLED_H
#define __OLED_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 硬件配置 ==================== */
#define OLED_I2C_HANDLE         hi2c1          /* 使用的I2C外设 */
#define OLED_I2C_ADDR           (0x3CU << 1)   /* I2C写地址 */
#define OLED_I2C_TIMEOUT        100             /* I2C超时时间(ms) */
#define OLED_RST_GPIO_PORT      GPIOA           /* 复位引脚端口 */
#define OLED_RST_GPIO_PIN       GPIO_PIN_8      /* 复位引脚 */

/* ==================== 屏幕参数 ==================== */
#define OLED_WIDTH              128             /* 屏幕宽度(像素) */
#define OLED_HEIGHT             64              /* 屏幕高度(像素) */
#define OLED_PAGES              (OLED_HEIGHT / 8)  /* 页数 = 8 */

/* ==================== 显示颜色 ==================== */
#define OLED_COLOR_BLACK        0x00            /* 黑色(像素关闭) */
#define OLED_COLOR_WHITE        0x01            /* 白色(像素点亮) */

/* ==================== 函数声明 ==================== */

/* ---------- 基础操作 ---------- */
void OLED_Init(void);                            /* 初始化OLED */
void OLED_DeInit(void);                          /* 反初始化OLED */
void OLED_Clear(void);                           /* 清屏(全黑) */
void OLED_Fill(uint8_t data);                    /* 全屏填充 */
void OLED_DisplayOn(void);                       /* 开启显示 */
void OLED_DisplayOff(void);                      /* 关闭显示(进入睡眠) */
void OLED_Refresh(void);                         /* 刷新显存到屏幕 */

/* ---------- 绘图函数 ---------- */
void OLED_SetPixel(uint8_t x, uint8_t y);        /* 画一个像素点 */
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);  /* 画线 */
void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);      /* 画矩形 */
void OLED_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);      /* 填充矩形 */
void OLED_DrawCircle(uint8_t x, uint8_t y, uint8_t r);                /* 画圆 */
void OLED_DrawTriangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                        uint8_t x2, uint8_t y2);                       /* 画三角形 */

/* ---------- 字符显示 ---------- */
void OLED_DrawChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t size);  /* 显示ASCII字符 */
void OLED_DrawString(uint8_t x, uint8_t y, const char *str, uint8_t size); /* 显示字符串 */
void OLED_DrawNumber(uint8_t x, uint8_t y, int32_t num, uint8_t len, uint8_t size);  /* 显示整数 */
void OLED_DrawFloat(uint8_t x, uint8_t y, float num, uint8_t int_len, uint8_t fra_len, uint8_t size); /* 显示浮点数 */

/* ---------- 显示页面 (环境监测专用) ---------- */
void OLED_ShowTempHumidityPage(float temp, float humi);   /* 页面1: 温湿度 */
void OLED_ShowLightPage(float lux);                       /* 页面2: 光照 */
void OLED_ShowPressurePage(float pressure);               /* 页面3: 气压 */
void OLED_ShowStatusPage(uint32_t uptime, uint8_t sensor_ok); /* 页面4: 系统状态 */

#ifdef __cplusplus
}
#endif

#endif /* __OLED_H */
