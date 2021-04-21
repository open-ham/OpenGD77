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
#include "functions/ticks.h"
#include "user_interface/menuSystem.h"
#include "user_interface/uiUtilities.h"
#include "user_interface/uiLocalisation.h"

static const int DISPLAYED_LINES_MAX = 3;

static bool displayLHDetails = false;
static menuStatus_t menuLastHeardExitCode;
static LinkItem_t *selectedItem;
static int lastHeardCount;
static int firstDisplayed;

static void displayTalkerAlias(uint8_t y, char *text, uint32_t time, uint32_t now, uint32_t TGorPC, size_t maxLen, bool displayDetails, bool itemIsSelected, bool isFirstRun);
static void promptsInit(bool isFirstRun);

menuStatus_t menuLastHeard(uiEvent_t *ev, bool isFirstRun)
{
	static uint32_t m = 0;

	if (isFirstRun)
	{
		menuLastHeardInit();
		menuLastHeardUpdateScreen(true, displayLHDetails, true);
		m = ev->time;

		return (menuLastHeardExitCode | (uiDataGlobal.lastHeardCount ? MENU_STATUS_LIST_TYPE : 0));
	}
	else
	{
		bool headHasChanged = (menuDataGlobal.startIndex != LinkHead->id);

		menuLastHeardExitCode = MENU_STATUS_SUCCESS;

		// do live update by checking if the item at the top of the list has changed
		if (headHasChanged || (uiDataGlobal.displayQSOState == QSO_DISPLAY_CALLER_DATA))
		{
			bool backlightOn = false;

			// the LH list has grown
			if (lastHeardCount != uiDataGlobal.lastHeardCount)
			{
				backlightOn = true;
				lastHeardCount = uiDataGlobal.lastHeardCount;
			}
			else
			{
				// Do not turn the backlight ON on caller data updates
				if (headHasChanged)
				{
					backlightOn = true;
				}
			}

			menuDataGlobal.startIndex = LinkHead->id;

			if (backlightOn)
			{
				displayLightTrigger(false);
			}

			menuLastHeardUpdateScreen(true, displayLHDetails, false);
		}

		if (ev->hasEvent)
		{
			m = ev->time;
			menuLastHeardHandleEvent(ev);
		}
		else
		{
			// Just refresh the list while displaying details, it's all about elapsed time
			if (displayLHDetails && ((ev->time - m) > (1000U * 60U)))
			{
				m = ev->time;
				menuLastHeardUpdateScreen(true, displayLHDetails, false);
			}
		}

	}

	return menuLastHeardExitCode;
}

void menuLastHeardUpdateScreen(bool showTitleOrHeader, bool displayDetails, bool isFirstRun)
{
	int numDisplayed = 0;
	LinkItem_t *item = LinkHead;
	uint32_t now = fw_millis();
	bool invertColour;
	bool displayTA;

	// Jumping here from <SK2> + 3, with an empty heard list won't announce "Last Heard", handle this here
	if (isFirstRun && (uiDataGlobal.lastHeardCount == 0) && (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1))
	{
		promptsInit(isFirstRun);
	}

	ucClearBuf();
	if (showTitleOrHeader)
	{
		menuDisplayTitle(currentLanguage->last_heard);
	}
	else
	{
		uiUtilityRenderHeader(false);
	}

	// skip over the first menuDataGlobal.currentItemIndex in the listing
	for(int i = 0; i < firstDisplayed; i++)
	{
		item = item->next;
	}

	if (uiDataGlobal.lastHeardCount > 0)
	{
		while((item != NULL) && (item->id != 0) && (numDisplayed < DISPLAYED_LINES_MAX))
		{
			displayTA = false;

			if (menuDataGlobal.currentItemIndex == (firstDisplayed + numDisplayed))
			{
				invertColour = true;
				ucFillRect(0, 16 + (numDisplayed * MENU_ENTRY_HEIGHT), DISPLAY_SIZE_X, MENU_ENTRY_HEIGHT, false);
				selectedItem = item;
			}
			else
			{
				invertColour = false;
			}

			switch (nonVolatileSettings.contactDisplayPriority)
			{
				case CONTACT_DISPLAY_PRIO_CC_DB_TA:
				case CONTACT_DISPLAY_PRIO_DB_CC_TA:
					// No contact found in codeplug and DMRIDs, use TA as fallback, if any.
					if ((strncmp(item->contact, "ID:", 3) == 0) && (item->talkerAlias[0] != 0x00))
					{
						displayTA = true;
					}
					break;
				case CONTACT_DISPLAY_PRIO_TA_CC_DB:
				case CONTACT_DISPLAY_PRIO_TA_DB_CC:
					if (item->talkerAlias[0] != 0x00)
					{
						displayTA = true;
					}
					break;
			}

			if (displayTA)
			{
				displayTalkerAlias(16 + (numDisplayed * MENU_ENTRY_HEIGHT) + LH_ENTRY_V_OFFSET, item->talkerAlias, item->time, now, item->talkGroupOrPcId, 32, displayDetails, invertColour, isFirstRun);
			}
			else
			{
				displayTalkerAlias(16 + (numDisplayed * MENU_ENTRY_HEIGHT) + LH_ENTRY_V_OFFSET, item->contact, item->time, now, item->talkGroupOrPcId, 21, displayDetails, invertColour, isFirstRun);
			}

			numDisplayed++;

			item = item->next;
		}
	}
	else
	{
		promptsPlayNotAfterTx();
	}

	ucRender();
	uiDataGlobal.displayQSOState = QSO_DISPLAY_IDLE;
}

void menuLastHeardHandleEvent(uiEvent_t *ev)
{
	bool isDirty = false;
	int currentMenu = menuSystemGetCurrentMenuNumber();

	if (currentMenu == MENU_LAST_HEARD)
	{
		if (ev->events & BUTTON_EVENT)
		{
			if (repeatVoicePromptOnSK1(ev))
			{
				return;
			}
		}
	}

	if (uiDataGlobal.lastHeardCount > 0)
	{
		if (KEYCHECK_SHORTUP(ev->keys, KEY_DOWN) && (BUTTONCHECK_DOWN(ev, BUTTON_SK2) == 0))
		{
			if (menuDataGlobal.currentItemIndex < (uiDataGlobal.lastHeardCount - 1))
			{
				isDirty = true;
				menuDataGlobal.currentItemIndex++;

				if (menuDataGlobal.currentItemIndex >= DISPLAYED_LINES_MAX)
				{
					if (((menuDataGlobal.currentItemIndex - DISPLAYED_LINES_MAX) == firstDisplayed) &&
							(menuDataGlobal.currentItemIndex <= (uiDataGlobal.lastHeardCount - 1)))
					{
						firstDisplayed++;
					}
				}
			}

			if (menuDataGlobal.currentItemIndex == 0)
			{
				menuLastHeardExitCode |= MENU_STATUS_LIST_TYPE;
			}
		}
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_DOWN) && BUTTONCHECK_DOWN(ev, BUTTON_SK2))
		{
			isDirty = true;
			menuDataGlobal.currentItemIndex = (uiDataGlobal.lastHeardCount - 1);
			firstDisplayed = SAFE_MAX((menuDataGlobal.currentItemIndex - (DISPLAYED_LINES_MAX - 1)), 0);

			if (menuDataGlobal.currentItemIndex == 0)
			{
				menuLastHeardExitCode |= MENU_STATUS_LIST_TYPE;
			}
		}
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_UP) && (BUTTONCHECK_DOWN(ev, BUTTON_SK2) == 0))
		{
			if (menuDataGlobal.currentItemIndex > 0)
			{
				isDirty = true;
				menuDataGlobal.currentItemIndex--;

				if (firstDisplayed && (menuDataGlobal.currentItemIndex < firstDisplayed))
				{
					firstDisplayed--;
				}
			}

			if (menuDataGlobal.currentItemIndex == 0)
			{
				menuLastHeardExitCode |= MENU_STATUS_LIST_TYPE;
			}
		}
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_UP) && BUTTONCHECK_DOWN(ev, BUTTON_SK2))
		{
			menuDataGlobal.currentItemIndex = 0;
			isDirty = true;
			firstDisplayed = 0;
			menuLastHeardExitCode |= MENU_STATUS_LIST_TYPE;
		}
		else if ((currentMenu == MENU_LAST_HEARD) && KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
		{
			int timeslot = selectedItem->receivedTS;

			setOverrideTGorPC(selectedItem->id, true);

			if ((timeslot != -1) && (timeslot != trxGetDMRTimeSlot()))
			{
				trxSetDMRTimeSlot(timeslot);
				tsSetManualOverride(((menuSystemGetRootMenuNumber() == UI_CHANNEL_MODE) ? CHANNEL_CHANNEL : (CHANNEL_VFO_A + nonVolatileSettings.currentVFONumber)), (timeslot + 1));
			}
			announceItem(PROMPT_SEQUENCE_CONTACT_TG_OR_PC, PROMPT_THRESHOLD_3);
			menuSystemPopAllAndDisplayRootMenu();
			return;
		}
	}

	if (currentMenu == MENU_LAST_HEARD)
	{
		if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
		{
			menuSystemPopPreviousMenu();
			return;
		}
		else if (KEYCHECK_SHORTUP_NUMBER(ev->keys) && (BUTTONCHECK_DOWN(ev, BUTTON_SK2)))
		{
			saveQuickkeyMenuIndex(ev->keys.key, menuSystemGetCurrentMenuNumber(), 0, 0);
			return;
		}

		// Toggles LH simple/details view on SK2 long press
		if (!displayLHDetails && BUTTONCHECK_LONGDOWN(ev, BUTTON_SK2))
		{
			isDirty = true;
			displayLHDetails = true;
		}
		else if (displayLHDetails && (BUTTONCHECK_DOWN(ev, BUTTON_SK2) == 0))
		{
			isDirty = true;
			displayLHDetails = false;
		}

		if (isDirty)
		{
			menuLastHeardUpdateScreen(true, displayLHDetails, false);// This will also setup the voice prompt

			if (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1)
			{
				voicePromptsPlay();
			}
		}
	}
	else
	{
		if (isDirty)
		{
			menuLastHeardUpdateScreen(false, false, false);
		}
	}
}

void menuLastHeardInit(void)
{
	menuDataGlobal.startIndex = LinkHead->id;// reuse this global to store the ID of the first item in the list
	menuDataGlobal.currentItemIndex = 0;
	menuDataGlobal.endIndex = uiDataGlobal.lastHeardCount;
	selectedItem = NULL;
	firstDisplayed = 0;
	displayLHDetails = false;
	lastHeardCount = uiDataGlobal.lastHeardCount;
	menuLastHeardExitCode = MENU_STATUS_SUCCESS;
}

static void promptsInit(bool isFirstRun)
{
	if (voicePromptsIsPlaying())
	{
		voicePromptsTerminate();
	}

	voicePromptsInit();
	if (isFirstRun)
	{
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendLanguageString(&currentLanguage->last_heard);
		voicePromptsAppendLanguageString(&currentLanguage->menu);
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendPrompt(PROMPT_SILENCE);

		if (uiDataGlobal.lastHeardCount == 0)
		{
			voicePromptsAppendLanguageString(&currentLanguage->empty_list);
		}
	}
}

static void displayTalkerAlias(uint8_t y, char *text, uint32_t time, uint32_t now, uint32_t TGorPC, size_t maxLen, bool displayDetails, bool itemIsSelected, bool isFirstRun)
{
	char buffer[37]; // Max: TA 27 (in 7bit format) + ' [' + 6 (Maidenhead)  + ']' + NULL
	char tg_Buffer[17];
	char timeBuffer[17];
	uint32_t tg = (TGorPC & 0xFFFFFF);
	bool isPC = ((TGorPC >> 24) == PC_CALL_FLAG);

	// Do TG and Time stuff first as its always needed for the Voice prompts

	snprintf(tg_Buffer, 17, "%s %u", (isPC ? currentLanguage->pc : currentLanguage->tg), tg);// PC or TG
	snprintf(timeBuffer, 6, "%d", (((now - time) / 1000U) / 60U));// Time

	if (itemIsSelected && (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1))
	{
		promptsInit(isFirstRun);
	}

	if (!displayDetails) // search for callsign + first name
	{
		if (strlen(text) >= 5)
		{
			int32_t  cpos;

			if ((cpos = getFirstSpacePos(text)) != -1) // Callsign found
			{
				// Check if second part is 'DMR ID:'
				// In this case, don't go further
				if (strncmp((text + cpos + 1), "DMR ID:", 7) == 0)
				{
					if (cpos > 15)
					{
						cpos = 16;
					}

					memcpy(buffer, text, cpos);
					buffer[cpos] = 0;

					ucPrintCore(0,y , chomp(buffer), FONT_SIZE_3,TEXT_ALIGN_CENTER, itemIsSelected);
				}
				else // Nope, look for first name
				{
					uint32_t npos;
					char nameBuf[17];
					char outputBuf[17];

					memset(nameBuf, 0, sizeof(nameBuf));

					// Look for the second part, aka first name
					if ((npos = getFirstSpacePos(text + cpos + 1)) != -1)
					{
						// Store callsign
						memcpy(buffer, text, cpos);
						buffer[cpos] = 0;

						// Store 'first name'
						memcpy(nameBuf, (text + cpos + 1), npos);
						nameBuf[npos] = 0;

						snprintf(outputBuf, 17, "%s %s", chomp(buffer), chomp(nameBuf));

						ucPrintCore(0,y, chomp(outputBuf), FONT_SIZE_3,TEXT_ALIGN_CENTER, itemIsSelected);
					}
					else
					{
						// Store callsign
						memcpy(buffer, text, cpos);
						buffer[cpos] = 0;

						memcpy(nameBuf, (text + cpos + 1), strlen(text) - cpos - 1);
						nameBuf[16] = 0;

						snprintf(outputBuf, 17, "%s %s", chomp(buffer), chomp(nameBuf));

						ucPrintCore(0,y, chomp(outputBuf), FONT_SIZE_3,TEXT_ALIGN_CENTER, itemIsSelected);
					}
				}
			}
			else
			{
				// No space found, use a chainsaw
				memcpy(buffer, text, 16);
				buffer[16] = 0;

				ucPrintCore(0, y, chomp(buffer), FONT_SIZE_3,TEXT_ALIGN_CENTER, itemIsSelected);
			}
		}
		else // short callsign
		{
			memcpy(buffer, text, strlen(text));
			buffer[strlen(text)] = 0;

			ucPrintCore(0, y, chomp(buffer), FONT_SIZE_3,TEXT_ALIGN_CENTER, itemIsSelected);
		}

		if (itemIsSelected && (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1))
		{
			voicePromptsAppendString(chomp(buffer));
			voicePromptsAppendString("  ");

			snprintf(buffer, 37, "%d ", tg);
			if (isPC)
			{
				voicePromptsAppendLanguageString(&currentLanguage->private_call);
				if (tg != trxDMRID)
				{
					voicePromptsAppendString(buffer);
				}
			}
			else
			{
				voicePromptsAppendPrompt(PROMPT_TALKGROUP);
				voicePromptsAppendString(buffer);
			}

			voicePromptsAppendString(timeBuffer);
			voicePromptsAppendPrompt(PROMPT_MINUTES);
			voicePromptsAppendString("   ");// Add some blank sound at the end of the callsign, to allow time for follow-on scrolling
		}
	}
	else
	{
		ucPrintCore(0, y, tg_Buffer, FONT_SIZE_3, TEXT_ALIGN_LEFT, itemIsSelected);

#if defined(PLATFORM_RD5R)
		ucPrintCore((DISPLAY_SIZE_X - (3 * 6)), y, "min", FONT_SIZE_1, TEXT_ALIGN_LEFT, itemIsSelected);
#else
		ucPrintCore((DISPLAY_SIZE_X - (3 * 6)), (y + 6), "min", FONT_SIZE_1, TEXT_ALIGN_LEFT, itemIsSelected);
#endif
		ucPrintCore((DISPLAY_SIZE_X - (strlen(timeBuffer) * 8) - (3 * 6) - 1), y, timeBuffer, FONT_SIZE_3, TEXT_ALIGN_LEFT, itemIsSelected);
	}

	if (isFirstRun)
	{
		promptsPlayNotAfterTx();
	}
}
