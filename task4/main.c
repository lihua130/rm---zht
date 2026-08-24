#include "ssd1306.h"
#include "stm32f103c8_regs.h"

#include <stdbool.h>
#include <stdint.h>

enum
{
    FRAME_HEAD = 0xAA,
    FRAME_TAIL = 0xBB,
    FRAME_DATA_SIZE = 4
};

static volatile uint8_t frame_data_[FRAME_DATA_SIZE];
static volatile uint8_t frame_index_;
static volatile bool frame_started_;
static volatile bool coordinates_ready_;
static volatile uint16_t coordinate_x_;
static volatile uint16_t coordinate_y_;

static void gpio_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN |
                    RCC_APB2ENR_IOPBEN;

    /* PA9: USART1_TX, alternate-function push-pull, 50 MHz.
       PA10: USART1_RX, floating input. */
    GPIOA->CRH &= ~((0xFU << 4U) | (0xFU << 8U));
    GPIOA->CRH |= (0xBU << 4U) | (0x4U << 8U);

    /* PB6/PB7: I2C1 SCL/SDA, alternate-function open-drain, 50 MHz. */
    GPIOB->CRL &= ~((0xFU << 24U) | (0xFU << 28U));
    GPIOB->CRL |= (0xFU << 24U) | (0xFU << 28U);
}

static void usart1_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    /* Reset clock is HSI 8 MHz. BRR=69 gives approximately 115200 baud. */
    USART1->BRR = 69U;
    USART1->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE |
                  USART_CR1_UE;
    USART1->CR2 = 0U;
    USART1->CR3 = 0U;

    nvic_enable_irq(USART1_IRQn);
}

static void parse_byte(uint8_t byte)
{
    if (!frame_started_)
    {
        if (byte == FRAME_HEAD)
        {
            frame_started_ = true;
            frame_index_ = 0U;
        }
        return;
    }

    if (frame_index_ < FRAME_DATA_SIZE)
    {
        frame_data_[frame_index_++] = byte;
        return;
    }

    if (byte == FRAME_TAIL)
    {
        coordinate_x_ = (uint16_t)(frame_data_[0] |
                                   ((uint16_t)frame_data_[1] << 8U));
        coordinate_y_ = (uint16_t)(frame_data_[2] |
                                   ((uint16_t)frame_data_[3] << 8U));
        coordinates_ready_ = true;
    }

    frame_started_ = false;
    frame_index_ = 0U;
}

void USART1_IRQHandler(void)
{
    if ((USART1->SR & USART_SR_RXNE) != 0U)
        parse_byte((uint8_t)USART1->DR);
}

int main(void)
{
    uint16_t x;
    uint16_t y;

    gpio_init();
    usart1_init();
    ssd1306_init();
    ssd1306_show_waiting();

    for (;;)
    {
        if (coordinates_ready_)
        {
            disable_irq();
            x = coordinate_x_;
            y = coordinate_y_;
            coordinates_ready_ = false;
            enable_irq();

            ssd1306_show_coordinates(x, y);
        }
    }
}
