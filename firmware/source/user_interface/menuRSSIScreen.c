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
#include "functions/calibration.h"
#include "functions/settings.h"
#include "functions/trx.h"
#include "user_interface/menuSystem.h"
#include "user_interface/uiUtilities.h"
#include "user_interface/uiLocalisation.h"

static menuStatus_t menuRSSIExitCode = MENU_STATUS_SUCCESS;
//static calibrationRSSIMeter_t rssiCalibration; // UNUSED
static void updateScreen(bool forceRedraw, bool firstRun);
static void handleEvent(uiEvent_t *ev);
static void updateVoicePrompts(bool flushIt, bool spellIt);

static int dBm = 0;

static const int barX = 9;
DECLARE_SMETER_ARRAY(rssiMeterBar, (DISPLAY_SIZE_X - (barX - 1)));


menuStatus_t menuRSSIScreen(uiEvent_t *ev, bool isFirstRun)
{
	static uint32_t m = 0;

	if (isFirstRun)
	{
		//calibrationGetRSSIMeterParams(&rssiCalibration); // UNUSED
		menuDataGlobal.endIndex = 0;
		ucClearBuf();
		menuDisplayTitle(currentLanguage->rssi);
		ucRenderRows(0, 2);

		updateScreen(true, true);
	}
	else
	{
		menuRSSIExitCode = MENU_STATUS_SUCCESS;
		if (ev->hasEvent)
		{
			handleEvent(ev);
		}

		if((ev->time - m) > RSSI_UPDATE_COUNTER_RELOAD)
		{
			m = ev->time;
			updateScreen(false, false);
		}
	}

	return menuRSSIExitCode;
}

// Returns S-Unit 0..9..10(S9+10dB)..15(S9+60)
static int32_t getSignalStrength(int dBm)
{
	if (dBm < DBM_LEVELS[1])
	{
		return 0;
	}

	for (int8_t i = 15; i >= 0; i--)
	{
		if (dBm >= DBM_LEVELS[i])
		{
			return i;
		}
	}

	return 0;
}

static void updateScreen(bool forceRedraw, bool isFirstRun)
{
	char buffer[17];
	int barWidth;

	dBm = getRSSIdBm();
	int rssi = dBm;

	if (isFirstRun && (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1))
	{
		if (voicePromptsIsPlaying())
		{
			voicePromptsTerminate();
		}
		voicePromptsInit();
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendLanguageString(&currentLanguage->rssi);
		voicePromptsAppendLanguageString(&currentLanguage->menu);
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		updateVoicePrompts(false, true);
	}

	if (forceRedraw)
	{
		// Clear whole drawing region
		ucFillRect(0, 14, DISPLAY_SIZE_X, DISPLAY_SIZE_Y - 14, true);

		// Draw S-Meter outer frame
		ucDrawRect((barX - 2), (DISPLAY_Y_POS_RSSI_BAR - 2), (DISPLAY_SIZE_X - (barX - 2)), (8 + 4), true);
		// Clear the right V line of the frame
		ucDrawFastVLine((DISPLAY_SIZE_X - 1), (DISPLAY_Y_POS_RSSI_BAR - 1), (8 + 2), false);
		// S9+xx H Dots
		for (int16_t i = ((barX - 2) + (rssiMeterBar[9] * 2) + 1); i < DISPLAY_SIZE_X; i += 4)
		{
			ucDrawFastHLine(i, (DISPLAY_Y_POS_RSSI_BAR - 2), 2, false);
		}
		// +10..60dB
		ucFillRect(((barX - 2) + (rssiMeterBar[9] * 2) + 2), (DISPLAY_Y_POS_RSSI_BAR + 8) + 2,
				(DISPLAY_SIZE_X - ((barX - 2) + (rssiMeterBar[9] * 2) + 2)), 2, false);

		// Draw S, Numbers and ticks
		ucPrintAt(1, DISPLAY_Y_POS_RSSI_BAR, "S", FONT_SIZE_1_BOLD);

		int xPos;
		int currentMode = trxGetMode();
		for (uint8_t i = 0; i < 10; i++)
		{
			// Scale the bar graph so values S0 - S9 take 70% of the scale width, and signals above S9 take the last 30%
			// On DMR the max signal is S9+10, so teh entire bar can be the sale scale
			// ON FM signals above S9, the scale is compressed to 2/5
			if ((i <= 9) || (currentMode == RADIO_MODE_DIGITAL))
			{
				xPos = rssiMeterBar[i];
			}
			else
			{
				xPos = ((rssiMeterBar[i] - rssiMeterBar[9]) / 5) + rssiMeterBar[9];
			}
			xPos *= 2;

			// V ticks
			ucDrawFastVLine(((barX - 2) + xPos), (DISPLAY_Y_POS_RSSI_BAR + 8) + 2, ((i % 2) ? 3 : 1), ((i < 10) ? true : false));

			if ((i % 2) && (i < 10))
			{
				char buf[2];

				sprintf(buf, "%d", i);
				ucPrintAt(((((barX - 2) + xPos) - 2) - 1)/* FONT_2 H offset */, DISPLAY_Y_POS_RSSI_BAR + 15
#if defined(PLATFORM_RD5R)
						-1
#endif
						, buf, FONT_SIZE_2);
			}
		}
	}
	else
	{
		// Clear dBm region value
		ucFillRect(((DISPLAY_SIZE_X - (7 * 8)) >> 1), DISPLAY_Y_POS_RSSI_VALUE, (7 * 8), FONT_SIZE_3_HEIGHT, true);
	}

	snprintf(buffer, 17, "%d%s", dBm, "dBm");
	ucPrintCentered(DISPLAY_Y_POS_RSSI_VALUE, buffer, FONT_SIZE_3);

#if 0 // DEBUG
	sprintf(buffer, "%d", trxRxSignal);
	ucFillRect((DISPLAY_SIZE_X - (4 * 8)), DISPLAY_Y_POS_RSSI_VALUE, (4 * 8), 8, true);
	ucPrintCore((DISPLAY_SIZE_X - ((strlen(buffer) + 1) * 8)), DISPLAY_Y_POS_RSSI_VALUE, buffer, FONT_SIZE_2, TEXT_ALIGN_RIGHT, false);
#endif

	if ((rssi > SMETER_S9) && (trxGetMode() == RADIO_MODE_ANALOG))
	{
		// In Analog mode, the max RSSI value from the hardware is over S9+60.
		// So scale this to fit in the last 30% of the display
		rssi = ((rssi - SMETER_S9) / 5) + SMETER_S9;
	}
	// Scale the entire bar by 2.
	// Because above S9 the values are scaled to 1/5. This results in the signal below S9 being doubled in scale
	// Signals above S9 the scales is compressed to 2/5.
	rssi = (rssi - SMETER_S0) * 2;

	barWidth = ((rssi * rssiMeterBarNumUnits) / rssiMeterBarDivider);
	barWidth = CLAMP((barWidth - 1), 0, (DISPLAY_SIZE_X - barX));

	if (barWidth)
	{
		ucFillRect(barX, DISPLAY_Y_POS_RSSI_BAR, barWidth, 8, false);
	}

	// Clear the end of the bar area, if needed
	if (barWidth < (DISPLAY_SIZE_X - barX))
	{
		ucFillRect(barX + barWidth, DISPLAY_Y_POS_RSSI_BAR, (DISPLAY_SIZE_X - barX) - barWidth, 8, true);
	}

	if (forceRedraw)
	{
		ucRender();
	}
	else
	{
#if defined(PLATFORM_RD5R)
		ucRenderRows((DISPLAY_Y_POS_RSSI_VALUE / 8), (DISPLAY_Y_POS_RSSI_VALUE / 8) + 3);
#else
		ucRenderRows((DISPLAY_Y_POS_RSSI_VALUE / 8), (DISPLAY_Y_POS_RSSI_VALUE / 8) + 2);
		ucRenderRows((DISPLAY_Y_POS_RSSI_BAR / 8), (DISPLAY_Y_POS_RSSI_BAR / 8) + 1);
#endif
	}
}

static void handleEvent(uiEvent_t *ev)
{
	if (ev->events & BUTTON_EVENT)
	{
		bool wasPlaying = false;

		if (BUTTONCHECK_SHORTUP(ev, BUTTON_SK1) && (ev->keys.key == 0))
		{
			// Stop playback or update signal strength
			if ((wasPlaying = voicePromptsIsPlaying()) == false)
			{
				updateVoicePrompts(true, false);
			}
		}

		if (repeatVoicePromptOnSK1(ev))
		{
			if (wasPlaying && voicePromptsIsPlaying())
			{
				voicePromptsTerminate();
			}
			return;
		}
	}

	if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN) || KEYCHECK_SHORTUP(ev->keys, KEY_RED))
	{
		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_SHORTUP_NUMBER(ev->keys)  && (BUTTONCHECK_DOWN(ev, BUTTON_SK2)))
	{
		saveQuickkeyMenuIndex(ev->keys.key, menuSystemGetCurrentMenuNumber(), 0, 0);
		return;
	}
}

static void updateVoicePrompts(bool flushIt, bool spellIt)
{
	if (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1)
	{
		uint8_t S = getSignalStrength(dBm);

		if (flushIt)
		{
			voicePromptsInit();
		}

		voicePromptsAppendPrompt(PROMPT_S);
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		if (S > 9)
		{
			voicePromptsAppendPrompt(PROMPT_9);
			voicePromptsAppendPrompt(PROMPT_PLUS);
			voicePromptsAppendInteger(10 * ((S - 10) + 1));
		}
		else
		{
			voicePromptsAppendPrompt(PROMPT_0 + S);
		}

		if (spellIt)
		{
			promptsPlayNotAfterTx();
		}
	}
}
