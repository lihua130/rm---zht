; Minimal STM32F103C8T6 startup for Keil MDK-ARM.
; The application uses the reset-default 8 MHz HSI clock.

Stack_Size      EQU     0x00000400
                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp

Heap_Size       EQU     0x00000000
                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit

                PRESERVE8
                THUMB

                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size
                IMPORT  USART1_IRQHandler

__Vectors       DCD     __initial_sp
                DCD     Reset_Handler
                DCD     Default_Handler       ; NMI
                DCD     Default_Handler       ; HardFault
                DCD     Default_Handler       ; MemManage
                DCD     Default_Handler       ; BusFault
                DCD     Default_Handler       ; UsageFault
                DCD     0
                DCD     0
                DCD     0
                DCD     0
                DCD     Default_Handler       ; SVC
                DCD     Default_Handler       ; DebugMon
                DCD     0
                DCD     Default_Handler       ; PendSV
                DCD     Default_Handler       ; SysTick

                ; STM32F103 medium-density external interrupts 0..37.
                DCD     Default_Handler       ; WWDG
                DCD     Default_Handler       ; PVD
                DCD     Default_Handler       ; TAMPER
                DCD     Default_Handler       ; RTC
                DCD     Default_Handler       ; FLASH
                DCD     Default_Handler       ; RCC
                DCD     Default_Handler       ; EXTI0
                DCD     Default_Handler       ; EXTI1
                DCD     Default_Handler       ; EXTI2
                DCD     Default_Handler       ; EXTI3
                DCD     Default_Handler       ; EXTI4
                DCD     Default_Handler       ; DMA1_Channel1
                DCD     Default_Handler       ; DMA1_Channel2
                DCD     Default_Handler       ; DMA1_Channel3
                DCD     Default_Handler       ; DMA1_Channel4
                DCD     Default_Handler       ; DMA1_Channel5
                DCD     Default_Handler       ; DMA1_Channel6
                DCD     Default_Handler       ; DMA1_Channel7
                DCD     Default_Handler       ; ADC1_2
                DCD     Default_Handler       ; USB_HP_CAN1_TX
                DCD     Default_Handler       ; USB_LP_CAN1_RX0
                DCD     Default_Handler       ; CAN1_RX1
                DCD     Default_Handler       ; CAN1_SCE
                DCD     Default_Handler       ; EXTI9_5
                DCD     Default_Handler       ; TIM1_BRK
                DCD     Default_Handler       ; TIM1_UP
                DCD     Default_Handler       ; TIM1_TRG_COM
                DCD     Default_Handler       ; TIM1_CC
                DCD     Default_Handler       ; TIM2
                DCD     Default_Handler       ; TIM3
                DCD     Default_Handler       ; TIM4
                DCD     Default_Handler       ; I2C1_EV
                DCD     Default_Handler       ; I2C1_ER
                DCD     Default_Handler       ; I2C2_EV
                DCD     Default_Handler       ; I2C2_ER
                DCD     Default_Handler       ; SPI1
                DCD     Default_Handler       ; SPI2
                DCD     USART1_IRQHandler
__Vectors_End
__Vectors_Size  EQU     __Vectors_End - __Vectors

                AREA    |.text|, CODE, READONLY

Reset_Handler   PROC
                EXPORT  Reset_Handler
                IMPORT  __main
                LDR     R0, =__main
                BX      R0
                ENDP

Default_Handler PROC
                EXPORT  Default_Handler
Default_Loop
                B       Default_Loop
                ENDP

                IF      :DEF:__MICROLIB
                EXPORT  __initial_sp
                EXPORT  __heap_base
                EXPORT  __heap_limit
                ELSE
                IMPORT  __use_two_region_memory
                EXPORT  __user_initial_stackheap
__user_initial_stackheap
                LDR     R0, =Heap_Mem
                LDR     R1, =(Stack_Mem + Stack_Size)
                LDR     R2, =(Heap_Mem + Heap_Size)
                LDR     R3, =Stack_Mem
                BX      LR
                ALIGN
                ENDIF

                END
