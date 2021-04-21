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


menuDataGlobal_t menuDataGlobal =
{
		.currentItemIndex 		= 0, // each menu can re-use this var to hold the position in their display list. To save wasted memory if they each had their own variable
		.startIndex 			= 0, // as above
		.endIndex 				= 0, // as above
		.lightTimer 			= -1,
		.currentMenuList		= NULL,

		.controlData =
		{
				.stackPosition 	= 0,
				.stack 			=
				{
						MENU_EMPTY, MENU_EMPTY, MENU_EMPTY, MENU_EMPTY, MENU_EMPTY, MENU_EMPTY, MENU_EMPTY, MENU_EMPTY, MENU_EMPTY, MENU_EMPTY, MENU_EMPTY, MENU_EMPTY, MENU_EMPTY, MENU_EMPTY, MENU_EMPTY, MENU_EMPTY
				}
		},

		/*
		 * ---------------------- IMPORTANT ----------------------------
		 *
		 * The menuFunctions array and the menusData array.....
		 *
		 * MUST match the enum MENU_SCREENS in menuSystem.h
		 *
		 * ---------------------- IMPORTANT ----------------------------
		 */
		.data 					=
		{
				NULL,// splash
				NULL,// power off
				NULL,// vfo mode
				NULL,// channel mode
				&menuDataMainMenu,
				&menuDataContact,
				NULL,// zone
				NULL,// Battery
				NULL,// Firmwareinfo
				NULL,// Numerical entry
				NULL,// Tx
				NULL,// RSSI
				NULL,// LastHeard
				NULL,// Options
				NULL,// Display options
				NULL,// Sound options
				NULL,// Credits
				NULL,// Channel Details
				NULL,// hotspot mode
				NULL,// CPS
				NULL,// Quick menu - Channel
				NULL,// Quick menu - VFO
				NULL,// Lock screen
				NULL,// Contact List
				NULL,// DTMF Contact List
				NULL,// Contact Quick List (SK2+#)
				NULL,// Contact List Quick Menu
				NULL,// Contact Details
				NULL,// New Contact
				NULL,// Language
				NULL,// Private Call
				NULL,// MessageBox
		}
};

static menuFunctionData_t menuFunctions[] =
{
		{ uiSplashScreen,           0 },
		{ uiPowerOff,               0 },
		{ uiVFOMode,                0 },
		{ uiChannelMode,            0 },
		{ menuDisplayMenuList,      0 },// display Main menu using the menu display system
		{ menuDisplayMenuList,      0 },// display Contact menu using the menu display system
		{ menuZoneList,             0 },
		{ menuRadioInfos,           0 },
		{ menuFirmwareInfoScreen,   0 },
		{ menuNumericalEntry,       0 },
		{ menuTxScreen,             0 },
		{ menuRSSIScreen,           0 },
		{ menuLastHeard,            0 },
		{ menuOptions,              0 },
		{ menuDisplayOptions,       0 },
		{ menuSoundOptions,         0 },
		{ menuCredits,              0 },
		{ menuChannelDetails,       0 },
		{ menuHotspotMode,          0 },
		{ uiCPS,                    0 },
		{ uiChannelModeQuickMenu,   0 },
		{ uiVFOModeQuickMenu,       0 },
		{ menuLockScreen,           0 },
		{ menuContactList,          0 },
		{ menuContactList,          0 },
		{ menuContactList,          0 },
		{ menuContactListSubMenu,   0 },
		{ menuContactDetails,       0 },
		{ menuContactDetails,       0 },
		{ menuLanguage,             0 },
		{ menuPrivateCall,          0 },
		{ uiMessageBox,             0 }
};

static void menuSystemCheckForFirstEntryAudible(menuStatus_t status)
{
	if (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_BEEP)
	{
		if (status & MENU_STATUS_ERROR)
		{
			nextKeyBeepMelody = (int *)MELODY_ERROR_BEEP;
		}
		else if (((status & MENU_STATUS_LIST_TYPE) && (menuDataGlobal.currentItemIndex == 0)) || (status & MENU_STATUS_FORCE_FIRST))
		{
			nextKeyBeepMelody = (int *)MELODY_KEY_BEEP_FIRST_ITEM;
		}
		else if (status & MENU_STATUS_INPUT_TYPE)
		{
			nextKeyBeepMelody = (int *)MELODY_ACK_BEEP;
		}
	}
}

static void menuSystemPushMenuFirstRun(void)
{
	uiEvent_t ev = { .buttons = 0, .keys = NO_KEYCODE, .rotary = 0, .function = 0, .events = NO_EVENT, .hasEvent = false, .time = fw_millis() };
	menuStatus_t status;

	// Due to QuickKeys, menu list won't go through menuDisplayMenuList() first, so those
	// two members won't get always initialized. Hence, we need to tag them as uninitialized,
	// and check if they got initialized after entering a menu.
	menuDataGlobal.endIndex = INT32_MIN;
	menuDataGlobal.currentMenuList = NULL;
	menuDataGlobal.currentItemIndex = menuFunctions[menuDataGlobal.controlData.stack[menuDataGlobal.controlData.stackPosition]].lastItemIndex;
	displayLightTrigger(false);
	status = menuFunctions[menuDataGlobal.controlData.stack[menuDataGlobal.controlData.stackPosition]].function(&ev, true);

	if (menuDataGlobal.endIndex == INT32_MIN)
	{
		menuDataGlobal.endIndex = ((menuDataGlobal.data[menuDataGlobal.controlData.stack[menuDataGlobal.controlData.stackPosition]] != NULL) ? menuDataGlobal.data[menuDataGlobal.controlData.stack[menuDataGlobal.controlData.stackPosition]]->numItems : 0);
	}

	if (menuDataGlobal.currentMenuList == NULL)
	{
		menuDataGlobal.currentMenuList = ((menuDataGlobal.data[menuDataGlobal.controlData.stackPosition] != NULL) ? (menuItemNewData_t *)menuDataGlobal.data[menuDataGlobal.controlData.stackPosition]->items : NULL);
	}

	if (menuDataGlobal.currentItemIndex > menuDataGlobal.endIndex)
	{
		menuDataGlobal.currentItemIndex = 0;
	}
	menuSystemCheckForFirstEntryAudible(status);
}

void menuSystemPushNewMenu(int menuNumber)
{
	if (menuDataGlobal.controlData.stackPosition < 15)
	{
		keyboardReset();
		menuFunctions[menuDataGlobal.controlData.stack[menuDataGlobal.controlData.stackPosition]].lastItemIndex = menuDataGlobal.currentItemIndex;
		menuDataGlobal.controlData.stackPosition++;
		menuDataGlobal.controlData.stack[menuDataGlobal.controlData.stackPosition] = menuNumber;
		menuSystemPushMenuFirstRun();
	}
}

void menuSystemPopPreviousMenu(void)
{
	keyboardReset();
	menuFunctions[menuDataGlobal.controlData.stack[menuDataGlobal.controlData.stackPosition]].lastItemIndex = menuDataGlobal.currentItemIndex;
	// Clear stackPosition + 1 menu trace
	if (menuDataGlobal.controlData.stackPosition < 15)
	{
		menuDataGlobal.controlData.stack[menuDataGlobal.controlData.stackPosition + 1] = MENU_EMPTY;
	}

	// Avoid crashing if something goes wrong.
	if (menuDataGlobal.controlData.stackPosition > 0)
	{
		menuDataGlobal.controlData.stackPosition -= 1;
	}
	menuSystemPushMenuFirstRun();
}

void menuSystemPopAllAndDisplayRootMenu(void)
{
	keyboardReset();
	menuFunctions[menuDataGlobal.controlData.stack[menuDataGlobal.controlData.stackPosition]].lastItemIndex = menuDataGlobal.currentItemIndex;
	// MENU_EMPTY is equal to -1 (0xFFFFFFFF), hence the following works, even if it's an int32_t array
	memset(&menuDataGlobal.controlData.stack[1], MENU_EMPTY, sizeof(menuDataGlobal.controlData.stack) - sizeof(int));
	menuDataGlobal.controlData.stackPosition = 0;
	menuSystemPushMenuFirstRun();
}

void menuSystemPopAllAndDisplaySpecificRootMenu(int newRootMenu, bool resetKeyboard)
{
	if (resetKeyboard)
	{
		keyboardReset();
	}
	menuFunctions[menuDataGlobal.controlData.stack[menuDataGlobal.controlData.stackPosition]].lastItemIndex = menuDataGlobal.currentItemIndex;
	// MENU_EMPTY is equal to -1 (0xFFFFFFFF), hence the following works, even if it's an int32_t array
	memset(&menuDataGlobal.controlData.stack[1], MENU_EMPTY, sizeof(menuDataGlobal.controlData.stack) - sizeof(int));
	menuDataGlobal.controlData.stack[0] = newRootMenu;
	menuDataGlobal.controlData.stackPosition = 0;
	menuSystemPushMenuFirstRun();
}

void menuSystemSetCurrentMenu(int menuNumber)
{
	keyboardReset();
	menuDataGlobal.controlData.stack[menuDataGlobal.controlData.stackPosition] = menuNumber;
	menuSystemPushMenuFirstRun();
}

int menuSystemGetCurrentMenuNumber(void)
{
	return menuDataGlobal.controlData.stack[menuDataGlobal.controlData.stackPosition];
}

int menuSystemGetPreviousMenuNumber(void)
{
	if (menuDataGlobal.controlData.stackPosition >= 1)
	{
		return menuDataGlobal.controlData.stack[menuDataGlobal.controlData.stackPosition - 1];
	}

	return -1;
}

int menuSystemGetPreviouslyPushedMenuNumber(void)
{
	if (menuDataGlobal.controlData.stackPosition < 15)
	{
		int m = menuDataGlobal.controlData.stack[menuDataGlobal.controlData.stackPosition + 1];
		menuDataGlobal.controlData.stack[menuDataGlobal.controlData.stackPosition + 1] = MENU_EMPTY;
		return m;
	}

	return MENU_EMPTY;
}

int menuSystemGetRootMenuNumber(void)
{
	return menuDataGlobal.controlData.stack[0];
}

static void menuSystemPreProcessEvent(uiEvent_t *ev)
{
	if (ev->hasEvent || ((uiDataGlobal.displayQSOState == QSO_DISPLAY_CALLER_DATA) && (nonVolatileSettings.backlightMode != BACKLIGHT_MODE_BUTTONS)) )
	{
		displayLightTrigger(true);
	}
}

static void menuSystemPostProcessEvent(uiEvent_t *ev)
{
	if (uiDataGlobal.displayQSOState == QSO_DISPLAY_CALLER_DATA)
	{
		uiDataGlobal.displayQSOState = QSO_DISPLAY_IDLE;
	}
}

void menuSystemCallCurrentMenuTick(uiEvent_t *ev)
{
	menuStatus_t status;

	menuSystemPreProcessEvent(ev);
	status = menuFunctions[menuDataGlobal.controlData.stack[menuDataGlobal.controlData.stackPosition]].function(ev, false);
	menuSystemPostProcessEvent(ev);
	if (ev->hasEvent)
	{
		menuSystemCheckForFirstEntryAudible(status);
	}
}

void displayLightTrigger(bool fromKeyEvent)
{
	if ((menuSystemGetCurrentMenuNumber() != UI_TX_SCREEN) &&
			(((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_AUTO) || (nonVolatileSettings.backlightMode == BACKLIGHT_MODE_SQUELCH))
					|| ((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_MANUAL) && displayIsBacklightLit())
					|| ((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_BUTTONS) && fromKeyEvent)))
	{
		if ((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_AUTO) ||
				(nonVolatileSettings.backlightMode == BACKLIGHT_MODE_SQUELCH) ||
				(nonVolatileSettings.backlightMode == BACKLIGHT_MODE_BUTTONS))
		{
			menuDataGlobal.lightTimer = nonVolatileSettings.backLightTimeout * 1000;
		}

		displayEnableBacklight(true);
	}
}

// use -1 to force LED on all the time
void displayLightOverrideTimeout(int timeout)
{
	int prevTimer = menuDataGlobal.lightTimer;

	menuDataGlobal.lightTimer = timeout;

	if ((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_AUTO) || (nonVolatileSettings.backlightMode == BACKLIGHT_MODE_SQUELCH))
	{
		// Backlight is OFF, or timeout override (-1) as just been set
		if ((displayIsBacklightLit() == false) || ((timeout == -1) && (prevTimer != -1)))
		{
			displayEnableBacklight(true);
		}
	}
}

void menuSystemInit(void)
{
	uiEvent_t ev = { .buttons = 0, .keys = NO_KEYCODE, .rotary = 0, .function = 0, .events = NO_EVENT, .hasEvent = false, .time = fw_millis() };

	menuDataGlobal.lightTimer = -1;
	menuDataGlobal.controlData.stack[menuDataGlobal.controlData.stackPosition] = UI_SPLASH_SCREEN;// set the very first screen as the splash screen
	menuDataGlobal.currentItemIndex = 0;
	displayLightTrigger(false);
	menuFunctions[menuDataGlobal.controlData.stack[menuDataGlobal.controlData.stackPosition]].function(&ev, true);// Init and display this screen
}

void menuSystemLanguageHasChanged(void)
{
	// Force full update of menuChannelMode() on next call (if isFirstRun arg. is true)
	if (menuSystemGetRootMenuNumber() == UI_CHANNEL_MODE)
	{
		uiChannelModeColdStart();
	}
}

const menuItemNewData_t mainMenuItems[] =
{
	{   3, MENU_ZONE_LIST       },
	{   4, MENU_RSSI_SCREEN     },
	{ 150, MENU_RADIO_INFOS     },
	{   6, MENU_CONTACTS_MENU   },
	{   7, MENU_LAST_HEARD      },
	{   8, MENU_FIRMWARE_INFO   },
	{   9, MENU_OPTIONS         },
	{  10, MENU_DISPLAY         },
	{  11, MENU_SOUND           },
	{  12, MENU_CHANNEL_DETAILS },
	{  13, MENU_LANGUAGE        },
	{   2, MENU_CREDITS         },
};

const menuItemsList_t menuDataMainMenu =
{
	.numItems = 12,
	.items = mainMenuItems
};

static const menuItemNewData_t contactMenuItems[] =
{
	{ 15,  MENU_CONTACT_LIST      },
	{ 139, MENU_DTMF_CONTACT_LIST },
	{ 14,  MENU_CONTACT_NEW       },
};

const menuItemsList_t menuDataContact =
{
	.numItems = 3,
	.items = contactMenuItems
};

void menuDisplayTitle(const char *title)
{
	ucDrawFastHLine(0, 13, DISPLAY_SIZE_X, true);
	ucPrintCore(0, 3, title, FONT_SIZE_2, TEXT_ALIGN_CENTER, false);
}

void menuDisplayEntry(int loopOffset, int focusedItem, const char *entryText)
{
	bool focused = (focusedItem == menuDataGlobal.currentItemIndex);

	if (focused)
	{
		ucFillRoundRect(0, DISPLAY_Y_POS_MENU_ENTRY_HIGHLIGHT + (loopOffset * MENU_ENTRY_HEIGHT), DISPLAY_SIZE_X, MENU_ENTRY_HEIGHT, 2, true);
	}

	ucPrintCore(0, DISPLAY_Y_POS_MENU_START + (loopOffset * MENU_ENTRY_HEIGHT), entryText, FONT_SIZE_3, TEXT_ALIGN_LEFT, focused);

}

int menuGetMenuOffset(int maxMenuEntries, int loopOffset)
{
	int offset = menuDataGlobal.currentItemIndex + loopOffset;

	if (offset < 0)
	{
		if ((maxMenuEntries == 1) && (maxMenuEntries < MENU_MAX_DISPLAYED_ENTRIES))
		{
			offset = MENU_MAX_DISPLAYED_ENTRIES - 1;
		}
		else
		{
			offset = maxMenuEntries + offset;
		}
	}

	if (maxMenuEntries < MENU_MAX_DISPLAYED_ENTRIES)
	{
		if (loopOffset == 1)
		{
			offset = MENU_MAX_DISPLAYED_ENTRIES - 1;
		}
	}
	else
	{
		if (offset >= maxMenuEntries)
		{
			offset = offset - maxMenuEntries;
		}
	}

	return offset;
}

/*
 * Returns 99 if key is unknown, or not numerical when digitsOnly is true
 */
int menuGetKeypadKeyValue(uiEvent_t *ev, bool digitsOnly)
{
	uint32_t keypadKeys[] =
	{
			KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
			KEY_LEFT, KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_STAR, KEY_HASH
	};

	for (int i = 0; i < ((sizeof(keypadKeys) / sizeof(keypadKeys[0])) - (digitsOnly ? 6 : 0 )); i++)
	{
		if (KEYCHECK_PRESS(ev->keys, keypadKeys[i]))
		{
				return i;
		}
	}

	return 99;
}

void menuUpdateCursor(int pos, bool moved, bool render)
{
#if defined(PLATFORM_RD5R)
	const int MENU_CURSOR_Y = 32;
#else
	const int MENU_CURSOR_Y = 46;
#endif

	static uint32_t lastBlink = 0;
	static bool     blink = false;
	uint32_t        m = fw_millis();

	if (moved)
	{
		blink = true;
	}

	if (moved || (m - lastBlink) > 500)
	{
		ucDrawFastHLine(pos * 8, MENU_CURSOR_Y, 8, blink);

		blink = !blink;
		lastBlink = m;

		if (render)
		{
			ucRenderRows(MENU_CURSOR_Y / 8, MENU_CURSOR_Y / 8 + 1);
		}
	}
}

void moveCursorLeftInString(char *str, int *pos, bool delete)
{
	int nLen = strlen(str);

	if (*pos > 0)
	{
		*pos -=1;
		announceChar(str[*pos]); // speak the new char or the char about to be backspaced out.

		if (delete)
		{
			for (int i = *pos; i <= nLen; i++)
			{
				str[i] = str[i + 1];
			}
		}
	}
}

void moveCursorRightInString(char *str, int *pos, int max, bool insert)
{
	int nLen = strlen(str);

	if (*pos < strlen(str))
	{
		if (insert)
		{
			if (nLen < max)
			{
				for (int i = nLen; i > *pos; i--)
				{
					str[i] = str[i - 1];
				}
				str[*pos] = ' ';
			}
		}

		if (*pos < max-1)
		{
			*pos += 1;
			announceChar(str[*pos]); // speak the new char or the char about to be backspaced out.
		}
	}
}

void menuSystemMenuIncrement(int32_t *currentItem, int32_t numItems)
{
	*currentItem = (*currentItem + 1) % numItems;
}

void menuSystemMenuDecrement(int32_t *currentItem, int32_t numItems)
{
	*currentItem = (*currentItem + numItems - 1) % numItems;
}

// For QuickKeys
void menuDisplaySettingOption(const char *entryText, const char *valueText)
{

#if defined(PLATFORM_RD5R)
	ucDrawRoundRect(2, DISPLAY_Y_POS_MENU_ENTRY_HIGHLIGHT - MENU_ENTRY_HEIGHT - 6, DISPLAY_SIZE_X - 4, (MENU_ENTRY_HEIGHT * 2) + 8, 2, true);
	ucFillRoundRect(2, DISPLAY_Y_POS_MENU_ENTRY_HIGHLIGHT - MENU_ENTRY_HEIGHT - 6, DISPLAY_SIZE_X - 4, MENU_ENTRY_HEIGHT + 3, 2, true);

	ucPrintCore(0, DISPLAY_Y_POS_MENU_START - MENU_ENTRY_HEIGHT - 4, entryText, FONT_SIZE_2, TEXT_ALIGN_CENTER, true);
#else
	ucDrawRoundRect(2, DISPLAY_Y_POS_MENU_ENTRY_HIGHLIGHT - MENU_ENTRY_HEIGHT - 2, DISPLAY_SIZE_X - 4, (MENU_ENTRY_HEIGHT * 2) + 4, 2, true);
	ucFillRoundRect(2, DISPLAY_Y_POS_MENU_ENTRY_HIGHLIGHT - MENU_ENTRY_HEIGHT - 2, DISPLAY_SIZE_X - 4, MENU_ENTRY_HEIGHT, 2, true);

	ucPrintCore(0, DISPLAY_Y_POS_MENU_START - MENU_ENTRY_HEIGHT + 2, entryText, FONT_SIZE_2, TEXT_ALIGN_CENTER, true);
#endif

	ucPrintCore(0, DISPLAY_Y_POS_MENU_START, valueText, FONT_SIZE_3, TEXT_ALIGN_CENTER, false);
}


