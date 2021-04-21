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

#ifndef _OPENGD77_BUTTONS_H_
#define _OPENGD77_BUTTONS_H_

#include "interfaces/gpio.h"


#define BUTTON_NONE                    0x00000000
#define BUTTON_PTT                     0x00000001
#define BUTTON_SK1                     0x00000002
#define BUTTON_SK1_SHORT_UP            0x00000004
#define BUTTON_SK1_LONG_DOWN           0x00000008
#define BUTTON_SK1_EXTRA_LONG_DOWN     0x00000010
#define BUTTON_SK2                     0x00000020
#define BUTTON_SK2_SHORT_UP            0x00000040
#define BUTTON_SK2_LONG_DOWN           0x00000080
#define BUTTON_SK2_EXTRA_LONG_DOWN     0x00000100
#if ! defined(PLATFORM_RD5R)
#define BUTTON_ORANGE                  0x00000200
#define BUTTON_ORANGE_SHORT_UP         0x00000400
#define BUTTON_ORANGE_LONG_DOWN        0x00000800
#define BUTTON_ORANGE_EXTRA_LONG_DOWN  0x00001000
#endif // ! PLATFORM_RD5R
#define BUTTON_WAIT_NEW_STATE          0x00002000

#define EVENT_BUTTON_NONE   0
#define EVENT_BUTTON_CHANGE 1

extern volatile bool PTTLocked;

void buttonsInit(void);
uint32_t buttonsRead(void);
void buttonsCheckButtonsEvent(uint32_t *buttons, int *event, bool keyIsDown);

#endif /* _OPENGD77_BUTTONS_H_ */
