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

#ifndef _OPENGD77_I2C_H_
#define _OPENGD77_I2C_H_

#include <FreeRTOS.h>
#include <task.h>
#include "fsl_i2c.h"


#define I2C_BAUDRATE (100000) /* 100K */
#define AT1846S_I2C_MASTER_SLAVE_ADDR_7BIT (0x71U)

extern volatile int isI2cInUse;

#if defined(PLATFORM_GD77) || defined(PLATFORM_GD77S)

// I2C0a to AT24C512 EEPROM & AT1846S
// OUT/ON E24 - I2C SCL to AT24C512 EEPROM & AT1846S
// OUT/ON E25 - I2C SDA to AT24C512 EEPROM & AT1846S
#define Port_I2C0a_SCL     PORTE
#define GPIO_I2C0a_SCL     GPIOE
#define Pin_I2C0a_SCL	   24
#define Port_I2C0a_SDA     PORTE
#define GPIO_I2C0a_SDA     GPIOE
#define Pin_I2C0a_SDA	   25

// I2C0b to ALPU-MP-1413
// OUT/ON B2 - I2C SCL to ALPU-MP-1413
// OUT/ON B3 - I2C SDA to ALPU-MP-1413
#define Port_I2C0b_SCL     PORTB
#define GPIO_I2C0b_SCL     GPIOB
#define Pin_I2C0b_SCL	   2
#define Port_I2C0b_SDA     PORTB
#define GPIO_I2C0b_SDA     GPIOB
#define Pin_I2C0b_SDA	   3

#elif defined(PLATFORM_DM1801)

// I2C0a to AT24C512 EEPROM & AT1846S
// OUT/ON E24 - I2C SCL to AT24C512 EEPROM & AT1846S
// OUT/ON E25 - I2C SDA to AT24C512 EEPROM & AT1846S
#define Port_I2C0a_SCL     PORTE
#define GPIO_I2C0a_SCL     GPIOE
#define Pin_I2C0a_SCL	   24
#define Port_I2C0a_SDA     PORTE
#define GPIO_I2C0a_SDA     GPIOE
#define Pin_I2C0a_SDA	   25

// I2C0b to ALPU-MP-1413
// OUT/ON B2 - I2C SCL to ALPU-MP-1413
// OUT/ON B3 - I2C SDA to ALPU-MP-1413
#define Port_I2C0b_SCL     PORTB
#define GPIO_I2C0b_SCL     GPIOB
#define Pin_I2C0b_SCL	   2
#define Port_I2C0b_SDA     PORTB
#define GPIO_I2C0b_SDA     GPIOB
#define Pin_I2C0b_SDA	   3

#elif defined(PLATFORM_RD5R)

// I2C0a to AT24C512 EEPROM & AT1846S
// OUT/ON E24 - I2C SCL to AT24C512 EEPROM & AT1846S
// OUT/ON E25 - I2C SDA to AT24C512 EEPROM & AT1846S
#define Port_I2C0a_SCL     PORTE
#define GPIO_I2C0a_SCL     GPIOE
#define Pin_I2C0a_SCL	   24
#define Port_I2C0a_SDA     PORTE
#define GPIO_I2C0a_SDA     GPIOE
#define Pin_I2C0a_SDA	   25

// I2C0b to ALPU-MP-1413
// OUT/ON B2 - I2C SCL to ALPU-MP-1413
// OUT/ON B3 - I2C SDA to ALPU-MP-1413
#define Port_I2C0b_SCL     PORTB
#define GPIO_I2C0b_SCL     GPIOB
#define Pin_I2C0b_SCL	   2
#define Port_I2C0b_SDA     PORTB
#define GPIO_I2C0b_SDA     GPIOB
#define Pin_I2C0b_SDA	   3

#endif

void I2C0aInit(void);
void I2C0bInit(void);
void I2C0Setup(void);


#endif /* _OPENGD77_I2C_H_ */
