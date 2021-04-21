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

#include "interfaces/pit.h"

volatile uint32_t timer_maintask;
volatile uint32_t timer_beeptask;
volatile uint32_t timer_hrc6000task;
volatile uint32_t timer_watchdogtask;
volatile uint32_t timer_keypad;
volatile uint32_t timer_keypad_timeout;
volatile uint32_t PITCounter;

volatile uint32_t timer_mbuttons[3];

void init_pit(void)
{
	taskENTER_CRITICAL();
	timer_maintask = 0;
	timer_beeptask = 0;
	timer_hrc6000task = 0;
	timer_watchdogtask = 0;
	timer_keypad = 0;
	timer_keypad_timeout = 0;
	timer_mbuttons[0] = timer_mbuttons[1] = timer_mbuttons[2] = 0;
	taskEXIT_CRITICAL();

	pit_config_t pitConfig;
	PIT_GetDefaultConfig(&pitConfig);
	PIT_Init(PIT, &pitConfig);

	PIT_SetTimerPeriod(PIT, kPIT_Chnl_0, USEC_TO_COUNT(100U, CLOCK_GetFreq(kCLOCK_BusClk)));
	PIT_EnableInterrupts(PIT, kPIT_Chnl_0, kPIT_TimerInterruptEnable);

	EnableIRQ(PIT0_IRQn);

    PIT_StartTimer(PIT, kPIT_Chnl_0);
}

void PIT0_IRQHandler(void)
{
	PITCounter++;// is unsigned so will wrap around

	if (timer_maintask > 0)
	{
		timer_maintask--;
	}
	if (timer_beeptask > 0)
	{
		timer_beeptask--;
	}
	if (timer_hrc6000task > 0)
	{
		timer_hrc6000task--;
	}
	if (timer_watchdogtask > 0)
	{
		timer_watchdogtask--;
	}
	if (timer_keypad > 0)
	{
		timer_keypad--;
	}
	if (timer_keypad_timeout > 0)
	{
		timer_keypad_timeout--;
	}
	if (timer_mbuttons[0] > 0)
	{
		timer_mbuttons[0]--;
	}

	if (timer_mbuttons[1] > 0)
	{
		timer_mbuttons[1]--;
	}

	if (timer_mbuttons[2] > 0)
	{
		timer_mbuttons[2]--;
	}

    /* Clear interrupt flag.*/
    PIT_ClearStatusFlags(PIT, kPIT_Chnl_0, kPIT_TimerFlag);
    __DSB();
}
