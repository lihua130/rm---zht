#ifndef STM32F103C8_REGS_H
#define STM32F103C8_REGS_H

#include <stdint.h>

typedef struct
{
    volatile uint32_t CRL;
    volatile uint32_t CRH;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t BRR;
    volatile uint32_t LCKR;
} GPIO_TypeDef;

typedef struct
{
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t APB1ENR;
} RCC_TypeDef;

typedef struct
{
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} USART_TypeDef;

typedef struct
{
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t DR;
    volatile uint32_t SR1;
    volatile uint32_t SR2;
    volatile uint32_t CCR;
    volatile uint32_t TRISE;
} I2C_TypeDef;

#define RCC ((RCC_TypeDef *)0x40021000UL)
#define GPIOA ((GPIO_TypeDef *)0x40010800UL)
#define GPIOB ((GPIO_TypeDef *)0x40010C00UL)
#define USART1 ((USART_TypeDef *)0x40013800UL)
#define I2C1 ((I2C_TypeDef *)0x40005400UL)

#define RCC_APB2ENR_AFIOEN (1UL << 0U)
#define RCC_APB2ENR_IOPAEN (1UL << 2U)
#define RCC_APB2ENR_IOPBEN (1UL << 3U)
#define RCC_APB2ENR_USART1EN (1UL << 14U)
#define RCC_APB1ENR_I2C1EN (1UL << 21U)

#define USART_SR_RXNE (1UL << 5U)
#define USART_CR1_RE (1UL << 2U)
#define USART_CR1_TE (1UL << 3U)
#define USART_CR1_RXNEIE (1UL << 5U)
#define USART_CR1_UE (1UL << 13U)

#define I2C_CR1_PE (1UL << 0U)
#define I2C_CR1_START (1UL << 8U)
#define I2C_CR1_STOP (1UL << 9U)
#define I2C_CR1_SWRST (1UL << 15U)
#define I2C_SR1_SB (1UL << 0U)
#define I2C_SR1_ADDR (1UL << 1U)
#define I2C_SR1_BTF (1UL << 2U)
#define I2C_SR1_TXE (1UL << 7U)
#define I2C_SR2_BUSY (1UL << 1U)

enum
{
    USART1_IRQn = 37
};

static inline void nvic_enable_irq(uint32_t irq_number)
{
    volatile uint32_t *iser = (volatile uint32_t *)0xE000E100UL;
    iser[irq_number >> 5U] = 1UL << (irq_number & 0x1FU);
}

#if defined(__CC_ARM)
/* ARM Compiler 5 provides these as instruction intrinsics. */
#define disable_irq() __disable_irq()
#define enable_irq() __enable_irq()
#else
/* GCC and ARM Compiler 6 both accept GNU-style inline assembly. */
static inline void disable_irq(void)
{
    __asm volatile("cpsid i" ::: "memory");
}

static inline void enable_irq(void)
{
    __asm volatile("cpsie i" ::: "memory");
}
#endif

#endif
