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

#include "io/rotary_switch.h"

#if defined(PLATFORM_GD77S)
static uint8_t prevPosition;
#endif

void rotarySwitchInit(void)
{
#if defined(PLATFORM_GD77S)
	gpioInitRotarySwitch();

	prevPosition = -1;
#endif
}

uint8_t rotarySwitchGetPosition(void)
{
#if defined(PLATFORM_GD77S)
	static const uint8_t rsPositions[] = { 1, 8, 2, 7, 4, 5, 3, 6, 16, 9, 15, 10, 13, 12, 14, 11 };
	return rsPositions[(~((GPIOD->PDIR) >> 4) & 0x0F)];
#else
	return 0;
#endif
}

void rotarySwitchCheckRotaryEvent(uint32_t *position, int *event)
{
#if ! defined(PLATFORM_GD77S)
	*position = 0;
	*event = EVENT_ROTARY_NONE;
#else
	uint8_t value = rotarySwitchGetPosition();

	*position = value; // set it anyway, as it could be checked even on no event

	if (prevPosition != value)
	{
		*event = EVENT_ROTARY_CHANGE;
		prevPosition = value;
	}
	else
	{
		*event = EVENT_ROTARY_NONE;
	}
#endif
}
