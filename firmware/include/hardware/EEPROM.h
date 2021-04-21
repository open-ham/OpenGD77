/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
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

#ifndef _OPENGD77_EEPROM_H_
#define _OPENGD77_EEPROM_H_

#include <stdbool.h>
#include <FreeRTOS.h>
#include <task.h>
#include "interfaces/i2c.h"

bool EEPROM_Read(int address,uint8_t *buf, int size);
bool EEPROM_Write(int address,uint8_t *buf, int size);

#endif /* _OPENGD77_EEPROM_H_ */
