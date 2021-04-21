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

static void updateScreen(bool isFirstRun);
static void handleEvent(uiEvent_t *ev);

//#define CREDIT_TEXT_LENGTH 33
#if defined(PLATFORM_RD5R)
static const int NUM_LINES_PER_SCREEN = 4;
#else
static const int NUM_LINES_PER_SCREEN = 6;
#endif

static const int NUM_CREDITS = 7;
static const char *creditTexts[] = {"Roger VK3KYY","Daniel F1RMB","Dzmitry EW1ADG","Colin G4EML","Alex DL4LEX","Kai DG4KLU","Jason VK7ZJA"};

static menuStatus_t menuCreditsExitCode = MENU_STATUS_SUCCESS;

menuStatus_t menuCredits(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		menuDataGlobal.endIndex = ((NUM_CREDITS >= NUM_LINES_PER_SCREEN) ? (NUM_CREDITS - NUM_LINES_PER_SCREEN) : (NUM_LINES_PER_SCREEN - NUM_CREDITS));
		updateScreen(true);
	}
	else
	{
		menuCreditsExitCode = MENU_STATUS_SUCCESS;
		if (ev->hasEvent)
		{
			handleEvent(ev);
		}
	}
	return menuCreditsExitCode;
}

static void updateScreen(bool isFirstRun)
{

	if (isFirstRun && (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1))
	{
		if (voicePromptsIsPlaying())
		{
			voicePromptsTerminate();
		}
		voicePromptsInit();
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendLanguageString(&currentLanguage->credits);
		voicePromptsAppendLanguageString(&currentLanguage->menu);
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		promptsPlayNotAfterTx();
	}

	ucClearBuf();
	menuDisplayTitle(currentLanguage->credits);


	for(int i = 0; i < NUM_LINES_PER_SCREEN; i++)
	{
		ucPrintCentered(i * 8 + 16, (char *)creditTexts[i + menuDataGlobal.currentItemIndex], FONT_SIZE_1);
	}

	ucRender();
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

	if (KEYCHECK_SHORTUP(ev->keys, KEY_RED) || KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
	{
		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_PRESS(ev->keys, KEY_DOWN))
	{
		if (menuDataGlobal.currentItemIndex < ((NUM_CREDITS >= NUM_LINES_PER_SCREEN) ? (NUM_CREDITS - NUM_LINES_PER_SCREEN) : (NUM_LINES_PER_SCREEN - NUM_CREDITS)))
		{
			menuDataGlobal.currentItemIndex++;
		}

		updateScreen(false);
	}
	else if (KEYCHECK_PRESS(ev->keys, KEY_UP))
	{
		if (menuDataGlobal.currentItemIndex > 0)
		{
			menuDataGlobal.currentItemIndex--;
		}

		updateScreen(false);
	}

	if (KEYCHECK_SHORTUP_NUMBER(ev->keys)  && (BUTTONCHECK_DOWN(ev, BUTTON_SK2)))
	{
		saveQuickkeyMenuIndex(ev->keys.key, menuSystemGetCurrentMenuNumber(), 0, 0);
		return;
	}
}
