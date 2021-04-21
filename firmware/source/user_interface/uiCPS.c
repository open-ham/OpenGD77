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
#include "functions/trx.h"
#include "user_interface/menuSystem.h"

static void handleTick(void);

typedef enum
{
	LED_NONE,
	LED_RED,
	LED_GREEN
} blinkLed_t;

static blinkLed_t mode = LED_NONE;
static uint32_t nextPIT;
static int ledState = 0;
static const int PIT_COUNTS_PER_UPDATE = 5000;
static int radioMode;
static int radioBandWidth;


menuStatus_t uiCPS(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		menuDataGlobal.endIndex = 0;
		radioMode = trxGetMode();
		radioBandWidth = trxGetBandwidthIs25kHz();
		trxSetModeAndBandwidth(RADIO_MODE_NONE, radioBandWidth);
		// Just clear the display and turn on the
//		UC1701_clearBuf();
//		UC1701_render();
		nextPIT = PITCounter + PIT_COUNTS_PER_UPDATE;
	}
	else
	{
		if (PITCounter >= nextPIT)
		{
			nextPIT = PITCounter + PIT_COUNTS_PER_UPDATE;
			handleTick();
		}
	}
	return MENU_STATUS_SUCCESS;
}

void uiCPSUpdate(uiCPSCommand_t command, int x, int y, ucFont_t fontSize, ucTextAlign_t alignment, bool isInverted, char *szMsg)
{
	switch(command)
	{
		case CPS2UI_COMMAND_CLEARBUF:
			ucClearBuf();
			break;
		case CPS2UI_COMMAND_PRINT:
			ucPrintCore(x, y, szMsg, fontSize, alignment, isInverted);
			break;
		case CPS2UI_COMMAND_RENDER_DISPLAY:
			ucRender();
			displayLightTrigger(true);
			break;
		case CPS2UI_COMMAND_BACKLIGHT:
			displayLightTrigger(true);
			break;
		case CPS2UI_COMMAND_GREEN_LED:
			mode = LED_GREEN;// flash green LED
			break;
		case CPS2UI_COMMAND_RED_LED:
			mode = LED_RED;// flash red LED
			break;
		case CPS2UI_COMMAND_END:
		    LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
		    LEDs_PinWrite(GPIO_LEDred, Pin_LEDred, 0);
		    mode = LED_NONE;
		    trxSetRX();// Rx would be turned off at start of CPS by setting radio mode to none
		    trxSetModeAndBandwidth(radioMode, radioBandWidth);
			menuSystemPopAllAndDisplayRootMenu();
			break;
		default:
			break;
	}
}

static void handleTick(void)
{
	switch(mode)
	{
		case LED_GREEN:
			if (ledState == 0)
			{
				ledState = 1;
			    LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 1);
			}
			else
			{
				ledState = 0;
			    LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
			}
			break;

		case LED_RED:
			if (ledState == 0)
			{
				ledState = 1;
			    LEDs_PinWrite(GPIO_LEDred, Pin_LEDred, 1);
			}
			else
			{
				ledState = 0;
			    LEDs_PinWrite(GPIO_LEDred, Pin_LEDred, 0);
			}
			break;

		case LED_NONE:
		default:
			break;
	}
}
