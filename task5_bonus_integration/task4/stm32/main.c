/**
 * 题目4 单片机端参考代码（STM32 HAL 库，不参与电脑端 CMake 编译）
 *
 * 硬件连接（32 套件 + 面包板）：
 *   USB 转 TTL 模块：TX -> STM32 PA10(USART1_RX)，GND -> GND（注意：模块 RX 可不接）
 *   OLED(SSD1306 I2C)：SCL -> PB6，SDA -> PB7，VCC -> 3.3V，GND -> GND
 *
 * 功能：串口中断接收电脑端下发的 6 字节帧：
 *   [0xAA][xL][xH][yL][yH][0xBB]
 * 解析出装甲板像素坐标 (x, y) 并在 OLED 上显示。
 *
 * 使用前请在 CubeMX 中配置：USART1 115200 8N1 并开启接收中断，I2C1(PB6/PB7)。
 * 本文件仅给出核心逻辑骨架，OLED 驱动（ssd1306_init/ssd1306_show_num）可用
 * 常见的 SSD1306 HAL 库替代。
 */

#include "main.h"
#include <stdio.h>

extern UART_HandleTypeDef huart1;
extern I2C_HandleTypeDef hi2c1;

/* 外部 OLED 驱动接口（自行移植 SSD1306 库实现） */
extern void ssd1306_init(I2C_HandleTypeDef *hi2c);
extern void ssd1306_clear(void);
extern void ssd1306_show_string(uint8_t x, uint8_t y, const char *str);

/* 接收状态机 */
static uint8_t rx_byte_;              /* 中断接收单字节缓冲 */
static uint8_t frame_buf_[4];         /* 4 字节数据区 xL xH yL yH */
static uint8_t frame_idx_ = 0;        /* 数据区下标 */
static uint8_t frame_started_ = 0;    /* 是否已收到帧头 0xAA */
static volatile uint8_t coord_ready_ = 0; /* 一帧解析完成标志 */
static uint16_t coord_x_ = 0, coord_y_ = 0;

/* 串口接收中断回调：逐字节解析 0xAA ... 0xBB 帧 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        uint8_t b = rx_byte_;
        if (!frame_started_)
        {
            if (b == 0xAA) /* 等待帧头 */
            {
                frame_started_ = 1;
                frame_idx_ = 0;
            }
        }
        else if (frame_idx_ < 4)
        {
            frame_buf_[frame_idx_++] = b; /* 收取 4 字节数据 */
        }
        else
        {
            if (b == 0xBB) /* 校验帧尾 */
            {
                coord_x_ = (uint16_t)(frame_buf_[0] | (frame_buf_[1] << 8));
                coord_y_ = (uint16_t)(frame_buf_[2] | (frame_buf_[3] << 8));
                coord_ready_ = 1;
            }
            frame_started_ = 0; /* 无论帧尾对错都重新等帧头 */
        }
        HAL_UART_Receive_IT(&huart1, &rx_byte_, 1); /* 继续接收下一字节 */
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    /* MX_GPIO_Init(); MX_USART1_UART_Init(); MX_I2C1_Init(); 由 CubeMX 生成 */

    ssd1306_init(&hi2c1);
    ssd1306_clear();
    ssd1306_show_string(0, 0, "waiting...");

    HAL_UART_Receive_IT(&huart1, &rx_byte_, 1); /* 启动串口中断接收 */

    while (1)
    {
        if (coord_ready_)
        {
            char line[24];
            coord_ready_ = 0;

            snprintf(line, sizeof(line), "x=%d", coord_x_);
            ssd1306_show_string(0, 0, line);   /* OLED 第一行显示 x */

            snprintf(line, sizeof(line), "y=%d", coord_y_);
            ssd1306_show_string(0, 16, line);  /* OLED 第二行显示 y */
        }
    }
}
