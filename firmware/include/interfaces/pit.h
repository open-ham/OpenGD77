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

#ifndef _OPENGD77_PIT_H_
#define _OPENGD77_PIT_H_

#include <FreeRTOS.h>
#include <task.h>

#include "fsl_pit.h"

extern volatile uint32_t timer_maintask;
extern volatile uint32_t timer_beeptask;
extern volatile uint32_t timer_hrc6000task;
extern volatile uint32_t timer_watchdogtask;
extern volatile uint32_t timer_keypad;
extern volatile uint32_t timer_keypad_timeout;
extern volatile uint32_t PITCounter;

// For long press handling
extern volatile uint32_t timer_mbuttons[3];

void init_pit(void);
void PIT0_IRQHandler(void);

#endif /* _OPENGD77_PIT_H_ */
