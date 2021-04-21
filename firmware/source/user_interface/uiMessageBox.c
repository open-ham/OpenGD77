/*
 * Copyright (C)2019-2020 Roger Clark. VK3KYY / G4KYF
 *                        Daniel Caujolle-Bert, F1RMB
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
#include "user_interface/uiUtilities.h"
#include "functions/ticks.h"
#include "utils.h"

static menuStatus_t menuMessageBoxExitCode;

static void handleEvent(uiEvent_t *ev);
static void updateScreen(bool forceRedraw);
static void displayMessage(void);
static void displayButtons(void);

menuStatus_t uiMessageBox(uiEvent_t *ev, bool isFirstRun)
{
	static uint32_t curm = 0;

	if (isFirstRun)
	{
		freqEnterReset();
		uiDataGlobal.MessageBox.keyPressed = 0;

		updateScreen(true);
		menuMessageBoxExitCode = MENU_STATUS_SUCCESS;
	}
	else
	{
		bool forceRedraw = false;

		menuMessageBoxExitCode = MENU_STATUS_SUCCESS;

		if (ev->events == NO_EVENT)
		{
			// We are entering digits, so update the screen as we have a cursor to blink
			if ((uiDataGlobal.MessageBox.type == MESSAGEBOX_TYPE_PIN_CODE) &&
					(uiDataGlobal.FreqEnter.index >= 0) && ((ev->time - curm) > 300))
			{
				curm = ev->time;
				forceRedraw = true;
			}

			if (forceRedraw)
			{
				updateScreen(false);
			}
		}
		else
		{

			if (ev->hasEvent)
			{
				handleEvent(ev);
			}
		}
	}

	return menuMessageBoxExitCode;
}

static void handleEvent(uiEvent_t *ev)
{
	if (ev->events & BUTTON_EVENT)
	{
		if (repeatVoicePromptOnSK1(ev))
		{
			return;
		}
	}

	if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
	{
		int greenHandled = (uiDataGlobal.MessageBox.buttons != MESSAGEBOX_BUTTONS_DISMISS);
		bool res = false;

		// Ensure we have a complete pin code before calling the validator function
		if ((uiDataGlobal.MessageBox.type == MESSAGEBOX_TYPE_PIN_CODE) &&
				(uiDataGlobal.FreqEnter.index < uiDataGlobal.MessageBox.pinLength))
		{
			return;
		}

		uiDataGlobal.MessageBox.keyPressed = KEY_GREEN;

		// Call validator
		if (uiDataGlobal.MessageBox.validatorCallback)
		{
			res = uiDataGlobal.MessageBox.validatorCallback();

			if (uiDataGlobal.MessageBox.type == MESSAGEBOX_TYPE_PIN_CODE)
			{
				freqEnterReset();

				if (res == false)
				{
					menuMessageBoxExitCode |= MENU_STATUS_ERROR;
				}
			}
		}

		if (greenHandled && (((uiDataGlobal.MessageBox.validatorCallback != NULL) && res) || (uiDataGlobal.MessageBox.validatorCallback == NULL)))
		{
			uiDataGlobal.MessageBox.validatorCallback = NULL;
			menuSystemPopPreviousMenu();
			return;
		}
	}
	else if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
	{
		bool redHandled = (uiDataGlobal.MessageBox.buttons != MESSAGEBOX_BUTTONS_OK);
		bool res = false;

		uiDataGlobal.MessageBox.keyPressed = KEY_RED;

		// Call validator
		if (uiDataGlobal.MessageBox.validatorCallback)
		{
			res = uiDataGlobal.MessageBox.validatorCallback();
		}

		if (redHandled && (((uiDataGlobal.MessageBox.validatorCallback != NULL) && res) || (uiDataGlobal.MessageBox.validatorCallback == NULL)))
		{
			uiDataGlobal.MessageBox.validatorCallback = NULL;
			menuSystemPopPreviousMenu();
			return;
		}
	}
	else
	{
		if (uiDataGlobal.MessageBox.type == MESSAGEBOX_TYPE_PIN_CODE)
		{
			bool redraw = false;

			if ((uiDataGlobal.FreqEnter.index > 0) && KEYCHECK_PRESS(ev->keys, KEY_LEFT))
			{
				uiDataGlobal.FreqEnter.index--;
				uiDataGlobal.FreqEnter.digits[uiDataGlobal.FreqEnter.index] = '-';
				redraw = true;
			}

			if (uiDataGlobal.FreqEnter.index < uiDataGlobal.MessageBox.pinLength)
			{
				int keyval = menuGetKeypadKeyValue(ev, true);

				if (keyval != 99)
				{
					voicePromptsInit();
					voicePromptsAppendPrompt(PROMPT_0 + keyval);
					voicePromptsPlay();

					uiDataGlobal.FreqEnter.digits[uiDataGlobal.FreqEnter.index] = (char) keyval + '0';
					uiDataGlobal.FreqEnter.index++;
					redraw = true;
				}
			}

			if (redraw)
			{
				updateScreen(false);
			}
		}
	}
}

static void updateScreen(bool forceRedraw)
{
	static bool blink = false;
	static uint32_t blinkTime = 0;

	if (forceRedraw)
	{
		ucClearBuf();
	}

	switch (uiDataGlobal.MessageBox.type)
	{
		case MESSAGEBOX_TYPE_INFO:
		{
			if (uiDataGlobal.MessageBox.decoration != MESSAGEBOX_DECORATION_NONE)
			{
				ucDrawRoundRectWithDropShadow(4, 4, 120, DISPLAY_SIZE_Y - (uiDataGlobal.MessageBox.buttons != MESSAGEBOX_BUTTONS_NONE ? FONT_SIZE_3_HEIGHT : 0) - 6, 5, true);
			}

			if (strlen(uiDataGlobal.MessageBox.message))
			{
				displayMessage();
			}

			displayButtons();
		}
		break;

		case MESSAGEBOX_TYPE_PIN_CODE:
		{
			char pinStr[17] = { 0 };
			int8_t xCursor = -1;
			int8_t yCursor = -1;

			if (forceRedraw)
			{
				displayMessage();

				uiDataGlobal.MessageBox.buttons = MESSAGEBOX_BUTTONS_OK;
				displayButtons();
			}
			else
			{
				// Clear input
				ucFillRect(0, (((DISPLAY_SIZE_Y / 8) - 1) * 3) + 2, DISPLAY_SIZE_X,
						(DISPLAY_SIZE_Y - (((DISPLAY_SIZE_Y / 8) - 1) * 3)) - FONT_SIZE_3_HEIGHT - 2, true);
			}

			for (uint8_t i = 0; i < SAFE_MIN(uiDataGlobal.MessageBox.pinLength, FREQ_ENTER_DIGITS_MAX); i++)
			{
				sprintf(pinStr, "%s%c", pinStr, uiDataGlobal.FreqEnter.digits[i]);
			}

			ucPrintCentered(DISPLAY_Y_POS_RX_FREQ, pinStr, FONT_SIZE_3);

			// Cursor
			if (uiDataGlobal.FreqEnter.index < uiDataGlobal.MessageBox.pinLength)
			{
				xCursor = ((DISPLAY_SIZE_X - (strlen(pinStr) * 8)) >> 1) + (uiDataGlobal.FreqEnter.index * 8);
				yCursor = DISPLAY_Y_POS_RX_FREQ + (FONT_SIZE_3_HEIGHT - 2);
			}

			if ((xCursor >= 0) && (yCursor >= 0))
			{
				ucDrawFastHLine(xCursor + 1, yCursor, 6, blink);

				if ((fw_millis() - blinkTime) > 500)
				{
					blinkTime = fw_millis();
					blink = !blink;
				}
			}
		}
		break;

		default:
			break;
	}

	ucRender();
}

static void displayMessage(void)
{
	char msg[17];
	int y = 0;
	bool decoration = (uiDataGlobal.MessageBox.decoration != MESSAGEBOX_DECORATION_NONE);
	char *p = strchr(uiDataGlobal.MessageBox.message, '\n');
	int linesCount = (p ? 1 : 0);
	int maxLines = (decoration ? 3 : 4);

	if (uiDataGlobal.MessageBox.type != MESSAGEBOX_TYPE_PIN_CODE)
	{
		if (uiDataGlobal.MessageBox.buttons != MESSAGEBOX_BUTTONS_NONE)
		{
			maxLines--;
		}

		while (p)
		{
			linesCount++;

			if (linesCount == maxLines)
			{
				break;
			}

			p = strchr(p + 1, '\n');
		}

		// Y start
		y = (DISPLAY_SIZE_Y - (uiDataGlobal.MessageBox.buttons != MESSAGEBOX_BUTTONS_NONE ? FONT_SIZE_3_HEIGHT : 0) - (linesCount * FONT_SIZE_3_HEIGHT)) >> 1;
	}

	if ((uiDataGlobal.MessageBox.type != MESSAGEBOX_TYPE_PIN_CODE) && linesCount)
	{
		int len = strlen(uiDataGlobal.MessageBox.message);
		char *pb = uiDataGlobal.MessageBox.message;
		char *pe = &uiDataGlobal.MessageBox.message[0] + len;
		int cnt;

		p = strchr(uiDataGlobal.MessageBox.message, '\n');

		while (linesCount > 1)
		{
			cnt = SAFE_MIN((p - pb), (decoration ? 14 : 16));

			memcpy(msg, pb, cnt);
			msg[cnt] = '\0';

			ucPrintCentered(y, msg, FONT_SIZE_3);
			y += FONT_SIZE_3_HEIGHT;

			pb = p + 1;
			p = strchr(pb, '\n');

			linesCount--;
		}

		if (pb < pe)
		{
			cnt = SAFE_MIN((pe - pb), (decoration ? 14 : 16));

			p++;
			memcpy(msg, pb, cnt);
			msg[cnt] = '\0';

			ucPrintCentered(y, msg, FONT_SIZE_3);
		}
	}
	else
	{
		y = ((DISPLAY_SIZE_Y - FONT_SIZE_3_HEIGHT) >> 1);

		if (uiDataGlobal.MessageBox.type == MESSAGEBOX_TYPE_PIN_CODE)
		{
			decoration = true; // Override decoration

			// Display title
			ucDrawRoundRectWithDropShadow(2, 2, (DISPLAY_SIZE_X - 6), ((DISPLAY_SIZE_Y / 8) - 1) * 3, 3, true);

			y = 3;
		}

		snprintf(msg, (decoration ? 15 : 17), "%s", uiDataGlobal.MessageBox.message);
		ucPrintCentered(y, msg, FONT_SIZE_3);
	}

}

static void displayButtons(void)
{
	switch (uiDataGlobal.MessageBox.buttons)
	{
		case MESSAGEBOX_BUTTONS_OK:
			ucDrawChoice(CHOICE_OK, false);
			break;
		case MESSAGEBOX_BUTTONS_YESNO:
			ucDrawChoice(CHOICE_YESNO, false);
			break;
		case MESSAGEBOX_BUTTONS_DISMISS:
			ucDrawChoice(CHOICE_DISMISS, false);
			break;
		default:
			break;
	}
}
