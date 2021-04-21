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
#include "user_interface/uiUtilities.h"
#include "functions/codeplug.h"
#include "user_interface/uiLocalisation.h"
#include "functions/ticks.h"

static void updateScreen(void);
static void handleEvent(uiEvent_t *ev);
static void exitSplashScreen(void);

#if ! defined(PLATFORM_GD77S)
static bool validatePinCodeCallback(void);
static void pincodeAudioAlert(void);
#endif

const uint32_t SILENT_PROMPT_HOLD_DURATION_MILLISECONDS = 2000;
static uint32_t initialEventTime;

#if ! defined(PLATFORM_GD77S)
static bool pinHandled = false;
static int32_t pinCode;
#endif

menuStatus_t uiSplashScreen(uiEvent_t *ev, bool isFirstRun)
{
	uint8_t melodyBuf[512];

	if (isFirstRun)
	{
#if ! defined(PLATFORM_GD77S)
		if (pinHandled)
		{
			pinHandled = false;
			keyboardReset();
		}
		else
		{
			int pinLength = codeplugGetPasswordPin(&pinCode);

			if (pinLength != 0)
			{
				snprintf(uiDataGlobal.MessageBox.message, MESSAGEBOX_MESSAGE_LEN_MAX, "%s", currentLanguage->pin_code);
				uiDataGlobal.MessageBox.type = MESSAGEBOX_TYPE_PIN_CODE;
				uiDataGlobal.MessageBox.pinLength = pinLength;
				uiDataGlobal.MessageBox.validatorCallback = validatePinCodeCallback;
				menuSystemPushNewMenu(UI_MESSAGE_BOX);

				addTimerCallback(pincodeAudioAlert, 500, false);// Need to delay playing this for a while, because otherwise it may get played before the volume is turned up enough to hear it.
				return MENU_STATUS_SUCCESS;
			}
		}
#endif

		initialEventTime = ev->time;

#if defined(PLATFORM_GD77S)
		// Don't play boot melody when the 77S is already speaking, otherwise if will mute the speech halfway
		if (voicePromptsIsPlaying() == false)
#endif
		{
			if (codeplugGetOpenGD77CustomData(CODEPLUG_CUSTOM_DATA_TYPE_BEEP, melodyBuf))
			{
				if ((melodyBuf[0] == 0) && (melodyBuf[1] == 0))
				{
					exitSplashScreen();
					return MENU_STATUS_SUCCESS;
				}
				else
				{
					soundCreateSong(melodyBuf);
					soundSetMelody(melody_generic);
				}

			}
			else
			{
				soundSetMelody(MELODY_POWER_ON);
			}
		}

		updateScreen();
	}
	else
	{
		handleEvent(ev);
	}

	return MENU_STATUS_SUCCESS;
}

static void updateScreen(void)
{
	char line1[17];
	char line2[17];
	uint8_t bootScreenType;
	bool customDataHasImage = false;

	codeplugGetBootScreenData(line1, line2, &bootScreenType);

	strcpy(talkAliasText, line1);
	strcat(talkAliasText, line2);

	if (bootScreenType == 0)
	{
		customDataHasImage = codeplugGetOpenGD77CustomData(CODEPLUG_CUSTOM_DATA_TYPE_IMAGE, ucGetDisplayBuffer());
	}

	if (!customDataHasImage)
	{
		ucClearBuf();

#if defined(PLATFORM_RD5R)
		ucPrintCentered(0, "OpenRD5R", FONT_SIZE_3);
#elif defined(PLATFORM_GD77)
		ucPrintCentered(8, "OpenGD77", FONT_SIZE_3);
#elif defined(PLATFORM_DM1801)
		ucPrintCentered(8, "OpenDM1801", FONT_SIZE_3);
#endif
		ucPrintCentered((DISPLAY_SIZE_Y / 4) * 2, line1, FONT_SIZE_3);
		ucPrintCentered((DISPLAY_SIZE_Y / 4) * 3, line2, FONT_SIZE_3);
	}

	ucRender();
}

static void handleEvent(uiEvent_t *ev)
{
	if (nonVolatileSettings.audioPromptMode != AUDIO_PROMPT_MODE_SILENT)
	{
		if ((melody_play == NULL) || (ev->events != NO_EVENT))
		{
			// Any button or key event, stop the melody then leave
			if (melody_play != NULL)
			{
				soundStopMelody();
			}

			exitSplashScreen();
		}
	}
	else
	{
		if ((ev->time - initialEventTime) > SILENT_PROMPT_HOLD_DURATION_MILLISECONDS)
		{
			exitSplashScreen();
		}
	}
}

static void exitSplashScreen(void)
{
	menuSystemSetCurrentMenu(nonVolatileSettings.initialMenuNumber);
}

#if ! defined(PLATFORM_GD77S)
static bool validatePinCodeCallback(void)
{
	if (uiDataGlobal.MessageBox.keyPressed == KEY_GREEN)
	{
		if (freqEnterRead(0, uiDataGlobal.MessageBox.pinLength) == pinCode)
		{
			pinHandled = true;
			return true;
		}
	}
	else if (uiDataGlobal.MessageBox.keyPressed == KEY_RED)
	{
		freqEnterReset();
	}

	return false;
}

static void pincodeAudioAlert(void)
{
	if (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1)
	{
		voicePromptsInit();
		voicePromptsAppendLanguageString(&currentLanguage->pin_code);
		voicePromptsPlay();
	}
	else
	{
		soundSetMelody(MELODY_ACK_BEEP);
	}
}

#endif
