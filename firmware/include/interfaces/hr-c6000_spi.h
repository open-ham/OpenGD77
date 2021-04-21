/*
 * Copyright (C)2019 Kai Ludwig, DG4KLU
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _OPENGD77_SPI_H_
#define _OPENGD77_SPI_H_

#include <FreeRTOS.h>
#include <task.h>
#include "fsl_dspi.h"

#if defined(PLATFORM_GD77) || defined(PLATFORM_GD77S)

// SPI1 to C6000 (V_SPI)
// OUT/ON  B10 - SPI /V_CS to C6000
// OUT/OFF B11 - SPI V_CLK to C6000
// OUT/ON  B16 - SPI V_DI to C6000
// IN      B17 - SPI V_DO to C6000
#define Port_SPI_CS_C6000_V  PORTB
#define GPIO_SPI_CS_C6000_V  GPIOB
#define Pin_SPI_CS_C6000_V   10
#define Port_SPI_CLK_C6000_V PORTB
#define GPIO_SPI_CLK_C6000_V GPIOB
#define Pin_SPI_CLK_C6000_V  11
#define Port_SPI_DI_C6000_V  PORTB
#define GPIO_SPI_DI_C6000_V  GPIOB
#define Pin_SPI_DI_C6000_V   16
#define Port_SPI_DO_C6000_V  PORTB
#define GPIO_SPI_DO_C6000_V  GPIOB
#define Pin_SPI_DO_C6000_V   17

// SPI0 to C6000 (U_SPI)
// OUT/ON  D0 - SPI /U_CS to C6000
// OUT/OFF D1 - SPI U_CLK to C6000
// OUT/ON  D2 - SPI U_DI to C6000
// IN      D3 - SPI U_DO to C6000
#define Port_SPI_CS_C6000_U  PORTD
#define GPIO_SPI_CS_C6000_U  GPIOD
#define Pin_SPI_CS_C6000_U   0
#define Port_SPI_CLK_C6000_U PORTD
#define GPIO_SPI_CLK_C6000_U GPIOD
#define Pin_SPI_CLK_C6000_U  1
#define Port_SPI_DI_C6000_U  PORTD
#define GPIO_SPI_DI_C6000_U  GPIOD
#define Pin_SPI_DI_C6000_U   2
#define Port_SPI_DO_C6000_U  PORTD
#define GPIO_SPI_DO_C6000_U  GPIOD
#define Pin_SPI_DO_C6000_U   3

#elif defined(PLATFORM_DM1801)

// SPI1 to C6000 (V_SPI)
// OUT/ON  B10 - SPI /V_CS to C6000
// OUT/OFF B11 - SPI V_CLK to C6000
// OUT/ON  B16 - SPI V_DI to C6000
// IN      B17 - SPI V_DO to C6000
#define Port_SPI_CS_C6000_V  PORTB
#define GPIO_SPI_CS_C6000_V  GPIOB
#define Pin_SPI_CS_C6000_V   10
#define Port_SPI_CLK_C6000_V PORTB
#define GPIO_SPI_CLK_C6000_V GPIOB
#define Pin_SPI_CLK_C6000_V  11
#define Port_SPI_DI_C6000_V  PORTB
#define GPIO_SPI_DI_C6000_V  GPIOB
#define Pin_SPI_DI_C6000_V   16
#define Port_SPI_DO_C6000_V  PORTB
#define GPIO_SPI_DO_C6000_V  GPIOB
#define Pin_SPI_DO_C6000_V   17

// SPI0 to C6000 (U_SPI)
// OUT/ON  D0 - SPI /U_CS to C6000
// OUT/OFF D1 - SPI U_CLK to C6000
// OUT/ON  D2 - SPI U_DI to C6000
// IN      D3 - SPI U_DO to C6000
#define Port_SPI_CS_C6000_U  PORTD
#define GPIO_SPI_CS_C6000_U  GPIOD
#define Pin_SPI_CS_C6000_U   0
#define Port_SPI_CLK_C6000_U PORTD
#define GPIO_SPI_CLK_C6000_U GPIOD
#define Pin_SPI_CLK_C6000_U  1
#define Port_SPI_DI_C6000_U  PORTD
#define GPIO_SPI_DI_C6000_U  GPIOD
#define Pin_SPI_DI_C6000_U   2
#define Port_SPI_DO_C6000_U  PORTD
#define GPIO_SPI_DO_C6000_U  GPIOD
#define Pin_SPI_DO_C6000_U   3

#elif defined(PLATFORM_RD5R)

// SPI1 to C6000 (V_SPI)
// OUT/ON  B10 - SPI /V_CS to C6000
// OUT/OFF B11 - SPI V_CLK to C6000
// OUT/ON  B16 - SPI V_DI to C6000
// IN      B17 - SPI V_DO to C6000
#define Port_SPI_CS_C6000_V  PORTB
#define GPIO_SPI_CS_C6000_V  GPIOB
#define Pin_SPI_CS_C6000_V   10
#define Port_SPI_CLK_C6000_V PORTB
#define GPIO_SPI_CLK_C6000_V GPIOB
#define Pin_SPI_CLK_C6000_V  11
#define Port_SPI_DI_C6000_V  PORTB
#define GPIO_SPI_DI_C6000_V  GPIOB
#define Pin_SPI_DI_C6000_V   16
#define Port_SPI_DO_C6000_V  PORTB
#define GPIO_SPI_DO_C6000_V  GPIOB
#define Pin_SPI_DO_C6000_V   17

// SPI0 to C6000 (U_SPI)
// OUT/ON  D0 - SPI /U_CS to C6000
// OUT/OFF D1 - SPI U_CLK to C6000
// OUT/ON  D2 - SPI U_DI to C6000
// IN      D3 - SPI U_DO to C6000
#define Port_SPI_CS_C6000_U  PORTD
#define GPIO_SPI_CS_C6000_U  GPIOD
#define Pin_SPI_CS_C6000_U   0
#define Port_SPI_CLK_C6000_U PORTD
#define GPIO_SPI_CLK_C6000_U GPIOD
#define Pin_SPI_CLK_C6000_U  1
#define Port_SPI_DI_C6000_U  PORTD
#define GPIO_SPI_DI_C6000_U  GPIOD
#define Pin_SPI_DI_C6000_U   2
#define Port_SPI_DO_C6000_U  PORTD
#define GPIO_SPI_DO_C6000_U  GPIOD
#define Pin_SPI_DO_C6000_U   3

#endif

void SPIInit(void);

int SPI0WritePageRegByte(uint8_t page, uint8_t reg, uint8_t val);
int SPI0ReadPageRegByte(uint8_t page, uint8_t reg, volatile uint8_t *val);
int SPI0SeClearPageRegByteWithMask(uint8_t page, uint8_t reg, uint8_t mask, uint8_t val);
int SPI0WritePageRegByteArray(uint8_t page, uint8_t reg, const uint8_t *values, uint8_t length);
int SPI0ReadPageRegBytAarray(uint8_t page, uint8_t reg, volatile uint8_t *values, uint8_t length);

int SPI1WritePageRegByteArray(uint8_t page, uint8_t reg, const uint8_t *values, uint8_t length);
int SPI1ReadPageRegByteArray(uint8_t page, uint8_t reg, volatile uint8_t *values, uint8_t length);

void SPI0Setup(void);
void SPI1Setup(void);

#endif /* _OPENGD77_SPI_H_ */
