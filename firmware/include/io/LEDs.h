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

#ifndef _OPENGD77_LEDS_H_
#define _OPENGD77_LEDS_H_

#include <stdint.h>
#include <stdbool.h>
#include "functions/settings.h"

#if ! defined(PLATFORM_GD77S)
extern uint8_t LEDsState[2];
#endif

void LEDsInit(void);

#if defined(PLATFORM_RD5R)
void torchToggle(void);
#endif


static inline void LEDs_PinWrite(GPIO_Type *base, uint32_t pin, uint8_t output)
{
#if ! defined(PLATFORM_GD77S)
	LEDsState[(base == GPIO_LEDred) ? 0 : 1] = output;

	if ((nonVolatileSettings.bitfieldOptions & BIT_ALL_LEDS_DISABLED) == 0)
#endif
	{
		GPIO_PinWrite(base, pin, output);
	}
}

static inline uint32_t LEDs_PinRead(GPIO_Type *base, uint32_t pin)
{
#if ! defined(PLATFORM_GD77S)
	if (nonVolatileSettings.bitfieldOptions & BIT_ALL_LEDS_DISABLED)
	{
		return LEDsState[(base == GPIO_LEDred) ? 0 : 1];
	}
#endif
	return GPIO_PinRead(base, pin);
}



#endif /* _OPENGD77_LEDS_H_ */
