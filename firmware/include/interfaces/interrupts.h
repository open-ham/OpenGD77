/*
 * Copyright (C)2019-2020 Roger Clark. VK3KYY / G4KYF
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
#ifndef _OPENGD77_INTERRUPTS_H_
#define _OPENGD77_INTERRUPTS_H_

#include <stdint.h>
#include "interfaces/gpio.h"


void interruptsInitC6000Interface(void);
bool interruptsWasPinTriggered(PORT_Type *port, uint32_t pin);
bool interruptsClearPinFlags(PORT_Type *port, uint32_t pin);

#endif
