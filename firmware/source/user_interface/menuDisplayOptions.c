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
#include "hardware/UC1701.h"
#include "functions/settings.h"
#include "user_interface/menuSystem.h"
#include "user_interface/uiLocalisation.h"
#include "user_interface/uiUtilities.h"

static void updateScreen(bool isFirstRun);
static void handleEvent(uiEvent_t *ev);
static void updateBacklightMode(uint8_t mode);
static void setDisplayInvert(bool invert);
static void checkMinBacklightValue(void);

static menuStatus_t menuDisplayOptionsExitCode = MENU_STATUS_SUCCESS;

static const int BACKLIGHT_MAX_TIMEOUT = 30;
#if defined (PLATFORM_RD5R)
	static const int CONTRAST_MAX_VALUE = 10;// Maximum value which still seems to be readable
	static const int CONTRAST_MIN_VALUE = 0;// Minimum value which still seems to be readable
#else
	static const int CONTRAST_MAX_VALUE = 30;// Maximum value which still seems to be readable
	static const int CONTRAST_MIN_VALUE = 5;// Minimum value which still seems to be readable
#endif

static const int BACKLIGHT_TIMEOUT_STEP = 5;
static const int BACKLIGHT_MAX_PERCENTAGE = 100;
static const int BACKLIGHT_PERCENTAGE_STEP = 10;
static const int BACKLIGHT_PERCENTAGE_STEP_SMALL = 1;

static const char *contactOrders[] = { "Ct/DB/TA", "DB/Ct/TA", "TA/Ct/DB", "TA/DB/Ct" };

enum DISPLAY_MENU_LIST { DISPLAY_MENU_BRIGHTNESS = 0, DISPLAY_MENU_BRIGHTNESS_OFF, DISPLAY_MENU_CONTRAST, DISPLAY_MENU_BACKLIGHT_MODE,
	DISPLAY_MENU_TIMEOUT, DISPLAY_MENU_COLOUR_INVERT, DISPLAY_MENU_CONTACT_DISPLAY_ORDER, DISPLAY_MENU_CONTACT_DISPLAY_SPLIT_CONTACT,
	DISPLAY_BATTERY_UNIT_IN_HEADER, DISPLAY_EXTENDED_INFOS, DISPLAY_ALL_LEDS_ENABLED,
	NUM_DISPLAY_MENU_ITEMS };

menuStatus_t menuDisplayOptions(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		menuDataGlobal.menuOptionsSetQuickkey = 0;
		menuDataGlobal.menuOptionsTimeout = 0;
		menuDataGlobal.newOptionSelected = true;
		menuDataGlobal.endIndex = NUM_DISPLAY_MENU_ITEMS;

		if (originalNonVolatileSettings.magicNumber == 0xDEADBEEF)
		{
			// Store original settings, used on cancel event.
			memcpy(&originalNonVolatileSettings, &nonVolatileSettings, sizeof(settingsStruct_t));
		}

		voicePromptsInit();
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendLanguageString(&currentLanguage->display_options);
		voicePromptsAppendLanguageString(&currentLanguage->menu);
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendPrompt(PROMPT_SILENCE);

		updateScreen(true);
		return (MENU_STATUS_LIST_TYPE | MENU_STATUS_SUCCESS);
	}
	else
	{
		menuDisplayOptionsExitCode = MENU_STATUS_SUCCESS;

		if (ev->hasEvent || (menuDataGlobal.menuOptionsTimeout > 0))
		{
			handleEvent(ev);
		}
	}
	return menuDisplayOptionsExitCode;
}


static void updateScreen(bool isFirstRun)
{
	int mNum = 0;
	static const int bufferLen = 17;
	char buf[bufferLen];
	char * const *leftSide = NULL;// initialize to please the compiler
	char * const *rightSideConst = NULL;// initialize to please the compiler
	char rightSideVar[bufferLen];
	voicePrompt_t rightSideUnitsPrompt;
	const char * rightSideUnitsStr;

	ucClearBuf();
	bool settingOption = uiShowQuickKeysChoices(buf, bufferLen, currentLanguage->display_options);

	// Can only display 3 of the options at a time menu at -1, 0 and +1
	for(int i = -1; i <= 1; i++)
	{
		if ((settingOption == false) || (i == 0))
		{
			mNum = menuGetMenuOffset(NUM_DISPLAY_MENU_ITEMS, i);
			buf[0] = 0;
			leftSide = NULL;
			rightSideConst = NULL;
			rightSideVar[0] = 0;
			rightSideUnitsPrompt = PROMPT_SILENCE;// use PROMPT_SILENCE as flag that the unit has not been set
			rightSideUnitsStr = NULL;

			switch(mNum)
			{
				case DISPLAY_MENU_BRIGHTNESS:
					leftSide = (char * const *)&currentLanguage->brightness;
					snprintf(rightSideVar, bufferLen, "%d%%", nonVolatileSettings.displayBacklightPercentage);
					break;
				case DISPLAY_MENU_BRIGHTNESS_OFF:
					leftSide = (char * const *)&currentLanguage->brightness_off;
					snprintf(rightSideVar, bufferLen, "%d%%", nonVolatileSettings.displayBacklightPercentageOff);
					break;
				case DISPLAY_MENU_CONTRAST:
					leftSide = (char * const *)&currentLanguage->contrast;
					snprintf(rightSideVar, bufferLen, "%d", nonVolatileSettings.displayContrast);
					break;
				case DISPLAY_MENU_BACKLIGHT_MODE:
					{
						const char * const *backlightModes[] = { &currentLanguage->Auto, &currentLanguage->squelch, &currentLanguage->manual, &currentLanguage->buttons, &currentLanguage->none };
						leftSide = (char * const *)&currentLanguage->mode;
						rightSideConst = (char * const *)backlightModes[nonVolatileSettings.backlightMode];
					}
					break;
				case DISPLAY_MENU_TIMEOUT:
					leftSide = (char * const *)&currentLanguage->backlight_timeout;
					if ((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_AUTO) ||
							(nonVolatileSettings.backlightMode == BACKLIGHT_MODE_SQUELCH) ||
							(nonVolatileSettings.backlightMode == BACKLIGHT_MODE_BUTTONS))
					{
						if (nonVolatileSettings.backLightTimeout == 0)
						{
							rightSideConst = (char * const *)&currentLanguage->no;
						}
						else
						{
							snprintf(rightSideVar, bufferLen, "%d", nonVolatileSettings.backLightTimeout);
							rightSideUnitsPrompt = PROMPT_SECONDS;
							rightSideUnitsStr = "s";
						}
					}
					else
					{
						rightSideConst = (char * const *)&currentLanguage->n_a;
					}
					break;
				case DISPLAY_MENU_COLOUR_INVERT:
					leftSide = (char * const *)&currentLanguage->display_background_colour;
					rightSideConst = settingsIsOptionBitSet(BIT_INVERSE_VIDEO) ? (char * const *)&currentLanguage->colour_invert : (char * const *)&currentLanguage->colour_normal;
					break;
				case DISPLAY_MENU_CONTACT_DISPLAY_ORDER:
					leftSide = (char * const *)&currentLanguage->priority_order;
					snprintf(rightSideVar, bufferLen, "%s",contactOrders[nonVolatileSettings.contactDisplayPriority]);
					break;
				case DISPLAY_MENU_CONTACT_DISPLAY_SPLIT_CONTACT:
					{
						const char * const *splitContact[] = { &currentLanguage->one_line, &currentLanguage->two_lines, &currentLanguage->Auto };
						leftSide = (char * const *)&currentLanguage->contact;
						rightSideConst = (char * const *)splitContact[nonVolatileSettings.splitContact];
					}
					break;
				case DISPLAY_BATTERY_UNIT_IN_HEADER:
					leftSide = (char * const *)&currentLanguage->battery;
					if (settingsIsOptionBitSet(BIT_BATTERY_VOLTAGE_IN_HEADER))
					{
						rightSideUnitsPrompt = PROMPT_VOLTS;
						rightSideUnitsStr = "V";
					}
					else
					{
						rightSideUnitsPrompt = PROMPT_PERCENT;
						rightSideUnitsStr = "%";
					}
					break;
				case DISPLAY_EXTENDED_INFOS:
					{
						const char * const *extendedInfos[] = { &currentLanguage->off, &currentLanguage->ts, &currentLanguage->pwr, &currentLanguage->both };
						leftSide = (char * const *)&currentLanguage->info;
						rightSideConst = (char * const *)extendedInfos[nonVolatileSettings.extendedInfosOnScreen];
					}
					break;
				case DISPLAY_ALL_LEDS_ENABLED:
					leftSide = (char * const *)&currentLanguage->leds;
					rightSideConst = settingsIsOptionBitSet(BIT_ALL_LEDS_DISABLED) ? (char * const *)&currentLanguage->off : (char * const *)&currentLanguage->on;
					break;
			}

			// workaround for non standard format of line for colour display
			snprintf(buf, bufferLen, "%s:%s", *leftSide, (rightSideVar[0] ? rightSideVar : (rightSideConst ? *rightSideConst : "")));

			if (i == 0)
			{
				bool wasPlaying = voicePromptsIsPlaying();

				if (!isFirstRun && (menuDataGlobal.menuOptionsSetQuickkey == 0))
				{
					voicePromptsInit();
				}

				if (!wasPlaying || menuDataGlobal.newOptionSelected)
				{
					voicePromptsAppendLanguageString((const char * const *)leftSide);
				}

				if ((rightSideVar[0] != 0) || ((rightSideVar[0] == 0) && (rightSideConst == NULL)))
				{
					voicePromptsAppendString(rightSideVar);
				}
				else
				{
					voicePromptsAppendLanguageString((const char * const *)rightSideConst);
				}

				if (rightSideUnitsPrompt != PROMPT_SILENCE)
				{
					voicePromptsAppendPrompt(rightSideUnitsPrompt);
				}

				if (rightSideUnitsStr != NULL)
				{
					strncat(rightSideVar, rightSideUnitsStr, bufferLen);
				}

				if (menuDataGlobal.menuOptionsTimeout != -1)
				{
					promptsPlayNotAfterTx();
				}
				else
				{
					menuDataGlobal.menuOptionsTimeout = 0;// clear flag indicating that a QuickKey has just been set
				}
			}

			// QuickKeys
			if (menuDataGlobal.menuOptionsTimeout > 0)
			{
				menuDisplaySettingOption(*leftSide, (rightSideVar[0] ? rightSideVar : *rightSideConst));
			}
			else
			{
				if (rightSideUnitsStr != NULL)
				{
					strncat(buf, rightSideUnitsStr, bufferLen);
				}

				menuDisplayEntry(i, mNum, buf);
			}
		}
	}

	ucRender();
}

static void handleEvent(uiEvent_t *ev)
{
	bool isDirty = false;

	if ((menuDataGlobal.menuOptionsTimeout > 0) && (!BUTTONCHECK_DOWN(ev, BUTTON_SK2)))
	{
		menuDataGlobal.menuOptionsTimeout--;
		if (menuDataGlobal.menuOptionsTimeout == 0)
		{
			resetOriginalSettingsData();
			menuSystemPopPreviousMenu();
			return;
		}
	}

	if (ev->events & FUNCTION_EVENT)
	{
		isDirty = true;
		if ((QUICKKEY_TYPE(ev->function) == QUICKKEY_MENU) && (QUICKKEY_ENTRYID(ev->function) < NUM_DISPLAY_MENU_ITEMS))
		{
			menuDataGlobal.currentItemIndex = QUICKKEY_ENTRYID(ev->function);
		}
		if ((QUICKKEY_FUNCTIONID(ev->function) != 0))
		{
			menuDataGlobal.menuOptionsTimeout = 1000;
		}
	}

	if (ev->events & BUTTON_EVENT)
	{
		if (repeatVoicePromptOnSK1(ev))
		{
			return;
		}
	}

	if ((ev->events & KEY_EVENT) && (menuDataGlobal.menuOptionsSetQuickkey == 0) && (menuDataGlobal.menuOptionsTimeout == 0))
	{
		if (KEYCHECK_PRESS(ev->keys, KEY_DOWN) && (menuDataGlobal.endIndex != 0))
		{
			isDirty = true;
			menuSystemMenuIncrement(&menuDataGlobal.currentItemIndex, NUM_DISPLAY_MENU_ITEMS);
			menuDataGlobal.newOptionSelected = true;
			menuDisplayOptionsExitCode |= MENU_STATUS_LIST_TYPE;
		}
		else if (KEYCHECK_PRESS(ev->keys, KEY_UP))
		{
			isDirty = true;
			menuSystemMenuDecrement(&menuDataGlobal.currentItemIndex, NUM_DISPLAY_MENU_ITEMS);
			menuDataGlobal.newOptionSelected = true;
			menuDisplayOptionsExitCode |= MENU_STATUS_LIST_TYPE;
		}
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
		{
			// All parameters has already been applied
			settingsSaveIfNeeded(true);
			resetOriginalSettingsData();
			menuSystemPopAllAndDisplayRootMenu();
			return;
		}
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
		{
			bool displayIsLit = displayIsBacklightLit();

			if (nonVolatileSettings.displayContrast != originalNonVolatileSettings.displayContrast)
			{
				settingsSet(nonVolatileSettings.displayContrast, originalNonVolatileSettings.displayContrast);
				ucSetContrast(nonVolatileSettings.displayContrast);
			}

			if ((nonVolatileSettings.bitfieldOptions & BIT_INVERSE_VIDEO) != (originalNonVolatileSettings.bitfieldOptions & BIT_INVERSE_VIDEO))
			{
				settingsSetOptionBit(BIT_INVERSE_VIDEO, ((originalNonVolatileSettings.bitfieldOptions & BIT_INVERSE_VIDEO) != 0));
				displayInit(settingsIsOptionBitSet(BIT_INVERSE_VIDEO));// Need to perform a full reset on the display to change back to non-inverted
			}

			settingsSet(nonVolatileSettings.displayBacklightPercentage, originalNonVolatileSettings.displayBacklightPercentage);
			settingsSet(nonVolatileSettings.displayBacklightPercentageOff, originalNonVolatileSettings.displayBacklightPercentageOff);
			settingsSet(nonVolatileSettings.backLightTimeout, originalNonVolatileSettings.backLightTimeout);

			if (nonVolatileSettings.backlightMode != originalNonVolatileSettings.backlightMode)
			{
				updateBacklightMode(originalNonVolatileSettings.backlightMode);
			}

			if ((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_MANUAL) && (!displayIsLit))
			{
				gpioSetDisplayBacklightIntensityPercentage(nonVolatileSettings.displayBacklightPercentageOff);
			}

			settingsSet(nonVolatileSettings.contactDisplayPriority, originalNonVolatileSettings.contactDisplayPriority);
			settingsSet(nonVolatileSettings.splitContact, originalNonVolatileSettings.splitContact);

			if ((nonVolatileSettings.bitfieldOptions & BIT_BATTERY_VOLTAGE_IN_HEADER) != (originalNonVolatileSettings.bitfieldOptions & BIT_BATTERY_VOLTAGE_IN_HEADER))
			{
				settingsSetOptionBit(BIT_BATTERY_VOLTAGE_IN_HEADER, ((originalNonVolatileSettings.bitfieldOptions & BIT_BATTERY_VOLTAGE_IN_HEADER) != 0));
			}

			settingsSet(nonVolatileSettings.extendedInfosOnScreen, originalNonVolatileSettings.extendedInfosOnScreen);

			if ((nonVolatileSettings.bitfieldOptions & BIT_ALL_LEDS_DISABLED) != (originalNonVolatileSettings.bitfieldOptions & BIT_ALL_LEDS_DISABLED))
			{
				int state = LEDs_PinRead(GPIO_LEDgreen, Pin_LEDgreen);

				settingsSetOptionBit(BIT_ALL_LEDS_DISABLED, ((originalNonVolatileSettings.bitfieldOptions & BIT_ALL_LEDS_DISABLED) != 0));
				GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, ((nonVolatileSettings.bitfieldOptions & BIT_ALL_LEDS_DISABLED) ? 0 : state));
			}


			settingsSaveIfNeeded(true);
			resetOriginalSettingsData();
			menuSystemPopPreviousMenu();
			return;
		}
		else if (KEYCHECK_SHORTUP_NUMBER(ev->keys) && BUTTONCHECK_DOWN(ev, BUTTON_SK2))
		{
			menuDataGlobal.menuOptionsSetQuickkey = ev->keys.key;
			isDirty = true;
		}
	}


	if ((ev->events & (KEY_EVENT | FUNCTION_EVENT)) && (menuDataGlobal.menuOptionsSetQuickkey == 0))
	{
		bool displayIsLit = displayIsBacklightLit();
		if (KEYCHECK_PRESS(ev->keys, KEY_RIGHT) || (QUICKKEY_FUNCTIONID(ev->function) == FUNC_RIGHT))
		{
			if (menuDataGlobal.menuOptionsTimeout > 0)
			{
				menuDataGlobal.menuOptionsTimeout = 1000;
			}
			isDirty = true;
			menuDataGlobal.newOptionSelected = false;
			switch(menuDataGlobal.currentItemIndex)
			{
				case DISPLAY_MENU_BRIGHTNESS:
					settingsIncrement(nonVolatileSettings.displayBacklightPercentage,
							(int8_t) ((nonVolatileSettings.displayBacklightPercentage < BACKLIGHT_PERCENTAGE_STEP) ? BACKLIGHT_PERCENTAGE_STEP_SMALL : BACKLIGHT_PERCENTAGE_STEP));

					if (nonVolatileSettings.displayBacklightPercentage > BACKLIGHT_MAX_PERCENTAGE)
					{
						settingsSet(nonVolatileSettings.displayBacklightPercentage, (int8_t) BACKLIGHT_MAX_PERCENTAGE);
					}
					break;
				case DISPLAY_MENU_BRIGHTNESS_OFF:
					if (nonVolatileSettings.displayBacklightPercentageOff < nonVolatileSettings.displayBacklightPercentage)
					{
						settingsIncrement(nonVolatileSettings.displayBacklightPercentageOff,
								(int8_t) ((nonVolatileSettings.displayBacklightPercentageOff < BACKLIGHT_PERCENTAGE_STEP) ? BACKLIGHT_PERCENTAGE_STEP_SMALL : BACKLIGHT_PERCENTAGE_STEP));

						if (nonVolatileSettings.displayBacklightPercentageOff > BACKLIGHT_MAX_PERCENTAGE)
						{
							settingsSet(nonVolatileSettings.displayBacklightPercentageOff, (int8_t) BACKLIGHT_MAX_PERCENTAGE);
						}

						checkMinBacklightValue();

						if ((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_MANUAL) && (!displayIsLit))
						{
							gpioSetDisplayBacklightIntensityPercentage(nonVolatileSettings.displayBacklightPercentageOff);
						}
					}
					break;
				case DISPLAY_MENU_CONTRAST:
					if (nonVolatileSettings.displayContrast < CONTRAST_MAX_VALUE)
					{
						settingsIncrement(nonVolatileSettings.displayContrast, 1);
					}
					ucSetContrast(nonVolatileSettings.displayContrast);
					break;
				case DISPLAY_MENU_BACKLIGHT_MODE:
					if (nonVolatileSettings.backlightMode < BACKLIGHT_MODE_NONE)
					{
						settingsIncrement(nonVolatileSettings.backlightMode, 1);
						updateBacklightMode(nonVolatileSettings.backlightMode);
					}
					break;
				case DISPLAY_MENU_TIMEOUT:
					if ((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_AUTO) ||
							(nonVolatileSettings.backlightMode == BACKLIGHT_MODE_SQUELCH) ||
							(nonVolatileSettings.backlightMode == BACKLIGHT_MODE_BUTTONS))
					{
						settingsIncrement(nonVolatileSettings.backLightTimeout, (uint8_t) BACKLIGHT_TIMEOUT_STEP);
						if (nonVolatileSettings.backLightTimeout > BACKLIGHT_MAX_TIMEOUT)
						{
							settingsSet(nonVolatileSettings.backLightTimeout, (uint8_t) BACKLIGHT_MAX_TIMEOUT);
						}
					}
					break;
				case DISPLAY_MENU_COLOUR_INVERT:
					setDisplayInvert(true);
					break;
				case DISPLAY_MENU_CONTACT_DISPLAY_ORDER:
					if (nonVolatileSettings.contactDisplayPriority < CONTACT_DISPLAY_PRIO_TA_DB_CC)
					{
						settingsIncrement(nonVolatileSettings.contactDisplayPriority, 1);
					}
					break;
				case DISPLAY_MENU_CONTACT_DISPLAY_SPLIT_CONTACT:
					if (nonVolatileSettings.splitContact < SPLIT_CONTACT_AUTO)
					{
						settingsIncrement(nonVolatileSettings.splitContact, 1);
					}
					break;
				case DISPLAY_BATTERY_UNIT_IN_HEADER:
					if (settingsIsOptionBitSet(BIT_BATTERY_VOLTAGE_IN_HEADER) == false)
					{
						settingsSetOptionBit(BIT_BATTERY_VOLTAGE_IN_HEADER, true);
					}
					break;
				case DISPLAY_EXTENDED_INFOS:
					if (nonVolatileSettings.extendedInfosOnScreen < INFO_ON_SCREEN_BOTH)
					{
						settingsIncrement(nonVolatileSettings.extendedInfosOnScreen, 1);
					}
					break;
				case DISPLAY_ALL_LEDS_ENABLED:
					if (settingsIsOptionBitSet(BIT_ALL_LEDS_DISABLED))
					{
						int state = LEDs_PinRead(GPIO_LEDgreen, Pin_LEDgreen);
						settingsSetOptionBit(BIT_ALL_LEDS_DISABLED, false);
						GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, state);
					}
					break;
			}
		}
		else if (KEYCHECK_PRESS(ev->keys, KEY_LEFT) || (QUICKKEY_FUNCTIONID(ev->function) == FUNC_LEFT))
		{
			if (menuDataGlobal.menuOptionsTimeout>0)
			{
				menuDataGlobal.menuOptionsTimeout = 1000;
			}
			isDirty = true;
			menuDataGlobal.newOptionSelected = false;
			switch(menuDataGlobal.currentItemIndex)
			{
				case DISPLAY_MENU_BRIGHTNESS:
					settingsDecrement(nonVolatileSettings.displayBacklightPercentage,
							(int8_t) ((nonVolatileSettings.displayBacklightPercentage <= BACKLIGHT_PERCENTAGE_STEP) ? 1 : BACKLIGHT_PERCENTAGE_STEP));

					if (nonVolatileSettings.displayBacklightPercentage < 0)
					{
						settingsSet(nonVolatileSettings.displayBacklightPercentage, 0);
					}

					checkMinBacklightValue();
					break;
				case DISPLAY_MENU_BRIGHTNESS_OFF:
					settingsDecrement(nonVolatileSettings.displayBacklightPercentageOff,
							(int8_t) ((nonVolatileSettings.displayBacklightPercentageOff <= BACKLIGHT_PERCENTAGE_STEP) ? BACKLIGHT_PERCENTAGE_STEP_SMALL : BACKLIGHT_PERCENTAGE_STEP));

					if (nonVolatileSettings.displayBacklightPercentageOff < 0)
					{
						settingsSet(nonVolatileSettings.displayBacklightPercentageOff, 0);
					}

					if ((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_MANUAL) && (!displayIsLit))
					{
						gpioSetDisplayBacklightIntensityPercentage(nonVolatileSettings.displayBacklightPercentageOff);
					}
					break;
				case DISPLAY_MENU_CONTRAST:
					if (nonVolatileSettings.displayContrast > CONTRAST_MIN_VALUE)
					{
						settingsDecrement(nonVolatileSettings.displayContrast, 1);
					}
					ucSetContrast(nonVolatileSettings.displayContrast);
					break;
				case DISPLAY_MENU_BACKLIGHT_MODE:
					if (nonVolatileSettings.backlightMode > BACKLIGHT_MODE_AUTO)
					{
						settingsDecrement(nonVolatileSettings.backlightMode, 1);
						updateBacklightMode(nonVolatileSettings.backlightMode);
					}
					break;
				case DISPLAY_MENU_TIMEOUT:
					if (((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_AUTO)
							&& (nonVolatileSettings.backLightTimeout >= BACKLIGHT_TIMEOUT_STEP)) ||
							(((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_SQUELCH) || (nonVolatileSettings.backlightMode == BACKLIGHT_MODE_BUTTONS))
									&& (nonVolatileSettings.backLightTimeout >= (BACKLIGHT_TIMEOUT_STEP * 2))))
					{
						settingsDecrement(nonVolatileSettings.backLightTimeout, (uint8_t) BACKLIGHT_TIMEOUT_STEP);
					}
					break;
				case DISPLAY_MENU_COLOUR_INVERT:
					setDisplayInvert(false);
					break;
				case DISPLAY_MENU_CONTACT_DISPLAY_ORDER:
					if (nonVolatileSettings.contactDisplayPriority > CONTACT_DISPLAY_PRIO_CC_DB_TA)
					{
						settingsDecrement(nonVolatileSettings.contactDisplayPriority, 1);
					}
					break;
				case DISPLAY_MENU_CONTACT_DISPLAY_SPLIT_CONTACT:
					if (nonVolatileSettings.splitContact > SPLIT_CONTACT_SINGLE_LINE_ONLY)
					{
						settingsDecrement(nonVolatileSettings.splitContact, 1);
					}
					break;
				case DISPLAY_BATTERY_UNIT_IN_HEADER:
					if (settingsIsOptionBitSet(BIT_BATTERY_VOLTAGE_IN_HEADER))
					{
						settingsSetOptionBit(BIT_BATTERY_VOLTAGE_IN_HEADER, false);
					}
					break;
				case DISPLAY_EXTENDED_INFOS:
					if (nonVolatileSettings.extendedInfosOnScreen > INFO_ON_SCREEN_OFF)
					{
						settingsDecrement(nonVolatileSettings.extendedInfosOnScreen, 1);
					}
					break;
				case DISPLAY_ALL_LEDS_ENABLED:
					if (settingsIsOptionBitSet(BIT_ALL_LEDS_DISABLED) == false)
					{
						GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
						settingsSetOptionBit(BIT_ALL_LEDS_DISABLED, true);
					}
					break;
			}
		}
		else if ((ev->keys.event & KEY_MOD_PRESS) && (menuDataGlobal.menuOptionsTimeout > 0))
		{
			menuDataGlobal.menuOptionsTimeout = 0;
			resetOriginalSettingsData();
			menuSystemPopPreviousMenu();
			return;
		}
	}

	if ((ev->events & KEY_EVENT) && (menuDataGlobal.menuOptionsSetQuickkey != 0) && (menuDataGlobal.menuOptionsTimeout == 0))
	{
		if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
		{
			menuDataGlobal.menuOptionsSetQuickkey = 0;
			menuDataGlobal.menuOptionsTimeout = 0;
			menuDisplayOptionsExitCode |= MENU_STATUS_ERROR;
		}
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
		{
			saveQuickkeyMenuIndex(menuDataGlobal.menuOptionsSetQuickkey, menuSystemGetCurrentMenuNumber(), menuDataGlobal.currentItemIndex, 0);
			menuDataGlobal.menuOptionsSetQuickkey = 0;
		}
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_LEFT))
		{
			saveQuickkeyMenuIndex(menuDataGlobal.menuOptionsSetQuickkey, menuSystemGetCurrentMenuNumber(), menuDataGlobal.currentItemIndex, FUNC_LEFT);
			menuDataGlobal.menuOptionsSetQuickkey = 0;
		}
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_RIGHT))
		{
			saveQuickkeyMenuIndex(menuDataGlobal.menuOptionsSetQuickkey, menuSystemGetCurrentMenuNumber(), menuDataGlobal.currentItemIndex, FUNC_RIGHT);
			menuDataGlobal.menuOptionsSetQuickkey = 0;
		}
		isDirty = true;
	}


	if (isDirty)
	{
		updateScreen(false);
	}
}

static void updateBacklightMode(uint8_t mode)
{
	settingsSet(nonVolatileSettings.backlightMode, mode);

	switch (mode)
	{
		case BACKLIGHT_MODE_MANUAL:
		case BACKLIGHT_MODE_NONE:
			displayEnableBacklight(false); // Could be MANUAL previously, but in OFF state, so turn it OFF blindly.
			break;
		case BACKLIGHT_MODE_SQUELCH:
		case BACKLIGHT_MODE_BUTTONS:
			if (nonVolatileSettings.backLightTimeout < BACKLIGHT_TIMEOUT_STEP)
			{
				settingsSet(nonVolatileSettings.backLightTimeout, (uint8_t) BACKLIGHT_TIMEOUT_STEP);
			}
		case BACKLIGHT_MODE_AUTO:
			displayLightTrigger(true);
			break;
	}
}

static void setDisplayInvert(bool invert)
{
	if (invert == settingsIsOptionBitSet(BIT_INVERSE_VIDEO))
	{
		return;// Don't update unless the setting is actually changing
	}

	settingsSetOptionBit(BIT_INVERSE_VIDEO, invert);
	displayInit(settingsIsOptionBitSet(BIT_INVERSE_VIDEO));// Need to perform a full reset on the display to change back to non-inverted
}

static void checkMinBacklightValue(void)
{
	if (nonVolatileSettings.displayBacklightPercentageOff >= nonVolatileSettings.displayBacklightPercentage)
	{
		settingsSet(nonVolatileSettings.displayBacklightPercentageOff,
				(int8_t) (nonVolatileSettings.displayBacklightPercentage ?
						(nonVolatileSettings.displayBacklightPercentage - ((nonVolatileSettings.displayBacklightPercentageOff <= BACKLIGHT_PERCENTAGE_STEP) ? BACKLIGHT_PERCENTAGE_STEP_SMALL : BACKLIGHT_PERCENTAGE_STEP)) : 0));
	}
}
