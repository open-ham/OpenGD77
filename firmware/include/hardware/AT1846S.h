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

#ifndef _OPENGD77_AT1846S_H_
#define _OPENGD77_AT1846S_H_

#include <FreeRTOS.h>
#include <task.h>
#include "interfaces/i2c.h"

#define AT1846_BYTES_PER_COMMAND 3
#define BANDWIDTH_12P5KHZ false
#define BANDWIDTH_25KHZ true

#define AT1846_VOICE_CHANNEL_NONE   0x00
#define AT1846_VOICE_CHANNEL_TONE1  0x10
#define AT1846_VOICE_CHANNEL_TONE2  0x20
#define AT1846_VOICE_CHANNEL_DTMF   0x30
#define AT1846_VOICE_CHANNEL_MIC    0x40

void AT1846Init(void);
void AT1846Postinit(void);
void AT1846SetBandwidth(void);
void AT1846SetMode(void);
void AT1846ReadVoxAndMicStrength(void);
void AT1846ReadRSSIAndNoise(void);
int AT1846SetClearReg2byteWithMask(uint8_t reg, uint8_t mask1, uint8_t mask2, uint8_t val1, uint8_t val2);
status_t AT1846SWriteReg2byte(uint8_t reg, uint8_t val1, uint8_t val2);
status_t AT1846SReadReg2byte(uint8_t reg, uint8_t *val1, uint8_t *val2);

#endif /* _OPENGD77_AT1846S_H_ */
