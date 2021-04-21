/*
 * Initial work for port to MK22FN512xxx12 Copyright (C)2019 Kai Ludwig, DG4KLU
 *
 * Code mainly re-written by Roger Clark. VK3KYY / G4KYF
 * based on information and code references from various sources, including
 * https://github.com/bitbank2/uc1701 and
 * https://os.mbed.com/users/Anaesthetix/code/UC1701/file/7494bdca926b/
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

#ifndef _OPENGD77_UC1701_H_
#define _OPENGD77_UC1701_H_

#include <stdbool.h>
#include <math.h>
#include <FreeRTOS.h>
#include <task.h>


typedef enum
{
	FONT_SIZE_1 = 0,
	FONT_SIZE_1_BOLD,
	FONT_SIZE_2,
	FONT_SIZE_3,
	FONT_SIZE_4
} ucFont_t;

typedef enum
{
	TEXT_ALIGN_LEFT = 0,
	TEXT_ALIGN_CENTER,
	TEXT_ALIGN_RIGHT
} ucTextAlign_t;

typedef enum
{
	CHOICE_OK = 0,
	CHOICE_YESNO,
	CHOICE_DISMISS,
	CHOICES_OKARROWS,// QuickKeys
	CHOICES_NUM
} ucChoice_t;

extern uint8_t screenBuf[];

#if defined(PLATFORM_RD5R)
#define FONT_SIZE_3_HEIGHT                        8
#define DISPLAY_SIZE_Y                           48
#else
#define FONT_SIZE_3_HEIGHT                       16
#define DISPLAY_SIZE_Y                           64
#endif

#define DISPLAY_SIZE_X                          128
#define DISPLAY_NUMBER_OF_ROWS  (DISPLAY_SIZE_Y / 8)



void ucBegin(bool isInverted);
void ucClearBuf(void);
void ucClearRows(int16_t startRow, int16_t endRow, bool isInverted);
void ucRender(void);
void ucRenderRows(int16_t startRow, int16_t endRow);
void ucPrintCentered(uint8_t y,const  char *text, ucFont_t fontSize);
void ucPrintAt(uint8_t x, uint8_t y,const  char *text, ucFont_t fontSize);
int ucPrintCore(int16_t x, int16_t y,const char *szMsg, ucFont_t fontSize, ucTextAlign_t alignment, bool isInverted);

int16_t ucSetPixel(int16_t x, int16_t y, bool color);

void ucDrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool color);
void ucDrawFastVLine(int16_t x, int16_t y, int16_t h, bool color);
void ucDrawFastHLine(int16_t x, int16_t y, int16_t w, bool color);

void ucDrawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, bool color);
void ucDrawCircle(int16_t x0, int16_t y0, int16_t r, bool color);
void ucFillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, bool color);
void ucFillCircle(int16_t x0, int16_t y0, int16_t r, bool color);

void ucDrawEllipse(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool color);

void ucDrawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool color);
void ucFillTriangle ( int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool color);

void ucFillArc(uint16_t x, uint16_t y, uint16_t radius, uint16_t thickness, float start, float end, bool color);

void ucDrawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool color);
void ucFillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool color);
void ucDrawRoundRectWithDropShadow(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool color);

void ucDrawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool color);
void ucFillRect(int16_t x, int16_t y, int16_t width, int16_t height, bool isInverted);
void ucDrawRectWithDropShadow(int16_t x, int16_t y, int16_t w, int16_t h, bool color);

void ucDrawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, bool color);
void ucDrawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, bool color);

void ucSetContrast(uint8_t contrast);
void ucSetInverseVideo(bool isInverted);
void ucSetDisplayPowerMode(bool wake);

void ucDrawChoice(ucChoice_t choice, bool clearRegion);

uint8_t * ucGetDisplayBuffer(void);

#endif /* _OPENGD77_UC1701_H_ */
