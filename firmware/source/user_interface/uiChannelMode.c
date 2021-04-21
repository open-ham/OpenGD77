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
#include "functions/settings.h"
#include "functions/trx.h"
#include "user_interface/menuSystem.h"
#include "user_interface/uiUtilities.h"
#include "user_interface/uiLocalisation.h"
#include "functions/voicePrompts.h"
#include "functions/ticks.h"

#if defined(PLATFORM_GD77S)
typedef enum
{
	GD77S_UIMODE_TG_OR_SQUELCH,
	GD77S_UIMODE_SCAN,
	GD77S_UIMODE_TS,
	GD77S_UIMODE_CC,
	GD77S_UIMODE_FILTER,
	GD77S_UIMODE_DTMF_CONTACTS,
	GD77S_UIMODE_ZONE,
	GD77S_UIMODE_POWER,
	GD77S_UIMODE_MAX
} GD77S_UIMODES_t;

typedef struct
{
	bool             firstRun;
	GD77S_UIMODES_t  uiMode;
	bool             channelOutOfBounds;
	uint16_t         dtmfListSelected;
	int32_t          dtmfListCount;
} GD77SParameters_t;

static GD77SParameters_t GD77SParameters =
{
		.firstRun = true,
		.uiMode = GD77S_UIMODE_TG_OR_SQUELCH,
		.channelOutOfBounds = false,
		.dtmfListSelected = 0,
		.dtmfListCount = 0
};

static void buildSpeechUiModeForGD77S(GD77S_UIMODES_t uiMode);

static void checkAndUpdateSelectedChannelForGD77S(uint16_t chanNum, bool forceSpeech);
static void handleEventForGD77S(uiEvent_t *ev);
static uint16_t getCurrentChannelInCurrentZoneForGD77S(void);

#else // ! PLATFORM_GD77S

static void selectPrevNextZone(bool nextZone);
static void handleUpKey(uiEvent_t *ev);
#endif // PLATFORM_GD77S

static void handleEvent(uiEvent_t *ev);
static void loadChannelData(bool useChannelDataInMemory, bool loadVoicePromptAnnouncement);

static void updateQuickMenuScreen(bool isFirstRun);
static void handleQuickMenuEvent(uiEvent_t *ev);

static void scanning(void);
static void scanStart(bool longPressBeep);
static void scanSearchForNextChannel(void);
static void scanApplyNextChannel(void);

static void updateTrxID(void);

static char currentZoneName[17];
static int directChannelNumber = 0;

static struct_codeplugChannel_t channelNextChannelData = { .rxFreq = 0 };
static bool nextChannelReady = false;
static int nextChannelIndex = 0;
static bool quickmenuChannelFromVFOHandled = false; // Quickmenu new channel confirmation window

static menuStatus_t menuChannelExitStatus = MENU_STATUS_SUCCESS;
static menuStatus_t menuQuickChannelExitStatus = MENU_STATUS_SUCCESS;

menuStatus_t uiChannelMode(uiEvent_t *ev, bool isFirstRun)
{
	static uint32_t m = 0, sqm = 0;

	if (isFirstRun)
	{
#if ! defined(PLATFORM_GD77S) // GD77S speech can be triggered in main(), so let it ends.
		voicePromptsTerminate();
#endif

		settingsSet(nonVolatileSettings.initialMenuNumber, (uint8_t) UI_CHANNEL_MODE);// This menu.
		uiDataGlobal.displayChannelSettings = false;
		uiDataGlobal.reverseRepeater = false;
		nextChannelReady = false;
		uiDataGlobal.displaySquelch = false;
		uiDataGlobal.Scan.refreshOnEveryStep = false;

		// We're in digital mode, RXing, and current talker is already at the top of last heard list,
		// hence immediately display complete contact/TG info on screen
		// This mostly happens when getting out of a menu.
		uiDataGlobal.displayQSOState = (isQSODataAvailableForCurrentTalker() ? QSO_DISPLAY_CALLER_DATA : QSO_DISPLAY_DEFAULT_SCREEN);

		lastHeardClearLastID();
		uiDataGlobal.displayQSOStatePrev = QSO_DISPLAY_IDLE;
		currentChannelData = &channelScreenChannelData;// Need to set this as currentChannelData is used by functions called by loadChannelData()

		if (currentChannelData->rxFreq != 0)
		{
			loadChannelData(true, false);
		}
		else
		{
			codeplugZoneGetDataForNumber(nonVolatileSettings.currentZone, &currentZone);
			codeplugUtilConvertBufToString(currentZone.name, currentZoneName, 16);// need to convert to zero terminated string
			loadChannelData(false, false);
		}

#if defined(PLATFORM_GD77S)
		trxSetPowerFromLevel(nonVolatileSettings.txPowerLevel);// ensure the power level is available for the Power announcement

		//Reset to first UiMode if the radio is in Analog mode and the current UiMode only applies to DMR
		if ((trxGetMode() == RADIO_MODE_ANALOG) &&
			((GD77SParameters.uiMode == GD77S_UIMODE_TS ) ||
			(GD77SParameters.uiMode == GD77S_UIMODE_CC ) ||
			(GD77SParameters.uiMode == GD77S_UIMODE_FILTER )))
		{
			GD77SParameters.uiMode = GD77S_UIMODE_TG_OR_SQUELCH;
		}

		// Ensure the correct channel is loaded, on the very first run
		if (GD77SParameters.firstRun)
		{
			if (voicePromptsIsPlaying() == false)
			{
				GD77SParameters.firstRun = false;
				checkAndUpdateSelectedChannelForGD77S(rotarySwitchGetPosition(), true);
			}


			GD77SParameters.dtmfListCount = codeplugDTMFContactsGetCount();
		}
#endif
		uiChannelModeUpdateScreen(0);

		if (uiDataGlobal.Scan.active == false)
		{
			uiDataGlobal.Scan.state = SCAN_SCANNING;
		}

		// Need to do this last, as other things in the screen init, need to know whether the main screen has just changed
		if (uiDataGlobal.VoicePrompts.inhibitInitial)
		{
			uiDataGlobal.VoicePrompts.inhibitInitial = false;
		}

		menuChannelExitStatus = MENU_STATUS_SUCCESS; // Due to Orange Quick Menu
	}
	else
	{
		menuChannelExitStatus = MENU_STATUS_SUCCESS;

#if defined(PLATFORM_GD77S)
		uiChannelModeHeartBeatActivityForGD77S(ev);
#endif

		if (ev->events == NO_EVENT)
		{
#if defined(PLATFORM_GD77S)
			// Just ensure rotary's selected channel is matching the already loaded one
			// as rotary selector could be turned while the GD is OFF, or in hotspot mode.
			if ((uiDataGlobal.Scan.active == false) && ((rotarySwitchGetPosition() != getCurrentChannelInCurrentZoneForGD77S()) || (GD77SParameters.firstRun == true)))
			{
				if (voicePromptsIsPlaying() == false)
				{
					checkAndUpdateSelectedChannelForGD77S(rotarySwitchGetPosition(), GD77SParameters.firstRun);

					// Opening channel number announce has not took place yet, probably because it was telling
					// parameter like new hotspot mode selection.
					if (GD77SParameters.firstRun)
					{
						GD77SParameters.firstRun = false;
					}
				}
			}
#endif

			// is there an incoming DMR signal
			if (uiDataGlobal.displayQSOState != QSO_DISPLAY_IDLE)
			{
				uiChannelModeUpdateScreen(0);
			}
			else
			{
				// Clear squelch region
				if (uiDataGlobal.displaySquelch && ((ev->time - sqm) > 1000))
				{
					uiDataGlobal.displaySquelch = false;
					uiUtilityDisplayInformation(NULL, DISPLAY_INFO_SQUELCH_CLEAR_AREA, -1);
					ucRenderRows(2, 4);
				}

				if ((ev->time - m) > RSSI_UPDATE_COUNTER_RELOAD)
				{
					m = ev->time;

					if (uiDataGlobal.Scan.active && (uiDataGlobal.Scan.state == SCAN_PAUSED))
					{
#if defined(PLATFORM_RD5R)
						ucClearRows(0, 1, false);
#else
						ucClearRows(0, 2, false);
#endif
						uiUtilityRenderHeader(false);
					}
					else
					{
						uiUtilityDrawRSSIBarGraph();
					}

					// Only render the second row which contains the bar graph, if we're not scanning,
					// as there is no need to redraw the rest of the screen
					ucRenderRows(((uiDataGlobal.Scan.active && (uiDataGlobal.Scan.state == SCAN_PAUSED)) ? 0 : 1), 2);
				}
			}

			if (uiDataGlobal.Scan.active == true)
			{
				scanning();
			}
		}
		else
		{
			if (ev->hasEvent)
			{
				if ((trxGetMode() == RADIO_MODE_ANALOG) &&
						(ev->events & KEY_EVENT) && ((ev->keys.key == KEY_LEFT) || (ev->keys.key == KEY_RIGHT)))
				{
					sqm = ev->time;
				}

				handleEvent(ev);
			}
		}

#if defined(PLATFORM_GD77S)
		dtmfSequenceTick(false);
#endif

	}
	return menuChannelExitStatus;
}

#if 0 // rename: we have an union declared (fw_sound.c) with the same name.
uint16_t byteSwap16(uint16_t in)
{
	return ((in & 0xff << 8) | (in >> 8));
}
#endif

static bool canCurrentZoneBeScanned(int *availableChannels)
{
	int enabledChannels = 0;
	int chanIdx = (CODEPLUG_ZONE_IS_ALLCHANNELS(currentZone) ? nonVolatileSettings.currentChannelIndexInAllZone : currentZone.channels[nonVolatileSettings.currentChannelIndexInZone]);

	if (currentZone.NOT_IN_CODEPLUGDATA_numChannelsInZone > 1)
	{
		int chansInZone = currentZone.NOT_IN_CODEPLUGDATA_numChannelsInZone;

		if (CODEPLUG_ZONE_IS_ALLCHANNELS(currentZone))
		{
			do
			{
				do
				{
					chanIdx = ((((chanIdx - 1) + 1) % currentZone.NOT_IN_CODEPLUGDATA_highestIndex) + 1);
				} while (!codeplugAllChannelsIndexIsInUse(chanIdx));

				chansInZone--;
				// Get flag4 only
				codeplugChannelGetDataWithOffsetAndLengthForIndex(chanIdx, &channelNextChannelData, CODEPLUG_CHANNEL_FLAG4_OFFSET, 1);

				if (CODEPLUG_CHANNEL_IS_FLAG_SET(&channelNextChannelData, CODEPLUG_CHANNEL_FLAG_ALL_SKIP) == false)
				{
					enabledChannels++;
				}

			} while (chansInZone > 0);
		}
		else
		{
			do
			{
				chanIdx = ((chanIdx + 1) % currentZone.NOT_IN_CODEPLUGDATA_numChannelsInZone);

				chansInZone--;
				// Get flag4 only
				codeplugChannelGetDataWithOffsetAndLengthForIndex(currentZone.channels[chanIdx], &channelNextChannelData, CODEPLUG_CHANNEL_FLAG4_OFFSET, 1);

				if (CODEPLUG_CHANNEL_IS_FLAG_SET(&channelNextChannelData, CODEPLUG_CHANNEL_FLAG_ZONE_SKIP) == false)
				{
					enabledChannels++;
				}

			} while (chansInZone > 0);
		}
	}

	*availableChannels = enabledChannels;

	return (enabledChannels > 1);
}

static void scanSearchForNextChannel(void)
{
	int channel = 0;

	// All Channels virtual zone
	if (CODEPLUG_ZONE_IS_ALLCHANNELS(currentZone))
	{
		do
		{
			do
			{
				// rollover (up/down) CODEPLUG_CHANNELS_MIN .. currentZone.NOT_IN_CODEPLUGDATA_highestIndex
				nextChannelIndex = ((uiDataGlobal.Scan.direction == 1) ?
						((((nextChannelIndex - 1) + 1) % currentZone.NOT_IN_CODEPLUGDATA_highestIndex) + 1) :
						((((nextChannelIndex - 1) + currentZone.NOT_IN_CODEPLUGDATA_highestIndex - 1) % currentZone.NOT_IN_CODEPLUGDATA_highestIndex) + 1));
			} while (!codeplugAllChannelsIndexIsInUse(nextChannelIndex));

			// Check if the channel is skipped.
			// Get flag4 only
			codeplugChannelGetDataWithOffsetAndLengthForIndex(nextChannelIndex, &channelNextChannelData, CODEPLUG_CHANNEL_FLAG4_OFFSET, 1);

		} while (CODEPLUG_CHANNEL_IS_FLAG_SET(&channelNextChannelData, CODEPLUG_CHANNEL_FLAG_ALL_SKIP));

		channel = nextChannelIndex;
		codeplugChannelGetDataForIndex(nextChannelIndex, &channelNextChannelData);
	}
	else
	{
		do
		{
			// rollover (up/down) 0 .. (currentZone.NOT_IN_CODEPLUGDATA_highestIndex - 1)
			nextChannelIndex = ((uiDataGlobal.Scan.direction == 1) ?
					((nextChannelIndex + 1) % currentZone.NOT_IN_CODEPLUGDATA_numChannelsInZone) :
					((nextChannelIndex + currentZone.NOT_IN_CODEPLUGDATA_numChannelsInZone - 1) % currentZone.NOT_IN_CODEPLUGDATA_numChannelsInZone));

			// Check if the channel is skipped.
			// Get flag4 only
			codeplugChannelGetDataWithOffsetAndLengthForIndex(currentZone.channels[nextChannelIndex], &channelNextChannelData, CODEPLUG_CHANNEL_FLAG4_OFFSET, 1);

		} while (CODEPLUG_CHANNEL_IS_FLAG_SET(&channelNextChannelData, CODEPLUG_CHANNEL_FLAG_ZONE_SKIP));

		channel = currentZone.channels[nextChannelIndex];
		codeplugChannelGetDataForIndex(currentZone.channels[nextChannelIndex], &channelNextChannelData);
	}

	//check all nuisance delete entries and skip channel if there is a match
	for (int i = 0; i < MAX_ZONE_SCAN_NUISANCE_CHANNELS; i++)
	{
		if (uiDataGlobal.Scan.nuisanceDelete[i] == -1)
		{
			break;
		}
		else
		{
			if(uiDataGlobal.Scan.nuisanceDelete[i] == channel)
			{
				return;
			}
		}
	}

	nextChannelReady = true;
}

static void scanApplyNextChannel(void)
{
	if (CODEPLUG_ZONE_IS_ALLCHANNELS(currentZone))
	{
		// All Channels virtual zone
		settingsSet(nonVolatileSettings.currentChannelIndexInAllZone, (int16_t) nextChannelIndex);
	}
	else
	{
		settingsSet(nonVolatileSettings.currentChannelIndexInZone, (int16_t) nextChannelIndex);
	}

	lastHeardClearLastID();

	memcpy(&channelScreenChannelData, &channelNextChannelData, CODEPLUG_CHANNEL_DATA_STRUCT_SIZE);

	loadChannelData(true, false);
	uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
	uiChannelModeUpdateScreen(0);

	if ((trxGetMode() == RADIO_MODE_DIGITAL) && (trxDMRModeRx != DMR_MODE_SFR) && (uiDataGlobal.Scan.stepTimeMilliseconds < SCAN_DMR_SIMPLEX_MIN_INTERVAL))
	{
		uiDataGlobal.Scan.timerReload = SCAN_DMR_SIMPLEX_MIN_INTERVAL;
	}
	else
	{
		uiDataGlobal.Scan.timerReload = uiDataGlobal.Scan.stepTimeMilliseconds;
	}
	uiDataGlobal.Scan.timer = uiDataGlobal.Scan.timerReload;
	uiDataGlobal.Scan.state = SCAN_SCANNING;
	nextChannelReady = false;
}

static void loadChannelData(bool useChannelDataInMemory, bool loadVoicePromptAnnouncement)
{
	bool rxGroupValid = true;

	if (CODEPLUG_ZONE_IS_ALLCHANNELS(currentZone))
	{
		// All Channels virtual zone
		uiDataGlobal.currentSelectedChannelNumber = nonVolatileSettings.currentChannelIndexInAllZone;
	}
	else
	{
		uiDataGlobal.currentSelectedChannelNumber = currentZone.channels[nonVolatileSettings.currentChannelIndexInZone];
	}

	if (!useChannelDataInMemory)
	{
		if (CODEPLUG_ZONE_IS_ALLCHANNELS(currentZone))
		{
			// All Channels virtual zone
			codeplugChannelGetDataForIndex(nonVolatileSettings.currentChannelIndexInAllZone, &channelScreenChannelData);
		}
		else
		{
			codeplugChannelGetDataForIndex(currentZone.channels[nonVolatileSettings.currentChannelIndexInZone], &channelScreenChannelData);
		}
	}

	clearActiveDMRID();
	trxSetFrequency(currentChannelData->rxFreq, currentChannelData->txFreq, DMR_MODE_AUTO);
	trxSetModeAndBandwidth(currentChannelData->chMode, ((currentChannelData->flag4 & 0x02) == 0x02));

	if (currentChannelData->chMode == RADIO_MODE_ANALOG)
	{
		trxSetRxCSS(currentChannelData->rxTone);
	}
	else
	{
		trxSetDMRColourCode(currentChannelData->txColor);
		if (currentChannelData->rxGroupList != lastLoadedRxGroup)
		{
			rxGroupValid = codeplugRxGroupGetDataForIndex(currentChannelData->rxGroupList, &currentRxGroupData);
			if (rxGroupValid)
			{
				lastLoadedRxGroup = currentChannelData->rxGroupList;
			}
			else
			{
				lastLoadedRxGroup = -1;// RxGroup Contacts are not valid
			}
		}

		// Current contact index is out of group list bounds, select first contact
		if (rxGroupValid && (nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE] > (currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup - 1)))
		{
			settingsSet(nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE], 0);
			menuChannelExitStatus |= (MENU_STATUS_LIST_TYPE | MENU_STATUS_FORCE_FIRST);
		}

		// Check if this channel has an Rx Group
		if (rxGroupValid && nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE] < currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup)
		{
			codeplugContactGetDataForIndex(currentRxGroupData.contacts[nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE]], &currentContactData);
		}
		else
		{
			codeplugContactGetDataForIndex(currentChannelData->contact, &currentContactData);
		}

		trxUpdateTsForCurrentChannelWithSpecifiedContact(&currentContactData);

		if (nonVolatileSettings.overrideTG == 0)
		{
			trxTalkGroupOrPcId = currentContactData.tgNumber;

			if (currentContactData.callType == CONTACT_CALLTYPE_PC)
			{
				trxTalkGroupOrPcId |= (PC_CALL_FLAG << 24);
			}
		}
		else
		{
			trxTalkGroupOrPcId = nonVolatileSettings.overrideTG;
		}
	}

#if ! defined(PLATFORM_GD77S) // GD77S handle voice prompts on its own
	int nextMenu = menuSystemGetPreviouslyPushedMenuNumber(); // used to determine if this screen has just been loaded after Tx ended (in loadChannelData()))
	if ((!uiDataGlobal.VoicePrompts.inhibitInitial || loadVoicePromptAnnouncement) &&
			((uiDataGlobal.Scan.active == false) ||
					(uiDataGlobal.Scan.active && ((uiDataGlobal.Scan.state = SCAN_SHORT_PAUSED) || (uiDataGlobal.Scan.state = SCAN_PAUSED)))))
	{
			announceItem(PROMPT_SEQUENCE_CHANNEL_NAME_AND_CONTACT_OR_VFO_FREQ_AND_MODE, ((nextMenu == UI_TX_SCREEN) || (nextMenu == UI_PRIVATE_CALL) || uiDataGlobal.Scan.active) ? PROMPT_THRESHOLD_NEVER_PLAY_IMMEDIATELY : PROMPT_THRESHOLD_3);
	}
#else
	// Force GD77S to always use the Master power level
	currentChannelData->libreDMR_Power = 0x00;
#endif

}

void uiChannelModeUpdateScreen(int txTimeSecs)
{
	int channelNumber;
	static const int nameBufferLen = 23;
	char nameBuf[nameBufferLen];
	static const int bufferLen = 17;
	char buffer[bufferLen];

	// Only render the header, then wait for the next run
	// Otherwise the screen could remain blank if TG and PC are == 0
	// since uiDataGlobal.displayQSOState won't be set to QSO_DISPLAY_IDLE
	if ((trxGetMode() == RADIO_MODE_DIGITAL) && (HRC6000GetReceivedTgOrPcId() == 0) &&
			((uiDataGlobal.displayQSOState == QSO_DISPLAY_CALLER_DATA) || (uiDataGlobal.displayQSOState == QSO_DISPLAY_CALLER_DATA_UPDATE)))
	{
		uiUtilityRedrawHeaderOnly(false);
		return;
	}

	// We're currently displaying details, and it shouldn't be overridden by QSO data
	if (uiDataGlobal.displayChannelSettings && ((uiDataGlobal.displayQSOState == QSO_DISPLAY_CALLER_DATA)
			|| (uiDataGlobal.displayQSOState == QSO_DISPLAY_CALLER_DATA_UPDATE)))
	{
		// We will not restore the previous QSO Data as a new caller just arose.
		uiDataGlobal.displayQSOStatePrev = QSO_DISPLAY_DEFAULT_SCREEN;
		uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
	}

	ucClearBuf();
	uiUtilityRenderHeader(false);

	switch(uiDataGlobal.displayQSOState)
	{
		case QSO_DISPLAY_DEFAULT_SCREEN:
			uiDataGlobal.displayQSOStatePrev = QSO_DISPLAY_DEFAULT_SCREEN;
			uiDataGlobal.isDisplayingQSOData = false;
			uiDataGlobal.receivedPcId = 0x00;
			if (trxTransmissionEnabled)
			{
				// Squelch is displayed, PTT was pressed
				// Clear its region
				if (uiDataGlobal.displaySquelch)
				{
					uiDataGlobal.displaySquelch = false;
					uiUtilityDisplayInformation(NULL, DISPLAY_INFO_SQUELCH_CLEAR_AREA, -1);
				}

				snprintf(buffer, bufferLen, " %d ", txTimeSecs);
				uiUtilityDisplayInformation(buffer, DISPLAY_INFO_TX_TIMER, -1);
			}
			else
			{
				// Display some channel settings
				if (uiDataGlobal.displayChannelSettings)
				{
					uiUtilityDisplayInformation(NULL, DISPLAY_INFO_TONE_AND_SQUELCH, -1);

					uiUtilityDisplayFrequency(DISPLAY_Y_POS_RX_FREQ, false, false, (uiDataGlobal.reverseRepeater ? currentChannelData->txFreq : currentChannelData->rxFreq), false, false, 0);
					uiUtilityDisplayFrequency(DISPLAY_Y_POS_TX_FREQ, true, false, (uiDataGlobal.reverseRepeater ? currentChannelData->rxFreq : currentChannelData->txFreq), false, false, 0);
				}
				else
				{
					if (CODEPLUG_ZONE_IS_ALLCHANNELS(currentZone))
					{
						// All Channels virtual zone
						channelNumber = nonVolatileSettings.currentChannelIndexInAllZone;
						if (directChannelNumber > 0)
						{
							snprintf(nameBuf, nameBufferLen, "%s %d", currentLanguage->gotoChannel, directChannelNumber);
						}
						else
						{
							snprintf(nameBuf, nameBufferLen, "%s Ch:%d",currentLanguage->all_channels, channelNumber);
						}
					}
					else
					{
						channelNumber = nonVolatileSettings.currentChannelIndexInZone + 1;
						if (directChannelNumber > 0)
						{
							snprintf(nameBuf, nameBufferLen, "%s %d", currentLanguage->gotoChannel, directChannelNumber);
						}
						else
						{
							snprintf(nameBuf, nameBufferLen, "%s Ch:%d", currentZoneName, channelNumber);
						}
					}
					uiUtilityDisplayInformation(nameBuf, DISPLAY_INFO_ZONE, -1);

				/*
				 * Roger Clark. Commented out
				 * functionality to display "Scanning" instead of the channel name  (and TG on DMR) whilst scanning
				 * if (uiDataGlobal.Scan.active && (uiDataGlobal.Scan.state == SCAN_SCANNING))
					{
						uiUtilityDisplayInformation(currentLanguage->scanning, DISPLAY_INFO_CONTACT, -1);
						ucRender();
						break;
					}*/
				}
			}

			if (!uiDataGlobal.displayChannelSettings)
			{
				codeplugUtilConvertBufToString(currentChannelData->name, nameBuf, 16);
				uiUtilityDisplayInformation(nameBuf, DISPLAY_INFO_CHANNEL, (trxTransmissionEnabled ? DISPLAY_Y_POS_CHANNEL_SECOND_LINE : -1));
			}

			if (trxGetMode() == RADIO_MODE_DIGITAL)
			{
				if (uiDataGlobal.displayChannelSettings)
				{
					uint32_t PCorTG = ((nonVolatileSettings.overrideTG != 0) ? nonVolatileSettings.overrideTG : currentContactData.tgNumber);

					snprintf(nameBuf, nameBufferLen, "%s %d",
							(((PCorTG >> 24) == PC_CALL_FLAG) ? currentLanguage->pc : currentLanguage->tg),
							(PCorTG & 0xFFFFFF));
				}
				else
				{
					if (nonVolatileSettings.overrideTG != 0)
					{
						uiUtilityBuildTgOrPCDisplayName(nameBuf, bufferLen);
						uiUtilityDisplayInformation(NULL, DISPLAY_INFO_CONTACT_OVERRIDE_FRAME, (trxTransmissionEnabled ? DISPLAY_Y_POS_CONTACT_TX_FRAME : -1));
					}
					else
					{
						codeplugUtilConvertBufToString(currentContactData.name, nameBuf, 16);
					}
				}

				uiUtilityDisplayInformation(nameBuf, DISPLAY_INFO_CONTACT, (trxTransmissionEnabled ? DISPLAY_Y_POS_CONTACT_TX : -1));
			}
			// Squelch will be cleared later, 1s after last change
			else if(uiDataGlobal.displaySquelch && !trxTransmissionEnabled && !uiDataGlobal.displayChannelSettings)
			{
				strncpy(buffer, currentLanguage->squelch, 9);
				buffer[8] = 0; // Avoid overlap with bargraph
				uiUtilityDisplayInformation(buffer, DISPLAY_INFO_SQUELCH, -1);
			}

			// SK1 is pressed, we don't want to clear the first info row after 1s
			if (uiDataGlobal.displayChannelSettings && uiDataGlobal.displaySquelch)
			{
				uiDataGlobal.displaySquelch = false;
			}

			ucRender();
			break;

		case QSO_DISPLAY_CALLER_DATA:
		case QSO_DISPLAY_CALLER_DATA_UPDATE:
			uiDataGlobal.displayQSOStatePrev = QSO_DISPLAY_CALLER_DATA;
			uiDataGlobal.isDisplayingQSOData = true;
			uiDataGlobal.displayChannelSettings = false;
			uiUtilityRenderQSOData();
			ucRender();
			break;

		case QSO_DISPLAY_IDLE:
			break;
	}

	uiDataGlobal.displayQSOState = QSO_DISPLAY_IDLE;
}

static void handleEvent(uiEvent_t *ev)
{
#if defined(PLATFORM_GD77S)
	handleEventForGD77S(ev);
	return;
#else

	if (uiDataGlobal.Scan.active && (ev->events & KEY_EVENT))
	{
		// Key pressed during scanning

		if (BUTTONCHECK_DOWN(ev, BUTTON_SK2) == 0)
		{
			// if we are scanning and down key is pressed then enter current channel into nuisance delete array.
			if((uiDataGlobal.Scan.state == SCAN_PAUSED) && (ev->keys.key == KEY_RIGHT))
			{
				// There is two channels available in the Zone, just stop scanning
				if (uiDataGlobal.Scan.nuisanceDeleteIndex == (uiDataGlobal.Scan.availableChannelsCount - 2))
				{
					uiDataGlobal.Scan.lastIteration = true;
				}

				uiDataGlobal.Scan.nuisanceDelete[uiDataGlobal.Scan.nuisanceDeleteIndex] = uiDataGlobal.currentSelectedChannelNumber;
				uiDataGlobal.Scan.nuisanceDeleteIndex = (uiDataGlobal.Scan.nuisanceDeleteIndex + 1) % MAX_ZONE_SCAN_NUISANCE_CHANNELS;
				uiDataGlobal.Scan.timer = SCAN_SKIP_CHANNEL_INTERVAL;	//force scan to continue;
				uiDataGlobal.Scan.state = SCAN_SCANNING;
				keyboardReset();
				return;
			}

			// Left key reverses the scan direction
			if (ev->keys.key == KEY_LEFT)
			{
				uiDataGlobal.Scan.direction *= -1;
				keyboardReset();
				return;
			}
		}
		// stop the scan on any button except UP without Shift (allows scan to be manually continued)
		// or SK2 on its own (allows Backlight to be triggered)
		if (((ev->keys.key == KEY_UP) && BUTTONCHECK_DOWN(ev, BUTTON_SK2) == 0) == false)
		{
			uiChannelModeStopScanning();
			keyboardReset();
			uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
			uiChannelModeUpdateScreen(0);
			loadChannelData(false, true);
			return;
		}
	}

	if (ev->events & FUNCTION_EVENT)
	{
		if (ev->function == FUNC_START_SCANNING)
		{
			directChannelNumber = 0;
			if (uiDataGlobal.Scan.active == false)
			{
				scanStart(false);
			}
			return;
		}
		else if (ev->function == FUNC_REDRAW)
		{
			uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
			uiChannelModeUpdateScreen(0);
			return;
		}
	}

	if ((uiDataGlobal.reverseRepeater == false) && handleMonitorMode(ev))
	{
		return;
	}

	if (ev->events & BUTTON_EVENT)
	{
		if (repeatVoicePromptOnSK1(ev))
		{
			return;
		}

		uint32_t tg = (LinkHead->talkGroupOrPcId & 0xFFFFFF);

		// If Blue button is pressed during reception it sets the Tx TG to the incoming TG
		if (uiDataGlobal.isDisplayingQSOData && BUTTONCHECK_SHORTUP(ev, BUTTON_SK2) && (trxGetMode() == RADIO_MODE_DIGITAL) &&
				((trxTalkGroupOrPcId != tg) ||
				((dmrMonitorCapturedTS != -1) && (dmrMonitorCapturedTS != trxGetDMRTimeSlot())) ||
				(trxGetDMRColourCode() != currentChannelData->txColor)))
		{
			lastHeardClearLastID();

			// Set TS to overriden TS
			if ((dmrMonitorCapturedTS != -1) && (dmrMonitorCapturedTS != trxGetDMRTimeSlot()))
			{
				trxSetDMRTimeSlot(dmrMonitorCapturedTS);
				tsSetManualOverride(CHANNEL_CHANNEL, (dmrMonitorCapturedTS + 1));
			}

			if (trxTalkGroupOrPcId != tg)
			{
				trxTalkGroupOrPcId = tg;
				settingsSet(nonVolatileSettings.overrideTG, trxTalkGroupOrPcId);
			}

			currentChannelData->txColor = trxGetDMRColourCode();// Set the CC to the current CC, which may have been determined by the CC finding algorithm in C6000.c
			announceItem(PROMPT_SEQUENCE_CONTACT_TG_OR_PC,PROMPT_THRESHOLD_NEVER_PLAY_IMMEDIATELY);

			uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
			uiChannelModeUpdateScreen(0);
			soundSetMelody(MELODY_ACK_BEEP);
			return;
		}

		if ((uiDataGlobal.reverseRepeater == false) && (BUTTONCHECK_DOWN(ev, BUTTON_SK1) && BUTTONCHECK_DOWN(ev, BUTTON_SK2)))
		{
			trxSetFrequency(currentChannelData->txFreq, currentChannelData->rxFreq, DMR_MODE_DMO);// Swap Tx and Rx freqs but force DMR Active
			uiDataGlobal.reverseRepeater = true;
			uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
			uiChannelModeUpdateScreen(0);
			return;
		}
		else if ((uiDataGlobal.reverseRepeater == true) && (BUTTONCHECK_DOWN(ev, BUTTON_SK2) == 0))
		{
			trxSetFrequency(currentChannelData->rxFreq, currentChannelData->txFreq, DMR_MODE_AUTO);
			uiDataGlobal.reverseRepeater = false;

			// We are still displaying channel details (SK1 has been released), force to update the screen
			if (uiDataGlobal.displayChannelSettings)
			{
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
				uiChannelModeUpdateScreen(0);
			}

			return;
		}
		// Display channel settings (RX/TX/etc) while SK1 is pressed
		else if ((uiDataGlobal.displayChannelSettings == false) && BUTTONCHECK_DOWN(ev, BUTTON_SK1))
		{
			if (uiDataGlobal.Scan.active == false)
			{
				int prevQSODisp = uiDataGlobal.displayQSOStatePrev;
				uiDataGlobal.displayChannelSettings = true;
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
				uiChannelModeUpdateScreen(0);
				uiDataGlobal.displayQSOStatePrev = prevQSODisp;
			}
			return;

		}
		else if ((uiDataGlobal.displayChannelSettings == true) && (BUTTONCHECK_DOWN(ev, BUTTON_SK1) == 0))
		{
			uiDataGlobal.displayChannelSettings = false;
			uiDataGlobal.displayQSOState = uiDataGlobal.displayQSOStatePrev;

			// Maybe QSO State has been overridden, double check if we could now
			// display QSO Data
			if (uiDataGlobal.displayQSOState == QSO_DISPLAY_DEFAULT_SCREEN)
			{
				if (isQSODataAvailableForCurrentTalker())
				{
					uiDataGlobal.displayQSOState = QSO_DISPLAY_CALLER_DATA;
				}
			}

			// Leaving Channel Details disable reverse repeater feature
			if (uiDataGlobal.reverseRepeater)
			{
				trxSetFrequency(currentChannelData->rxFreq, currentChannelData->txFreq, DMR_MODE_AUTO);
				uiDataGlobal.reverseRepeater = false;
			}

			uiChannelModeUpdateScreen(0);
			return;
		}

#if !defined(PLATFORM_RD5R)
		if (BUTTONCHECK_SHORTUP(ev, BUTTON_ORANGE) && (BUTTONCHECK_DOWN(ev, BUTTON_SK1) == 0))
		{
			if (BUTTONCHECK_DOWN(ev, BUTTON_SK2))
			{
				announceItem(PROMPT_SEQUENCE_BATTERY, AUDIO_PROMPT_MODE_VOICE_LEVEL_1);
			}
			else
			{
				// Quick Menu
				menuSystemPushNewMenu(UI_CHANNEL_QUICK_MENU);

				// Trick to beep (AudioAssist), since ORANGE button doesn't produce any beep event
				ev->keys.event |= KEY_MOD_UP;
				ev->keys.key = 127;
				menuChannelExitStatus |= (MENU_STATUS_LIST_TYPE | MENU_STATUS_FORCE_FIRST);
				// End Trick
			}

			return;
		}
#endif
	}

	if (ev->events & KEY_EVENT)
	{
		if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
		{
			if (directChannelNumber > 0)
			{
				if(CODEPLUG_ZONE_IS_ALLCHANNELS(currentZone))
				{
					// All Channels virtual zone
					if (codeplugAllChannelsIndexIsInUse(directChannelNumber))
					{
						settingsSet(nonVolatileSettings.currentChannelIndexInAllZone, (int16_t) directChannelNumber);
						loadChannelData(false, true);

					}
					else
					{
						soundSetMelody(MELODY_ERROR_BEEP);
					}
				}
				else
				{
					if ((directChannelNumber - 1) < currentZone.NOT_IN_CODEPLUGDATA_numChannelsInZone)
					{
						settingsSet(nonVolatileSettings.currentChannelIndexInZone, (int16_t) (directChannelNumber - 1));
						loadChannelData(false, true);
					}
					else
					{
						soundSetMelody(MELODY_ERROR_BEEP);
					}

				}
				directChannelNumber = 0;
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
				uiChannelModeUpdateScreen(0);
			}
			else if (BUTTONCHECK_DOWN(ev, BUTTON_SK2))
			{
				menuSystemPushNewMenu(MENU_CHANNEL_DETAILS);
			}
			else
			{
				menuSystemPushNewMenu(MENU_MAIN_MENU);
			}
			return;
		}
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_HASH))
		{
			if (BUTTONCHECK_DOWN(ev, BUTTON_SK2) != 0)
			{
				menuSystemPushNewMenu(MENU_CONTACT_QUICKLIST);
			}
			else
			{
				if (trxGetMode() == RADIO_MODE_DIGITAL)
				{
					menuSystemPushNewMenu(MENU_NUMERICAL_ENTRY);
				}
			}
			return;
		}
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
		{
			if (BUTTONCHECK_DOWN(ev, BUTTON_SK2) && (uiDataGlobal.tgBeforePcMode != 0))
			{
				settingsSet(nonVolatileSettings.overrideTG, uiDataGlobal.tgBeforePcMode);
				menuPrivateCallClear();

				updateTrxID();
				uiDataGlobal.displayQSOState= QSO_DISPLAY_DEFAULT_SCREEN;// Force redraw
				uiChannelModeUpdateScreen(0);
				return;// The event has been handled
			}
			if(directChannelNumber > 0)
			{
				announceItem(PROMPT_SEQUENCE_CHANNEL_NAME_OR_VFO_FREQ, PROMPT_THRESHOLD_NEVER_PLAY_IMMEDIATELY);

				directChannelNumber = 0;
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
				uiChannelModeUpdateScreen(0);
			}
			else
			{
#if defined(PLATFORM_GD77)
				menuSystemSetCurrentMenu(UI_VFO_MODE);
#endif
				return;
			}
		}
#if defined(PLATFORM_DM1801) || defined(PLATFORM_RD5R)
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_VFO_MR))
		{
			directChannelNumber = 0;
			menuSystemSetCurrentMenu(UI_VFO_MODE);
			return;
		}
#endif
#if defined(PLATFORM_RD5R)
		else if (KEYCHECK_LONGDOWN(ev->keys, KEY_VFO_MR) && (BUTTONCHECK_DOWN(ev, BUTTON_SK1) == 0))
		{
			if (BUTTONCHECK_DOWN(ev, BUTTON_SK2))
			{
				announceItem(PROMPT_SEQUENCE_BATTERY, AUDIO_PROMPT_MODE_VOICE_LEVEL_1);
			}
			else
			{
				menuSystemPushNewMenu(UI_CHANNEL_QUICK_MENU);

				// Trick to beep (AudioAssist), since ORANGE button doesn't produce any beep event
				ev->keys.event |= KEY_MOD_UP;
				ev->keys.key = 127;
				menuChannelExitStatus |= (MENU_STATUS_LIST_TYPE | MENU_STATUS_FORCE_FIRST);
				// End Trick
			}

			return;
		}
#endif
		else if (KEYCHECK_LONGDOWN(ev->keys, KEY_RIGHT))
		{
			// Long press allows the 5W+ power setting to be selected immediately
			if (BUTTONCHECK_DOWN(ev, BUTTON_SK2))
			{
				if (increasePowerLevel(true))
				{
					uiUtilityRedrawHeaderOnly(false);
				}
			}
		}
		else if (KEYCHECK_PRESS(ev->keys, KEY_RIGHT))
		{
			if (BUTTONCHECK_DOWN(ev, BUTTON_SK2))
			{
				if (increasePowerLevel(false))
				{
					uiUtilityRedrawHeaderOnly(false);
				}
			}
			else
			{
				if (trxGetMode() == RADIO_MODE_DIGITAL)
				{
					if (currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup > 1)
					{
						if (nonVolatileSettings.overrideTG == 0)
						{
							settingsIncrement(nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE], 1);
							if (nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE] > (currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup - 1))
							{
								settingsSet(nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE], 0);
								menuChannelExitStatus |= (MENU_STATUS_LIST_TYPE | MENU_STATUS_FORCE_FIRST);
							}
						}
					}
					settingsSet(nonVolatileSettings.overrideTG, 0);// setting the override TG to 0 indicates the TG is not overridden
					menuPrivateCallClear();
					updateTrxID();
					// We're in digital mode, RXing, and current talker is already at the top of last heard list,
					// hence immediately display complete contact/TG info on screen
					uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;//(isQSODataAvailableForCurrentTalker() ? QSO_DISPLAY_CALLER_DATA : QSO_DISPLAY_DEFAULT_SCREEN);
					if (isQSODataAvailableForCurrentTalker())
					{
						addTimerCallback(uiUtilityRenderQSODataAndUpdateScreen, 2000, true);
					}
					uiChannelModeUpdateScreen(0);
					announceItem(PROMPT_SEQUENCE_CONTACT_TG_OR_PC,PROMPT_THRESHOLD_3);
				}
				else
				{
					if(currentChannelData->sql == 0)			//If we were using default squelch level
					{
						currentChannelData->sql = nonVolatileSettings.squelchDefaults[trxCurrentBand[TRX_RX_FREQ_BAND]];			//start the adjustment from that point.
					}

					if (currentChannelData->sql < CODEPLUG_MAX_VARIABLE_SQUELCH)
					{
						currentChannelData->sql++;
					}

					announceItem(PROMPT_SQUENCE_SQUELCH,PROMPT_THRESHOLD_3);

					uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
					uiDataGlobal.displaySquelch = true;
					uiChannelModeUpdateScreen(0);
				}
			}

		}
		else if (KEYCHECK_PRESS(ev->keys, KEY_LEFT))
		{
			if (BUTTONCHECK_DOWN(ev, BUTTON_SK2))
			{
				if (decreasePowerLevel())
				{
					uiUtilityRedrawHeaderOnly(false);
				}

				if (trxGetPowerLevel() == 0)
				{
					menuChannelExitStatus |= (MENU_STATUS_LIST_TYPE | MENU_STATUS_FORCE_FIRST);
				}
			}
			else
			{
				if (trxGetMode() == RADIO_MODE_DIGITAL)
				{
					if (currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup > 1)
					{
						// To Do change TG in on same channel freq
						if (nonVolatileSettings.overrideTG == 0)
						{
							settingsDecrement(nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE], 1);
							if (nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE] < 0)
							{
								settingsSet(nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE],
										(int16_t) (currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup - 1));
							}

							if (nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE] == 0)
							{
								menuChannelExitStatus |= (MENU_STATUS_LIST_TYPE | MENU_STATUS_FORCE_FIRST);
							}
						}
					}
					settingsSet(nonVolatileSettings.overrideTG, 0);// setting the override TG to 0 indicates the TG is not overridden
					menuPrivateCallClear();
					updateTrxID();
					// We're in digital mode, RXing, and current talker is already at the top of last heard list,
					// hence immediately display complete contact/TG info on screen
					uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;//(isQSODataAvailableForCurrentTalker() ? QSO_DISPLAY_CALLER_DATA : QSO_DISPLAY_DEFAULT_SCREEN);
					if (isQSODataAvailableForCurrentTalker())
					{
						addTimerCallback(uiUtilityRenderQSODataAndUpdateScreen, 2000, true);
					}
					uiChannelModeUpdateScreen(0);
					announceItem(PROMPT_SEQUENCE_CONTACT_TG_OR_PC,PROMPT_THRESHOLD_3);
				}
				else
				{
					if(currentChannelData->sql == 0)			//If we were using default squelch level
					{
						currentChannelData->sql = nonVolatileSettings.squelchDefaults[trxCurrentBand[TRX_RX_FREQ_BAND]];			//start the adjustment from that point.
					}

					if (currentChannelData->sql > CODEPLUG_MIN_VARIABLE_SQUELCH)
					{
						currentChannelData->sql--;
					}

					announceItem(PROMPT_SQUENCE_SQUELCH,PROMPT_THRESHOLD_3);

					uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
					uiDataGlobal.displaySquelch = true;
					uiChannelModeUpdateScreen(0);
				}

			}
		}
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_STAR))
		{
			if (BUTTONCHECK_DOWN(ev, BUTTON_SK2))  // Toggle Channel Mode
			{
				if (trxGetMode() == RADIO_MODE_ANALOG)
				{
					currentChannelData->chMode = RADIO_MODE_DIGITAL;
					uiDataGlobal.VoicePrompts.inhibitInitial = true;// Stop VP playing in loadChannelData
					loadChannelData(true, false);
					uiDataGlobal.VoicePrompts.inhibitInitial = false;
					menuChannelExitStatus |= MENU_STATUS_FORCE_FIRST;
				}
				else
				{
					currentChannelData->chMode = RADIO_MODE_ANALOG;
					trxSetModeAndBandwidth(currentChannelData->chMode, ((currentChannelData->flag4 & 0x02) == 0x02));
					trxSetRxCSS(currentChannelData->rxTone);
				}

				announceItem(PROMPT_SEQUENCE_MODE, PROMPT_THRESHOLD_3);
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
				uiChannelModeUpdateScreen(0);
			}
			else
			{
				if (trxGetMode() == RADIO_MODE_DIGITAL)
				{
					// Toggle timeslot
					trxSetDMRTimeSlot(1 - trxGetDMRTimeSlot());
					tsSetManualOverride(CHANNEL_CHANNEL, (trxGetDMRTimeSlot() + 1));

					if ((nonVolatileSettings.overrideTG == 0) && (currentContactData.reserve1 & 0x01) == 0x00)
					{
						tsSetContactHasBeenOverriden(CHANNEL_CHANNEL, true);
					}

					//	init_digital();
					disableAudioAmp(AUDIO_AMP_MODE_RF);
					clearActiveDMRID();
					lastHeardClearLastID();
					uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
					uiChannelModeUpdateScreen(0);

					if (trxGetDMRTimeSlot() == 0)
					{
						menuChannelExitStatus |= MENU_STATUS_FORCE_FIRST;
					}
					announceItem(PROMPT_SEQUENCE_TS,PROMPT_THRESHOLD_3);
				}
				else
				{
					soundSetMelody(MELODY_ERROR_BEEP);
				}
			}
		}
		else if (KEYCHECK_LONGDOWN(ev->keys, KEY_STAR) && (BUTTONCHECK_DOWN(ev, BUTTON_SK2) == 0))
		{
			if (trxGetMode() == RADIO_MODE_DIGITAL)
			{
				tsSetManualOverride(CHANNEL_CHANNEL, TS_NO_OVERRIDE);
				tsSetContactHasBeenOverriden(CHANNEL_CHANNEL, false);

				if ((currentRxGroupData.name[0] != 0) && (nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE] < currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup))
				{
					codeplugContactGetDataForIndex(currentRxGroupData.contacts[nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE]], &currentContactData);
				}
				else
				{
					codeplugContactGetDataForIndex(currentChannelData->contact, &currentContactData);
				}

				trxUpdateTsForCurrentChannelWithSpecifiedContact(&currentContactData);

				clearActiveDMRID();
				lastHeardClearLastID();
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
				uiChannelModeUpdateScreen(0);
			}
		}
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_DOWN) || KEYCHECK_LONGDOWN_REPEAT(ev->keys, KEY_DOWN))
		{
			uiDataGlobal.displaySquelch = false;

			if (BUTTONCHECK_DOWN(ev, BUTTON_SK2))
			{
				selectPrevNextZone(false);
				menuSystemPopAllAndDisplaySpecificRootMenu(UI_CHANNEL_MODE, false);
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN; // Force screen redraw

				if (nonVolatileSettings.currentZone == 0)
				{
					menuChannelExitStatus |= (MENU_STATUS_LIST_TYPE | MENU_STATUS_FORCE_FIRST);
				}

				return;
			}
			else
			{
				int16_t prevChan;

				lastHeardClearLastID();
				if (CODEPLUG_ZONE_IS_ALLCHANNELS(currentZone))
				{
					if (currentZone.NOT_IN_CODEPLUGDATA_numChannelsInZone > 1)
					{
						prevChan = nonVolatileSettings.currentChannelIndexInAllZone;

						// All Channels virtual zone
						do
						{
							prevChan = ((((prevChan - 1) + currentZone.NOT_IN_CODEPLUGDATA_highestIndex - 1) % currentZone.NOT_IN_CODEPLUGDATA_highestIndex) + 1);

						} while (!codeplugAllChannelsIndexIsInUse(prevChan));

						settingsSet(nonVolatileSettings.currentChannelIndexInAllZone, prevChan);

						if (nonVolatileSettings.currentChannelIndexInAllZone == 1)
						{
							menuChannelExitStatus |= (MENU_STATUS_LIST_TYPE | MENU_STATUS_FORCE_FIRST);
						}
					}
				}
				else
				{
					if (currentZone.NOT_IN_CODEPLUGDATA_numChannelsInZone > 1)
					{
						prevChan = ((nonVolatileSettings.currentChannelIndexInZone + currentZone.NOT_IN_CODEPLUGDATA_numChannelsInZone - 1) % currentZone.NOT_IN_CODEPLUGDATA_numChannelsInZone);

						settingsSet(nonVolatileSettings.currentChannelIndexInZone, prevChan);

						if (nonVolatileSettings.currentChannelIndexInZone == 0)
						{
							menuChannelExitStatus |= (MENU_STATUS_LIST_TYPE | MENU_STATUS_FORCE_FIRST);
						}
					}
				}
			}
			loadChannelData(false, true);
			uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
			uiChannelModeUpdateScreen(0);
		}
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_UP) || KEYCHECK_LONGDOWN_REPEAT(ev->keys, KEY_UP))
		{
			handleUpKey(ev);
			return;
		}
		else if (KEYCHECK_LONGDOWN(ev->keys, KEY_UP) && (BUTTONCHECK_DOWN(ev, BUTTON_SK2) == 0))
		{
			if (uiDataGlobal.Scan.active == false)
			{
				scanStart(true);
			}
		}
		else
		{
			int keyval = menuGetKeypadKeyValue(ev, true);

			if ((keyval < 10) && (!BUTTONCHECK_DOWN(ev, BUTTON_SK2)))
			{
				directChannelNumber = (directChannelNumber * 10) + keyval;
				if (CODEPLUG_ZONE_IS_ALLCHANNELS(currentZone))
				{
					// All Channels virtual zone
					if(directChannelNumber > CODEPLUG_CONTACTS_MAX)
					{
						directChannelNumber = 0;
						soundSetMelody(MELODY_ERROR_BEEP);
					}
				}
				else
				{
					if(directChannelNumber > currentZone.NOT_IN_CODEPLUGDATA_numChannelsInZone)
					{
						directChannelNumber = 0;
						soundSetMelody(MELODY_ERROR_BEEP);
					}
				}

				if (directChannelNumber > 0)
				{
					voicePromptsInit();
					if (directChannelNumber < 10)
					{
						voicePromptsAppendLanguageString(&currentLanguage->gotoChannel);
					}
					voicePromptsAppendPrompt(PROMPT_0 + keyval);
					voicePromptsPlay();
				}
				else
				{
					announceItem(PROMPT_SEQUENCE_CHANNEL_NAME_OR_VFO_FREQ, PROMPT_THRESHOLD_3);
				}

				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
				uiChannelModeUpdateScreen(0);
			}
		}
	}
#endif // ! PLATFORM_GD77S
}

#if ! defined(PLATFORM_GD77S)
static void selectPrevNextZone(bool nextZone)
{
	int numZones = codeplugZonesGetCount();

	if (nextZone)
	{
		settingsIncrement(nonVolatileSettings.currentZone, 1);

		if (nonVolatileSettings.currentZone >= numZones)
		{
			settingsSet(nonVolatileSettings.currentZone, 0);
		}
	}
	else
	{
		if (nonVolatileSettings.currentZone == 0)
		{
			settingsSet(nonVolatileSettings.currentZone, (int16_t) (numZones - 1));
		}
		else
		{
			settingsDecrement(nonVolatileSettings.currentZone, 1);
		}
	}

/*
 * VK3KYY
 * I don't understand why these should be reset when changing zones.
 * Only the change to TG override will be clear to the operator.
 *
 * Changing zones should only change the zone.
 * I can't see a compelling reason to override what the operator has setup just because they change zones
 *
	settingsSet(nonVolatileSettings.overrideTG, 0); // remove any TG override

	tsSetManualOverride(CHANNEL_CHANNEL, TS_NO_OVERRIDE);// remove any TS override
*/
	settingsSet(nonVolatileSettings.currentChannelIndexInZone, 0);// Since we are switching zones the channel index should be reset
	currentChannelData->rxFreq = 0x00; // Flag to the Channel screen that the channel data is now invalid and needs to be reloaded
}

static void handleUpKey(uiEvent_t *ev)
{
	uiDataGlobal.displaySquelch = false;

	if (BUTTONCHECK_DOWN(ev, BUTTON_SK2))
	{
		selectPrevNextZone(true);
		menuSystemPopAllAndDisplaySpecificRootMenu(UI_CHANNEL_MODE, false);
		uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN; // Force screen redraw

		if (nonVolatileSettings.currentZone == 0)
		{
			menuChannelExitStatus |= (MENU_STATUS_LIST_TYPE | MENU_STATUS_FORCE_FIRST);
		}
		return;
	}
	else
	{
		int16_t nextChan;

		lastHeardClearLastID();
		if (CODEPLUG_ZONE_IS_ALLCHANNELS(currentZone))
		{
			if (currentZone.NOT_IN_CODEPLUGDATA_numChannelsInZone > 1)
			{
				nextChan = nonVolatileSettings.currentChannelIndexInAllZone;

				// All Channels virtual zone
				do
				{
					nextChan = ((((nextChan - 1) + 1) % currentZone.NOT_IN_CODEPLUGDATA_highestIndex) + 1);

					if (nextChan == CODEPLUG_CONTACTS_MIN)
					{
						menuChannelExitStatus |= (MENU_STATUS_LIST_TYPE | MENU_STATUS_FORCE_FIRST);
					}

				} while (!codeplugAllChannelsIndexIsInUse(nextChan));

				settingsSet(nonVolatileSettings.currentChannelIndexInAllZone, nextChan);
			}
		}
		else
		{
			if (currentZone.NOT_IN_CODEPLUGDATA_numChannelsInZone > 1)
			{
				nextChan = ((nonVolatileSettings.currentChannelIndexInZone + 1) % currentZone.NOT_IN_CODEPLUGDATA_numChannelsInZone);

				settingsSet(nonVolatileSettings.currentChannelIndexInZone, nextChan);

				if (nonVolatileSettings.currentChannelIndexInZone == 0)
				{
					menuChannelExitStatus |= (MENU_STATUS_LIST_TYPE | MENU_STATUS_FORCE_FIRST);
				}
			}
		}
		uiDataGlobal.Scan.timer = 500;
		uiDataGlobal.Scan.state = SCAN_SCANNING;
	}

	loadChannelData(false, true);
	uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
	uiChannelModeUpdateScreen(0);
}
#endif // ! PLATFORM_GD77S


// Quick Menu functions

enum CHANNEL_SCREEN_QUICK_MENU_ITEMS {  CH_SCREEN_QUICK_MENU_COPY2VFO = 0, CH_SCREEN_QUICK_MENU_COPY_FROM_VFO,
	CH_SCREEN_QUICK_MENU_FILTER_FM,
	CH_SCREEN_QUICK_MENU_FILTER_DMR,
	CH_SCREEN_QUICK_MENU_FILTER_DMR_CC,
	CH_SCREEN_QUICK_MENU_FILTER_DMR_TS,
	NUM_CH_SCREEN_QUICK_MENU_ITEMS };// The last item in the list is used so that we automatically get a total number of items in the list

menuStatus_t uiChannelModeQuickMenu(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		if (quickmenuChannelFromVFOHandled)
		{
			quickmenuChannelFromVFOHandled = false;
			menuSystemPopAllAndDisplayRootMenu();
			return MENU_STATUS_SUCCESS;
		}

		uiChannelModeStopScanning();
		uiDataGlobal.QuickMenu.tmpDmrDestinationFilterLevel = nonVolatileSettings.dmrDestinationFilter;
		uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel = nonVolatileSettings.dmrCcTsFilter;
		uiDataGlobal.QuickMenu.tmpAnalogFilterLevel = nonVolatileSettings.analogFilterLevel;
		menuDataGlobal.endIndex = NUM_CH_SCREEN_QUICK_MENU_ITEMS;

		voicePromptsInit();
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendLanguageString(&currentLanguage->quick_menu);
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		voicePromptsAppendPrompt(PROMPT_SILENCE);

		updateQuickMenuScreen(true);
		return (MENU_STATUS_LIST_TYPE | MENU_STATUS_SUCCESS);
	}
	else
	{
		menuQuickChannelExitStatus = MENU_STATUS_SUCCESS;

		if (ev->hasEvent)
		{
			handleQuickMenuEvent(ev);
		}
	}

	return menuQuickChannelExitStatus;
}

static bool validateOverwriteChannel(void)
{
	quickmenuChannelFromVFOHandled = true;

	if (uiDataGlobal.MessageBox.keyPressed == KEY_GREEN)
	{
		struct_codeplugContact_t vfoContact;
		int8_t vfoTS = -1;

		uiVFOLoadContact(&vfoContact);
		memcpy(&currentChannelData->rxFreq, &settingsVFOChannel[nonVolatileSettings.currentVFONumber].rxFreq, CODEPLUG_CHANNEL_DATA_STRUCT_SIZE - 16);// Don't copy the name of the vfo, which are in the first 16 bytes
		// Find out which TS was in use.
		if ((nonVolatileSettings.overrideTG == 0) && (vfoContact.reserve1 & 0x01) == 0x00)
		{
			vfoTS = ((vfoContact.reserve1 & 0x02) != 0);
		}
		else
		{
			vfoTS = tsGetManualOverride((Channel_t)nonVolatileSettings.currentVFONumber);

			// No Override, use the TS from the Channel
			if (vfoTS == 0)
			{
				vfoTS = ((settingsVFOChannel[nonVolatileSettings.currentVFONumber].flag2 & 0x40) != 0);
			}
			else
			{
				vfoTS--; // convert to real TS
			}
		}

		if (vfoTS == 0)
		{
			currentChannelData->flag2 &= 0xBF;
		}
		else
		{
			currentChannelData->flag2 |= 0x40;
		}

		codeplugChannelSaveDataForIndex(uiDataGlobal.currentSelectedChannelNumber, currentChannelData);
	}

	return true;
}

static void updateQuickMenuScreen(bool isFirstRun)
{
	int mNum = 0;
	static const int bufferLen = 17;
	char buf[bufferLen];
	char * const *leftSide;// initialise to please the compiler
	char * const *rightSideConst;// initialise to please the compiler
	char rightSideVar[bufferLen];

	ucClearBuf();
	menuDisplayTitle(currentLanguage->quick_menu);

	for(int i =- 1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(NUM_CH_SCREEN_QUICK_MENU_ITEMS, i);
		buf[0] = 0;
		rightSideVar[0] = 0;
		rightSideConst = NULL;
		leftSide = NULL;

		switch(mNum)
		{
			case CH_SCREEN_QUICK_MENU_COPY2VFO:
				rightSideConst = (char * const *)&currentLanguage->channelToVfo;
				break;
			case CH_SCREEN_QUICK_MENU_COPY_FROM_VFO:
				rightSideConst = (char * const *)&currentLanguage->vfoToChannel;
				break;
			case CH_SCREEN_QUICK_MENU_FILTER_FM:
				leftSide = (char * const *)&currentLanguage->filter;
				if (uiDataGlobal.QuickMenu.tmpAnalogFilterLevel == 0)
				{
					rightSideConst = (char * const *)&currentLanguage->none;
				}
				else
				{
					snprintf(rightSideVar, bufferLen, "%s", ANALOG_FILTER_LEVELS[uiDataGlobal.QuickMenu.tmpAnalogFilterLevel - 1]);
				}
				break;
			case CH_SCREEN_QUICK_MENU_FILTER_DMR:
				leftSide = (char * const *)&currentLanguage->dmr_filter;
				if (uiDataGlobal.QuickMenu.tmpDmrDestinationFilterLevel == 0)
				{
					rightSideConst = (char * const *)&currentLanguage->none;
				}
				else
				{
					snprintf(rightSideVar, bufferLen, "%s", DMR_DESTINATION_FILTER_LEVELS[uiDataGlobal.QuickMenu.tmpDmrDestinationFilterLevel - 1]);
				}
				break;
			case CH_SCREEN_QUICK_MENU_FILTER_DMR_CC:
				leftSide = (char * const *)&currentLanguage->dmr_cc_filter;
				rightSideConst = (uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel & DMR_CC_FILTER_PATTERN)?(char * const *)&currentLanguage->on:(char * const *)&currentLanguage->off;
				break;
			case CH_SCREEN_QUICK_MENU_FILTER_DMR_TS:
				leftSide = (char * const *)&currentLanguage->dmr_ts_filter;
				rightSideConst = (uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel & DMR_TS_FILTER_PATTERN)?(char * const *)&currentLanguage->on:(char * const *)&currentLanguage->off;
				break;
			default:
				buf[0] = 0;
		}

		if (leftSide != NULL)
		{
			snprintf(buf, bufferLen, "%s:%s", *leftSide, (rightSideVar[0] ? rightSideVar : *rightSideConst));
		}
		else
		{
			snprintf(buf, bufferLen, "%s", (rightSideVar[0] ? rightSideVar : *rightSideConst));
		}

		if (i == 0)
		{
			if (!isFirstRun)
			{
				voicePromptsInit();
			}

			if (leftSide != NULL)
			{
				voicePromptsAppendLanguageString((const char * const *)leftSide);
			}

			if (rightSideVar[0] != 0)
			{
				voicePromptsAppendString(rightSideVar);
			}
			else
			{
				voicePromptsAppendLanguageString((const char * const *)rightSideConst);
			}
			promptsPlayNotAfterTx();
		}
		menuDisplayEntry(i, mNum, buf);
	}

	ucRender();
}

static void handleQuickMenuEvent(uiEvent_t *ev)
{
	bool isDirty = false;

	if (ev->events & BUTTON_EVENT)
	{
		if (repeatVoicePromptOnSK1(ev))
		{
			return;
		}
	}

	if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
	{
		uiChannelModeStopScanning();
		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
	{
		switch(menuDataGlobal.currentItemIndex)
		{
			case CH_SCREEN_QUICK_MENU_COPY2VFO:
				{
					int8_t currentTS = trxGetDMRTimeSlot();

					memcpy(&settingsVFOChannel[nonVolatileSettings.currentVFONumber].rxFreq, &currentChannelData->rxFreq, CODEPLUG_CHANNEL_DATA_STRUCT_SIZE - 16);// Don't copy the name of channel, which are in the first 16 bytes
					settingsVFOChannel[nonVolatileSettings.currentVFONumber].rxTone = currentChannelData->rxTone;
					settingsVFOChannel[nonVolatileSettings.currentVFONumber].txTone = currentChannelData->txTone;

					if (nonVolatileSettings.overrideTG == 0)
					{
						nonVolatileSettings.overrideTG = trxTalkGroupOrPcId;
					}

					// Set TS override
					tsSetManualOverride(((Channel_t)nonVolatileSettings.currentVFONumber), currentTS + 1);

					if (currentTS == 0)
					{
						settingsVFOChannel[nonVolatileSettings.currentVFONumber].flag2 &= 0xBF;
					}
					else
					{
						settingsVFOChannel[nonVolatileSettings.currentVFONumber].flag2 |= 0x40;
					}

					menuSystemPopAllAndDisplaySpecificRootMenu(UI_VFO_MODE, true);
				}
				break;
			case CH_SCREEN_QUICK_MENU_COPY_FROM_VFO:
				if (quickmenuChannelFromVFOHandled == false)
				{
					snprintf(uiDataGlobal.MessageBox.message, MESSAGEBOX_MESSAGE_LEN_MAX, "%s\n%s", currentLanguage->overwrite_qm, currentLanguage->please_confirm);
					uiDataGlobal.MessageBox.type = MESSAGEBOX_TYPE_INFO;
					uiDataGlobal.MessageBox.decoration = MESSAGEBOX_DECORATION_FRAME;
					uiDataGlobal.MessageBox.buttons = MESSAGEBOX_BUTTONS_YESNO;
					uiDataGlobal.MessageBox.validatorCallback = validateOverwriteChannel;

					menuSystemPushNewMenu(UI_MESSAGE_BOX);
					voicePromptsInit();
					voicePromptsAppendLanguageString(&currentLanguage->overwrite_qm);
					voicePromptsAppendLanguageString(&currentLanguage->please_confirm);
					voicePromptsPlay();
				}
				return;
				break;
			case CH_SCREEN_QUICK_MENU_FILTER_FM:
				settingsSet(nonVolatileSettings.analogFilterLevel, (uint8_t) uiDataGlobal.QuickMenu.tmpAnalogFilterLevel);
				trxSetAnalogFilterLevel(nonVolatileSettings.analogFilterLevel);
				menuSystemPopAllAndDisplaySpecificRootMenu(UI_CHANNEL_MODE, true);
				break;
			case CH_SCREEN_QUICK_MENU_FILTER_DMR:
				settingsSet(nonVolatileSettings.dmrDestinationFilter, (uint8_t) uiDataGlobal.QuickMenu.tmpDmrDestinationFilterLevel);
				if (trxGetMode() == RADIO_MODE_DIGITAL)
				{
					init_digital_DMR_RX();
					disableAudioAmp(AUDIO_AMP_MODE_RF);
				}
				menuSystemPopAllAndDisplaySpecificRootMenu(UI_CHANNEL_MODE, true);
				break;
			case CH_SCREEN_QUICK_MENU_FILTER_DMR_CC:
			case CH_SCREEN_QUICK_MENU_FILTER_DMR_TS:
				settingsSet(nonVolatileSettings.dmrCcTsFilter, (uint8_t) uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel);
				if (trxGetMode() == RADIO_MODE_DIGITAL)
				{
					init_digital_DMR_RX();
					disableAudioAmp(AUDIO_AMP_MODE_RF);
				}
				menuSystemPopAllAndDisplaySpecificRootMenu(UI_CHANNEL_MODE, true);
				break;
		}
		return;
	}
	else if (KEYCHECK_PRESS(ev->keys, KEY_RIGHT))
	{
		isDirty = true;
		switch(menuDataGlobal.currentItemIndex)
		{
			case CH_SCREEN_QUICK_MENU_FILTER_FM:
				if (uiDataGlobal.QuickMenu.tmpAnalogFilterLevel < NUM_ANALOG_FILTER_LEVELS - 1)
				{
					uiDataGlobal.QuickMenu.tmpAnalogFilterLevel++;
				}
				break;
			case CH_SCREEN_QUICK_MENU_FILTER_DMR:
				if (uiDataGlobal.QuickMenu.tmpDmrDestinationFilterLevel < NUM_DMR_DESTINATION_FILTER_LEVELS - 1)
				{
					uiDataGlobal.QuickMenu.tmpDmrDestinationFilterLevel++;
				}
				break;
			case CH_SCREEN_QUICK_MENU_FILTER_DMR_CC:
				if (!(uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel & DMR_CC_FILTER_PATTERN))
				{
					uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel |= DMR_CC_FILTER_PATTERN;
				}
				break;
			case CH_SCREEN_QUICK_MENU_FILTER_DMR_TS:
				if (!(uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel & DMR_TS_FILTER_PATTERN))
				{
					uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel |= DMR_TS_FILTER_PATTERN;
				}
				break;

		}
	}
	else
	{
		if (KEYCHECK_PRESS(ev->keys, KEY_LEFT))
		{
			isDirty = true;
			switch(menuDataGlobal.currentItemIndex)
			{
			case CH_SCREEN_QUICK_MENU_FILTER_FM:
				if (uiDataGlobal.QuickMenu.tmpAnalogFilterLevel > ANALOG_FILTER_NONE)
				{
					uiDataGlobal.QuickMenu.tmpAnalogFilterLevel--;
				}
				break;
			case CH_SCREEN_QUICK_MENU_FILTER_DMR:
				if (uiDataGlobal.QuickMenu.tmpDmrDestinationFilterLevel > DMR_DESTINATION_FILTER_NONE)
				{
					uiDataGlobal.QuickMenu.tmpDmrDestinationFilterLevel--;
				}
				break;
			case CH_SCREEN_QUICK_MENU_FILTER_DMR_CC:
				if ((uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel & DMR_CC_FILTER_PATTERN))
				{
					uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel &= ~DMR_CC_FILTER_PATTERN;
				}
				break;
			case CH_SCREEN_QUICK_MENU_FILTER_DMR_TS:
				if ((uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel & DMR_TS_FILTER_PATTERN))
				{
					uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel &= ~DMR_TS_FILTER_PATTERN;
				}
				break;
			}
		}
		else
		{
			if (KEYCHECK_PRESS(ev->keys, KEY_DOWN))
			{
				isDirty = true;
				menuSystemMenuIncrement(&menuDataGlobal.currentItemIndex, NUM_CH_SCREEN_QUICK_MENU_ITEMS);
				menuQuickChannelExitStatus |= MENU_STATUS_LIST_TYPE;
			}
			else
			{
				if (KEYCHECK_PRESS(ev->keys, KEY_UP))
				{
					isDirty = true;
					menuSystemMenuDecrement(&menuDataGlobal.currentItemIndex, NUM_CH_SCREEN_QUICK_MENU_ITEMS);
					menuQuickChannelExitStatus |= MENU_STATUS_LIST_TYPE;
				}
			}
		}
	}

	if (isDirty)
	{
		updateQuickMenuScreen(false);
	}
}

//Scan Mode
static void scanStart(bool longPressBeep)
{
	// At least two channels are needed to run a scan process.
	if (canCurrentZoneBeScanned(&uiDataGlobal.Scan.availableChannelsCount) == false)
	{
		menuChannelExitStatus |= MENU_STATUS_ERROR;
		return;
	}

	if (voicePromptsIsPlaying())
	{
		voicePromptsTerminate();
	}

	directChannelNumber = 0;
	uiDataGlobal.Scan.direction = 1;

	// Clear all nuisance delete channels at start of scanning
	for (int i = 0; i < MAX_ZONE_SCAN_NUISANCE_CHANNELS; i++)
	{
		uiDataGlobal.Scan.nuisanceDelete[i] = -1;
	}
	uiDataGlobal.Scan.nuisanceDeleteIndex = 0;

	uiDataGlobal.Scan.active = true;
	uiDataGlobal.Scan.timer = SCAN_SHORT_PAUSE_TIME;
	uiDataGlobal.Scan.state = SCAN_SCANNING;
	uiDataGlobal.Scan.lastIteration = false;

	uiDataGlobal.Scan.stepTimeMilliseconds = settingsGetScanStepTimeMilliseconds();

	if ((trxGetMode() == RADIO_MODE_DIGITAL) && (trxDMRModeRx != DMR_MODE_SFR) && (uiDataGlobal.Scan.stepTimeMilliseconds < SCAN_DMR_SIMPLEX_MIN_INTERVAL))
	{
		uiDataGlobal.Scan.timerReload = SCAN_DMR_SIMPLEX_MIN_INTERVAL;
	}
	else
	{
		uiDataGlobal.Scan.timerReload = uiDataGlobal.Scan.stepTimeMilliseconds;
	}

	uiDataGlobal.Scan.scanType = SCAN_TYPE_NORMAL_STEP;

	// Need to set the melody here, otherwise long press will remain silent
	// since beeps aren't allowed while scanning
	if (longPressBeep)
	{
		soundSetMelody(MELODY_KEY_LONG_BEEP);
	}

	// Set current channel index
	if (CODEPLUG_ZONE_IS_ALLCHANNELS(currentZone))
	{
		nextChannelIndex = nonVolatileSettings.currentChannelIndexInAllZone;
	}
	else
	{
		nextChannelIndex = currentZone.channels[nonVolatileSettings.currentChannelIndexInZone];
	}

	nextChannelReady = false;
}

static void updateTrxID(void)
{
	if (nonVolatileSettings.overrideTG != 0)
	{
		trxTalkGroupOrPcId = nonVolatileSettings.overrideTG;
	}
	else
	{
		/*
		 * VK3KYY
		 * I can't see a compelling reason to remove the TS override when changing TG
		 * The function trxUpdateTsForCurrentChannelWithSpecifiedContact(), used below, is used to handle the TS
		 *
			tsSetManualOverride(CHANNEL_CHANNEL, TS_NO_OVERRIDE);
		*/
		if ((currentRxGroupData.name[0] != 0) && (nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE] < currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup))
		{
			codeplugContactGetDataForIndex(currentRxGroupData.contacts[nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE]], &currentContactData);
		}
		else
		{
			codeplugContactGetDataForIndex(currentChannelData->contact, &currentContactData);
		}

		tsSetContactHasBeenOverriden(CHANNEL_CHANNEL, false);

		trxUpdateTsForCurrentChannelWithSpecifiedContact(&currentContactData);
		trxTalkGroupOrPcId = currentContactData.tgNumber;

		if (currentContactData.callType == CONTACT_CALLTYPE_PC)
		{
			trxTalkGroupOrPcId |= (PC_CALL_FLAG << 24);
		}
	}
	lastHeardClearLastID();
	menuPrivateCallClear();
}

static void scanning(void)
{
	if((uiDataGlobal.Scan.state == SCAN_SCANNING) && (uiDataGlobal.Scan.timer > SCAN_SKIP_CHANNEL_INTERVAL) && (uiDataGlobal.Scan.timer < (uiDataGlobal.Scan.timerReload - SCAN_FREQ_CHANGE_SETTLING_INTERVAL)))							    			//after initial settling time
	{
		//test for presence of RF Carrier.
		// In FM mode the DMR slot_state will always be DMR_STATE_IDLE
		if ((slot_state != DMR_STATE_IDLE) && ((dmrMonitorCapturedTS != -1) &&
				(((trxDMRModeRx == DMR_MODE_DMO) && (dmrMonitorCapturedTS == trxGetDMRTimeSlot())) || trxDMRModeRx == DMR_MODE_RMO)))
		{
			uiDataGlobal.Scan.state = SCAN_PAUSED;

#if ! defined(PLATFORM_GD77S) // GD77S handle voice prompts on its own
			// Reload the channel as voice prompts aren't set while scanning
			if (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1)
			{
				loadChannelData(false, true);
			}
#endif

			if (nonVolatileSettings.scanModePause == SCAN_MODE_STOP)
			{
				uiChannelModeStopScanning();
				return;
			}
			else
			{
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN; // Force screen refresh

				uiDataGlobal.Scan.timer = nonVolatileSettings.scanDelay * 1000;
			}
		}
		else
		{
			if(trxCarrierDetected())
			{
#if ! defined(PLATFORM_GD77S) // GD77S handle voice prompts on its own
				// Reload the channel as voice prompts aren't set while scanning
				if (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1)
				{
					loadChannelData(false, true);
				}
#endif
				if (nonVolatileSettings.scanModePause == SCAN_MODE_STOP)
				{
					uiChannelModeStopScanning();
					return;
				}
				else
				{
					uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN; // Force screen refresh

					uiDataGlobal.Scan.timer = SCAN_SHORT_PAUSE_TIME;	//start short delay to allow full detection of signal
					uiDataGlobal.Scan.state = SCAN_SHORT_PAUSED;		//state 1 = pause and test for valid signal that produces audio
				}

			}
		}
	}

	// Only do this once if scan mode is PAUSE do it every time if scan mode is HOLD
	if(((uiDataGlobal.Scan.state == SCAN_PAUSED) && (nonVolatileSettings.scanModePause == SCAN_MODE_HOLD)) || (uiDataGlobal.Scan.state == SCAN_SHORT_PAUSED))
	{
	    if (getAudioAmpStatus() & AUDIO_AMP_MODE_RF)
	    {
	    	uiDataGlobal.Scan.timer = nonVolatileSettings.scanDelay * 1000;
	    	uiDataGlobal.Scan.state = SCAN_PAUSED;
	    }
	}

	if(uiDataGlobal.Scan.timer > 0)
	{
		if (nextChannelReady == false)
		{
			scanSearchForNextChannel();
		}

		uiDataGlobal.Scan.timer--;
	}
	else
	{
		if (nextChannelReady)
		{
			scanApplyNextChannel();

			// When less than 2 channel remain in the Zone
			if (uiDataGlobal.Scan.lastIteration)
			{
				uiChannelModeStopScanning();
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
				uiChannelModeUpdateScreen(0);
				return;
			}
		}

		uiDataGlobal.Scan.state = SCAN_SCANNING;													//state 0 = settling and test for carrier present.
	}
}

void uiChannelModeStopScanning(void)
{
	uiDataGlobal.Scan.active = false;
	uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN; // Force screen refresh

#if ! defined(PLATFORM_GD77S) // GD77S handle voice prompts on its own
	// Reload the channel as voice prompts aren't set while scanning
	if (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1)
	{
		loadChannelData(false, true);
	}
#endif
}

bool uiChannelModeIsScanning(void)
{
	return uiDataGlobal.Scan.active;
}

void uiChannelModeColdStart(void)
{
	currentChannelData->rxFreq = 0;	// Force to re-read codeplug data (needed due to "All Channels" translation)
}



#if defined(PLATFORM_GD77S)
bool uiChannelModeTransmitDTMFContactForGD77S(void)
{
	if (GD77SParameters.uiMode == GD77S_UIMODE_DTMF_CONTACTS)
	{
		if (GD77SParameters.dtmfListCount > 0)
		{
			// start dtmf sequence
			if(dtmfSequenceIsKeying() == false)
			{
				struct_codeplugDTMFContact_t dtmfContact;

				if (voicePromptsIsPlaying())
				{
					voicePromptsTerminate();
				}

				codeplugDTMFContactGetDataForNumber(GD77SParameters.dtmfListSelected + 1, &dtmfContact);
				dtmfSequencePrepare(dtmfContact.code, true);
			}
			else
			{
				dtmfSequenceStop();
				dtmfSequenceTick(false);
				dtmfSequenceReset();
			}
		}
		else
		{
			voicePromptsInit();
			voicePromptsAppendLanguageString(&currentLanguage->empty_list);
			voicePromptsPlay();
		}

		return true;
	}

	return false;
}

void toggleTimeslotForGD77S(void)
{
	if (trxGetMode() == RADIO_MODE_DIGITAL)
	{
		// Toggle timeslot
		trxSetDMRTimeSlot(1 - trxGetDMRTimeSlot());
		tsSetManualOverride(CHANNEL_CHANNEL, (trxGetDMRTimeSlot() + 1));

		//	init_digital();
		disableAudioAmp(AUDIO_AMP_MODE_RF);
		clearActiveDMRID();
		lastHeardClearLastID();
		uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
		uiChannelModeUpdateScreen(0);
	}
}

void uiChannelModeHeartBeatActivityForGD77S(uiEvent_t *ev)
{
	static const uint32_t periods[] = { 5000, 100, 100, 100, 100, 100 };
	static const uint32_t periodsScan[] = { 2000, 50, 2000, 50, 2000, 50 };
	static uint8_t        beatRoll = 0;
	static uint32_t       mTime = 0;

	// <paranoid_mode>
	//   We use real time GPIO readouts, as LED could be turned on/off by another task.
	// </paranoid_mode>
	if ((LEDs_PinRead(GPIO_LEDred, Pin_LEDred) || LEDs_PinRead(GPIO_LEDgreen, Pin_LEDgreen)) // Any led is ON
			&& (trxTransmissionEnabled || (uiDataGlobal.DTMFContactList.isKeying) || (ev->buttons & BUTTON_PTT) || (getAudioAmpStatus() & (AUDIO_AMP_MODE_RF | AUDIO_AMP_MODE_BEEP | AUDIO_AMP_MODE_PROMPT)) || trxCarrierDetected() || ev->hasEvent)) // we're transmitting, or receiving, or user interaction.
	{
		// Turn off the red LED, if not transmitting
		if (LEDs_PinRead(GPIO_LEDred, Pin_LEDred) // Red is ON
				&& ((uiDataGlobal.DTMFContactList.isKeying == false) && ((trxTransmissionEnabled == false) || ((ev->buttons & BUTTON_PTT) == 0)))) // No TX
		{
			LEDs_PinWrite(GPIO_LEDred, Pin_LEDred, 0);
		}

		// Turn off the green LED, if not receiving, or no AF output
		if (LEDs_PinRead(GPIO_LEDgreen, Pin_LEDgreen)) // Green is ON
		{
			if ((trxTransmissionEnabled || (uiDataGlobal.DTMFContactList.isKeying) || (ev->buttons & BUTTON_PTT))
					|| ((trxGetMode() == RADIO_MODE_DIGITAL) && (slot_state != DMR_STATE_IDLE))
					|| (((getAudioAmpStatus() & (AUDIO_AMP_MODE_RF | AUDIO_AMP_MODE_BEEP | AUDIO_AMP_MODE_PROMPT)) != 0) || trxCarrierDetected()))
			{
				if ((ev->buttons & BUTTON_PTT) && (trxTransmissionEnabled == false)) // RX Only or Out of Band
				{
					LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
				}
			}
			else
			{
				LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
			}
		}

		// Reset pattern sequence
		beatRoll = 0;
		// And update the timer for the next first starting (OFF for 5 seconds) blink sequence.
		mTime = ev->time;
		return;
	}

	// Nothing is happening, blink
	if (((trxTransmissionEnabled == false) && (uiDataGlobal.DTMFContactList.isKeying == false) && ((ev->buttons & BUTTON_PTT) == 0))
			&& ((ev->hasEvent == false) && ((getAudioAmpStatus() & (AUDIO_AMP_MODE_RF | AUDIO_AMP_MODE_BEEP | AUDIO_AMP_MODE_PROMPT)) == 0) && (trxCarrierDetected() == false)))
	{
		// Blink both LEDs to have Orange color
		if ((ev->time - mTime) > (uiDataGlobal.Scan.active ? periodsScan[beatRoll] : periods[beatRoll]))
		{
			mTime = ev->time;
			beatRoll = (beatRoll + 1) % (uiDataGlobal.Scan.active ? (sizeof(periodsScan) / sizeof(periodsScan[0])) : (sizeof(periods) / sizeof(periods[0])));
			LEDs_PinWrite(GPIO_LEDred, Pin_LEDred, (beatRoll % 2));
			LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, (beatRoll % 2));
		}
	}
	else
	{
		// Reset pattern sequence
		beatRoll = 0;
		// And update the timer for the next first starting (OFF for 5 seconds) blink sequence.
		mTime = ev->time;
	}
}

static uint16_t getCurrentChannelInCurrentZoneForGD77S(void)
{
	return (CODEPLUG_ZONE_IS_ALLCHANNELS(currentZone) ? nonVolatileSettings.currentChannelIndexInAllZone : nonVolatileSettings.currentChannelIndexInZone + 1);
}

static void checkAndUpdateSelectedChannelForGD77S(uint16_t chanNum, bool forceSpeech)
{
	bool updateDisplay = false;

	if(CODEPLUG_ZONE_IS_ALLCHANNELS(currentZone))
	{
		GD77SParameters.channelOutOfBounds = false;
		if (codeplugAllChannelsIndexIsInUse(chanNum))
		{
			if (chanNum != nonVolatileSettings.currentChannelIndexInAllZone)
			{
				settingsSet(nonVolatileSettings.currentChannelIndexInAllZone, (int16_t) chanNum);
				loadChannelData(false, false);
				updateDisplay = true;
			}
		}
		else
		{
			GD77SParameters.channelOutOfBounds = true;
			if (voicePromptsIsPlaying() == false)
			{
				voicePromptsInit();
				voicePromptsAppendPrompt(PROMPT_CHANNEL);
				voicePromptsAppendLanguageString(&currentLanguage->error);
				voicePromptsPlay();
			}
		}
	}
	else
	{
		if ((chanNum - 1) < currentZone.NOT_IN_CODEPLUGDATA_numChannelsInZone)
		{
			GD77SParameters.channelOutOfBounds = false;
			if ((chanNum - 1) != nonVolatileSettings.currentChannelIndexInZone)
			{
				settingsSet(nonVolatileSettings.currentChannelIndexInZone, (int16_t) (chanNum - 1));
				loadChannelData(false, false);
				updateDisplay = true;
			}
		}
		else
		{
			GD77SParameters.channelOutOfBounds = true;
			if (voicePromptsIsPlaying() == false)
			{
				voicePromptsInit();
				voicePromptsAppendPrompt(PROMPT_CHANNEL);
				voicePromptsAppendLanguageString(&currentLanguage->error);
				voicePromptsPlay();
			}
		}
	}

	// Prevent TXing while an invalid channel is selected
	if (getCurrentChannelInCurrentZoneForGD77S() != chanNum)
	{
		PTTLocked = true;
	}
	else
	{
		if (PTTLocked)
		{
			PTTLocked = false;
			forceSpeech = true;
		}
	}

	if (updateDisplay || forceSpeech)
	{
		if (GD77SParameters.channelOutOfBounds == false)
		{
			char buf[17];

			voicePromptsInit();
			voicePromptsAppendPrompt(PROMPT_CHANNEL);
			voicePromptsAppendInteger(chanNum);
			voicePromptsAppendPrompt(PROMPT_SILENCE);
			codeplugUtilConvertBufToString(currentChannelData->name, buf, 16);
			voicePromptsAppendString(buf);
			voicePromptsPlay();
		}

		if (!forceSpeech)
		{
			uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
			uiChannelModeUpdateScreen(0);
		}
	}
}

static void buildSpeechChannelDetailsForGD77S()
{
	char buf[17];

	codeplugUtilConvertBufToString(currentChannelData->name, buf, 16);
	voicePromptsAppendString(buf);

	voicePromptsAppendPrompt(PROMPT_SILENCE);
	announceFrequency();
	voicePromptsAppendPrompt(PROMPT_SILENCE);

	if (trxGetMode() == RADIO_MODE_DIGITAL)
	{
		announceContactNameTgOrPc(voicePromptsIsPlaying());
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		announceTS();
		voicePromptsAppendPrompt(PROMPT_SILENCE);
		announceCC();
	}
	else
	{
		if (currentChannelData->rxTone != CODEPLUG_CSS_NONE)
		{
			bool isCTCSS = codeplugChannelToneIsCTCSS(currentChannelData->rxTone);

			buildCSSCodeVoicePrompts(currentChannelData->rxTone,
					(isCTCSS ? CSS_CTCSS : ((currentChannelData->rxTone & CODEPLUG_DCS_INVERTED_MASK) ? CSS_DCS_INVERTED : CSS_DCS)), DIRECTION_RECEIVE, true);
			voicePromptsAppendPrompt(PROMPT_SILENCE);
		}

		if (currentChannelData->txTone != CODEPLUG_CSS_NONE)
		{
			bool isCTCSS = codeplugChannelToneIsCTCSS(currentChannelData->txTone);

			buildCSSCodeVoicePrompts(currentChannelData->txTone,
					(isCTCSS ? CSS_CTCSS : ((currentChannelData->txTone & CODEPLUG_DCS_INVERTED_MASK) ? CSS_DCS_INVERTED : CSS_DCS)), DIRECTION_TRANSMIT, true);
		}
	}
}

static void buildSpeechUiModeForGD77S(GD77S_UIMODES_t uiMode)
{
	char buf[17];

	if (voicePromptsIsPlaying())
	{
		voicePromptsTerminate();
	}

	switch (uiMode)
	{
		case GD77S_UIMODE_TG_OR_SQUELCH: // Channel
			codeplugUtilConvertBufToString(currentChannelData->name, buf, 16);
			voicePromptsAppendString(buf);

			if (trxGetMode() == RADIO_MODE_DIGITAL)
			{
				announceTS();
			}
			else
			{
				announceSquelchLevel(voicePromptsIsPlaying());
			}
			break;

		case GD77S_UIMODE_SCAN: // Scan
			voicePromptsAppendLanguageString(&currentLanguage->scan);
			voicePromptsAppendLanguageString(uiDataGlobal.Scan.active ? &currentLanguage->on : &currentLanguage->off);
			break;

		case GD77S_UIMODE_TS: // Timeslot
			if (trxGetMode() == RADIO_MODE_DIGITAL)
			{
				announceTS();
			}
			break;

		case GD77S_UIMODE_CC: // Color code
			if (trxGetMode() == RADIO_MODE_DIGITAL)
			{
				announceCC();
			}
			break;

		case GD77S_UIMODE_FILTER: // DMR/Analog filter
			voicePromptsAppendLanguageString(&currentLanguage->filter);
			if (trxGetMode() == RADIO_MODE_DIGITAL)
			{
				if (nonVolatileSettings.dmrDestinationFilter == DMR_DESTINATION_FILTER_NONE)
				{
					voicePromptsAppendLanguageString(&currentLanguage->none);
				}
				else
				{
					voicePromptsAppendString((char *)DMR_DESTINATION_FILTER_LEVELS[nonVolatileSettings.dmrDestinationFilter - 1]);
				}

			}
			else
			{
				if (nonVolatileSettings.analogFilterLevel == ANALOG_FILTER_NONE)
				{
					voicePromptsAppendLanguageString(&currentLanguage->none);
				}
				else
				{
					voicePromptsAppendString((char *)ANALOG_FILTER_LEVELS[nonVolatileSettings.analogFilterLevel - 1]);
				}
			}
			break;

		case GD77S_UIMODE_DTMF_CONTACTS:
			if (GD77SParameters.dtmfListCount > 0)
			{
				struct_codeplugDTMFContact_t dtmfContact;

				codeplugDTMFContactGetDataForNumber(GD77SParameters.dtmfListSelected + 1, &dtmfContact);
				codeplugUtilConvertBufToString(dtmfContact.name, buf, 16);
				voicePromptsAppendString(buf);
			}
			else
			{
				voicePromptsAppendLanguageString(&currentLanguage->empty_list);
			}
			break;

		case GD77S_UIMODE_ZONE: // Zone
			announceZoneName(voicePromptsIsPlaying());
			break;

		case GD77S_UIMODE_POWER: // Power
			announcePowerLevel(voicePromptsIsPlaying());
			break;

		case GD77S_UIMODE_MAX:
			break;
	}
}

static void handleEventForGD77S(uiEvent_t *ev)
{
	if (ev->events & ROTARY_EVENT)
	{
		if (dtmfSequenceIsKeying())
		{
			dtmfSequenceStop();
		}

		if (!trxTransmissionEnabled && (ev->rotary > 0))
		{
			if (uiDataGlobal.Scan.active)
			{
				uiChannelModeStopScanning();
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
				uiChannelModeUpdateScreen(0);
			}

			settingsSet(nonVolatileSettings.overrideTG, 0);
			checkAndUpdateSelectedChannelForGD77S(ev->rotary, false);
			clearActiveDMRID();
			lastHeardClearLastID();
		}
	}

	if (handleMonitorMode(ev))
	{
		return;
	}

	if (ev->events & BUTTON_EVENT)
	{
		if (dtmfSequenceIsKeying() && (ev->buttons & (BUTTON_SK1 | BUTTON_SK2 | BUTTON_ORANGE)))
		{
			dtmfSequenceStop();
		}

		if (BUTTONCHECK_SHORTUP(ev, BUTTON_ORANGE) && uiDataGlobal.Scan.active)
		{
			uiChannelModeStopScanning();
			uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
			uiChannelModeUpdateScreen(0);

			if (voicePromptsIsPlaying())
			{
				voicePromptsTerminate();
			}

			voicePromptsInit();
			buildSpeechUiModeForGD77S(GD77S_UIMODE_SCAN);
			voicePromptsPlay();
			return;
		}

		if (BUTTONCHECK_LONGDOWN(ev, BUTTON_ORANGE) && (uiDataGlobal.DTMFContactList.isKeying == false))
		{
			announceItem(PROMPT_SEQUENCE_BATTERY, PROMPT_THRESHOLD_3);
			return;
		}
		else if (BUTTONCHECK_SHORTUP(ev, BUTTON_ORANGE) && (uiDataGlobal.DTMFContactList.isKeying == false))
		{
			voicePrompt_t vp = NUM_VOICE_PROMPTS;
			char * const *vpString = NULL;

			GD77SParameters.uiMode = (GD77S_UIMODES_t) (GD77SParameters.uiMode + 1) % GD77S_UIMODE_MAX;

			//skip over Digital controls if the radio is in Analog mode
			if (trxGetMode() == RADIO_MODE_ANALOG)
			{
				// Analog
				if ((GD77SParameters.uiMode == GD77S_UIMODE_TS) ||
					(GD77SParameters.uiMode == GD77S_UIMODE_CC))
				{
					GD77SParameters.uiMode = GD77S_UIMODE_FILTER;
				}
			}
			else
			{
				// digital
				if (GD77SParameters.uiMode == GD77S_UIMODE_DTMF_CONTACTS)
				{
					GD77SParameters.uiMode = GD77S_UIMODE_ZONE;
				}
			}

			switch (GD77SParameters.uiMode)
			{
				case GD77S_UIMODE_TG_OR_SQUELCH: // Channel Mode
					if (trxGetMode() == RADIO_MODE_DIGITAL)
					{
						vp = PROMPT_CHANNEL_MODE;
					}
					else
					{
						vpString = (char * const *)&currentLanguage->squelch;
					}
					break;

				case GD77S_UIMODE_SCAN:
					vp = PROMPT_SCAN_MODE;
					break;

				case GD77S_UIMODE_TS: // Timeslot Mode
					vp = PROMPT_TIMESLOT_MODE;
					break;

				case GD77S_UIMODE_CC: // ColorCode Mode
					vp = PROMPT_COLORCODE_MODE;
					break;

				case GD77S_UIMODE_FILTER: // DMR/Analog Filter
					vp = PROMPT_FILTER_MODE;
					break;

				case GD77S_UIMODE_DTMF_CONTACTS:
					vpString = (char * const *)&currentLanguage->dtmf_contact_list;
					break;

				case GD77S_UIMODE_ZONE: // Zone Mode
					vp = PROMPT_ZONE_MODE;
					break;

				case GD77S_UIMODE_POWER: // Power Mode
					vp = PROMPT_POWER_MODE;
					break;

				case GD77S_UIMODE_MAX:
					break;
			}

			if ((vp != NUM_VOICE_PROMPTS) || (vpString != NULL))
			{
				voicePromptsInit();
				if (vpString)
				{
					voicePromptsAppendLanguageString((const char * const *)vpString);
					voicePromptsAppendPrompt(PROMPT_MODE);
				}
				else
				{
					voicePromptsAppendPrompt(vp);
				}
				voicePromptsAppendPrompt(PROMPT_SILENCE);
				buildSpeechUiModeForGD77S(GD77SParameters.uiMode);
				voicePromptsPlay();
			}
		}
		else if (BUTTONCHECK_LONGDOWN(ev, BUTTON_SK1) && (monitorModeData.isEnabled == false) && (uiDataGlobal.DTMFContactList.isKeying == false))
		{
			if (GD77SParameters.channelOutOfBounds == false)
			{
				voicePromptsInit();
				buildSpeechChannelDetailsForGD77S();
				voicePromptsPlay();
			}
		}
		else if (BUTTONCHECK_SHORTUP(ev, BUTTON_SK1) && (uiDataGlobal.DTMFContactList.isKeying == false))
		{
			switch (GD77SParameters.uiMode)
			{
				case GD77S_UIMODE_TG_OR_SQUELCH:
					if (trxGetMode() == RADIO_MODE_DIGITAL)
					{
						// Next in TGList
						if (nonVolatileSettings.overrideTG == 0)
						{
							settingsIncrement(nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE], 1);
							if (nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE] > (currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup - 1))
							{
								settingsSet(nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE], 0);
							}
						}
						settingsSet(nonVolatileSettings.overrideTG, 0);// setting the override TG to 0 indicates the TG is not overridden
						menuPrivateCallClear();
						updateTrxID();
						uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
						uiChannelModeUpdateScreen(0);
						announceItem(PROMPT_SEQUENCE_CONTACT_TG_OR_PC, PROMPT_THRESHOLD_3);
					}
					else
					{
						if(currentChannelData->sql == 0)			//If we were using default squelch level
						{
							currentChannelData->sql = nonVolatileSettings.squelchDefaults[trxCurrentBand[TRX_RX_FREQ_BAND]];			//start the adjustment from that point.
						}

						if (currentChannelData->sql < CODEPLUG_MAX_VARIABLE_SQUELCH)
						{
							currentChannelData->sql++;
						}

						announceItem(PROMPT_SQUENCE_SQUELCH, PROMPT_THRESHOLD_3);
					}
					break;

				case GD77S_UIMODE_SCAN:
					if (uiDataGlobal.Scan.active)
					{
						uiChannelModeStopScanning();
						uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
						uiChannelModeUpdateScreen(0);
					}
					else
					{
						scanStart(false);
					}

					voicePromptsInit();
					voicePromptsAppendLanguageString(&currentLanguage->scan);
					voicePromptsAppendLanguageString(uiDataGlobal.Scan.active ? &currentLanguage->on : &currentLanguage->off);
					voicePromptsPlay();
					break;

				case GD77S_UIMODE_TS:
					if (trxGetMode() == RADIO_MODE_DIGITAL)
					{
						toggleTimeslotForGD77S();
						announceItem(PROMPT_SEQUENCE_TS, PROMPT_THRESHOLD_3);
					}
					break;

				case GD77S_UIMODE_CC:
					if (trxGetMode() == RADIO_MODE_DIGITAL)
					{
						if (currentChannelData->rxColor < 15)
						{
							currentChannelData->rxColor++;
							trxSetDMRColourCode(currentChannelData->rxColor);
						}

						voicePromptsInit();
						announceCC();
						voicePromptsPlay();
					}
					break;

				case GD77S_UIMODE_FILTER:
					if (trxGetMode() == RADIO_MODE_DIGITAL)
					{
						if (nonVolatileSettings.dmrDestinationFilter < NUM_DMR_DESTINATION_FILTER_LEVELS - 1)
						{
							settingsIncrement(nonVolatileSettings.dmrDestinationFilter, 1);
							init_digital_DMR_RX();
							disableAudioAmp(AUDIO_AMP_MODE_RF);
						}
					}
					else
					{
						if (nonVolatileSettings.analogFilterLevel < NUM_ANALOG_FILTER_LEVELS - 1)
						{
							settingsIncrement(nonVolatileSettings.analogFilterLevel, 1);
							trxSetAnalogFilterLevel(nonVolatileSettings.analogFilterLevel);
						}
					}

					voicePromptsInit();
					buildSpeechUiModeForGD77S(GD77SParameters.uiMode);
					voicePromptsPlay();
					break;

				case GD77S_UIMODE_DTMF_CONTACTS:
					// select next DTMF contact and spell it
					if (GD77SParameters.dtmfListCount > 0)
					{
						GD77SParameters.dtmfListSelected = (GD77SParameters.dtmfListSelected + 1) % GD77SParameters.dtmfListCount;
					}
					voicePromptsInit();
					buildSpeechUiModeForGD77S(GD77SParameters.uiMode);
					voicePromptsPlay();

					break;

				case GD77S_UIMODE_ZONE: // Zones
					// No "All Channels" on GD77S
					nonVolatileSettings.currentZone = (nonVolatileSettings.currentZone + 1) % (codeplugZonesGetCount() - 1);

					settingsSet(nonVolatileSettings.overrideTG, 0); // remove any TG override
					tsSetManualOverride(CHANNEL_CHANNEL, TS_NO_OVERRIDE);
					settingsSet(nonVolatileSettings.currentChannelIndexInZone, (int16_t) -2); // Will be updated when reloading the UiChannelMode screen
					currentChannelData->rxFreq = 0x00; // Flag to the Channel screen that the channel data is now invalid and needs to be reloaded

					menuSystemPopAllAndDisplaySpecificRootMenu(UI_CHANNEL_MODE, true);
					GD77SParameters.uiMode = GD77S_UIMODE_ZONE;

					announceItem(PROMPT_SEQUENCE_ZONE, PROMPT_THRESHOLD_3);
					break;

				case GD77S_UIMODE_POWER: // Power
					increasePowerLevel(true);// true = Allow 5W++
					break;

				case GD77S_UIMODE_MAX:
					break;
			}
		}
		else if (BUTTONCHECK_LONGDOWN(ev, BUTTON_SK2) && (monitorModeData.isEnabled == false) && (uiDataGlobal.DTMFContactList.isKeying == false))
		{
			uint32_t tg = (LinkHead->talkGroupOrPcId & 0xFFFFFF);

			// If Blue button is long pressed during reception it sets the Tx TG to the incoming TG
			if (uiDataGlobal.isDisplayingQSOData && BUTTONCHECK_DOWN(ev, BUTTON_SK2) && (trxGetMode() == RADIO_MODE_DIGITAL) &&
					((trxTalkGroupOrPcId != tg) ||
							((dmrMonitorCapturedTS != -1) && (dmrMonitorCapturedTS != trxGetDMRTimeSlot())) ||
							(trxGetDMRColourCode() != currentChannelData->rxColor)))
			{
				voicePromptsInit();
				voicePromptsAppendLanguageString(&currentLanguage->select_tx);
				voicePromptsPlay();

				lastHeardClearLastID();

				// Set TS to overriden TS
				if ((dmrMonitorCapturedTS != -1) && (dmrMonitorCapturedTS != trxGetDMRTimeSlot()))
				{
					trxSetDMRTimeSlot(dmrMonitorCapturedTS);
					tsSetManualOverride(CHANNEL_CHANNEL, (dmrMonitorCapturedTS + 1));
				}
				if (trxTalkGroupOrPcId != tg)
				{
					if ((tg >> 24) & PC_CALL_FLAG)
					{
						acceptPrivateCall(tg & 0xffffff, -1);
					}
					else
					{
						trxTalkGroupOrPcId = tg;
						settingsSet(nonVolatileSettings.overrideTG, trxTalkGroupOrPcId);
					}
				}

				currentChannelData->rxColor = trxGetDMRColourCode();// Set the CC to the current CC, which may have been determined by the CC finding algorithm in C6000.c

				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
				uiChannelModeUpdateScreen(0);
				return;
			}
		}
		else if (BUTTONCHECK_SHORTUP(ev, BUTTON_SK2) && (uiDataGlobal.DTMFContactList.isKeying == false))
		{
			switch (GD77SParameters.uiMode)
			{
				case GD77S_UIMODE_TG_OR_SQUELCH: // Previous in TGList
					if (trxGetMode() == RADIO_MODE_DIGITAL)
					{
						// To Do change TG in on same channel freq
						if (nonVolatileSettings.overrideTG == 0)
						{
							settingsDecrement(nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE], 1);
							if (nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE] < 0)
							{
								settingsSet(nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE], (int16_t) (currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup - 1));
							}
						}
						settingsSet(nonVolatileSettings.overrideTG, 0);// setting the override TG to 0 indicates the TG is not overridden
						menuPrivateCallClear();
						updateTrxID();
						uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
						uiChannelModeUpdateScreen(0);
						announceItem(PROMPT_SEQUENCE_CONTACT_TG_OR_PC, PROMPT_THRESHOLD_3);
					}
					else
					{
						if(currentChannelData->sql == 0)			//If we were using default squelch level
						{
							currentChannelData->sql = nonVolatileSettings.squelchDefaults[trxCurrentBand[TRX_RX_FREQ_BAND]];			//start the adjustment from that point.
						}

						if (currentChannelData->sql > CODEPLUG_MIN_VARIABLE_SQUELCH)
						{
							currentChannelData->sql--;
						}

						announceItem(PROMPT_SQUENCE_SQUELCH, PROMPT_THRESHOLD_3);
					}
					break;

				case GD77S_UIMODE_SCAN:
					if (uiDataGlobal.Scan.active)
					{
						// if we are scanning and down key is pressed then enter current channel into nuisance delete array.
						if(uiDataGlobal.Scan.state == SCAN_PAUSED)
						{
							// There is two channels available in the Zone, just stop scanning
							if (uiDataGlobal.Scan.nuisanceDeleteIndex == (uiDataGlobal.Scan.availableChannelsCount - 2))
							{
								uiDataGlobal.Scan.lastIteration = true;
							}

							uiDataGlobal.Scan.nuisanceDelete[uiDataGlobal.Scan.nuisanceDeleteIndex] = uiDataGlobal.currentSelectedChannelNumber;
							uiDataGlobal.Scan.nuisanceDeleteIndex = (uiDataGlobal.Scan.nuisanceDeleteIndex + 1) % MAX_ZONE_SCAN_NUISANCE_CHANNELS;
							uiDataGlobal.Scan.timer = SCAN_SKIP_CHANNEL_INTERVAL;	//force scan to continue;
							uiDataGlobal.Scan.state = SCAN_SCANNING;
							return;
						}

						// Left key reverses the scan direction
						if (uiDataGlobal.Scan.state == SCAN_SCANNING)
						{
							uiDataGlobal.Scan.direction *= -1;
							return;
						}
					}
					break;

				case GD77S_UIMODE_TS:
					if (trxGetMode() == RADIO_MODE_DIGITAL)
					{
						toggleTimeslotForGD77S();
						announceItem(PROMPT_SEQUENCE_TS, PROMPT_THRESHOLD_3);
					}
					break;

				case GD77S_UIMODE_CC:
					if (trxGetMode() == RADIO_MODE_DIGITAL)
					{
						if (currentChannelData->rxColor > 0)
						{
							currentChannelData->rxColor--;
							trxSetDMRColourCode(currentChannelData->rxColor);
						}

						voicePromptsInit();
						announceCC();
						voicePromptsPlay();
					}
					break;

				case GD77S_UIMODE_FILTER:
					if (trxGetMode() == RADIO_MODE_DIGITAL)
					{
						if (nonVolatileSettings.dmrDestinationFilter > DMR_DESTINATION_FILTER_NONE)
						{
							settingsDecrement(nonVolatileSettings.dmrDestinationFilter, 1);
							init_digital_DMR_RX();
							disableAudioAmp(AUDIO_AMP_MODE_RF);
						}
					}
					else
					{
						if (nonVolatileSettings.analogFilterLevel > ANALOG_FILTER_NONE)
						{
							settingsDecrement(nonVolatileSettings.analogFilterLevel, 1);
							trxSetAnalogFilterLevel(nonVolatileSettings.analogFilterLevel);
						}
					}

					voicePromptsInit();
					buildSpeechUiModeForGD77S(GD77SParameters.uiMode);
					voicePromptsPlay();
					break;

				case GD77S_UIMODE_DTMF_CONTACTS:
					// select previous DTMF contact and spell it
					if (GD77SParameters.dtmfListCount > 0)
					{
						GD77SParameters.dtmfListSelected = (GD77SParameters.dtmfListSelected + GD77SParameters.dtmfListCount - 1) % GD77SParameters.dtmfListCount;
					}
					voicePromptsInit();
					buildSpeechUiModeForGD77S(GD77SParameters.uiMode);
					voicePromptsPlay();
					break;

				case GD77S_UIMODE_ZONE: // Zones
					// No "All Channels" on GD77S
					nonVolatileSettings.currentZone = (nonVolatileSettings.currentZone + (codeplugZonesGetCount() - 1) - 1) % (codeplugZonesGetCount() - 1);

					settingsSet(nonVolatileSettings.overrideTG, 0); // remove any TG override
					tsSetManualOverride(CHANNEL_CHANNEL, TS_NO_OVERRIDE);
					settingsSet(nonVolatileSettings.currentChannelIndexInZone, (int16_t) -2); // Will be updated when reloading the UiChannelMode screen
					currentChannelData->rxFreq = 0x00; // Flag to the Channel screeen that the channel data is now invalid and needs to be reloaded

					menuSystemPopAllAndDisplaySpecificRootMenu(UI_CHANNEL_MODE, true);
					GD77SParameters.uiMode = GD77S_UIMODE_ZONE;

					announceItem(PROMPT_SEQUENCE_ZONE, PROMPT_THRESHOLD_3);
					break;

				case GD77S_UIMODE_POWER: // Power
					decreasePowerLevel();
					break;

				case GD77S_UIMODE_MAX:
					break;
			}
		}
	}
}
#endif // PLATFORM_GD77S
