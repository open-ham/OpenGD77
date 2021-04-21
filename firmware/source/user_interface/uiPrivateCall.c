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

static bool privateCallCallback(void);
static bool handled = false;

menuStatus_t menuPrivateCall(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		static const int bufferLen = 17;
		char buffer[bufferLen];
		dmrIdDataStruct_t currentRec;

		// privateCallCallback() has been called, so this screen is
		// reloading, but we need to leave it ASAP
		if (handled)
		{
			handled = false;
			keyboardReset();
			menuSystemPopPreviousMenu();
			return MENU_STATUS_SUCCESS;
		}

		soundSetMelody(MELODY_PRIVATE_CALL);
		uiDataGlobal.PrivateCall.state = PRIVATE_CALL_ACCEPT;
		uiDataGlobal.receivedPcId = LinkHead->id;
		uiDataGlobal.receivedPcTS = (dmrMonitorCapturedTS != -1) ? dmrMonitorCapturedTS : trxGetDMRTimeSlot();

		if (!contactIDLookup(uiDataGlobal.receivedPcId, CONTACT_CALLTYPE_PC, buffer))
		{
			dmrIDLookup(uiDataGlobal.receivedPcId, &currentRec);
			strncpy(buffer, currentRec.text, 16);
			buffer[16] = 0;
		}

		snprintf(uiDataGlobal.MessageBox.message, MESSAGEBOX_MESSAGE_LEN_MAX, "%s\n%s\n%s",
		currentLanguage->private_call, currentLanguage->accept_call, buffer);
		uiDataGlobal.MessageBox.type = MESSAGEBOX_TYPE_INFO;
		uiDataGlobal.MessageBox.buttons = MESSAGEBOX_BUTTONS_YESNO;
		uiDataGlobal.MessageBox.decoration = MESSAGEBOX_DECORATION_NONE;
		uiDataGlobal.MessageBox.validatorCallback = privateCallCallback;

		menuSystemPushNewMenu(UI_MESSAGE_BOX);
	}

	return MENU_STATUS_SUCCESS;
}

void menuPrivateCallClear(void)
{
	uiDataGlobal.PrivateCall.state = PRIVATE_CALL_NOT_IN_CALL;
	uiDataGlobal.PrivateCall.lastID = 0;
	uiDataGlobal.tgBeforePcMode = 0;
	uiDataGlobal.receivedPcId = 0x00;
}

void menuPrivateCallDismiss(void)
{
	handled = true;
	uiDataGlobal.MessageBox.validatorCallback = NULL;
	menuSystemPopPreviousMenu();
}

static bool privateCallCallback(void)
{
	if (uiDataGlobal.MessageBox.keyPressed == KEY_RED)
	{
		uiDataGlobal.PrivateCall.state = PRIVATE_CALL_DECLINED;
		uiDataGlobal.PrivateCall.lastID = 0;
	}
	else if (uiDataGlobal.MessageBox.keyPressed == KEY_GREEN)
	{
		acceptPrivateCall(uiDataGlobal.receivedPcId, uiDataGlobal.receivedPcTS);
	}

	handled = true;

	return true;
}
