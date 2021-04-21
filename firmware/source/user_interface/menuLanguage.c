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
#include "functions/settings.h"
#include "user_interface/menuSystem.h"
#include "user_interface/uiLocalisation.h"
#include "user_interface/uiUtilities.h"

#define CHAR_CONSTANTS_ONLY // Needed to get FONT_CHAR_* glyph offsets
#if defined(LANGUAGE_BUILD_JAPANESE)
#include "hardware/UC1701_charset_JA.h"
#else
#include "hardware/UC1701_charset.h"
#endif

static void updateScreen(void);
static void handleEvent(uiEvent_t *ev);
static menuStatus_t menuLanguageExitCode = MENU_STATUS_SUCCESS;


static void clearNonLatinChar(uint8_t *str)
{
#if ! defined(LANGUAGE_BUILD_JAPANESE)
	uint8_t *p = str;

	while (*p != '\0')
	{
		switch (*p)
		{
			case FONT_CHAR_CAPITAL_A_WITH_OGONEK: // Ą
			case FONT_CHAR_SMALL_A_WITH_OGONEK:   // ą
			case 192: // À
			case 193: // Á
			case 194: // Â
			case 195: // Ã
			case 196: // Ä
			case 197: // Å
			case 224: // à
			case 225: // á
			case 226: // â
			case 227: // ã
			case 228: // ä
			case 229: // å
				*p = 'a';
				break;

			case FONT_CHAR_CAPITAL_C_WITH_CARON: // Č
			case FONT_CHAR_SMALL_C_WITH_CARON:   // č
			case FONT_CHAR_CAPITAL_C_WITH_ACUTE: // Ć
			case FONT_CHAR_SMALL_C_WITH_ACUTE:   // ć
			case 199: // Ç
			case 231: // ç
				*p = 'c';
				break;

			case FONT_CHAR_CAPITAL_E_WITH_OGONEK: // Ę
			case FONT_CHAR_SMALL_E_WITH_OGONEK:   // ę
			case FONT_CHAR_CAPITAL_E_WITH_CARON:  // Ě
			case FONT_CHAR_SMALL_E_WITH_CARON:    // ě
			case 200: // È
			case 201: // É
			case 202: // Ê
			case 203: // Ë
			case 232: // è
			case 233: // é
			case 234: // ê
			case 235: // ë
				*p = 'e';
				break;

			case FONT_CHAR_G_CAPITAL_WITH_BREVE: // Ğ
			case FONT_CHAR_G_SMALL_WITH_BREVE:   // ğ
				*p = 'g';
				break;

			case FONT_CHAR_SMALL_I_DOTLESS:    // ı
			case FONT_CHAR_CAPITAL_I_WITH_DOT: // İ
			case 204: // Ì
			case 205: // Í
			case 206: // Î
			case 207: // Ï
			case 236: // ì
			case 237: // í
			case 238: // î
			case 239: // ï
				*p ='i';
				break;

			case FONT_CHAR_CAPITAL_L_WITH_STROKE: // Ł
			case FONT_CHAR_SMALL_L_WITH_STROKE:   // ł
				*p = 'l';
				break;

			case FONT_CHAR_CAPITAL_N_WITH_ACUTE: // Ń
			case FONT_CHAR_SMALL_N_WITH_ACUTE:   // ń
			case 209: // Ñ
			case 241: // ñ
				*p = 'n';
				break;

			case 210: // Ò
			case 211: // Ó
			case 212: // Ô
			case 213: // Õ
			case 214: // Ö
			case 216: // Ø
			case 242: // ò
			case 243: // ó
			case 244: // ô
			case 245: // õ
			case 246: // ö
			case 248: // ø
			case 240: // ð
				*p = 'o';
				break;

			case FONT_CHAR_CAPITAL_R_WITH_CARON: // Ř
			case FONT_CHAR_SMALL_R_WITH_CARON:   // ř
			case 208: // Ð
				*p = 'r';
				break;

			case FONT_CHAR_CAPITAL_S_WITH_ACUTE:   // Ś
			case FONT_CHAR_SMALL_S_WITH_ACUTE:     // ś
			case FONT_CHAR_CAPITAL_S_WITH_CARON:   // Š
			case FONT_CHAR_SMALL_S_WITH_CARON:     // š
			case FONT_CHAR_S_CAPITAL_WITH_CEDILLA: // Ş
			case FONT_CHAR_S_SMALL_WITH_CEDILLA:   // ş
			case 223: // ß
				*p = 's';
				break;

			case FONT_CHAR_CAPITAL_U_WITH_RING_ABOVE: // Ů
			case FONT_CHAR_SMALL_U_WITH_RING_ABOVE:   // ů
			case 217: // Ù
			case 218: // Ú
			case 219: // Û
			case 220: // Ü
			case 249: // ù
			case 250: // ú
			case 251: // û
			case 252: // ü
				*p = 'u';
				break;

			case FONT_CHAR_CAPITAL_Y_WITH_DIAERESIS: // Ÿ
			case 221: // Ý
			case 253: // ý
			case 255: // ÿ
				*p = 'y';
				break;

			case FONT_CHAR_SMALL_Z_WITH_CARON:       // ž
			case FONT_CHAR_CAPITAL_Z_WITH_CARON:     // Ž
			case FONT_CHAR_CAPITAL_Z_WITH_ACUTE:     // Ź
			case FONT_CHAR_SMALL_Z_WITH_ACUTE:       // ź
			case FONT_CHAR_CAPITAL_Z_WITH_DOT_ABOVE: // Ż
			case FONT_CHAR_SMALL_Z_WITH_DOT_ABOVE:   // ż
				*p = 'z';
				break;
		}
		p++;
	}
#endif
}

menuStatus_t menuLanguage(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		menuDataGlobal.endIndex = NUM_LANGUAGES;
		updateScreen();
		return (MENU_STATUS_LIST_TYPE | MENU_STATUS_SUCCESS);
	}
	else
	{
		menuLanguageExitCode = MENU_STATUS_SUCCESS;

		if (ev->hasEvent)
		{
			handleEvent(ev);
		}
	}
	return menuLanguageExitCode;
}

static void updateScreen(void)
{
	int mNum = 0;

	ucClearBuf();
	menuDisplayTitle(currentLanguage->language);

	for(int i = -1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(NUM_LANGUAGES, i);

		if (mNum < NUM_LANGUAGES)
		{
			menuDisplayEntry(i, mNum, (char *)languages[LANGUAGE_DISPLAY_ORDER[mNum]].LANGUAGE_NAME);

			if (i == 0)
			{
				if (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1)
				{
					char buffer[17];

					snprintf(buffer, 17, "%s", (char *)languages[LANGUAGE_DISPLAY_ORDER[mNum]].LANGUAGE_NAME);

					clearNonLatinChar((uint8_t *)&buffer[0]);

					voicePromptsInit();
					voicePromptsAppendString(buffer);
					promptsPlayNotAfterTx();
				}
			}
		}
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
	if (ev->events & FUNCTION_EVENT)
	{
		if ((QUICKKEY_TYPE(ev->function) == QUICKKEY_MENU) && (QUICKKEY_ENTRYID(ev->function) < NUM_LANGUAGES))
		{
			menuDataGlobal.currentItemIndex = QUICKKEY_ENTRYID(ev->function);
			settingsSet(nonVolatileSettings.languageIndex, menuDataGlobal.currentItemIndex);
			currentLanguage = &languages[menuDataGlobal.currentItemIndex];
			settingsSaveIfNeeded(true);
			menuSystemLanguageHasChanged();
			menuSystemPopAllAndDisplayRootMenu();
		}
		return;
	}

	if (KEYCHECK_PRESS(ev->keys, KEY_DOWN) && (menuDataGlobal.endIndex != 0))
	{
		menuSystemMenuIncrement(&menuDataGlobal.currentItemIndex, NUM_LANGUAGES);
		updateScreen();
		menuLanguageExitCode |= MENU_STATUS_LIST_TYPE;
	}
	else if (KEYCHECK_PRESS(ev->keys, KEY_UP))
	{
		menuSystemMenuDecrement(&menuDataGlobal.currentItemIndex, NUM_LANGUAGES);
		updateScreen();
		menuLanguageExitCode |= MENU_STATUS_LIST_TYPE;
	}
	else if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
	{
		settingsSet(nonVolatileSettings.languageIndex, (uint8_t) LANGUAGE_DISPLAY_ORDER[menuDataGlobal.currentItemIndex]);
		currentLanguage = &languages[LANGUAGE_DISPLAY_ORDER[menuDataGlobal.currentItemIndex]];
		settingsSaveIfNeeded(true);
		menuSystemLanguageHasChanged();
		menuSystemPopAllAndDisplayRootMenu();
		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
	{
		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_SHORTUP_NUMBER(ev->keys) && BUTTONCHECK_DOWN(ev, BUTTON_SK2))
	{
		saveQuickkeyMenuIndex(ev->keys.key, menuSystemGetCurrentMenuNumber(), menuDataGlobal.currentItemIndex, 0);
		return;
	}
}
