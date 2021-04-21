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
#include "hardware/HR-C6000.h"
#include "functions/settings.h"
#include "functions/sound.h"
#include "user_interface/menuSystem.h"
#include "user_interface/uiUtilities.h"
#include "user_interface/uiLocalisation.h"

#include "interfaces/clockManager.h"

typedef enum
{
	TXSTOP_TIMEOUT,
	TXSTOP_RX_ONLY,
	TXSTOP_OUT_OF_BAND
} txTerminationReason_t;

static void updateScreen(void);
static void handleEvent(uiEvent_t *ev);
static void handleTxTermination(uiEvent_t *ev, txTerminationReason_t reason);

static const int PIT_COUNTS_PER_SECOND = 10000;
static int timeInSeconds;
static uint32_t nextSecondPIT;
static bool isShowingLastHeard;
static bool startBeepPlayed;
static uint32_t m = 0, micm = 0, mto = 0;
static uint32_t xmitErrorTimer = 0;
static bool keepScreenShownOnError = false;
static bool pttWasReleased = false;
static bool isTransmittingTone = false;



menuStatus_t menuTxScreen(uiEvent_t *ev, bool isFirstRun)
{

	if (isFirstRun)
	{
		voicePromptsTerminate();
		startBeepPlayed = false;
		uiDataGlobal.Scan.active = false;
		isTransmittingTone = false;
		isShowingLastHeard = false;
		keepScreenShownOnError = false;
		timeInSeconds = 0;
		pttWasReleased = false;

		if (trxGetMode() == RADIO_MODE_DIGITAL)
		{
			clockManagerSetRunMode(kAPP_PowerModeHsrun);
		}

		// If the user was currently entering a new frequency and the PTT get pressed, "leave" that input screen.
		if (uiDataGlobal.FreqEnter.index > 0)
		{
			freqEnterReset();
			updateScreen();
		}

		if (((currentChannelData->flag4 & 0x04) == 0x00) && ((nonVolatileSettings.txFreqLimited == BAND_LIMITS_NONE) || trxCheckFrequencyInAmateurBand(currentChannelData->txFreq)))
		{
			nextSecondPIT = PITCounter + PIT_COUNTS_PER_SECOND;
			timeInSeconds = currentChannelData->tot * 15;

			LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
			LEDs_PinWrite(GPIO_LEDred, Pin_LEDred, 1);

			txstopdelay = 0;
			clearIsWakingState();
			if (trxGetMode() == RADIO_MODE_ANALOG)
			{
				trxSetTxCSS(currentChannelData->txTone);
				trxSetTX();
			}
			else
			{
				// RADIO_MODE_DIGITAL
				if (!((slot_state >= DMR_STATE_REPEATER_WAKE_1) && (slot_state <= DMR_STATE_REPEATER_WAKE_3)) )
				{
					trxSetTX();
				}
			}

			updateScreen();
		}
		else
		{
			handleTxTermination(ev, (((currentChannelData->flag4 & 0x04) != 0x00) ? TXSTOP_RX_ONLY : TXSTOP_OUT_OF_BAND));
		}

		m = micm = ev->time;
	}
	else
	{

#if defined(PLATFORM_GD77S)
		uiChannelModeHeartBeatActivityForGD77S(ev);
#endif

		// Keep displaying the "RX Only" or "Out Of Band" error message
		if (xmitErrorTimer > 0)
		{
			// Wait the voice ends, then count-down 200ms;
			if (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1)
			{
				if (voicePromptsIsPlaying())
				{
					xmitErrorTimer = (20 * 10U);
					return MENU_STATUS_SUCCESS;
				}
			}

			xmitErrorTimer--;

			if (xmitErrorTimer == 0)
			{
				ev->buttons &= ~BUTTON_PTT; // prevent screen lockout if the operator keeps pressing the PTT button.
			}
			else
			{
				return MENU_STATUS_SUCCESS;
			}
		}

		if (trxTransmissionEnabled && (getIsWakingState() == WAKING_MODE_NONE))
		{
			if (PITCounter >= nextSecondPIT)
			{
				if (currentChannelData->tot == 0)
				{
					timeInSeconds++;
				}
				else
				{
					timeInSeconds--;
					if (timeInSeconds <= (nonVolatileSettings.txTimeoutBeepX5Secs * 5))
					{
						if ((timeInSeconds % 5) == 0)
						{
							soundSetMelody(MELODY_KEY_BEEP);
						}
					}
				}

				if ((currentChannelData->tot != 0) && (timeInSeconds == 0))
				{
					handleTxTermination(ev, TXSTOP_TIMEOUT);
					keepScreenShownOnError = true;
				}
				else
				{
					if (!isShowingLastHeard)
					{
						updateScreen();
					}
				}

				nextSecondPIT = PITCounter + PIT_COUNTS_PER_SECOND;
			}
			else
			{
				int mode = trxGetMode();

				if (mode == RADIO_MODE_DIGITAL)
				{
					if ((nonVolatileSettings.beepOptions & BEEP_TX_START) &&
							(startBeepPlayed == false) && (trxIsTransmitting == true)
							&& (melody_play == NULL))
					{
						startBeepPlayed = true;// set this even if the beep is not actaully played because of the vox, as otherwise this code will get continuously run
						// If VOX is running, don't send a beep as it will reset its the trigger status.
						if ((voxIsEnabled() == false) || (voxIsEnabled() && (voxIsTriggered() == false)))
						{
							soundSetMelody(MELODY_DMR_TX_START_BEEP);
						}
					}
				}

				// Do not update Mic level on Timeout.
				if ((((currentChannelData->tot != 0) && (timeInSeconds == 0)) == false) && (ev->time - micm) > 100)
				{
					if (mode == RADIO_MODE_DIGITAL)
					{
						uiUtilityDrawDMRMicLevelBarGraph();
					}
					else
					{
						uiUtilityDrawFMMicLevelBarGraph();
					}

					ucRenderRows(1, 2);
					micm = ev->time;
				}

			}
		}

		// Timeout happened, postpone going further otherwise timeout
		// screen won't be visible at all.
		if (((currentChannelData->tot != 0) && (timeInSeconds == 0)) || keepScreenShownOnError)
		{
			// Wait the voice ends, then count-down 500ms;
			if (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1)
			{
				if (voicePromptsIsPlaying())
				{
					mto = ev->time;
					return MENU_STATUS_SUCCESS;
				}
			}

			if ((ev->time - mto) < 500)
			{
				return MENU_STATUS_SUCCESS;
			}

			keepScreenShownOnError = false;
			ev->buttons &= ~BUTTON_PTT; // prevent screen lockout if the operator keeps pressing the PTT button.
		}

		// Once the PTT key has been released, it is forbidden to re-key before the whole TX chain
		// has finished (see the first statement in handleEvent())
		// That's important in DMR mode, otherwise quickly press/release the PTT key will left
		// the system in an unexpected state (RED led on, displayed TXScreen, but PA off).
		// It doesn't have any impact on FM mode.
		if (((ev->buttons & BUTTON_PTT) == 0) && (pttWasReleased == false))
		{
			pttWasReleased = true;
		}

		if ((ev->buttons & BUTTON_PTT) && pttWasReleased)
		{
			ev->buttons &= ~BUTTON_PTT;
		}
		//


		// Got an event, or
		if (ev->hasEvent || // PTT released, Timeout triggered,
				( (((ev->buttons & BUTTON_PTT) == 0) || ((currentChannelData->tot != 0) && (timeInSeconds == 0))) ||
						// or waiting for DMR ending (meanwhile, updating every 100ms)
						((trxTransmissionEnabled == false) && ((ev->time - m) > 100))))
		{
			handleEvent(ev);
			m = ev->time;
		}
		else
		{
			if ((getIsWakingState() == WAKING_MODE_FAILED) && (trxTransmissionEnabled == true))
			{
				trxTransmissionEnabled = false;
				handleTxTermination(ev, TXSTOP_TIMEOUT);
				keepScreenShownOnError = true;
			}
		}
	}
	return MENU_STATUS_SUCCESS;
}

bool menuTxScreenDisplaysLastHeard(void)
{
	return isShowingLastHeard;
}

static void updateScreen(void)
{
	uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
	if (menuDataGlobal.controlData.stack[0] == UI_VFO_MODE)
	{
		uiVFOModeUpdateScreen(timeInSeconds);
	}
	else
	{
		uiChannelModeUpdateScreen(timeInSeconds);
	}

	if (nonVolatileSettings.backlightMode != BACKLIGHT_MODE_BUTTONS)
	{
		displayLightOverrideTimeout(-1);
	}
}

static void handleEvent(uiEvent_t *ev)
{
	// Xmiting ends (normal or timeouted)
	if (((ev->buttons & BUTTON_PTT) == 0)
			|| ((currentChannelData->tot != 0) && (timeInSeconds == 0)))
	{
		if (trxTransmissionEnabled)
		{
			trxTransmissionEnabled = false;
			isTransmittingTone = false;

			if (trxGetMode() == RADIO_MODE_ANALOG)
			{
				// In analog mode. Stop transmitting immediately
				LEDs_PinWrite(GPIO_LEDred, Pin_LEDred, 0);

				// Need to wrap this in Task Critical to avoid bus contention on the I2C bus.
				trxSetRxCSS(currentChannelData->rxTone);
				//taskENTER_CRITICAL();
				trxActivateRx();
				trxIsTransmitting = false;
				//taskEXIT_CRITICAL();

				menuSystemPopPreviousMenu();
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN; // we need immediate redraw
			}
			else
			{
				if (isShowingLastHeard)
				{
					isShowingLastHeard = false;
					updateScreen();
				}
			}
			// When not in analog mode, only the trxIsTransmitting flag is cleared
			// This screen keeps getting called via the handleEvent function and goes into the else clause - below.
		}
		else
		{
			// In DMR mode, wait for the DMR system to finish before exiting
			if (trxIsTransmitting == false)
			{
				if ((nonVolatileSettings.beepOptions & BEEP_TX_STOP) && (melody_play == NULL))
				{
					soundSetMelody(MELODY_DMR_TX_STOP_BEEP);
				}

				LEDs_PinWrite(GPIO_LEDred, Pin_LEDred, 0);

				// If there is a signal, lit the Green LED
				if ((LEDs_PinRead(GPIO_LEDgreen, Pin_LEDgreen) == 0) && (trxCarrierDetected() || (getAudioAmpStatus() & AUDIO_AMP_MODE_RF)))
				{
					LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 1);
				}

				if (trxGetMode() == RADIO_MODE_DIGITAL)
				{
					clockManagerSetRunMode(kAPP_PowerModeRun);
				}

				menuSystemPopPreviousMenu();
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN; // we need immediate redraw
			}
		}

		// Waiting for TX to end, no need to go further.
		return;
	}

	// Key action while xmitting (ANALOG), Tone triggering
	if (!isTransmittingTone && ((ev->buttons & BUTTON_PTT) != 0) && trxTransmissionEnabled && (trxGetMode() == RADIO_MODE_ANALOG))
	{
		// Send 1750Hz
		if (BUTTONCHECK_DOWN(ev, BUTTON_SK2))
		{
			isTransmittingTone = true;
			trxSetTone1(1750);
			trxSelectVoiceChannel(AT1846_VOICE_CHANNEL_TONE1);
			enableAudioAmp(AUDIO_AMP_MODE_RF);
			GPIO_PinWrite(GPIO_RX_audio_mux, Pin_RX_audio_mux, 1);
		}
		else
		{ // Send DTMF
			int keyval = menuGetKeypadKeyValue(ev, false);

			if (keyval != 99)
			{
				trxSetDTMF(keyval);
				isTransmittingTone = true;
				trxSelectVoiceChannel(AT1846_VOICE_CHANNEL_DTMF);
				enableAudioAmp(AUDIO_AMP_MODE_RF);
				GPIO_PinWrite(GPIO_RX_audio_mux, Pin_RX_audio_mux, 1);
			}
		}
	}

	// Stop xmitting Tone
	if (isTransmittingTone && (BUTTONCHECK_DOWN(ev, BUTTON_SK2) == 0) && ((ev->keys.key == 0) || (ev->keys.event & KEY_MOD_UP)))
	{
		isTransmittingTone = false;
		trxSelectVoiceChannel(AT1846_VOICE_CHANNEL_MIC);
		disableAudioAmp(AUDIO_AMP_MODE_RF);
	}

	if ((trxGetMode() == RADIO_MODE_DIGITAL) && BUTTONCHECK_SHORTUP(ev, BUTTON_SK1) && (trxTransmissionEnabled == true))
	{
		isShowingLastHeard = !isShowingLastHeard;
		if (isShowingLastHeard)
		{
			menuLastHeardInit();
			menuLastHeardUpdateScreen(false, false, false);
		}
		else
		{
			updateScreen();
		}
	}

	// Forward key events to LH screen, if shown
	if (isShowingLastHeard && (ev->events & KEY_EVENT))
	{
		menuLastHeardHandleEvent(ev);
	}

}

static void handleTxTermination(uiEvent_t *ev, txTerminationReason_t reason)
{
	PTTToggledDown = false;
	voxReset();

	ucClearBuf();

	voicePromptsTerminate();
	voicePromptsInit();

	ucDrawRoundRectWithDropShadow(4, 4, 120, (DISPLAY_SIZE_Y - 6), 5, true);

	switch (reason)
	{
		case TXSTOP_RX_ONLY:
		case TXSTOP_OUT_OF_BAND:
			ucPrintCentered(4, currentLanguage->error, FONT_SIZE_4);

			voicePromptsAppendLanguageString(&currentLanguage->error);
			voicePromptsAppendPrompt(PROMPT_SILENCE);

			if ((currentChannelData->flag4 & 0x04) != 0x00)
			{
				ucPrintCentered((DISPLAY_SIZE_Y - 24), currentLanguage->rx_only, FONT_SIZE_3);
				voicePromptsAppendLanguageString(&currentLanguage->rx_only);
			}
			else
			{
				ucPrintCentered((DISPLAY_SIZE_Y - 24), currentLanguage->out_of_band, FONT_SIZE_3);
				voicePromptsAppendLanguageString(&currentLanguage->out_of_band);
			}
			xmitErrorTimer = (100 * 10U);
			break;

		case TXSTOP_TIMEOUT:
			ucPrintCentered(16, currentLanguage->timeout, FONT_SIZE_4);
			voicePromptsAppendLanguageString(&currentLanguage->timeout);
			mto = ev->time;
			break;
	}

	ucRender();
	displayLightOverrideTimeout(-1);

	if (nonVolatileSettings.audioPromptMode < AUDIO_PROMPT_MODE_VOICE_LEVEL_1)
	{
		soundSetMelody((reason == TXSTOP_TIMEOUT) ? MELODY_TX_TIMEOUT_BEEP : MELODY_ERROR_BEEP);
	}
	else
	{
		voicePromptsPlay();
	}
}
