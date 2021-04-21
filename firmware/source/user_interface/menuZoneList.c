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
#include "functions/codeplug.h"
#include "main.h"
#include "functions/settings.h"
#include "user_interface/menuSystem.h"
#include "user_interface/uiUtilities.h"
#include "user_interface/uiLocalisation.h"

static void updateScreen(bool isFirstRun);
static void handleEvent(uiEvent_t *ev);
static void setZoneToUserSelection(void);

static menuStatus_t menuZoneExitCode = MENU_STATUS_SUCCESS;

menuStatus_t menuZoneList(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		menuDataGlobal.endIndex = codeplugZonesGetCount();
		menuDataGlobal.currentItemIndex = nonVolatileSettings.currentZone;

		voicePromptsInit();
		voicePromptsAppendLanguageString(&currentLanguage->zone);
		voicePromptsAppendPrompt(PROMPT_SILENCE);

		updateScreen(true);
		return (MENU_STATUS_LIST_TYPE | MENU_STATUS_SUCCESS);
	}
	else
	{
		menuZoneExitCode = MENU_STATUS_SUCCESS;

		if (ev->hasEvent)
		{
			handleEvent(ev);
		}
	}
	return menuZoneExitCode;
}

static void updateScreen(bool isFirstRun)
{
	char nameBuf[17];
	int mNum;
	struct_codeplugZone_t zoneBuf;

	ucClearBuf();
	menuDisplayTitle(currentLanguage->zones);

	for(int i = -1; i <= 1; i++)
	{
		if (menuDataGlobal.endIndex <= (i + 1))
		{
			break;
		}

		mNum = menuGetMenuOffset(menuDataGlobal.endIndex, i);

		codeplugZoneGetDataForNumber(mNum, &zoneBuf);
		codeplugUtilConvertBufToString(zoneBuf.name, nameBuf, 16);// need to convert to zero terminated string

		menuDisplayEntry(i, mNum, (char *)nameBuf);

		if (i == 0)
		{
			if (!isFirstRun)
			{
				voicePromptsInit();
			}

			if (strcmp(nameBuf,currentLanguage->all_channels) == 0)
			{
				voicePromptsAppendLanguageString(&currentLanguage->all_channels);
			}
			else
			{
				voicePromptsAppendString(nameBuf);
			}

			promptsPlayNotAfterTx();
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
		if ((QUICKKEY_TYPE(ev->function) == QUICKKEY_MENU) && (QUICKKEY_LONGENTRYID(ev->function) > 0) && (QUICKKEY_LONGENTRYID(ev->function) <= menuDataGlobal.endIndex))
		{
			menuDataGlobal.currentItemIndex = QUICKKEY_LONGENTRYID(ev->function)-1;
			setZoneToUserSelection();
		}
		return;
	}


	if (KEYCHECK_PRESS(ev->keys, KEY_DOWN))
	{
		menuSystemMenuIncrement(&menuDataGlobal.currentItemIndex, menuDataGlobal.endIndex);
		updateScreen(false);
		menuZoneExitCode |= MENU_STATUS_LIST_TYPE;
	}
	else if (KEYCHECK_PRESS(ev->keys, KEY_UP))
	{
		menuSystemMenuDecrement(&menuDataGlobal.currentItemIndex, menuDataGlobal.endIndex);
		updateScreen(false);
		menuZoneExitCode |= MENU_STATUS_LIST_TYPE;
	}
	else if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
	{

		setZoneToUserSelection();
		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
	{
		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_SHORTUP_NUMBER(ev->keys) && BUTTONCHECK_DOWN(ev, BUTTON_SK2))
	{
		saveQuickkeyMenuLongValue(ev->keys.key, menuSystemGetCurrentMenuNumber(), menuDataGlobal.currentItemIndex + 1);
		return;
	}
}


static void setZoneToUserSelection(void)
{
	settingsSet(nonVolatileSettings.overrideTG, 0); // remove any TG override
	settingsSet(nonVolatileSettings.currentZone, (int16_t) menuDataGlobal.currentItemIndex);
	settingsSet(nonVolatileSettings.currentChannelIndexInZone , 0);// Since we are switching zones the channel index should be reset
	settingsSet(nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE], 0);// Since we are switching zones the TRx Group index should be reset
	channelScreenChannelData.rxFreq = 0x00; // Flag to the Channel screen that the channel data is now invalid and needs to be reloaded

	settingsSaveIfNeeded(true);
	menuSystemPopAllAndDisplaySpecificRootMenu(UI_CHANNEL_MODE, true);
	uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN; // Force screen redraw
}
