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
#include "user_interface/menuSystem.h"
#include "user_interface/uiLocalisation.h"

static void updateScreen(void);
static void handleEvent(uiEvent_t *ev);
static uint32_t initialEventTime;
const uint32_t POWEROFF_DURATION_MILLISECONDS = 500;

menuStatus_t uiPowerOff(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		updateScreen();
		initialEventTime = ev->time;
	}
	else
	{
		handleEvent(ev);
	}
	return MENU_STATUS_SUCCESS;
}

static void updateScreen(void)
{
	ucClearBuf();
	ucPrintCentered(12, currentLanguage->power_off, FONT_SIZE_3);
	ucPrintCentered(32, "73", FONT_SIZE_3);
	ucRender();
}

static void handleEvent(uiEvent_t *ev)
{
#if defined(PLATFORM_RD5R)
	if (batteryVoltage > CUTOFF_VOLTAGE_LOWER_HYST)
#else
	if ((GPIO_PinRead(GPIO_Power_Switch, Pin_Power_Switch) == 0) && (batteryVoltage > CUTOFF_VOLTAGE_LOWER_HYST))
#endif
	{
		// I think this is to handle if the power button is turned back on during shutdown
		menuSystemPopPreviousMenu();
		initialEventTime = 0; // Reset timeout
		return;
	}

	if ((ev->time - initialEventTime) > POWEROFF_DURATION_MILLISECONDS)
	{
		powerOffFinalStage();
	}
}
