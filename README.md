# STM32F401CCU6-DMA-BLINK

[![Build](https://github.com/a5021/STM32F401CCU6-DMA-BLINK/actions/workflows/build.yml/badge.svg)](https://github.com/a5021/STM32F401CCU6-DMA-BLINK/actions/workflows/build.yml) [![MCU](https://img.shields.io/badge/MCU-STM32F401CCU6-00A9E0)]() [![Core](https://img.shields.io/badge/Core-Cortex--M4F-00A9E0)]() [![License](https://img.shields.io/badge/License-MIT-yellow)]()

Triple-blink demo for STM32F401CCU6 using TIM1 update events to trigger DMA2 transfers from a memory buffer to the GPIO BSRR register — no CPU intervention after initialisation. Demonstrates register-level programming without HAL.

## Features

- Register-level, bare-metal firmware (no HAL, no CMSIS-DSP)
- System clock 200 MHz from HSE 25 MHz via PLL (25 / 12 x 96)
- DMA2 stream 5 configured in circular mode, memory-to-peripheral, 32-bit
- TIM1 update events (100 Hz) trigger each DMA transfer
- DMA writes alternating `BS13` / `BR13` bits to GPIOC->BSRR — toggles PC13 LED
- Three-step blink pattern: on, off, on, off, on, off
- CPU enters `WFI` after init and never wakes — DMA does all work
- Instruction & data cache enabled, 2 wait states, prefetch active
- BSRR register is accessed via DMA without CPU polling

## Hardware Specification

| Component | Detail |
|-----------|--------|
| MCU | STMicroelectronics STM32F401CCU6 (ARM Cortex-M4F, 256 KB Flash, 64 KB RAM) |
| Clock | HSE 25 MHz → PLL → 200 MHz SYSCLK |
| Cache | 2 WS, ICache + DCache + prefetch enabled |
| LED | PC13 (on-board) |
| DMA | DMA2 stream 5, channel 6, circular mode |
| Timer | TIM1, 100 Hz update rate, DMA request on UDE |
| Debug | SWD on PA13/PA14 |

## Pin Assignment

| Signal | Pin | Peripheral | Notes |
|--------|-----|------------|-------|
| LED | PC13 | GPIO output | On-board LED, push-pull |
| SWCLK | PA14 | AF0 | SWD |
| SWDIO | PA13 | AF0 | SWD |

## Clock Tree

```
HSE 25 MHz
  |
  v
PLLM = 12   25 / 12 = 2.083 MHz
PLLN = 96   2.083 x 96 = 200 MHz
PLLP = /2   (default, not used - SYSCLK = PLL output)
  |
  v
SYSCLK = 200 MHz
  |
  +-- AHB1 (HPRE = /1)  → HCLK = 200 MHz
  |     +-- APB1 (PPRE1 = /2) → PCLK1 = 100 MHz
  |     +-- APB2 (PPRE2 = /1) → PCLK2 = 200 MHz
  |
  +-- FLASH: 2 WS, ICache + DCache + Prefetch ON
```

## DMA Configuration

| Parameter | Value |
|-----------|-------|
| Stream | DMA2 Stream 5 |
| Channel | 6 (TIM1_UP) |
| Direction | Memory → Peripheral |
| Source | `led_data[30]` in Flash |
| Destination | GPIOC->BSRR (0x40020818) |
| Data size | 32-bit (both mem and periph) |
| Mode | Circular |
| Memory increment | Enabled |
| NDTR | 30 |

## Trigger Chain

```
TIM1 (ARR = 33333 - 1, PSC = 100 - 1)
  |  f_update = 200 MHz / 100 / 33333 ≈ 60 Hz
  |  UDE (update DMA request) on each update event
  v
DMA2 Stream 5 (CHSEL = 6)
  |  Reads led_data[i], writes to GPIOC->BSRR
  |  Circular: repeats indefinitely
  v
GPIOC->BSRR (BS13 / BR13)
  |  Sets or resets PC13 pin
  v
PC13 LED (on / off / on / off / on / off ...)
```

## Firmware Architecture

```
  Reset
    |
  main()
    |
  init()
    |-- RCC: HSE → PLL → 200 MHz SYSCLK
    |-- FLASH: 2 WS, cache + prefetch
    |-- GPIO: PC13 as output
    |-- DMA2 S5: mem → GPIOC->BSRR, circular
    |-- TIM1: PSC=100-1, ARR=33333-1, UDE enabled
    |-- TIM1: CR1.CEN = 1 (counter start)
    |-- return
    |
  __WFI()
    |
  (CPU sleeps forever, DMA runs independently)
```

## Power Management

After initialisation the CPU executes `WFI` and never wakes. The DMA controller and TIM1 remain clocked. Total current is dominated by the running peripherals:

| Block | State |
|-------|-------|
| CPU core | Sleep (WFI) |
| DMA2 | Active (circular transfers) |
| TIM1 | Active (counting, generating UDE) |
| GPIOC | Active (pin toggling) |
| FLASH | Standby (read during DMA) |

## Getting Started

### Prerequisites

- ARM GCC toolchain (`arm-none-eabi-gcc`)
- GNU Make

### Build

```sh
make
```

Output in `build/`: `project.elf`, `project.hex`, `project.bin`.

### Flash

```sh
make program       # ST-LINK
make jprogram       # J-Link
```

## Project Structure

```
src/
+-- main.c                       Application entry + DMA + timer init
+-- system_stm32f4xx.c           CMSIS system initialisation
+-- startup_stm32f401xc.s        GCC startup
inc/
+-- cmsis_*.h, core_cm4.h        CMSIS-CORE headers
+-- stm32f401xc.h, stm32f4xx.h   MCU headers
+-- system_stm32f4xx.h           System header
MDK-ARM/                         Keil uVision project
Makefile                         GCC build system
STM32F401CCUX_FLASH.ld           Linker script
stm32f401cc.jflash               J-Flash project
project.jdebug                   J-Link debugger project
```