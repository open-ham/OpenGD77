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
#include "user_interface/uiUtilities.h"
#include "functions/settings.h"
#include "functions/ticks.h"

enum LOCK_STATE { LOCK_NONE = 0x00, LOCK_KEYPAD = 0x01, LOCK_PTT = 0x02, LOCK_BOTH = 0x03 };

static void updateScreen(bool update);
static void handleEvent(uiEvent_t *ev);

static bool lockDisplayed = false;
static const uint32_t TIMEOUT_MS = 500;
static int lockState = LOCK_NONE;

menuStatus_t menuLockScreen(uiEvent_t *ev, bool isFirstRun)
{
	static uint32_t m = 0;

	if (isFirstRun)
	{
		m = fw_millis();

		updateScreen(lockDisplayed);
	}
	else
	{
		if ((lockDisplayed) && (
				((nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1) && (voicePromptsIsPlaying() == false)) ||
				((nonVolatileSettings.audioPromptMode <= AUDIO_PROMPT_MODE_BEEP) && ((ev->time - m) > TIMEOUT_MS))))
		{
			lockDisplayed = false;
			menuSystemPopPreviousMenu();
			return MENU_STATUS_SUCCESS;
		}

		if (ev->hasEvent)
		{
			m = fw_millis(); // reset timer on each key button/event.

			handleEvent(ev);
		}
	}
	return MENU_STATUS_SUCCESS;
}

static void redrawScreen(bool update, bool state)
{
	if (update)
	{
		// Clear inner rect only
		ucFillRoundRect(5, 3, 118, DISPLAY_SIZE_Y - 8, 5, false);
	}
	else
	{
		// Clear whole screen
		ucClearBuf();
		ucDrawRoundRectWithDropShadow(4, 4, 120, DISPLAY_SIZE_Y - 6, 5, true);
	}

	if (state)
	{
		int bufferLen = strlen(currentLanguage->keypad) + 3 + strlen(currentLanguage->ptt) + 1;
		char buf[bufferLen];

		memset(buf, 0, bufferLen);

		if (keypadLocked)
		{
			strcat(buf, currentLanguage->keypad);
		}

		if (PTTLocked)
		{
			if (keypadLocked)
			{
				strcat(buf, " & ");
			}

			strcat(buf, currentLanguage->ptt);
		}
		buf[bufferLen - 1] = 0;

		ucPrintCentered(6, buf, FONT_SIZE_3);

#if defined(PLATFORM_RD5R)

		ucPrintCentered(14, currentLanguage->locked, FONT_SIZE_3);
		ucPrintCentered(24, currentLanguage->press_blue_plus_star, FONT_SIZE_1);
		ucPrintCentered(32, currentLanguage->to_unlock, FONT_SIZE_1);
#else
		ucPrintCentered(22, currentLanguage->locked, FONT_SIZE_3);
		ucPrintCentered(40, currentLanguage->press_blue_plus_star, FONT_SIZE_1);
		ucPrintCentered(48, currentLanguage->to_unlock, FONT_SIZE_1);
#endif

		voicePromptsInit();
		voicePromptsAppendPrompt(PROMPT_SILENCE);

		if (lockState & LOCK_KEYPAD)
		{
			voicePromptsAppendLanguageString(&currentLanguage->keypad);
			voicePromptsAppendPrompt(PROMPT_SILENCE);
		}

		if (lockState & LOCK_PTT)
		{
			voicePromptsAppendLanguageString(&currentLanguage->ptt);
			voicePromptsAppendPrompt(PROMPT_SILENCE);
		}

		voicePromptsAppendLanguageString(&currentLanguage->locked);
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendLanguageString(&currentLanguage->press_blue_plus_star);
		voicePromptsAppendLanguageString(&currentLanguage->to_unlock);
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsPlay();
	}
	else
	{
		ucPrintCentered((DISPLAY_SIZE_Y - 16) / 2, currentLanguage->unlocked, FONT_SIZE_3);

		voicePromptsInit();
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendLanguageString(&currentLanguage->unlocked);
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsPlay();
	}

	ucRender();
	lockDisplayed = true;
}

static void updateScreen(bool updateOnly)
{
	bool keypadChanged = false;
	bool PTTChanged = false;

	if (keypadLocked)
	{
		if ((lockState & LOCK_KEYPAD) == 0)
		{
			keypadChanged = true;
			lockState |= LOCK_KEYPAD;
		}
	}
	else
	{
		if ((lockState & LOCK_KEYPAD))
		{
			keypadChanged = true;
			lockState &= ~LOCK_KEYPAD;
		}
	}

	if (PTTLocked)
	{
		if ((lockState & LOCK_PTT) == 0)
		{
			PTTChanged = true;
			lockState |= LOCK_PTT;
		}
	}
	else
	{
		if ((lockState & LOCK_PTT))
		{
			PTTChanged = true;
			lockState &= ~LOCK_PTT;
		}
	}

	if (updateOnly)
	{
		if (keypadChanged || PTTChanged)
		{
			redrawScreen(updateOnly, ((lockState & LOCK_KEYPAD) || (lockState & LOCK_PTT)));
		}
		else
		{
			if (lockDisplayed == false)
			{
				redrawScreen(updateOnly, false);
			}
		}
	}
	else
	{
		// Draw everything
		redrawScreen(false, keypadLocked || PTTLocked);
	}
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

	if (KEYCHECK_DOWN(ev->keys, KEY_STAR) && BUTTONCHECK_DOWN(ev, BUTTON_SK2))
	{
		if ((nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1) && voicePromptsIsPlaying())
		{
			voicePromptsTerminate();
		}

		keypadLocked = false;
		PTTLocked = false;
		lockDisplayed = false;
		menuSystemPopAllAndDisplayRootMenu();
		menuSystemPushNewMenu(UI_LOCK_SCREEN);
	}
	else if ((nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1) && voicePromptsIsPlaying() && (ev->keys.key != 0) && (ev->keys.event & KEY_MOD_UP))
	{
		// Cancel the voice on any key event (that hides the lock screen earlier)
		voicePromptsTerminate();
	}
}

void menuLockScreenPop(void)
{
	lockDisplayed = false;

	if (menuSystemGetCurrentMenuNumber() == UI_LOCK_SCREEN)
	{
		menuSystemPopPreviousMenu();
	}
}
