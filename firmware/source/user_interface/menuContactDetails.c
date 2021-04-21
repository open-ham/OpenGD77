/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
 * 				and	 Colin Durbridge, G4EML
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
#include "functions/settings.h"
#include "functions/trx.h"
#include "user_interface/menuSystem.h"
#include "user_interface/uiUtilities.h"
#include "user_interface/uiLocalisation.h"

static void updateScreen(bool isFirstRun, bool updateScreen);
static void updateCursor(bool moved);
static void handleEvent(uiEvent_t *ev);

static menuStatus_t menuContactDetailsExitCode = MENU_STATUS_SUCCESS;

static struct_codeplugContact_t tmpContact;
static const char *callTypeString[3];// = { "Group", "Private", "All" };
static int contactDetailsIndex;
static char digits[9];
static char contactName[20];
static int namePos;
static int numPos;

enum CONTACT_DETAILS_DISPLAY_LIST { CONTACT_DETAILS_NAME = 0, CONTACT_DETAILS_TG, CONTACT_DETAILS_CALLTYPE, CONTACT_DETAILS_TS,
	NUM_CONTACT_DETAILS_ITEMS };// The last item in the list is used so that we automatically get a total number of items in the list

static int menuContactDetailsState;
static int menuContactDetailsTimeout;
enum MENU_CONTACT_DETAILS_STATE { MENU_CONTACT_DETAILS_DISPLAY = 0, MENU_CONTACT_DETAILS_SAVED, MENU_CONTACT_DETAILS_EXISTS };

menuStatus_t menuContactDetails(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		callTypeString[0] = currentLanguage->group;
		callTypeString[1] = currentLanguage->private;
		callTypeString[2] = currentLanguage->all;

		voicePromptsInit();

		if (uiDataGlobal.currentSelectedContactIndex == 0)
		{
			contactDetailsIndex = codeplugContactGetFreeIndex();
			tmpContact.name[0] = 0x00;
			tmpContact.callType = CONTACT_CALLTYPE_TG;
			tmpContact.reserve1 = 0xff;
			tmpContact.tgNumber = 0;
			digits[0] = 0x00;
			memset(contactName, 0, 20);
			namePos = 0;
			numPos = 0;
			voicePromptsAppendLanguageString(&currentLanguage->new_contact);
		}
		else
		{
			contactDetailsIndex = contactListContactData.NOT_IN_CODEPLUGDATA_indexNumber;
			memcpy(&tmpContact, &contactListContactData, CONTACT_DATA_STRUCT_SIZE);
			itoa(tmpContact.tgNumber, digits, 10);
			codeplugUtilConvertBufToString(tmpContact.name, contactName, 16);
			namePos = strlen(contactName);
			numPos = 0;
		}

		menuContactDetailsState = MENU_CONTACT_DETAILS_DISPLAY;
		menuDataGlobal.currentItemIndex = CONTACT_DETAILS_NAME;
		menuDataGlobal.endIndex = NUM_CONTACT_DETAILS_ITEMS;

		updateScreen(isFirstRun, true);
		updateCursor(true);

		return (MENU_STATUS_LIST_TYPE | MENU_STATUS_SUCCESS);
	}
	else
	{
		menuContactDetailsExitCode = MENU_STATUS_SUCCESS;

		updateCursor(false);
		if (ev->hasEvent || (menuContactDetailsTimeout > 0))
		{
			handleEvent(ev);
		}
	}
	return menuContactDetailsExitCode;
}

static void updateCursor(bool moved)
{
	switch (menuDataGlobal.currentItemIndex)
	{
		case CONTACT_DETAILS_NAME:
			menuUpdateCursor(namePos, moved, true);
			break;
		case CONTACT_DETAILS_TG:
			menuUpdateCursor(numPos, moved, true);
			break;
	}
}

static void updateScreen(bool isFirstRun, bool allowedToSpeakUpdate)
{
	int mNum = 0;
	static const int bufferLen = 17;
	char buf[bufferLen];
	char * const *leftSide = NULL;// initialise to please the compiler
	char * const *leftSideConst = NULL;// initialise to please the compiler
	voicePrompt_t leftSidePrompt;
	char * const *rightSideConst = NULL;// initialise to please the compiler
	char rightSideVar[bufferLen];
	voicePrompt_t rightSidePrompt;

	ucClearBuf();

	if (tmpContact.name[0] == 0x00)
	{
		snprintf(buf, bufferLen, "%s", currentLanguage->new_contact);
	}
	else
	{
		codeplugUtilConvertBufToString(tmpContact.name, buf, 16);
	}

	menuDisplayTitle(buf);

	if (menuDataGlobal.currentItemIndex == CONTACT_DETAILS_NAME)
	{
		keypadAlphaEnable = true;
	}
	else
	{
		keypadAlphaEnable = false;
	}

	switch (menuContactDetailsState)
	{
		case MENU_CONTACT_DETAILS_DISPLAY:
			// Can only display 3 of the options at a time menu at -1, 0 and +1
			for(int i = -1; i <= 1; i++)
			{
				mNum = menuGetMenuOffset(NUM_CONTACT_DETAILS_ITEMS, i);
				buf[0] = 0;
				leftSide = NULL;
				leftSideConst = NULL;
				leftSidePrompt = PROMPT_SILENCE;
				rightSideConst = NULL;
				rightSideVar[0] = 0;
				rightSidePrompt = PROMPT_SILENCE;// use PROMPT_SILENCE as flag that the unit has not been set

				switch (mNum)
				{
					case CONTACT_DETAILS_NAME:
						strncpy(rightSideVar, contactName, 17);
						break;
					case CONTACT_DETAILS_TG:
						{
							int nLen = 0;

							switch (tmpContact.callType)
							{
								case CONTACT_CALLTYPE_TG:
									leftSide = (char * const *)&currentLanguage->tg;
									leftSidePrompt = PROMPT_TALKGROUP;

									if (!(nLen = strlen(digits)))
									{
										rightSideConst = (char * const *)&currentLanguage->none;
									}
									else
									{
										snprintf(rightSideVar, bufferLen, "%s", digits);
									}
									break;
								case CONTACT_CALLTYPE_PC: // Private
									leftSide = (char * const *)&currentLanguage->pc;
									leftSideConst = (char * const *)&currentLanguage->private_call;

									if (!(nLen = strlen(digits)))
									{
										rightSideConst = (char * const *)&currentLanguage->none;
									}
									else
									{
										snprintf(rightSideVar, bufferLen, "%s", digits);
									}
									break;
								case CONTACT_CALLTYPE_ALL: // All Call
									leftSide = (char * const *)&currentLanguage->all;
									snprintf(rightSideVar, bufferLen, "%d", 16777215);
									leftSideConst = (char * const *)&currentLanguage->all_call;
									break;
							}

							if (i == 0)
							{
								numPos = strlen(*leftSide) + 1 + (nLen ? nLen : strlen(rightSideVar));
							}
						}
						break;
						case CONTACT_DETAILS_CALLTYPE:
							leftSide = (char * const *)&currentLanguage->type;
							leftSideConst = (char * const *)&currentLanguage->type;
							snprintf(rightSideVar, bufferLen, "%s", callTypeString[tmpContact.callType]);

							switch (tmpContact.callType)
							{
								case CONTACT_CALLTYPE_TG:
									rightSidePrompt = PROMPT_TALKGROUP;
									break;
								case CONTACT_CALLTYPE_PC: // Private
									rightSideConst = (char * const *)&currentLanguage->private_call;
									break;
								case CONTACT_CALLTYPE_ALL: // All Call
									rightSideConst = (char * const *)&currentLanguage->all_call;
									break;
							}
							break;
						case CONTACT_DETAILS_TS:
							leftSide = (char * const *)&currentLanguage->timeSlot;
							leftSidePrompt = PROMPT_TIMESLOT;

							switch (tmpContact.reserve1 & 0x3)
							{
								case 1:
								case 3:
									snprintf(rightSideVar, bufferLen, "%s", currentLanguage->none);
									rightSideConst = (char * const *)&currentLanguage->none;
									break;
								case 0:
									snprintf(rightSideVar, bufferLen, "%d", 1);
									break;
								case 2:
									snprintf(rightSideVar, bufferLen, "%d", 2);
									break;
							}
							break;
				}

				if (leftSide != NULL)
				{
					snprintf(buf, bufferLen, "%s:%s", *leftSide, (rightSideVar[0] ? rightSideVar : (rightSideConst ? *rightSideConst : "")));
				}
				else
				{
					strcpy(buf, rightSideVar);
				}

				if ((i == 0) && allowedToSpeakUpdate)
				{
					if (!isFirstRun)
					{
						voicePromptsInit();
					}

					if (leftSidePrompt != PROMPT_SILENCE)
					{
						voicePromptsAppendPrompt(leftSidePrompt);
						voicePromptsAppendPrompt(PROMPT_SILENCE);
					}
					else if ((leftSideConst != NULL) || (leftSide != NULL))
					{
						voicePromptsAppendLanguageString((const char * const *) (leftSideConst ? leftSideConst : leftSide));
					}

					if (rightSidePrompt != PROMPT_SILENCE)
					{
						voicePromptsAppendPrompt(rightSidePrompt);
					}
					else if (rightSideConst != NULL)
					{
						voicePromptsAppendLanguageString((const char * const *)rightSideConst);
					}
					else if (rightSideVar[0] != 0)
					{
						voicePromptsAppendString(rightSideVar);
					}

					promptsPlayNotAfterTx();
				}

				menuDisplayEntry(i, mNum, buf);
			}
			break;
		case MENU_CONTACT_DETAILS_SAVED:
			ucPrintCentered(16, currentLanguage->contact_saved, FONT_SIZE_3);
			ucDrawChoice(CHOICE_OK, false);
			break;
		case MENU_CONTACT_DETAILS_EXISTS:
			ucPrintCentered(16, currentLanguage->duplicate, FONT_SIZE_3);
			ucPrintCentered(32, currentLanguage->contact, FONT_SIZE_3);
			ucDrawChoice(CHOICE_OK, false);
			break;
	}
	ucRender();
}

static void handleEvent(uiEvent_t *ev)
{
	dmrIdDataStruct_t foundRecord;
	static const int bufferLen = 17;
	char buf[bufferLen];
	int sLen = strlen(digits);

	if (ev->events & BUTTON_EVENT)
	{
		if (repeatVoicePromptOnSK1(ev))
		{
			return;
		}
	}

	// Not entering a frequency numeric digit
	bool allowedToSpeakUpdate = true;

	switch (menuContactDetailsState)
	{
		case MENU_CONTACT_DETAILS_DISPLAY:
			if (ev->events & KEY_EVENT)
			{
				if (KEYCHECK_PRESS(ev->keys, KEY_DOWN))
				{
					menuSystemMenuIncrement(&menuDataGlobal.currentItemIndex, NUM_CONTACT_DETAILS_ITEMS);
					updateScreen(false, true);
					menuContactDetailsExitCode |= MENU_STATUS_LIST_TYPE;
				}
				else if (KEYCHECK_PRESS(ev->keys, KEY_UP))
				{
					menuSystemMenuDecrement(&menuDataGlobal.currentItemIndex, NUM_CONTACT_DETAILS_ITEMS);
					updateScreen(false, true);
					menuContactDetailsExitCode |= MENU_STATUS_LIST_TYPE;
				}
				else if (KEYCHECK_LONGDOWN(ev->keys, KEY_RIGHT) || KEYCHECK_LONGDOWN_REPEAT(ev->keys, KEY_RIGHT))
				{
					if (menuDataGlobal.currentItemIndex == CONTACT_DETAILS_NAME)
					{
						namePos = strlen(contactName);
						updateScreen(false, !voicePromptsIsPlaying());
					}
				}
				else if (KEYCHECK_SHORTUP(ev->keys, KEY_RIGHT))
				{
					switch(menuDataGlobal.currentItemIndex)
					{
						case CONTACT_DETAILS_NAME:
							moveCursorRightInString(contactName, &namePos, 16, BUTTONCHECK_DOWN(ev, BUTTON_SK2));
							updateCursor(true);
							allowedToSpeakUpdate = false;
							break;
						case CONTACT_DETAILS_TG:
							break;
						case CONTACT_DETAILS_CALLTYPE:
							if (tmpContact.callType < CONTACT_CALLTYPE_ALL)
							{
								tmpContact.callType++;
							}
							itoa(tmpContact.callType == CONTACT_CALLTYPE_ALL ? 16777215 : tmpContact.tgNumber, digits, 10);
							break;
						case CONTACT_DETAILS_TS:
							if ((tmpContact.reserve1 & 0x01) && ((tmpContact.reserve1 & 0x02) == 0))
							{
								tmpContact.reserve1 |= 0x03;
							}

							if (tmpContact.reserve1 & 0x01 || ((tmpContact.reserve1 & 0x03) == 0x00))
							{
								tmpContact.reserve1 &= ~0x01;
								tmpContact.reserve1 ^= 0x02;
							}
							break;
					}
					updateScreen(false, allowedToSpeakUpdate);
				}
				else if (KEYCHECK_LONGDOWN(ev->keys, KEY_LEFT) || KEYCHECK_LONGDOWN_REPEAT(ev->keys, KEY_LEFT))
				{
					if (menuDataGlobal.currentItemIndex == CONTACT_DETAILS_NAME)
					{
						namePos = 0;
						updateScreen(false, !voicePromptsIsPlaying());
					}
				}
				else if (KEYCHECK_SHORTUP(ev->keys, KEY_LEFT))
				{
					switch(menuDataGlobal.currentItemIndex)
					{
						case CONTACT_DETAILS_NAME:
							moveCursorLeftInString(contactName, &namePos, BUTTONCHECK_DOWN(ev, BUTTON_SK2));
							updateCursor(true);
							allowedToSpeakUpdate = false;
							break;
						case CONTACT_DETAILS_TG:
							if (sLen > 0)
							{
								digits[sLen - 1] = 0x00;
							}
							updateCursor(true);
							break;
						case CONTACT_DETAILS_CALLTYPE:
							if (tmpContact.callType > 0)
							{
								tmpContact.callType--;
							}
							itoa(tmpContact.callType == CONTACT_CALLTYPE_ALL ? 16777215 : tmpContact.tgNumber, digits, 10);
							break;
						case CONTACT_DETAILS_TS:
							if (((tmpContact.reserve1 & 0x01) == 0x0) || ((tmpContact.reserve1 & 0x03) == 0x00))
							{
								if (((tmpContact.reserve1 & 0x03) == 0x00))
								{
									tmpContact.reserve1 ^= 0x01;
								}
								else
								{
									tmpContact.reserve1 ^= 0x02;
								}
							}
							break;
					}
					updateScreen(false, allowedToSpeakUpdate);
				}
				else if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
				{
					if (tmpContact.callType == CONTACT_CALLTYPE_ALL)
					{
						tmpContact.tgNumber = 16777215;
					}
					else
					{
						tmpContact.tgNumber = atoi(digits);
					}

					if ((tmpContact.tgNumber > 0) && (tmpContact.tgNumber <= MAX_TG_OR_PC_VALUE))// 9999999)
					{
						codeplugUtilConvertStringToBuf(contactName, tmpContact.name, 16);
						if ((contactDetailsIndex >= CODEPLUG_CONTACTS_MIN) && (contactDetailsIndex <= CODEPLUG_CONTACTS_MAX))
						{
							if (tmpContact.name[0] == 0xff)
							{
								if (tmpContact.callType == CONTACT_CALLTYPE_PC)
								{
									if (dmrIDLookup(tmpContact.tgNumber, &foundRecord))
									{
										codeplugUtilConvertStringToBuf(foundRecord.text, tmpContact.name, 16);
									}
									else
									{
										snprintf(buf, bufferLen, "%s %d", currentLanguage->pc, tmpContact.tgNumber);
										codeplugUtilConvertStringToBuf(buf, tmpContact.name, 16);
									}
								}
								else
								{
									snprintf(buf, bufferLen, "%s %d", currentLanguage->tg, tmpContact.tgNumber);
									codeplugUtilConvertStringToBuf(buf, tmpContact.name, 16);
								}
							}
							codeplugContactSaveDataForIndex(contactDetailsIndex, &tmpContact);
							menuContactDetailsTimeout = 2000;
							menuContactDetailsState = MENU_CONTACT_DETAILS_SAVED;
							voicePromptsInit();
							voicePromptsAppendLanguageString(&currentLanguage->contact_saved);
							voicePromptsPlay();
						}
					}
					updateScreen(false, allowedToSpeakUpdate);
				}
				else if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
				{
					menuSystemPopPreviousMenu();
					return;
				}
				else
				{
					if (menuDataGlobal.currentItemIndex == CONTACT_DETAILS_TG)
					{
						if (tmpContact.callType != CONTACT_CALLTYPE_ALL)
						{
							// Add a digit
							if (sLen < NUM_PC_OR_TG_DIGITS)
							{
								int keyval = menuGetKeypadKeyValue(ev, true);

								char c[2] = {0, 0};

								if (keyval != 99)
								{
									c[0] = keyval + '0';
									strcat(digits, c);
								}
								updateScreen(false, allowedToSpeakUpdate);
							}
						}
					}
					else if (menuDataGlobal.currentItemIndex == CONTACT_DETAILS_NAME)
					{
						if ((ev->keys.event == KEY_MOD_PREVIEW) && (namePos < 16))
						{
							contactName[namePos] = ev->keys.key;
							updateCursor(true);
							announceChar(ev->keys.key);
							updateScreen(false, false);
						}
						else if ((ev->keys.event == KEY_MOD_PRESS) && (namePos < 16))
						{
							contactName[namePos] = ev->keys.key;
							if (namePos < strlen(contactName) && namePos < 15)
							{
								namePos++;
							}
							updateCursor(true);
							announceChar(ev->keys.key);
							updateScreen(false, false);
						}
					}
				}
			}
			break;

		case MENU_CONTACT_DETAILS_SAVED:
			menuContactDetailsTimeout--;
			if ((menuContactDetailsTimeout == 0) || KEYCHECK_SHORTUP(ev->keys, KEY_GREEN) || KEYCHECK_SHORTUP(ev->keys, KEY_RED))
			{
				menuSystemPopPreviousMenu();
				return;
			}
			updateScreen(false, allowedToSpeakUpdate);
			break;

		case MENU_CONTACT_DETAILS_EXISTS:
			menuContactDetailsTimeout--;
			if ((menuContactDetailsTimeout == 0) || KEYCHECK_SHORTUP(ev->keys, KEY_GREEN) || KEYCHECK_SHORTUP(ev->keys, KEY_RED))
			{
				menuContactDetailsState = MENU_CONTACT_DETAILS_DISPLAY;
			}
			updateScreen(false, allowedToSpeakUpdate);
			break;
	}
}
