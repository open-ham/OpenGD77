/*
 * Copyright (C)2019 Kai Ludwig, DG4KLU
 *
 * PWM modifications by Roger Clark VK3KYY / G4KYF
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

#include <FreeRTOS.h>
#include "io/display.h"
#include "hardware/UC1701.h"
#include "functions/settings.h"
#include "interfaces/gpio.h"


void displayInit(bool isInverseColour)
{
#if ! defined(PLATFORM_GD77S)

	// Init pins
	GPIO_PinWrite(GPIO_Display_CS, Pin_Display_CS, 1);
	GPIO_PinWrite(GPIO_Display_RST, Pin_Display_RST, 1);
	GPIO_PinWrite(GPIO_Display_RS, Pin_Display_RS, 1);
	GPIO_PinWrite(GPIO_Display_SCK, Pin_Display_SCK, 1);
	GPIO_PinWrite(GPIO_Display_SDA, Pin_Display_SDA, 1);

	// Reset LCD
	GPIO_PinWrite(GPIO_Display_RST, Pin_Display_RST, 0);
	vTaskDelay(portTICK_PERIOD_MS * 1);
	GPIO_PinWrite(GPIO_Display_RST, Pin_Display_RST, 1);
	vTaskDelay(portTICK_PERIOD_MS * 5);

	ucBegin(isInverseColour);
#endif // ! PLATFORM_GD77S
}

void displayEnableBacklight(bool enable)
{
#if ! defined(PLATFORM_GD77S)
	if (enable)
	{
#ifdef DISPLAY_LED_PWM
		gpioSetDisplayBacklightIntensityPercentage(nonVolatileSettings.displayBacklightPercentage);
#else
		GPIO_PinWrite(GPIO_Display_Light, Pin_Display_Light, 1);
#endif
	}
	else
	{
#ifdef DISPLAY_LED_PWM

		gpioSetDisplayBacklightIntensityPercentage(((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_NONE) ? 0 : nonVolatileSettings.displayBacklightPercentageOff));
#else
		GPIO_PinWrite(GPIO_Display_Light, Pin_Display_Light, 0);
#endif
	}
#endif // ! PLATFORM_GD77S
}

bool displayIsBacklightLit(void)
{
#if defined(PLATFORM_GD77S)
	return false;
#else
#ifdef DISPLAY_LED_PWM
	uint32_t cnv = BOARD_FTM_BASEADDR->CONTROLS[BOARD_FTM_CHANNEL].CnV;
	uint32_t mod = BOARD_FTM_BASEADDR->MOD;
	uint32_t dutyCyclePercent = 0;

	// Calculate dutyCyclePercent value
	if (cnv == (mod + 1))
	{
		dutyCyclePercent = 100;
	}
	else
	{
		dutyCyclePercent = (uint32_t)((((float)cnv / (float)mod) * 100.0) + 0.5);
	}
	return (dutyCyclePercent != nonVolatileSettings.displayBacklightPercentageOff);
#else
	return (GPIO_PinRead(GPIO_Display_Light, Pin_Display_Light) == 1);
#endif
#endif // PLATFORM_GD77S
}


