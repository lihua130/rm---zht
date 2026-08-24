#include "ssd1306.h"
#include "stm32f103c8_regs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define OLED_ADDRESS 0x3CU
#define OLED_WIDTH 128U
#define OLED_HEIGHT 64U
#define I2C_TIMEOUT 100000UL

static uint8_t framebuffer_[OLED_WIDTH * OLED_HEIGHT / 8U];

static bool wait_for_set(volatile uint32_t *reg, uint32_t mask)
{
    uint32_t timeout = I2C_TIMEOUT;
    while (((*reg) & mask) == 0U)
    {
        if (--timeout == 0U)
            return false;
    }
    return true;
}

static bool wait_for_clear(volatile uint32_t *reg, uint32_t mask)
{
    uint32_t timeout = I2C_TIMEOUT;
    while (((*reg) & mask) != 0U)
    {
        if (--timeout == 0U)
            return false;
    }
    return true;
}

static void i2c_recover(void)
{
    I2C1->CR1 |= I2C_CR1_STOP;
    I2C1->CR1 |= I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_SWRST;
    I2C1->CR2 = 8U;
    I2C1->OAR1 = 1U << 14U;
    I2C1->OAR2 = 0U;
    I2C1->CCR = 40U;
    I2C1->TRISE = 9U;
    I2C1->CR1 = I2C_CR1_PE;
}

static bool i2c_begin(void)
{
    volatile uint32_t discard;

    if (!wait_for_clear(&I2C1->SR2, I2C_SR2_BUSY))
    {
        i2c_recover();
        return false;
    }
    I2C1->CR1 |= I2C_CR1_START;
    if (!wait_for_set(&I2C1->SR1, I2C_SR1_SB))
        return false;

    I2C1->DR = OLED_ADDRESS << 1U;
    if (!wait_for_set(&I2C1->SR1, I2C_SR1_ADDR))
        return false;

    discard = I2C1->SR1;
    discard = I2C1->SR2;
    (void)discard;
    return true;
}

static bool i2c_write(uint8_t value)
{
    if (!wait_for_set(&I2C1->SR1, I2C_SR1_TXE))
        return false;
    I2C1->DR = value;
    return true;
}

static bool i2c_end(void)
{
    if (!wait_for_set(&I2C1->SR1, I2C_SR1_BTF))
    {
        i2c_recover();
        return false;
    }
    I2C1->CR1 |= I2C_CR1_STOP;
    return true;
}

static void write_commands(const uint8_t *commands, size_t count)
{
    size_t i;
    if (!i2c_begin() || !i2c_write(0x00U))
    {
        i2c_recover();
        return;
    }
    for (i = 0U; i < count; ++i)
    {
        if (!i2c_write(commands[i]))
        {
            i2c_recover();
            return;
        }
    }
    (void)i2c_end();
}

static void clear_buffer(void)
{
    size_t i;
    for (i = 0U; i < sizeof(framebuffer_); ++i)
        framebuffer_[i] = 0U;
}

static void update_display(void)
{
    uint8_t page;
    uint8_t column;
    for (page = 0U; page < 8U; ++page)
    {
        const uint8_t page_commands[] = {
            (uint8_t)(0xB0U + page), 0x00U, 0x10U};
        write_commands(page_commands, sizeof(page_commands));

        if (!i2c_begin() || !i2c_write(0x40U))
        {
            i2c_recover();
            return;
        }
        for (column = 0U; column < OLED_WIDTH; ++column)
        {
            if (!i2c_write(framebuffer_[page * OLED_WIDTH + column]))
            {
                i2c_recover();
                return;
            }
        }
        if (!i2c_end())
            return;
    }
}

static const uint8_t *glyph(char character)
{
    static const uint8_t digits[10][5] = {
        {0x3EU, 0x51U, 0x49U, 0x45U, 0x3EU},
        {0x00U, 0x42U, 0x7FU, 0x40U, 0x00U},
        {0x42U, 0x61U, 0x51U, 0x49U, 0x46U},
        {0x21U, 0x41U, 0x45U, 0x4BU, 0x31U},
        {0x18U, 0x14U, 0x12U, 0x7FU, 0x10U},
        {0x27U, 0x45U, 0x45U, 0x45U, 0x39U},
        {0x3CU, 0x4AU, 0x49U, 0x49U, 0x30U},
        {0x01U, 0x71U, 0x09U, 0x05U, 0x03U},
        {0x36U, 0x49U, 0x49U, 0x49U, 0x36U},
        {0x06U, 0x49U, 0x49U, 0x29U, 0x1EU}};
    static const uint8_t x_glyph[5] = {0x63U, 0x14U, 0x08U, 0x14U, 0x63U};
    static const uint8_t y_glyph[5] = {0x03U, 0x04U, 0x78U, 0x04U, 0x03U};
    static const uint8_t colon_glyph[5] = {0x00U, 0x36U, 0x36U, 0x00U, 0x00U};
    static const uint8_t dash_glyph[5] = {0x08U, 0x08U, 0x08U, 0x08U, 0x08U};
    static const uint8_t blank_glyph[5] = {0U, 0U, 0U, 0U, 0U};

    if (character >= '0' && character <= '9')
        return digits[(unsigned int)(character - '0')];
    if (character == 'X')
        return x_glyph;
    if (character == 'Y')
        return y_glyph;
    if (character == ':')
        return colon_glyph;
    if (character == '-')
        return dash_glyph;
    return blank_glyph;
}

static void draw_character(uint8_t x, uint8_t y, char character, uint8_t scale)
{
    const uint8_t *bitmap = glyph(character);
    uint8_t column;
    uint8_t row;
    uint8_t dx;
    uint8_t dy;

    for (column = 0U; column < 5U; ++column)
    {
        for (row = 0U; row < 7U; ++row)
        {
            if ((bitmap[column] & (1U << row)) == 0U)
                continue;

            for (dx = 0U; dx < scale; ++dx)
            {
                for (dy = 0U; dy < scale; ++dy)
                {
                    uint8_t px = (uint8_t)(x + column * scale + dx);
                    uint8_t py = (uint8_t)(y + row * scale + dy);
                    if (px < OLED_WIDTH && py < OLED_HEIGHT)
                        framebuffer_[px + (py / 8U) * OLED_WIDTH] |=
                            (uint8_t)(1U << (py & 7U));
                }
            }
        }
    }
}

static void draw_text(uint8_t x, uint8_t y, const char *text, uint8_t scale)
{
    while (*text != '\0')
    {
        draw_character(x, y, *text, scale);
        x = (uint8_t)(x + 6U * scale);
        ++text;
    }
}

static void uint16_to_text(uint16_t value, char text[6])
{
    char reversed[5];
    uint8_t count = 0U;
    uint8_t i;

    do
    {
        reversed[count++] = (char)('0' + value % 10U);
        value = (uint16_t)(value / 10U);
    } while (value != 0U && count < 5U);

    for (i = 0U; i < count; ++i)
        text[i] = reversed[count - i - 1U];
    text[count] = '\0';
}

void ssd1306_init(void)
{
    static const uint8_t init_commands[] = {
        0xAEU, 0x20U, 0x02U, 0xB0U, 0xC8U, 0x00U, 0x10U, 0x40U,
        0x81U, 0x7FU, 0xA1U, 0xA6U, 0xA8U, 0x3FU, 0xA4U, 0xD3U,
        0x00U, 0xD5U, 0x80U, 0xD9U, 0xF1U, 0xDAU, 0x12U, 0xDBU,
        0x40U, 0x8DU, 0x14U, 0xAFU};

    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    I2C1->CR1 = I2C_CR1_SWRST;
    I2C1->CR1 = 0U;
    I2C1->CR2 = 8U;
    I2C1->OAR1 = 1U << 14U;
    I2C1->OAR2 = 0U;
    I2C1->CCR = 40U;
    I2C1->TRISE = 9U;
    I2C1->CR1 = I2C_CR1_PE;

    write_commands(init_commands, sizeof(init_commands));
    clear_buffer();
    update_display();
}

void ssd1306_show_waiting(void)
{
    clear_buffer();
    draw_text(22U, 22U, "X:----", 2U);
    draw_text(22U, 42U, "Y:----", 2U);
    update_display();
}

void ssd1306_show_coordinates(uint16_t x, uint16_t y)
{
    char x_text[6];
    char y_text[6];

    uint16_to_text(x, x_text);
    uint16_to_text(y, y_text);
    clear_buffer();
    draw_text(10U, 10U, "X:", 2U);
    draw_text(34U, 10U, x_text, 2U);
    draw_text(10U, 38U, "Y:", 2U);
    draw_text(34U, 38U, y_text, 2U);
    update_display();
}
