/*
 * Copyright (C)2019 Roger Clark, VK3KYY / G4KYF
 *
 * Development informed by work from  Rustem Iskuzhin (in 2014)
 * and https://github.com/bitbank2/uc1701/
 * and https://os.mbed.com/users/Anaesthetix/code/UC1701/file/7494bdca926b/
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
#include "io/display.h"
#include "hardware/UC1701.h"
#include "functions/settings.h"
#include "interfaces/gpio.h"

/*
 * IMPORTANT
 *
 * DO NOT ENABLE OPIMISATION ON THIS FILE.
 *
 * This file implements software SPI which is messed up if compiler optimisation is enabled.
 */





#if ! defined(PLATFORM_GD77S)
static void UC1701_transfer(register uint8_t data1);
static void UC1701_setCommandMode(void);
static void UC1701_setDataMode(void);
static bool isAwake = true;
static bool isInverted = false;

static void UC1701_setCommandMode(void)
{
	GPIO_Display_RS->PCOR = 1U << Pin_Display_RS;// set the command / data pin low to signify Command mode
}

static void UC1701_setDataMode(void)
{
	GPIO_Display_RS->PSOR = 1U << Pin_Display_RS;// set the command / data pin low to signify Data mode
}
#endif // ! PLATFORM_GD77S

void ucRenderRows(int16_t startRow, int16_t endRow)
{
#if ! defined(PLATFORM_GD77S)
	uint8_t *rowPos = (screenBuf + startRow * DISPLAY_SIZE_X);

	GPIO_PinWrite(GPIO_Display_CS, Pin_Display_CS, 0);// Enable CS

	for(int16_t row = startRow; row < endRow; row++)
	{
		UC1701_setCommandMode();
		UC1701_transfer(0xb0 | row); // set Y
		UC1701_transfer(0x10 | 0); // set X (high MSB)

// Note there are 4 pixels at the left which are no in the hardware of the LCD panel, but are in the RAM buffer of the controller
#if !defined(PLATFORM_RD5R)
		UC1701_transfer(0x00 | 4); // set X (low MSB).
#endif

		UC1701_setDataMode();
		uint8_t data1;
		for(int16_t line = 0; line < DISPLAY_SIZE_X; line++)
		{
			data1 = *rowPos;
			for (register int i = 0; i < 8; i++)
			{
				GPIO_Display_SCK->PCOR = 1U << Pin_Display_SCK;

				if ((data1 & 0x80) == 0U)
				{
					GPIO_Display_SDA->PCOR = 1U << Pin_Display_SDA;// Hopefully the compiler will optimise this to a value rather than using a shift
				}
				else
				{
					GPIO_Display_SDA->PSOR = 1U << Pin_Display_SDA;// Hopefully the compiler will optimise this to a value rather than using a shift
				}

				GPIO_Display_SCK->PSOR = 1U << Pin_Display_SCK;// Hopefully the compiler will optimise this to a value rather than using a shift

				data1 = data1 << 1;
			}
			rowPos++;
		}
	}

	GPIO_PinWrite(GPIO_Display_CS, Pin_Display_CS, 1);// Disable CS
#endif // ! PLATFORM_GD77S
}
#if ! defined(PLATFORM_GD77S)
static void UC1701_transfer(register uint8_t data1)
{

	for (register int i = 0; i < 8; i++)
	{
		GPIO_Display_SCK->PCOR = 1U << Pin_Display_SCK;

		if ((data1 & 0x80) == 0U)
		{
			GPIO_Display_SDA->PCOR = 1U << Pin_Display_SDA;// Hopefully the compiler will otimise this to a value rather than using a shift
		}
		else
		{
			GPIO_Display_SDA->PSOR = 1U << Pin_Display_SDA;// Hopefully the compiler will otimise this to a value rather than using a shift
		}
		GPIO_Display_SCK->PSOR = 1U << Pin_Display_SCK;// Hopefully the compiler will otimise this to a value rather than using a shift

		data1 = data1 << 1;
	}
}
#endif // ! PLATFORM_GD77S

void ucSetInverseVideo(bool inverted)
{
#if ! defined(PLATFORM_GD77S)
	isInverted = inverted;
	GPIO_PinWrite(GPIO_Display_CS, Pin_Display_CS, 0);// Enable CS
	UC1701_setCommandMode();
	if (isInverted)
	{
		UC1701_transfer(0xA7); // Black background, white pixels
	}
	else
	{
		UC1701_transfer(0xA4); // White background, black pixels
	}

    UC1701_transfer(0xAF); // Set Display Enable
    UC1701_setDataMode();
	GPIO_PinWrite(GPIO_Display_CS, Pin_Display_CS, 1);// Disable CS
#endif // ! PLATFORM_GD77S
}

void ucBegin(bool isInverted)
{
#if ! defined(PLATFORM_GD77S)
	GPIO_PinWrite(GPIO_Display_CS, Pin_Display_CS, 0);// Enable CS
    // Set the LCD parameters...
	UC1701_setCommandMode();
	UC1701_transfer(0xE2); // System Reset
	UC1701_transfer(0x2F); // Voltage Follower On
	UC1701_transfer(0x81); // Set Electronic Volume = 15
	UC1701_transfer(nonVolatileSettings.displayContrast); //
	UC1701_transfer(0xA2); // Set Bias = 1/9
#if defined(PLATFORM_RD5R)
	UC1701_transfer(0xA0); // Set SEG Direction
	UC1701_transfer(0xC8); // Set COM Direction
#else
	UC1701_transfer(0xA1); // A0 Set SEG Direction
	UC1701_transfer(0xC0); // Set COM Direction
#endif
	if (isInverted)
	{
		UC1701_transfer(0xA7); // Black background, white pixels
	}
	else
	{
		UC1701_transfer(0xA4); // White background, black pixels
	}

    UC1701_transfer(0xAF); // Set Display Enable
	GPIO_PinWrite(GPIO_Display_CS, Pin_Display_CS, 1);// Disable CS
    ucClearBuf();
    ucRender();
#endif // ! PLATFORM_GD77S
}

void ucSetContrast(uint8_t contrast)
{
#if ! defined(PLATFORM_GD77S)
	GPIO_PinWrite(GPIO_Display_CS, Pin_Display_CS, 0);// Enable CS
	UC1701_setCommandMode();
	UC1701_transfer(0x81);              // command to set contrast
	UC1701_transfer(contrast);          // set contrast
	UC1701_setDataMode();
	GPIO_PinWrite(GPIO_Display_CS, Pin_Display_CS, 1);// Disable CS
#endif // ! PLATFORM_GD77S
}


// Note.
// Entering "Sleep" mode makes the display go blank
void ucSetDisplayPowerMode(bool wake)
{
#if ! defined(PLATFORM_GD77S)

	if (isAwake == wake)
	{
		return;
	}

	isAwake = wake;
	GPIO_PinWrite(GPIO_Display_CS, Pin_Display_CS, 0);// Enable CS
	UC1701_setCommandMode();

	if (wake)
	{
		// enter normal display mode
		if (isInverted)
		{
			UC1701_transfer(0xA7); // Black background, white pixels
		}
		else
		{
			UC1701_transfer(0xA4); // White background, black pixels
		}
		UC1701_transfer(0xAF); // White background, black pixels
	}
	else
	{
		// Enter sleep mode
		UC1701_transfer(0xAE); // "Set Display OFF" (text from datasheet)
		UC1701_transfer(0xA5); // "Set All-Pixel-ON" (text from datasheet)
	}

	UC1701_setDataMode();
	GPIO_PinWrite(GPIO_Display_CS, Pin_Display_CS, 1);// Disable CS

#endif
}
