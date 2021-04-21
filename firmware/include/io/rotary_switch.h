/*
 * Copyright (C)2020	Kai Ludwig, DG4KLU
 *               and	Roger Clark, VK3KYY / G4KYF
 *               and	Daniel Caujolle-Bert, F1RMB
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
#ifndef _OPENGD77_ROTARY_SWITCH_H_
#define _OPENGD77_ROTARY_SWITCH_H_

#include "interfaces/gpio.h"


void rotarySwitchInit(void);
uint8_t rotarySwitchGetPosition(void);
void rotarySwitchCheckRotaryEvent(uint32_t *position, int *event);

#define EVENT_ROTARY_NONE   0
#define EVENT_ROTARY_CHANGE 1

#endif // _OPENGD77_ROTARY_SWITCH_H_
