#include "stm32f4xx.h"
                                                                                       /*
                            ***  TRIPPLE BLINK ***
          Small code example how to write to the periperal register using DMA
                                                                                       */

__STATIC_INLINE void init(void);

int main(void) {

  init(); /* configure sysclock, peripherals etc.. */

  for(;;) { /*** No superloop body ***/ }

  #if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunreachable-code-return"
  #elif defined(__CC_ARM)
    #pragma diag_suppress 111
  #elif defined(__ICCARM__)
    #pragma diag_suppress = Pe111
  #endif

  return 0;

  #if defined(__clang__) && !defined(__CC_ARM)
    #pragma clang diagnostic pop
  #endif

} /* main() */

#define LED(STATE) GPIO_BSRR_B ## STATE ## 13

static const volatile unsigned led_data[30] = {   /* data to send to GPIOC->BSTR once TIM1 update event occures   */
  LED(R), LED(S), LED(R), LED(S), LED(R), LED(S)  /* tripple led_on/led_off data                                  */
};

__STATIC_INLINE void init(void) {

  RCC->CR = RCC_CR_HSEON;          /* turn external 25mHz crystall on                                             */

  RCC->PLLCFGR = (                 /* 0x40023804: RCC PLL configuration register, Address offset: 0x04            */
    RCC_PLLCFGR_PLLM_2     |       /* (0x04 << 0)    divide crystall clock                           0x00000004   */
    RCC_PLLCFGR_PLLM_3     |       /* (0x08 << 0)           by 12                                    0x00000008   */
    RCC_PLLCFGR_PLLN_5     |       /* (0x020 << 6)       multiply it                                 0x00000800   */
    RCC_PLLCFGR_PLLN_6     |       /* (0x040 << 6)          by 96                                    0x00001000   */
    RCC_PLLCFGR_PLLSRC_HSE         /* (1 << 22)    configure external clock as PLL source            0x00400000   */
  );

  RCC->CR = RCC_CR_HSEON + RCC_CR_PLLON; /* turn PLL on                                                           */

  #define RCC_CFGR (       \
    RCC_CFGR_HPRE_DIV1     |       /* 0x00000000    AHB1: SYSCLK is not divided                                   */\
    RCC_CFGR_PPRE1_DIV2    |       /* 0x00001000    APB1: HCLK is divided by 2                                    */\
    RCC_CFGR_PPRE2_DIV1            /* 0x00000000    APB2: HCLK is not divided                                     */\
  )

  RCC->CFGR = RCC_CFGR;            /* 0x40023808: RCC clock configuration register, Address offset: 0x08          */

  #define CLK_READY (RCC_CR_HSERDY | RCC_CR_PLLRDY)

  while ((RCC->CR & CLK_READY) != CLK_READY) {} /* wait until HSE and PLL become ready                            */

  FLASH->ACR = (                   /* 0x40023C00: FLASH access control register, Address offset: 0x00             */
    FLASH_ACR_LATENCY_2WS  |       /* (1 << 1)    Two wait states                                    0x00000002   */
    FLASH_ACR_PRFTEN       |       /* (1 << 8)    Prefetch is enabled                                0x00000100   */
    FLASH_ACR_ICEN         |       /* (1 << 9)    Instruction cache is enabled                       0x00000200   */
    FLASH_ACR_DCEN                 /* (1 << 10)   Data cache is enabled                              0x00000400   */
  );

  RCC->CFGR = RCC_CFGR + RCC_CFGR_SW_PLL; /* Select PLL as system clock                                           */
  while ((RCC->CFGR & RCC_CFGR_SWS_PLL) != RCC_CFGR_SWS_PLL) {} /* Wait until the clock becomes stable            */

  RCC->AHB1ENR = (                 /* 0x40023830: RCC AHB1 peripheral clock register, Address offset: 0x30        */
    RCC_AHB1ENR_GPIOCEN    |       /* (1 << 2)  Enable clock for GPIO Port C                         0x00000004   */
    RCC_AHB1ENR_DMA2EN             /* (1 << 22) Enable clock for DMA2 controller                     0x00400000   */
  );

  RCC->APB2ENR =                   /* 0x40023844: RCC APB2 peripheral clock enable register, Address offset: 0x44 */
    RCC_APB2ENR_TIM1EN;            /* (1 << 0)  Enable clock for TIM1 peripheral                     0x00000000   */

  GPIOC->MODER =                   /* 0x40020800: GPIOC port mode register, Address offset: 0x00                  */
    GPIO_MODER_MODER13_0;          /* Configure PC13 as Output                                       0x00000000   */

  DMA2_Stream5->M0AR = (unsigned) &led_data; /* 0x40026494: DMA2 stream 5 memory 0 address register               */
  DMA2_Stream5->NDTR = sizeof(led_data) / sizeof(unsigned); /* 0x4002648C: DMA2 stream 5 number of data register  */
  DMA2_Stream5->PAR  = (unsigned) &GPIOC->BSRR;    /* 0x40026490: DMA2 stream 5 peripheral address register       */

  DMA2_Stream5->CR = (             /* 0x40026488: DMA stream 5 configuration register                             */
    DMA_SxCR_CHSEL_1       |       /* 0x04000000    0x6 -- 6th channel                                            */
    DMA_SxCR_CHSEL_2       |       /* 0x08000000                                                                  */
    DMA_SxCR_MSIZE_1       |       /* (2 << 13)     Memory Source data:          32bit               0x00004000   */
    DMA_SxCR_PSIZE_1       |       /* (2 << 11)     Peripheral Destination data: 32bit               0x00001000   */
    DMA_SxCR_MINC          |       /* (1 << 10)     Increment momory address mode                    0x00000400   */
    DMA_SxCR_CIRC          |       /* (1 << 8)      Circular memory buffer                           0x00000100   */
    DMA_SxCR_DIR_0         |       /* (1 << 6)      From memory to peripheral direction              0x00000040   */
    DMA_SxCR_EN                    /* (1 << 0)      Enable DMA2 stream 5                             0x00000001   */
  );

  TIM1->PSC  = 100 - 1;            /* 0x40010028: TIM prescaler, Address offset: 0x28                             */
  TIM1->EGR  = TIM_EGR_UG;         /* 0x40010014: TIM event generation register, Address offset: 0x14             */
  TIM1->ARR  = 33333 - 1;          /* 0x4001002C: TIM auto-reload register, Address offset: 0x2C                  */
  TIM1->DIER = TIM_DIER_UDE;       /* 0x4001000C: TIM DMA/interrupt enable register, Address offset: 0x0C         */
  TIM1->CR1  = TIM_CR1_CEN;        /* 0x40010000: TIM control register 1, Address offset: 0x00                    */

}
