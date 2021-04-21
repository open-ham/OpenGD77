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

typedef enum
{
	MENU_CONTACT_LIST_DISPLAY = 0,
	MENU_CONTACT_LIST_CONFIRM,
	MENU_CONTACT_LIST_DELETED,
	MENU_CONTACT_LIST_TG_IN_RXGROUP
} contactListState_t;

typedef enum
{
	MENU_CONTACT_LIST_CONTACT_DIGITAL = 0,
	MENU_CONTACT_LIST_CONTACT_DTMF
} contactListContactType_t;


static void updateScreen(bool isFirstRun);
static void handleEvent(uiEvent_t *ev);

static struct_codeplugContact_t     contact;
static struct_codeplugDTMFContact_t dtmfContact;

static int contactListType;
static int contactCallType;
static contactListState_t contactListDisplayState;
static contactListState_t contactListOverrideState = MENU_CONTACT_LIST_DISPLAY;
static int contactListTimeout;
static menuStatus_t menuContactListExitCode = MENU_STATUS_SUCCESS;
static menuStatus_t menuContactListSubMenuExitCode = MENU_STATUS_SUCCESS;

// Apply contact + its TS on selection for TX (contact list of quick list).
static void overrideWithSelectedContact(void)
{
	menuPrivateCallClear();
	setOverrideTGorPC(contactListContactData.tgNumber, (contactListContactData.callType == CONTACT_CALLTYPE_PC));
	// Contact has a TS override set
	if ((contactListContactData.reserve1 & 0x01) == 0x00)
	{
		int ts = ((contactListContactData.reserve1 & 0x02) >> 1);
		trxSetDMRTimeSlot(ts);
		tsSetManualOverride(((menuSystemGetRootMenuNumber() == UI_CHANNEL_MODE) ? CHANNEL_CHANNEL : (CHANNEL_VFO_A + nonVolatileSettings.currentVFONumber)), (ts + 1));
	}
}

static void reloadContactList(contactListContactType_t type)
{
	menuDataGlobal.endIndex = (type == MENU_CONTACT_LIST_CONTACT_DIGITAL) ? codeplugContactsGetCount(contactCallType) : codeplugDTMFContactsGetCount();

	if (menuDataGlobal.endIndex > 0)
	{
		if (menuDataGlobal.currentItemIndex >= menuDataGlobal.endIndex)
		{
			menuDataGlobal.currentItemIndex = 0;
		}
		uiDataGlobal.currentSelectedContactIndex = (type == MENU_CONTACT_LIST_CONTACT_DIGITAL)
				? codeplugContactGetDataForNumberInType(menuDataGlobal.currentItemIndex + 1, contactCallType, &contactListContactData)
				: codeplugDTMFContactGetDataForNumber(menuDataGlobal.currentItemIndex + 1, &contactListDTMFContactData);
	}
	else
	{
		uiDataGlobal.currentSelectedContactIndex = 0;
	}
}

menuStatus_t menuContactList(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		if (contactListOverrideState == MENU_CONTACT_LIST_DISPLAY)
		{
			if (uiDataGlobal.currentSelectedContactIndex == 0)
			{
				int currentMenu = menuSystemGetCurrentMenuNumber();

				// Shows digital contact list if called from "contact list" menu entry, or from <SK2>+# in digital.
				// Otherwise displays DTMF contact list
				contactListType = ((currentMenu == MENU_CONTACT_LIST) || ((currentMenu == MENU_CONTACT_QUICKLIST) && (trxGetMode() != RADIO_MODE_ANALOG))) ? MENU_CONTACT_LIST_CONTACT_DIGITAL : MENU_CONTACT_LIST_CONTACT_DTMF;
				contactCallType = CONTACT_CALLTYPE_TG;

				dtmfSequenceReset();
			}
			else
			{
				if (contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL)
				{
					codeplugContactGetDataForIndex(uiDataGlobal.currentSelectedContactIndex, &contactListContactData);
					contactCallType = contactListContactData.callType;
				}
				else
				{
					codeplugDTMFContactGetDataForIndex(uiDataGlobal.currentSelectedContactIndex, &contactListDTMFContactData);
				}
			}

			reloadContactList(contactListType);
			contactListDisplayState = MENU_CONTACT_LIST_DISPLAY;

			voicePromptsInit();
			voicePromptsAppendPrompt(PROMPT_SILENCE);
			voicePromptsAppendPrompt(PROMPT_SILENCE);
			if (contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL)
			{
				voicePromptsAppendLanguageString(&currentLanguage->contacts);
				voicePromptsAppendLanguageString(&currentLanguage->menu);
				voicePromptsAppendPrompt(PROMPT_SILENCE);
				voicePromptsAppendPrompt(PROMPT_SILENCE);
			}
			else
			{
				if (menuSystemGetCurrentMenuNumber() == MENU_CONTACT_QUICKLIST)
				{
					voicePromptsAppendLanguageString(&currentLanguage->dtmf_contact_list);
					voicePromptsAppendPrompt(PROMPT_SILENCE);
					voicePromptsAppendPrompt(PROMPT_SILENCE);
				}
			}
		}
		else
		{
			if (contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL)
			{
				codeplugContactGetDataForIndex(uiDataGlobal.currentSelectedContactIndex, &contactListContactData);
			}
			else
			{
				codeplugDTMFContactGetDataForIndex(uiDataGlobal.currentSelectedContactIndex, &contactListDTMFContactData);
			}
			contactListDisplayState = contactListOverrideState;
			contactListOverrideState = MENU_CONTACT_LIST_DISPLAY;
		}

		updateScreen(true);
		menuContactListExitCode = (MENU_STATUS_LIST_TYPE | MENU_STATUS_SUCCESS);
	}
	else
	{
		menuContactListExitCode = MENU_STATUS_SUCCESS;

		if (ev->hasEvent || (contactListTimeout > 0))
		{
			handleEvent(ev);
		}
	}

	// Play the DTMF contact sequence from Quicklist
	dtmfSequenceTick(false);

	return menuContactListExitCode;
}

static void updateScreen(bool isFirstRun)
{
	char nameBuf[33];
	int mNum;
	int idx;
	const char *calltypeName[] = { currentLanguage->group_call, currentLanguage->private_call, currentLanguage->all_call, "DTMF" };

	ucClearBuf();

	switch (contactListDisplayState)
	{
		case MENU_CONTACT_LIST_DISPLAY:
			menuDisplayTitle((char *) calltypeName[((contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL) ? contactCallType : 3)]);

			if (!isFirstRun)
			{
				voicePromptsInit();
			}

			if (menuDataGlobal.endIndex == 0)
			{
				ucPrintCentered((DISPLAY_SIZE_Y / 2), currentLanguage->empty_list, FONT_SIZE_3);

				voicePromptsAppendLanguageString(&currentLanguage->empty_list);
				promptsPlayNotAfterTx();
			}
			else
			{
				for (int i = -1; i <= 1; i++)
				{
					mNum = menuGetMenuOffset(menuDataGlobal.endIndex, i);
					idx = (contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL)
							? codeplugContactGetDataForNumberInType(mNum + 1, contactCallType, &contact)
							: codeplugDTMFContactGetDataForNumber(mNum + 1, &dtmfContact);

					if (idx > 0)
					{
						codeplugUtilConvertBufToString(((contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL) ? contact.name : dtmfContact.name), nameBuf, 16); // need to convert to zero terminated string
						menuDisplayEntry(i, mNum, (char*) nameBuf);
					}

					if (i == 0)
					{
						voicePromptsAppendString(nameBuf);
						promptsPlayNotAfterTx();
					}
				}
			}
			break;
		case MENU_CONTACT_LIST_CONFIRM:
			codeplugUtilConvertBufToString(contactListContactData.name, nameBuf, 16);
			menuDisplayTitle(nameBuf);
			ucPrintCentered(16, currentLanguage->delete_contact_qm, FONT_SIZE_3);
			ucDrawChoice(CHOICE_YESNO, false);
			break;
		case MENU_CONTACT_LIST_DELETED:
			codeplugUtilConvertBufToString(contactListContactData.name, nameBuf, 16);
			ucPrintCentered(16, currentLanguage->contact_deleted, FONT_SIZE_3);
			ucDrawChoice(CHOICE_DISMISS, false);
			break;
		case MENU_CONTACT_LIST_TG_IN_RXGROUP:
			codeplugUtilConvertBufToString(contactListContactData.name, nameBuf, 16);
			menuDisplayTitle(nameBuf);
			ucPrintCentered(16, currentLanguage->contact_used, FONT_SIZE_3);
			ucPrintCentered((DISPLAY_SIZE_Y/2), currentLanguage->in_rx_group, FONT_SIZE_3);
			ucDrawChoice(CHOICE_DISMISS, false);
			break;
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

	// DTMF sequence is playing, stop it.
	if (uiDataGlobal.DTMFContactList.isKeying && ((ev->keys.key != 0) || BUTTONCHECK_DOWN(ev, BUTTON_PTT)
#if ! defined(PLATFORM_RD5R)
													|| BUTTONCHECK_DOWN(ev, BUTTON_ORANGE)
#endif
	))
	{
		uiDataGlobal.DTMFContactList.poLen = 0U;
		return;
	}

	switch (contactListDisplayState)
	{
		case MENU_CONTACT_LIST_DISPLAY:
			if (KEYCHECK_PRESS(ev->keys, KEY_DOWN))
			{
				menuSystemMenuIncrement(&menuDataGlobal.currentItemIndex, menuDataGlobal.endIndex);
				uiDataGlobal.currentSelectedContactIndex = (contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL)
						? codeplugContactGetDataForNumberInType(menuDataGlobal.currentItemIndex + 1, contactCallType, &contactListContactData)
						: codeplugDTMFContactGetDataForNumber(menuDataGlobal.currentItemIndex + 1, &contactListDTMFContactData);
				updateScreen(false);
				menuContactListExitCode |= MENU_STATUS_LIST_TYPE;
			}
			else if (KEYCHECK_PRESS(ev->keys, KEY_UP))
			{
				menuSystemMenuDecrement(&menuDataGlobal.currentItemIndex, menuDataGlobal.endIndex);
				uiDataGlobal.currentSelectedContactIndex = (contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL)
						? codeplugContactGetDataForNumberInType(menuDataGlobal.currentItemIndex + 1, contactCallType, &contactListContactData)
						: codeplugDTMFContactGetDataForNumber(menuDataGlobal.currentItemIndex + 1, &contactListDTMFContactData);
				updateScreen(false);
				menuContactListExitCode |= MENU_STATUS_LIST_TYPE;
			}
			else if (KEYCHECK_SHORTUP(ev->keys, KEY_HASH))
			{
				if (contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL)
				{
					contactCallType = (contactCallType + 1) % 3;
					reloadContactList(contactListType);
					updateScreen(false);
				}
			}
			else if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
			{
				int currentMenu = menuSystemGetCurrentMenuNumber();
				int currentMode = trxGetMode();

				if (currentMenu == MENU_CONTACT_QUICKLIST)
				{
					if (contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL)
					{
						if (menuDataGlobal.endIndex > 0)
						{
							if (currentMode == RADIO_MODE_DIGITAL)
							{
								overrideWithSelectedContact();
								uiDataGlobal.currentSelectedContactIndex = 0;
								announceItem(PROMPT_SEQUENCE_CONTACT_TG_OR_PC, PROMPT_THRESHOLD_3);
								menuSystemPopAllAndDisplayRootMenu();
							}
							else
							{
								menuContactListExitCode |= MENU_STATUS_ERROR;
							}
						}
						return;
					}
					else // MENU_CONTACT_LIST_CONTACT_DTMF
					{
						if (currentMode == RADIO_MODE_ANALOG)
						{
							dtmfSequencePrepare(contactListDTMFContactData.code, true);
						}
						else
						{
							menuContactListExitCode |= MENU_STATUS_ERROR;
						}
						return;
					}
				}

				// Display submenu for DTMF contact list
				if (menuDataGlobal.endIndex > 0) // display action list only if contact list is non empty
				{
					if ((currentMenu == MENU_CONTACT_LIST) ||
							((currentMenu == MENU_DTMF_CONTACT_LIST) && (currentMode == RADIO_MODE_ANALOG)))
					{
						menuSystemPushNewMenu(MENU_CONTACT_LIST_SUBMENU);
					}
					else
					{
						menuContactListExitCode |= MENU_STATUS_ERROR;
					}
				}
				return;
			}
			else if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
			{
				uiDataGlobal.currentSelectedContactIndex = 0;
				menuSystemPopPreviousMenu();
				return;
			}

			if ((contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL) &&
					(KEYCHECK_SHORTUP_NUMBER(ev->keys) && (BUTTONCHECK_DOWN(ev, BUTTON_SK2))))
			{
				saveQuickkeyContactIndex(ev->keys.key, (uint16_t)contactListContactData.NOT_IN_CODEPLUGDATA_indexNumber);
				return;
			}

			break;

		case MENU_CONTACT_LIST_CONFIRM:
			if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
			{
				memset(contact.name, 0xff, 16);
				contact.tgNumber = 0;
				contact.callType = 0xFF;
				codeplugContactSaveDataForIndex(uiDataGlobal.currentSelectedContactIndex, &contact);
				uiDataGlobal.currentSelectedContactIndex = 0;
				contactListTimeout = 2000;
				contactListDisplayState = MENU_CONTACT_LIST_DELETED;
				reloadContactList(contactListType);
				updateScreen(false);
				voicePromptsAppendLanguageString(&currentLanguage->contact_deleted);
				voicePromptsPlay();
			}
			else if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
			{
				contactListDisplayState = MENU_CONTACT_LIST_DISPLAY;
				reloadContactList(contactListType);
				updateScreen(false);
			}
			break;

		case MENU_CONTACT_LIST_DELETED:
		case MENU_CONTACT_LIST_TG_IN_RXGROUP:
			contactListTimeout--;
			if ((contactListTimeout == 0) || KEYCHECK_SHORTUP(ev->keys, KEY_GREEN) || KEYCHECK_SHORTUP(ev->keys, KEY_RED))
			{
				contactListDisplayState = MENU_CONTACT_LIST_DISPLAY;
				reloadContactList(contactListType);
			}
			updateScreen(false);
			break;
	}
}

enum CONTACT_LIST_QUICK_MENU_ITEMS
{
	CONTACT_LIST_QUICK_MENU_SELECT = 0,
	CONTACT_LIST_QUICK_MENU_EDIT,
	CONTACT_LIST_QUICK_MENU_DELETE,
	NUM_CONTACT_LIST_QUICK_MENU_ITEMS    // The last item in the list is used so that we automatically get a total number of items in the list
};

static void updateSubMenuScreen(void)
{
	int mNum = 0;
	static const int bufferLen = 17;
	char buf[bufferLen];
	char * const *langTextConst = NULL;// initialise to please the compiler

	voicePromptsInit();

	ucClearBuf();

	codeplugUtilConvertBufToString((contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL) ? contactListContactData.name : contactListDTMFContactData.name, buf, 16);
	menuDisplayTitle(buf);

	for(int i = -1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(((contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL) ? NUM_CONTACT_LIST_QUICK_MENU_ITEMS : 1), i);
		buf[0] = 0;

		switch(mNum)
		{
			case CONTACT_LIST_QUICK_MENU_SELECT:
				langTextConst = (char * const *)&currentLanguage->select_tx;
				break;
			case CONTACT_LIST_QUICK_MENU_EDIT:
				langTextConst = (contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL) ? (char * const *)&currentLanguage->edit_contact : NULL;
				break;
			case CONTACT_LIST_QUICK_MENU_DELETE:
				langTextConst = (contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL) ? (char * const *)&currentLanguage->delete_contact : NULL;
				break;
		}

		if (langTextConst != NULL)
		{
			strncpy(buf, *langTextConst, 17);
		}
		else
		{
			strncpy(buf, " ", 17);
		}

		if ((i == 0) && (langTextConst != NULL))
		{
			voicePromptsAppendLanguageString((const char * const *)langTextConst);
			promptsPlayNotAfterTx();
		}

		menuDisplayEntry(i, mNum, buf);
	}

	ucRender();
}

static void handleSubMenuEvent(uiEvent_t *ev)
{
	if (ev->events & BUTTON_EVENT)
	{
		if (repeatVoicePromptOnSK1(ev))
		{
			return;
		}
	}

	// DTMF sequence is playing, stop it.
	if (uiDataGlobal.DTMFContactList.isKeying && ((ev->keys.key != 0) || BUTTONCHECK_DOWN(ev, BUTTON_PTT)
#if ! defined(PLATFORM_RD5R)
													|| BUTTONCHECK_DOWN(ev, BUTTON_ORANGE)
#endif
	))
	{
		uiDataGlobal.DTMFContactList.poLen = 0U;
		return;
	}

	if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
	{
		menuSystemPopPreviousMenu();
	}
	else if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
	{
		contactListOverrideState = MENU_CONTACT_LIST_DISPLAY;
		switch (menuDataGlobal.currentItemIndex)
		{
			case CONTACT_LIST_QUICK_MENU_SELECT:
				if (contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL)
				{
					if (trxGetMode() == RADIO_MODE_DIGITAL)
					{
						overrideWithSelectedContact();
						uiDataGlobal.currentSelectedContactIndex = 0;
						announceItem(PROMPT_SEQUENCE_CONTACT_TG_OR_PC, PROMPT_THRESHOLD_3);
						uiDataGlobal.VoicePrompts.inhibitInitial = true;
						menuSystemPopAllAndDisplayRootMenu();
					}
					else
					{
						menuContactListSubMenuExitCode |= MENU_STATUS_ERROR;
					}
					return;
				}
				else
				{
					dtmfSequencePrepare(contactListDTMFContactData.code, true);
				}
				break;
			case CONTACT_LIST_QUICK_MENU_EDIT:
				if (contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL)
				{
					menuSystemSetCurrentMenu(MENU_CONTACT_DETAILS);
				}
				break;
			case CONTACT_LIST_QUICK_MENU_DELETE:
				if (uiDataGlobal.currentSelectedContactIndex > 0)
				{
					if (contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL)
					{
						if ((contactListContactData.callType == CONTACT_CALLTYPE_TG) &&
								codeplugContactGetRXGroup(contactListContactData.NOT_IN_CODEPLUGDATA_indexNumber))
						{
							contactListTimeout = 2000;
							contactListOverrideState = MENU_CONTACT_LIST_TG_IN_RXGROUP;
							voicePromptsAppendLanguageString(&currentLanguage->contact_used);
							voicePromptsAppendLanguageString(&currentLanguage->in_rx_group);
							voicePromptsPlay();
						}
						else
						{
							contactListOverrideState = MENU_CONTACT_LIST_CONFIRM;
							voicePromptsAppendLanguageString(&currentLanguage->delete_contact_qm);
							voicePromptsPlay();
						}
					}
					menuSystemPopPreviousMenu();
				}
				break;
		}
	}
	else if (KEYCHECK_PRESS(ev->keys, KEY_DOWN))
	{
		menuSystemMenuIncrement(&menuDataGlobal.currentItemIndex, ((contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL) ? NUM_CONTACT_LIST_QUICK_MENU_ITEMS : 1));
		updateSubMenuScreen();
	}
	else if (KEYCHECK_PRESS(ev->keys, KEY_UP))
	{
		menuSystemMenuDecrement(&menuDataGlobal.currentItemIndex, ((contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL) ? NUM_CONTACT_LIST_QUICK_MENU_ITEMS : 1));
		updateSubMenuScreen();
	}

	if ((menuDataGlobal.currentItemIndex == CONTACT_LIST_QUICK_MENU_SELECT) && (contactListType == MENU_CONTACT_LIST_CONTACT_DIGITAL) &&
			(KEYCHECK_SHORTUP_NUMBER(ev->keys) && (BUTTONCHECK_DOWN(ev, BUTTON_SK2))))
	{
		saveQuickkeyContactIndex(ev->keys.key, (uint16_t)contactListContactData.NOT_IN_CODEPLUGDATA_indexNumber);
		return;
	}

}

menuStatus_t menuContactListSubMenu(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		updateSubMenuScreen();
		keyboardInit();
		menuContactListSubMenuExitCode = (MENU_STATUS_LIST_TYPE | MENU_STATUS_SUCCESS);
	}
	else
	{
		menuContactListSubMenuExitCode = MENU_STATUS_SUCCESS;

		if (ev->hasEvent)
		{
			handleSubMenuEvent(ev);
		}
	}

	dtmfSequenceTick(true);

	return menuContactListSubMenuExitCode;
}
