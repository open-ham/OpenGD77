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

static menuStatus_t menuSoundExitCode = MENU_STATUS_SUCCESS;

enum SOUND_MENU_LIST { OPTIONS_MENU_TIMEOUT_BEEP = 0, OPTIONS_MENU_BEEP_VOLUME, OPTIONS_MENU_DMR_BEEP, OPTIONS_MIC_GAIN_DMR, OPTIONS_MIC_GAIN_FM,
	OPTIONS_VOX_THRESHOLD, OPTIONS_VOX_TAIL, OPTIONS_AUDIO_PROMPT_MODE,
	NUM_SOUND_MENU_ITEMS};

menuStatus_t menuSoundOptions(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		menuDataGlobal.menuOptionsSetQuickkey = 0;
		menuDataGlobal.menuOptionsTimeout = 0;
		menuDataGlobal.newOptionSelected = true;
		menuDataGlobal.endIndex = NUM_SOUND_MENU_ITEMS;

		if (originalNonVolatileSettings.magicNumber == 0xDEADBEEF)
		{
			// Store original settings, used on cancel event.
			memcpy(&originalNonVolatileSettings, &nonVolatileSettings, sizeof(settingsStruct_t));
		}

		voicePromptsInit();
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendLanguageString(&currentLanguage->sound_options);
		voicePromptsAppendLanguageString(&currentLanguage->menu);
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendPrompt(PROMPT_SILENCE);

		updateScreen(isFirstRun);
		return (MENU_STATUS_LIST_TYPE | MENU_STATUS_SUCCESS);
	}
	else
	{
		menuSoundExitCode = MENU_STATUS_SUCCESS;

		if (ev->hasEvent || (menuDataGlobal.menuOptionsTimeout > 0))
		{
			handleEvent(ev);
		}
	}
	return menuSoundExitCode;
}


static void updateScreen(bool isFirstRun)
{
	int mNum = 0;
	static const int bufferLen = 17;
	char buf[bufferLen];
	char * const *leftSide = NULL;// initialise to please the compiler
	char * const *rightSideConst = NULL;// initialise to please the compiler
	char rightSideVar[bufferLen];
	voicePrompt_t rightSideUnitsPrompt;
	const char * rightSideUnitsStr;

	ucClearBuf();
	bool settingOption = uiShowQuickKeysChoices(buf, bufferLen,currentLanguage->sound_options);

	// Can only display 3 of the options at a time menu at -1, 0 and +1
	for(int i = -1; i <= 1; i++)
	{
		if ((settingOption == false) || (i == 0))
		{
			mNum = menuGetMenuOffset(NUM_SOUND_MENU_ITEMS, i);
			buf[0] = 0;
			leftSide = NULL;
			rightSideConst = NULL;
			rightSideVar[0] = 0;
			rightSideUnitsPrompt = PROMPT_SILENCE;// use PROMPT_SILENCE as flag that the unit has not been set
			rightSideUnitsStr = NULL;

			switch(mNum)
			{
				case OPTIONS_MENU_TIMEOUT_BEEP:
					leftSide = (char * const *)&currentLanguage->timeout_beep;
					if (nonVolatileSettings.audioPromptMode == AUDIO_PROMPT_MODE_SILENT)
					{
						rightSideConst = (char * const *)&currentLanguage->n_a;
					}
					else
					{
						if (nonVolatileSettings.txTimeoutBeepX5Secs != 0)
						{
							snprintf(rightSideVar, bufferLen, "%d", nonVolatileSettings.txTimeoutBeepX5Secs * 5);
							rightSideUnitsPrompt = PROMPT_SECONDS;
							rightSideUnitsStr = "s";
						}
						else
						{
							rightSideConst = (char * const *)&currentLanguage->n_a;
						}
					}
					break;
				case OPTIONS_MENU_BEEP_VOLUME: // Beep volume reduction
					leftSide = (char * const *)&currentLanguage->beep_volume;
					if (nonVolatileSettings.audioPromptMode == AUDIO_PROMPT_MODE_SILENT)
					{
						rightSideConst = (char * const *)&currentLanguage->n_a;
					}
					else
					{
						snprintf(rightSideVar, bufferLen, "%ddB", (2 - nonVolatileSettings.beepVolumeDivider) * 3);
						soundBeepVolumeDivider = nonVolatileSettings.beepVolumeDivider;
					}

					break;
				case OPTIONS_MENU_DMR_BEEP:
					leftSide = (char * const *)&currentLanguage->dmr_beep;
					if (nonVolatileSettings.audioPromptMode == AUDIO_PROMPT_MODE_SILENT)
					{
						rightSideConst = (char * const *)&currentLanguage->n_a;
					}
					else
					{
						const char * const *beepTX[] = { &currentLanguage->none, &currentLanguage->start, &currentLanguage->stop, &currentLanguage->both };
						rightSideConst = (char * const *)beepTX[nonVolatileSettings.beepOptions];
					}
					break;
				case OPTIONS_MIC_GAIN_DMR: // DMR Mic gain
					leftSide = (char * const *)&currentLanguage->dmr_mic_gain;
					snprintf(rightSideVar, bufferLen, "%ddB", (nonVolatileSettings.micGainDMR - 11) * 3);
					break;
				case OPTIONS_MIC_GAIN_FM: // FM Mic gain
					leftSide = (char * const *)&currentLanguage->fm_mic_gain;
					snprintf(rightSideVar, bufferLen, "%d", (nonVolatileSettings.micGainFM - 16));
					break;
				case OPTIONS_VOX_THRESHOLD:
					leftSide = (char * const *)&currentLanguage->vox_threshold;
					snprintf(rightSideVar, bufferLen, "%d", nonVolatileSettings.voxThreshold);
					break;
				case OPTIONS_VOX_TAIL:
					leftSide = (char * const *)&currentLanguage->vox_tail;
					if (nonVolatileSettings.voxThreshold != 0)
					{
						float tail = (nonVolatileSettings.voxTailUnits * 0.5);
						uint8_t secs = (uint8_t)tail;
						uint8_t fracSec = (tail - secs) * 10;

						snprintf(rightSideVar, bufferLen, "%d.%d", secs, fracSec);
						rightSideUnitsPrompt = PROMPT_SECONDS;
						rightSideUnitsStr = "s";
					}
					else
					{
						rightSideConst = (char * const *)&currentLanguage->n_a;
					}
					break;
				case OPTIONS_AUDIO_PROMPT_MODE:
					{
						leftSide = (char * const *)&currentLanguage->audio_prompt;
						const char * const *audioPromptOption[] = { &currentLanguage->silent, &currentLanguage->beep,
								&currentLanguage->voice_prompt_level_1, &currentLanguage->voice_prompt_level_2, &currentLanguage->voice_prompt_level_3 };
						rightSideConst = (char * const *)audioPromptOption[nonVolatileSettings.audioPromptMode];
					}
					break;
			}

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

	if (ev->events & BUTTON_EVENT)
	{
		if (repeatVoicePromptOnSK1(ev))
		{
			return;
		}
	}

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
		if ((QUICKKEY_TYPE(ev->function) == QUICKKEY_MENU) && (QUICKKEY_ENTRYID(ev->function) < NUM_SOUND_MENU_ITEMS))
		{
			menuDataGlobal.currentItemIndex = QUICKKEY_ENTRYID(ev->function);
		}
		if ((QUICKKEY_FUNCTIONID(ev->function) != 0))
		{
			menuDataGlobal.menuOptionsTimeout = 1000;
		}
	}


	if ((ev->events & KEY_EVENT) && (menuDataGlobal.menuOptionsSetQuickkey == 0) && (menuDataGlobal.menuOptionsTimeout == 0))
	{
		if (KEYCHECK_PRESS(ev->keys, KEY_DOWN) && (menuDataGlobal.endIndex != 0))
		{
			isDirty = true;
			menuSystemMenuIncrement(&menuDataGlobal.currentItemIndex, NUM_SOUND_MENU_ITEMS);
			menuDataGlobal.newOptionSelected = true;
			menuSoundExitCode |= MENU_STATUS_LIST_TYPE;
		}
		else if (KEYCHECK_PRESS(ev->keys, KEY_UP))
		{
			isDirty = true;
			menuSystemMenuDecrement(&menuDataGlobal.currentItemIndex, NUM_SOUND_MENU_ITEMS);
			menuDataGlobal.newOptionSelected = true;
			menuSoundExitCode |= MENU_STATUS_LIST_TYPE;
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
			// Restore original settings.
			memcpy(&nonVolatileSettings, &originalNonVolatileSettings, sizeof(settingsStruct_t));
			soundBeepVolumeDivider = nonVolatileSettings.beepVolumeDivider;
			setMicGainDMR(nonVolatileSettings.micGainDMR);
			trxSetMicGainFM(nonVolatileSettings.micGainFM);
			voxSetParameters(nonVolatileSettings.voxThreshold, nonVolatileSettings.voxTailUnits);
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
		if (KEYCHECK_PRESS(ev->keys, KEY_RIGHT) || (QUICKKEY_FUNCTIONID(ev->function) == FUNC_RIGHT))
		{
			isDirty = true;
			menuDataGlobal.newOptionSelected = false;
			switch(menuDataGlobal.currentItemIndex)
			{
				case OPTIONS_MENU_TIMEOUT_BEEP:
					if (nonVolatileSettings.audioPromptMode != AUDIO_PROMPT_MODE_SILENT)
					{
						if (nonVolatileSettings.txTimeoutBeepX5Secs < 4)
						{
							settingsIncrement(nonVolatileSettings.txTimeoutBeepX5Secs, 1);
						}
					}
					break;
				case OPTIONS_MENU_BEEP_VOLUME:
					if (nonVolatileSettings.audioPromptMode != AUDIO_PROMPT_MODE_SILENT)
					{
						if (nonVolatileSettings.beepVolumeDivider > 0)
						{
							settingsDecrement(nonVolatileSettings.beepVolumeDivider, 1);
						}
					}
					break;
				case OPTIONS_MENU_DMR_BEEP:
					if (nonVolatileSettings.audioPromptMode != AUDIO_PROMPT_MODE_SILENT)
					{
						if (nonVolatileSettings.beepOptions < (BEEP_TX_START | BEEP_TX_STOP))
						{
							settingsIncrement(nonVolatileSettings.beepOptions, 1);
						}
					}
					break;
				case OPTIONS_MIC_GAIN_DMR: // DMR Mic gain
					if (nonVolatileSettings.micGainDMR < 15)
					{
						settingsIncrement(nonVolatileSettings.micGainDMR, 1);
						setMicGainDMR(nonVolatileSettings.micGainDMR);
					}
					break;
				case OPTIONS_MIC_GAIN_FM: // FM Mic gain
					if (nonVolatileSettings.micGainFM < 31)
					{
						settingsIncrement(nonVolatileSettings.micGainFM, 1);
						trxSetMicGainFM(nonVolatileSettings.micGainFM);
					}
					break;
				case OPTIONS_VOX_THRESHOLD:
					if (nonVolatileSettings.voxThreshold < 30)
					{
						settingsIncrement(nonVolatileSettings.voxThreshold, 1);
						voxSetParameters(nonVolatileSettings.voxThreshold, nonVolatileSettings.voxTailUnits);
					}
					break;
				case OPTIONS_VOX_TAIL:
					if (nonVolatileSettings.voxTailUnits < 10) // 5 seconds max
					{
						settingsIncrement(nonVolatileSettings.voxTailUnits, 1);
						voxSetParameters(nonVolatileSettings.voxThreshold, nonVolatileSettings.voxTailUnits);
					}
					break;
				case OPTIONS_AUDIO_PROMPT_MODE:
					if (nonVolatileSettings.audioPromptMode < (NUM_AUDIO_PROMPT_MODES - 2 + (int)voicePromptDataIsLoaded))
					{
						if ((nonVolatileSettings.audioPromptMode == AUDIO_PROMPT_MODE_BEEP) && !voicePromptDataIsLoaded)
						{
							soundSetMelody(MELODY_ERROR_BEEP);
						}
						else
						{
							settingsIncrement(nonVolatileSettings.audioPromptMode, 1);
						}
					}
					break;

			}
		}
		else if (KEYCHECK_PRESS(ev->keys, KEY_LEFT) || (QUICKKEY_FUNCTIONID(ev->function) == FUNC_LEFT))
		{
			isDirty = true;
			menuDataGlobal.newOptionSelected = false;
			switch(menuDataGlobal.currentItemIndex)
			{
				case OPTIONS_MENU_TIMEOUT_BEEP:
					if (nonVolatileSettings.audioPromptMode != AUDIO_PROMPT_MODE_SILENT)
					{
						if (nonVolatileSettings.txTimeoutBeepX5Secs > 0)
						{
							settingsDecrement(nonVolatileSettings.txTimeoutBeepX5Secs, 1);
						}
					}
					break;
				case OPTIONS_MENU_BEEP_VOLUME:
					if (nonVolatileSettings.audioPromptMode != AUDIO_PROMPT_MODE_SILENT)
					{
						if (nonVolatileSettings.beepVolumeDivider < 10)
						{
							settingsIncrement(nonVolatileSettings.beepVolumeDivider, 1);
						}
					}
					break;
				case OPTIONS_MENU_DMR_BEEP:
					if (nonVolatileSettings.audioPromptMode != AUDIO_PROMPT_MODE_SILENT)
					{
						if (nonVolatileSettings.beepOptions > BEEP_TX_NONE)
						{
							settingsDecrement(nonVolatileSettings.beepOptions, 1);
						}
					}
					break;
				case OPTIONS_MIC_GAIN_DMR: // DMR Mic gain
					if (nonVolatileSettings.micGainDMR > 0)
					{
						settingsDecrement(nonVolatileSettings.micGainDMR, 1);
						setMicGainDMR(nonVolatileSettings.micGainDMR);
					}
					break;
				case OPTIONS_MIC_GAIN_FM: // FM Mic gain
					if (nonVolatileSettings.micGainFM > 1) // Limit to min 1, as 0: no audio
					{
						settingsDecrement(nonVolatileSettings.micGainFM, 1);
						trxSetMicGainFM(nonVolatileSettings.micGainFM);
					}
					break;
				case OPTIONS_VOX_THRESHOLD:
					// threshold of 1 is too low. So only allow the value to go down to 2.
					if (nonVolatileSettings.voxThreshold > 2)
					{
						settingsDecrement(nonVolatileSettings.voxThreshold, 1);
						voxSetParameters(nonVolatileSettings.voxThreshold, nonVolatileSettings.voxTailUnits);
					}
					break;
				case OPTIONS_VOX_TAIL:
					if (nonVolatileSettings.voxTailUnits > 1) // .5 minimum
					{
						settingsDecrement(nonVolatileSettings.voxTailUnits, 1);
						voxSetParameters(nonVolatileSettings.voxThreshold, nonVolatileSettings.voxTailUnits);
					}
					break;
				case OPTIONS_AUDIO_PROMPT_MODE:
					if (nonVolatileSettings.audioPromptMode > AUDIO_PROMPT_MODE_SILENT)
					{
						// Stop the voice prompt playback as soon as the level is set to 'Beep'
						if ((nonVolatileSettings.audioPromptMode == AUDIO_PROMPT_MODE_VOICE_LEVEL_1) && voicePromptsIsPlaying())
						{
							voicePromptsTerminate();
						}

						settingsDecrement(nonVolatileSettings.audioPromptMode, 1);
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
			menuSoundExitCode |= MENU_STATUS_ERROR;
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
