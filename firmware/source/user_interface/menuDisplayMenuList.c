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
#include "main.h"
#include "user_interface/menuSystem.h"
#include "user_interface/uiLocalisation.h"
#include "user_interface/uiUtilities.h"

static void updateScreen(bool isFirstRun);
static void handleEvent(uiEvent_t *ev);

static menuStatus_t menuDisplayListExitCode = MENU_STATUS_SUCCESS;

menuStatus_t menuDisplayMenuList(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		int currentMenuNumber = menuSystemGetCurrentMenuNumber();
		menuDataGlobal.currentMenuList = (menuItemNewData_t *)menuDataGlobal.data[currentMenuNumber]->items;
		menuDataGlobal.endIndex = menuDataGlobal.data[currentMenuNumber]->numItems;

		voicePromptsInit();
		voicePromptsAppendLanguageString(&currentLanguage->menu);
		voicePromptsAppendPrompt(PROMPT_SILENCE);

		updateScreen(true);

		return (MENU_STATUS_LIST_TYPE | MENU_STATUS_SUCCESS);
	}
	else
	{
		menuDisplayListExitCode = MENU_STATUS_SUCCESS;

		if (ev->hasEvent)
		{
			handleEvent(ev);
		}
	}
	return menuDisplayListExitCode;
}

static void updateScreen(bool isFirstRun)
{
	int mNum;
	const char *mName = currentLanguage->menu;

	ucClearBuf();

	// Apply some menu title override(s)
	switch (menuSystemGetCurrentMenuNumber())
	{
		case MENU_CONTACTS_MENU:
			mName = currentLanguage->contacts;
			break;
	}

	menuDisplayTitle(mName);

	for(int i = -1; i <= 1 ; i++)
	{
		mNum = menuGetMenuOffset(menuDataGlobal.endIndex, i);

		if (mNum < menuDataGlobal.endIndex)
		{
			if (menuDataGlobal.currentMenuList[mNum].stringOffset >= 0)
			{
				char **menuName = (char **)((int)&currentLanguage->LANGUAGE_NAME + (menuDataGlobal.currentMenuList[mNum].stringOffset * sizeof(char *)));
				menuDisplayEntry(i, mNum, (const char *)*menuName);

				if (i == 0)
				{
					if (!isFirstRun)
					{
						voicePromptsInit();
					}
					voicePromptsAppendLanguageString((const char * const *)menuName);
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
		if ((QUICKKEY_TYPE(ev->function) == QUICKKEY_MENU) && (QUICKKEY_ENTRYID(ev->function) < menuDataGlobal.endIndex))
		{
			menuDataGlobal.currentItemIndex = QUICKKEY_ENTRYID(ev->function);
			updateScreen(false);
		}
		return;
	}

	if (KEYCHECK_PRESS(ev->keys, KEY_DOWN))
	{
		menuSystemMenuIncrement(&menuDataGlobal.currentItemIndex, menuDataGlobal.endIndex);
		updateScreen(false);
		menuDisplayListExitCode |= MENU_STATUS_LIST_TYPE;
	}
	else if (KEYCHECK_PRESS(ev->keys, KEY_UP))
	{
		menuSystemMenuDecrement(&menuDataGlobal.currentItemIndex, menuDataGlobal.endIndex);
		updateScreen(false);
		menuDisplayListExitCode |= MENU_STATUS_LIST_TYPE;
	}
	else if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
	{
		if (menuDataGlobal.currentMenuList[menuDataGlobal.currentItemIndex].menuNum != -1)
		{
			menuSystemPushNewMenu(menuDataGlobal.currentMenuList[menuDataGlobal.currentItemIndex].menuNum);
		}
		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
	{
		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys, KEY_STAR) && (menuSystemGetCurrentMenuNumber() == MENU_MAIN_MENU))
	{
		keypadLocked = true;
		menuSystemPopAllAndDisplayRootMenu();
		menuSystemPushNewMenu(UI_LOCK_SCREEN);
		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys, KEY_HASH) && (menuSystemGetCurrentMenuNumber() == MENU_MAIN_MENU))
	{
		PTTLocked = !PTTLocked;
		menuSystemPopAllAndDisplayRootMenu();
		menuSystemPushNewMenu(UI_LOCK_SCREEN);
		return;
	}
	else if (KEYCHECK_SHORTUP_NUMBER(ev->keys) && (BUTTONCHECK_DOWN(ev, BUTTON_SK2)))
	{
		if (menuDataGlobal.currentMenuList[menuDataGlobal.currentItemIndex].menuNum != -1)
		{
			saveQuickkeyMenuIndex(ev->keys.key, menuDataGlobal.currentMenuList[menuDataGlobal.currentItemIndex].menuNum, 0, 0);
		}
		return;
	}
}
