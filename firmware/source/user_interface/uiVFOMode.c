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
#include "hardware/HR-C6000.h"
#include "functions/settings.h"
#include "functions/trx.h"
#include "functions/ticks.h"
#include "functions/rxPowerSaving.h"
#include "user_interface/menuSystem.h"
#include "user_interface/uiUtilities.h"
#include "user_interface/uiLocalisation.h"
#include "utils.h"

typedef enum
{
	VFO_SELECTED_FREQUENCY_INPUT_RX,
	VFO_SELECTED_FREQUENCY_INPUT_TX
} vfoSelectedFrequencyInput_t;

typedef enum
{
	VFO_SCREEN_OPERATION_NORMAL,
	VFO_SCREEN_OPERATION_SCAN,
	VFO_SCREEN_OPERATION_DUAL_SCAN
} vfoScreenOperationMode_t;

// internal prototypes
static void handleEvent(uiEvent_t *ev);
static void handleQuickMenuEvent(uiEvent_t *ev);
static void updateQuickMenuScreen(bool isFirstRun);
static void loadChannelData(void);
static void updateFrequency(int frequency, bool announceImmediately);
static void stepFrequency(int increment);
static void toneScan(void);
static void scanning(void);
static void initScan(void);
static void updateTrxID(void );
static void setCurrentFreqToScanLimits(void);
static void handleUpKey(uiEvent_t *ev);

static vfoSelectedFrequencyInput_t selectedFreq = VFO_SELECTED_FREQUENCY_INPUT_RX;

static const int SCAN_TONE_INTERVAL = 200;//time between each tone for lowest tone. (higher tones take less time.)
static int scanToneIndex = 0;
static CSSTypes_t toneScanType = CSS_CTCSS;
static CSSTypes_t toneScanCSS = CSS_NONE; // Here, CSS_NONE means *ALL* CSS types
static uint16_t prevCSSTone = (CODEPLUG_CSS_NONE - 1);

static vfoScreenOperationMode_t screenOperationMode[2] = { VFO_SCREEN_OPERATION_NORMAL, VFO_SCREEN_OPERATION_NORMAL };// For VFO A and B

static menuStatus_t menuVFOExitStatus = MENU_STATUS_SUCCESS;
static menuStatus_t menuQuickVFOExitStatus = MENU_STATUS_SUCCESS;

static bool quickmenuNewChannelHandled = false; // Quickmenu new channel confirmation window

// Public interface
menuStatus_t uiVFOMode(uiEvent_t *ev, bool isFirstRun)
{
	static uint32_t m = 0, sqm = 0, curm = 0;

	if (isFirstRun)
	{
		uiDataGlobal.FreqEnter.index = 0;

		uiDataGlobal.isDisplayingQSOData = false;
		uiDataGlobal.reverseRepeater = false;
		uiDataGlobal.displaySquelch = false;
		settingsSet(nonVolatileSettings.initialMenuNumber, (uint8_t) UI_VFO_MODE);
		uiDataGlobal.displayQSOStatePrev = QSO_DISPLAY_IDLE;
		currentChannelData = &settingsVFOChannel[nonVolatileSettings.currentVFONumber];
		currentChannelData->libreDMR_Power = 0x00;// Force channel to the Master power

		uiDataGlobal.currentSelectedChannelNumber = CH_DETAILS_VFO_CHANNEL;// This is not a regular channel. Its the special VFO channel!
		uiDataGlobal.displayChannelSettings = false;

		trxSetFrequency(currentChannelData->rxFreq, currentChannelData->txFreq, DMR_MODE_AUTO);

		//Need to load the Rx group if specified even if TG is currently overridden as we may need it later when the left or right button is pressed
		if (currentChannelData->rxGroupList != 0)
		{
			if (currentChannelData->rxGroupList != lastLoadedRxGroup)
			{
				if (codeplugRxGroupGetDataForIndex(currentChannelData->rxGroupList, &currentRxGroupData))
				{
					lastLoadedRxGroup = currentChannelData->rxGroupList;
				}
				else
				{
					lastLoadedRxGroup = -1;
				}
			}
		}
		else
		{
			memset(&currentRxGroupData, 0xFF, sizeof(struct_codeplugRxGroup_t));// If the VFO doesnt have an Rx Group ( TG List) the global var needs to be cleared, otherwise it contains the data from the previous screen e.g. Channel screen
			lastLoadedRxGroup = -1;
		}

		loadChannelData();

		// We're in digital mode, RXing, and current talker is already at the top of last heard list,
		// hence immediately display complete contact/TG info on screen
		// This mostly happens when getting out of a menu.
		uiDataGlobal.displayQSOState = (isQSODataAvailableForCurrentTalker() ? QSO_DISPLAY_CALLER_DATA : QSO_DISPLAY_DEFAULT_SCREEN);

		lastHeardClearLastID();
		freqEnterReset();
		uiVFOModeUpdateScreen(0);
		settingsSetVFODirty();

		int nextMenu = menuSystemGetPreviouslyPushedMenuNumber(); // used to determine if this screen has just been loaded after Tx ended (in loadChannelData()))
		if (!uiDataGlobal.VoicePrompts.inhibitInitial)
		{
			uiDataGlobal.VoicePrompts.inhibitInitial = false;
			announceItem(PROMPT_SEQUENCE_CHANNEL_NAME_AND_CONTACT_OR_VFO_FREQ_AND_MODE, ((nextMenu == UI_TX_SCREEN) || (nextMenu == UI_PRIVATE_CALL)) ? PROMPT_THRESHOLD_NEVER_PLAY_IMMEDIATELY : PROMPT_THRESHOLD_3);
		}

		if (screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_SCAN)
		{
			// Refresh on every step if scan boundaries is equal to one frequency step.
			uiDataGlobal.Scan.refreshOnEveryStep = ((nonVolatileSettings.vfoScanHigh[nonVolatileSettings.currentVFONumber] - nonVolatileSettings.vfoScanLow[nonVolatileSettings.currentVFONumber]) <= VFO_FREQ_STEP_TABLE[(currentChannelData->VFOflag5 >> 4)]);
		}


		menuVFOExitStatus = MENU_STATUS_SUCCESS;
	}
	else
	{
		menuVFOExitStatus = MENU_STATUS_SUCCESS;

		if (ev->events == NO_EVENT)
		{
			// We are entering digits, so update the screen as we have a cursor to blink
			if ((uiDataGlobal.FreqEnter.index > 0) && ((ev->time - curm) > 300))
			{
				curm = ev->time;
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN; // Redraw will happen just below
			}

			// is there an incoming DMR signal
			if (uiDataGlobal.displayQSOState != QSO_DISPLAY_IDLE)
			{
				uiVFOModeUpdateScreen(0);
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
					bool doRendering = true;

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
						if (uiVFOModeDualWatchIsScanning())
						{
							// Header needs to be updated, if Dual Watch is scanning
							uiUtilityRedrawHeaderOnly(true);
							doRendering = false;
						}
						else
						{
							uiUtilityDrawRSSIBarGraph();
						}
					}

					// Only render the second row which contains the bar graph, if we're not scanning,
					// as there is no need to redraw the rest of the screen
					if (doRendering)
					{
						ucRenderRows(((uiDataGlobal.Scan.active && (uiDataGlobal.Scan.state == SCAN_PAUSED)) ? 0 : 1), 2);
					}
				}

			}

			if (uiDataGlobal.Scan.toneActive)
			{
				toneScan();
			}

			if (uiDataGlobal.Scan.active)
			{
				scanning();
			}
		}
		else
		{
			if (ev->hasEvent)
			{
				if ((currentChannelData->chMode == RADIO_MODE_ANALOG) &&
						(ev->events & KEY_EVENT) && ((ev->keys.key == KEY_LEFT) || (ev->keys.key == KEY_RIGHT)))
				{
					sqm = ev->time;
				}

				// Scanning barrier
				if (uiDataGlobal.Scan.toneActive)
				{
#if defined(PLATFORM_RD5R) // virtual ORANGE button will be implemented later, this CPP will be removed then.
					if ((ev->keys.key != 0) && (ev->keys.event & KEY_MOD_UP))
#else
					// PTT key is already handled in main().
					if (((ev->events & BUTTON_EVENT) && BUTTONCHECK_SHORTUP(ev, BUTTON_ORANGE)) ||
							((ev->keys.key != 0) && (ev->keys.event & KEY_MOD_UP)))
#endif
					{
						uiVFOModeStopScanning();
					}

					return MENU_STATUS_SUCCESS;
				}

				handleEvent(ev);
			}

		}
	}
	return menuVFOExitStatus;
}

void uiVFOModeUpdateScreen(int txTimeSecs)
{
	static bool blink = false;
	static uint32_t blinkTime = 0;
	static const int bufferLen = 17;
	char buffer[bufferLen];

	// Only render the header, then wait for the next run
	// Otherwise the screen could remain blank if TG and PC are == 0
	// since uiDataGlobal.displayQSOState won't be set to QSO_DISPLAY_IDLE
	if ((trxGetMode() == RADIO_MODE_DIGITAL) && (HRC6000GetReceivedTgOrPcId() == 0) &&
			((uiDataGlobal.displayQSOState == QSO_DISPLAY_CALLER_DATA) || (uiDataGlobal.displayQSOState == QSO_DISPLAY_CALLER_DATA_UPDATE)))
	{
		uiUtilityRedrawHeaderOnly(uiVFOModeDualWatchIsScanning());
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
	uiUtilityRenderHeader(uiVFOModeDualWatchIsScanning());

	switch(uiDataGlobal.displayQSOState)
	{
		case QSO_DISPLAY_DEFAULT_SCREEN:
			if (uiDataGlobal.Scan.active &&
					(screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_DUAL_SCAN) && (uiDataGlobal.Scan.state == SCAN_SCANNING))
			{
				uiUtilityDisplayFrequency(DISPLAY_Y_POS_RX_FREQ, false, false, settingsVFOChannel[CHANNEL_VFO_A].rxFreq, true, true, 1);
				uiUtilityDisplayFrequency(DISPLAY_Y_POS_TX_FREQ, false, false, settingsVFOChannel[CHANNEL_VFO_B].rxFreq, true, true, 2);
			}
			else
			{
				uiDataGlobal.displayQSOStatePrev = QSO_DISPLAY_DEFAULT_SCREEN;
				uiDataGlobal.isDisplayingQSOData = false;
				uiDataGlobal.receivedPcId = 0x00;

				if (trxGetMode() == RADIO_MODE_DIGITAL)
				{
					if (uiDataGlobal.displayChannelSettings)
					{
						uint32_t PCorTG = ((nonVolatileSettings.overrideTG != 0) ? nonVolatileSettings.overrideTG : currentContactData.tgNumber);

						snprintf(buffer, bufferLen, "%s %d",
								(((PCorTG >> 24) == PC_CALL_FLAG) ? currentLanguage->pc : currentLanguage->tg),
								(PCorTG & 0xFFFFFF));
					}
					else
					{
						if (nonVolatileSettings.overrideTG != 0)
						{
							uiUtilityBuildTgOrPCDisplayName(buffer, bufferLen);
							uiUtilityDisplayInformation(NULL, DISPLAY_INFO_CONTACT_OVERRIDE_FRAME, (trxTransmissionEnabled ? DISPLAY_Y_POS_CONTACT_TX_FRAME : -1));
						}
						else
						{
							codeplugUtilConvertBufToString(currentContactData.name, buffer, 16);
						}
					}

					uiUtilityDisplayInformation(buffer, DISPLAY_INFO_CONTACT, (trxTransmissionEnabled ? DISPLAY_Y_POS_CONTACT_TX : -1));
				}
				else
				{
					// Display some channel settings
					if (uiDataGlobal.displayChannelSettings)
					{
						uiUtilityDisplayInformation(NULL, DISPLAY_INFO_TONE_AND_SQUELCH, -1);
					}

					// Squelch will be cleared later, 1s after last change
					if(uiDataGlobal.displaySquelch && !uiDataGlobal.displayChannelSettings)
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

					if(uiDataGlobal.Scan.toneActive)
					{
						switch (toneScanType)
						{
						case CSS_CTCSS:
							sprintf(buffer, "CTCSS %3d.%dHz", currentChannelData->rxTone / 10, currentChannelData->rxTone % 10);
							break;
						case CSS_DCS:
							sprintf(buffer, "DCS D%03oN", currentChannelData->rxTone & 0777);
							break;
						case CSS_DCS_INVERTED:
							sprintf(buffer, "DCS D%03oI", currentChannelData->rxTone & 0777);
							break;
						default:
							sprintf(buffer, "%s", "TONE ERROR");
							break;
						}

						uiUtilityDisplayInformation(buffer, DISPLAY_INFO_CONTACT, -1);
					}

				}

				if (uiDataGlobal.FreqEnter.index == 0)
				{
					if (!trxTransmissionEnabled)
					{
						// if CC scan is active, Rx freq is moved down to the Tx location,
						// as Contact Info will be displayed here
						uiUtilityDisplayFrequency(DISPLAY_Y_POS_RX_FREQ, false, (selectedFreq == VFO_SELECTED_FREQUENCY_INPUT_RX),
								(uiDataGlobal.reverseRepeater ? currentChannelData->txFreq : currentChannelData->rxFreq), true,
								(screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_SCAN), 0);
					}
					else
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

					if (((screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_NORMAL) ||
							(screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_DUAL_SCAN)) || trxTransmissionEnabled)
					{
						uiUtilityDisplayFrequency(DISPLAY_Y_POS_TX_FREQ, true, (selectedFreq == VFO_SELECTED_FREQUENCY_INPUT_TX || trxTransmissionEnabled),
								(uiDataGlobal.reverseRepeater ? currentChannelData->rxFreq : currentChannelData->txFreq), true, false, 0);
					}
					else
					{
						// Low/High scanning freqs
						snprintf(buffer, bufferLen, "%d.%03d", nonVolatileSettings.vfoScanLow[nonVolatileSettings.currentVFONumber] / 100000, (nonVolatileSettings.vfoScanLow[nonVolatileSettings.currentVFONumber] - (nonVolatileSettings.vfoScanLow[nonVolatileSettings.currentVFONumber] / 100000) * 100000)/100);

						ucPrintAt(2, DISPLAY_Y_POS_TX_FREQ, buffer, FONT_SIZE_3);

						snprintf(buffer, bufferLen, "%d.%03d", nonVolatileSettings.vfoScanHigh[nonVolatileSettings.currentVFONumber] / 100000, (nonVolatileSettings.vfoScanHigh[nonVolatileSettings.currentVFONumber] - (nonVolatileSettings.vfoScanHigh[nonVolatileSettings.currentVFONumber] / 100000) * 100000)/100);

						ucPrintAt(DISPLAY_SIZE_X - ((7 * 8) + 2), DISPLAY_Y_POS_TX_FREQ, buffer, FONT_SIZE_3);
						// Scanning direction arrow
						static const int scanDirArrow[2][6] = {
								{ // Down
										59, (DISPLAY_Y_POS_TX_FREQ + (FONT_SIZE_3_HEIGHT / 2) - 1),
										67, (DISPLAY_Y_POS_TX_FREQ + (FONT_SIZE_3_HEIGHT / 2) - (FONT_SIZE_3_HEIGHT / 4) - 1),
										67, (DISPLAY_Y_POS_TX_FREQ + (FONT_SIZE_3_HEIGHT / 2) + (FONT_SIZE_3_HEIGHT / 4) - 1)
								}, // Up
								{
										59, (DISPLAY_Y_POS_TX_FREQ + (FONT_SIZE_3_HEIGHT / 2) + (FONT_SIZE_3_HEIGHT / 4) - 1),
										59, (DISPLAY_Y_POS_TX_FREQ + (FONT_SIZE_3_HEIGHT / 2) - (FONT_SIZE_3_HEIGHT / 4) - 1),
										67, (DISPLAY_Y_POS_TX_FREQ + (FONT_SIZE_3_HEIGHT / 2) - 1)
								}
						};

						ucFillTriangle(scanDirArrow[(uiDataGlobal.Scan.direction > 0)][0], scanDirArrow[(uiDataGlobal.Scan.direction > 0)][1],
								scanDirArrow[(uiDataGlobal.Scan.direction > 0)][2], scanDirArrow[(uiDataGlobal.Scan.direction > 0)][3],
								scanDirArrow[(uiDataGlobal.Scan.direction > 0)][4], scanDirArrow[(uiDataGlobal.Scan.direction > 0)][5], true);
					}
				}
				else // Entering digits
				{
					int8_t xCursor = -1;
					int8_t yCursor = -1;
					int labelsVOffset =
#if defined(PLATFORM_RD5R)
							4;
#else
							0;
#endif

					if ((screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_NORMAL) ||
							(screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_DUAL_SCAN))
					{
#if defined(PLATFORM_RD5R)
						const char *FREQ_DISP_STR = "%c%c%c.%c%c%c%c%c";
#else
						const char *FREQ_DISP_STR = "%c%c%c.%c%c%c%c%c MHz";
#endif

						snprintf(buffer, bufferLen, FREQ_DISP_STR, uiDataGlobal.FreqEnter.digits[0], uiDataGlobal.FreqEnter.digits[1], uiDataGlobal.FreqEnter.digits[2],
								uiDataGlobal.FreqEnter.digits[3], uiDataGlobal.FreqEnter.digits[4], uiDataGlobal.FreqEnter.digits[5], uiDataGlobal.FreqEnter.digits[6], uiDataGlobal.FreqEnter.digits[7]);

						ucPrintCentered((selectedFreq == VFO_SELECTED_FREQUENCY_INPUT_TX) ? DISPLAY_Y_POS_TX_FREQ : DISPLAY_Y_POS_RX_FREQ, buffer, FONT_SIZE_3);

						// Cursor
						if (uiDataGlobal.FreqEnter.index < 8)
						{
							xCursor = ((DISPLAY_SIZE_X - (strlen(buffer) * 8)) >> 1) + ((uiDataGlobal.FreqEnter.index + ((uiDataGlobal.FreqEnter.index > 2) ? 1 : 0)) * 8);
							yCursor = ((selectedFreq == VFO_SELECTED_FREQUENCY_INPUT_TX) ? DISPLAY_Y_POS_TX_FREQ : DISPLAY_Y_POS_RX_FREQ) + (FONT_SIZE_3_HEIGHT - 2);
						}
					}
					else
					{
						uint8_t hiX = DISPLAY_SIZE_X - ((7 * 8) + 2);
						ucPrintAt(5, DISPLAY_Y_POS_RX_FREQ - labelsVOffset, "Low", FONT_SIZE_3);
						ucDrawFastVLine(0, DISPLAY_Y_POS_RX_FREQ - labelsVOffset, ((FONT_SIZE_3_HEIGHT * 2) + labelsVOffset), true);
						ucDrawFastHLine(1, DISPLAY_Y_POS_TX_FREQ - (labelsVOffset / 2), 57, true);

						sprintf(buffer, "%c%c%c.%c%c%c", uiDataGlobal.FreqEnter.digits[0], uiDataGlobal.FreqEnter.digits[1], uiDataGlobal.FreqEnter.digits[2],
								uiDataGlobal.FreqEnter.digits[3], uiDataGlobal.FreqEnter.digits[4], uiDataGlobal.FreqEnter.digits[5]);

						ucPrintAt(2, DISPLAY_Y_POS_TX_FREQ, buffer, FONT_SIZE_3);

						ucPrintAt(73, DISPLAY_Y_POS_RX_FREQ - labelsVOffset, "High", FONT_SIZE_3);
						ucDrawFastVLine(68, DISPLAY_Y_POS_RX_FREQ - labelsVOffset, ((FONT_SIZE_3_HEIGHT * 2) + labelsVOffset), true);
						ucDrawFastHLine(69, DISPLAY_Y_POS_TX_FREQ - (labelsVOffset / 2), 57, true);

						sprintf(buffer, "%c%c%c.%c%c%c", uiDataGlobal.FreqEnter.digits[6], uiDataGlobal.FreqEnter.digits[7], uiDataGlobal.FreqEnter.digits[8],
								uiDataGlobal.FreqEnter.digits[9], uiDataGlobal.FreqEnter.digits[10], uiDataGlobal.FreqEnter.digits[11]);

						ucPrintAt(hiX, DISPLAY_Y_POS_TX_FREQ, buffer, FONT_SIZE_3);

						// Cursor
						if (uiDataGlobal.FreqEnter.index < FREQ_ENTER_DIGITS_MAX)
						{
							xCursor = ((uiDataGlobal.FreqEnter.index < 6) ? 10 : hiX) // X start
										+ (((uiDataGlobal.FreqEnter.index < 6) ? (uiDataGlobal.FreqEnter.index - 1) : (uiDataGlobal.FreqEnter.index - 7)) * 8) // Length
										+ ((uiDataGlobal.FreqEnter.index > 2 ? (uiDataGlobal.FreqEnter.index > 8 ? 2 : 1) : 0) * 8); // MHz/kHz separator(s)

							yCursor = DISPLAY_Y_POS_TX_FREQ + (FONT_SIZE_3_HEIGHT - 2);
						}
					}

					if ((xCursor >= 0) && (yCursor >= 0))
					{
						ucDrawFastHLine(xCursor + 1, yCursor, 6, blink);

						if ((fw_millis() - blinkTime) > 500)
						{
							blinkTime = fw_millis();
							blink = !blink;
						}
					}

				}
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

void uiVFOModeStopScanning(void)
{
	if (uiDataGlobal.Scan.toneActive)
	{
		if (prevCSSTone != (CODEPLUG_CSS_NONE - 1))
		{
			currentChannelData->rxTone = prevCSSTone;
			prevCSSTone = (CODEPLUG_CSS_NONE - 1);
		}

		trxSetRxCSS(currentChannelData->rxTone);
		uiDataGlobal.Scan.toneActive = false;
		trxSetAnalogFilterLevel(nonVolatileSettings.analogFilterLevel);// Restore the filter setting after the tone scan
	}

	uiDataGlobal.Scan.active = false;
	uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;

	if (screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_DUAL_SCAN)
	{
		screenOperationMode[CHANNEL_VFO_A] = screenOperationMode[CHANNEL_VFO_B] = VFO_SCREEN_OPERATION_NORMAL;
		settingsSet(nonVolatileSettings.currentVFONumber, nonVolatileSettings.currentVFONumber);

		rxPowerSavingSetLevel(nonVolatileSettings.ecoLevel);// Level is reduced by 1 when Dual Watch , so re-instate it back to the correct setting
	}

	uiVFOModeUpdateScreen(0); // Needs to redraw the screen now
}

static void updateFrequency(int frequency, bool announceImmediately)
{
	if (selectedFreq == VFO_SELECTED_FREQUENCY_INPUT_TX)
	{
		if (trxGetBandFromFrequency(frequency) != -1)
		{
			currentChannelData->txFreq = frequency;
			trxSetFrequency(currentChannelData->rxFreq, currentChannelData->txFreq, DMR_MODE_AUTO);
			soundSetMelody(MELODY_ACK_BEEP);
		}
	}
	else
	{
		int deltaFrequency = frequency - currentChannelData->rxFreq;
		if (trxGetBandFromFrequency(frequency) != -1)
		{
			currentChannelData->rxFreq = frequency;
			currentChannelData->txFreq = currentChannelData->txFreq + deltaFrequency;
			trxSetFrequency(currentChannelData->rxFreq, currentChannelData->txFreq, DMR_MODE_AUTO);

			if (trxGetBandFromFrequency(currentChannelData->txFreq) != -1)
			{
				soundSetMelody(MELODY_ACK_BEEP);
			}
			else
			{
				currentChannelData->txFreq = frequency;
				soundSetMelody(MELODY_ERROR_BEEP);
			}
		}
		else
		{
			soundSetMelody(MELODY_ERROR_BEEP);
		}
	}
	announceItem(PROMPT_SEQUENCE_CHANNEL_NAME_OR_VFO_FREQ, announceImmediately);

	menuPrivateCallClear();
	settingsSetVFODirty();
}

static void loadChannelData(void)
{
	trxSetModeAndBandwidth(currentChannelData->chMode, ((currentChannelData->flag4 & 0x02) == 0x02));

	if (currentChannelData->chMode == RADIO_MODE_ANALOG)
	{
		if (!uiDataGlobal.Scan.toneActive)
		{
			trxSetRxCSS(currentChannelData->rxTone);
		}

		if (uiDataGlobal.Scan.active == false)
		{
			uiDataGlobal.Scan.state = SCAN_SCANNING;
		}
	}
	else
	{
		trxSetDMRColourCode(currentChannelData->txColor);

		if (nonVolatileSettings.overrideTG == 0)
		{
			uiVFOLoadContact(&currentContactData);

			// Check whether the contact data seems valid
			if ((currentContactData.name[0] == 0) || (currentContactData.tgNumber == 0) || (currentContactData.tgNumber > 9999999))
			{
				settingsSet(nonVolatileSettings.overrideTG, 9);// If the VFO does not have an Rx Group list assigned to it. We can't get a TG from the codeplug. So use TG 9.
				trxTalkGroupOrPcId = nonVolatileSettings.overrideTG;
				trxSetDMRTimeSlot(((currentChannelData->flag2 & 0x40) != 0));
				tsSetContactHasBeenOverriden(((Channel_t)nonVolatileSettings.currentVFONumber), false);
			}
			else
			{
				trxTalkGroupOrPcId = currentContactData.tgNumber;
				trxUpdateTsForCurrentChannelWithSpecifiedContact(&currentContactData);
			}
		}
		else
		{
			int manTS = tsGetManualOverrideFromCurrentChannel();

			trxTalkGroupOrPcId = nonVolatileSettings.overrideTG;
			trxSetDMRTimeSlot((manTS ? (manTS - 1) : ((currentChannelData->flag2 & 0x40) != 0)));
		}
	}
}

static void checkAndFixIndexInRxGroup(void)
{
	if ((currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup > 0) &&
			(nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber] > (currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup - 1)))
	{
		settingsSet(nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber], 0);
	}
}

void uiVFOLoadContact(struct_codeplugContact_t *contact)
{
	// Check if this channel has an Rx Group
	if ((currentRxGroupData.name[0] != 0) && (nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber] < currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup))
	{
		codeplugContactGetDataForIndex(currentRxGroupData.contacts[nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber]], contact);
	}
	else
	{
		/* 2020.10.27 vk3kyy. The Contact should not be forced to none just because the Rx group list is none
		if (currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup == 0)
		{
			currentChannelData->contact = 0;
		}*/

		codeplugContactGetDataForIndex(currentChannelData->contact, contact);
	}
}

static void handleEvent(uiEvent_t *ev)
{
	if (uiDataGlobal.Scan.active && (ev->events & KEY_EVENT))
	{
		if (BUTTONCHECK_DOWN(ev, BUTTON_SK2) == 0)
		{
			// Right key sets the current frequency as a 'nuisance' frequency.
			if((uiDataGlobal.Scan.state == SCAN_PAUSED) && (ev->keys.key == KEY_RIGHT) && (screenOperationMode[nonVolatileSettings.currentVFONumber] != VFO_SCREEN_OPERATION_DUAL_SCAN))
			{
				uiDataGlobal.Scan.nuisanceDelete[uiDataGlobal.Scan.nuisanceDeleteIndex] = currentChannelData->rxFreq;
				uiDataGlobal.Scan.nuisanceDeleteIndex = (uiDataGlobal.Scan.nuisanceDeleteIndex + 1) % MAX_ZONE_SCAN_NUISANCE_CHANNELS;
				uiDataGlobal.Scan.timer = SCAN_SKIP_CHANNEL_INTERVAL;//force scan to continue;
				uiDataGlobal.Scan.state = SCAN_SCANNING;
				keyboardReset();
				return;
			}

			// Left key reverses the scan direction
			if ((ev->keys.key == KEY_LEFT) && (screenOperationMode[nonVolatileSettings.currentVFONumber] != VFO_SCREEN_OPERATION_DUAL_SCAN))
			{
				uiDataGlobal.Scan.direction *= -1;
				keyboardReset();
				return;
			}
		}

		// Stop the scan on any key except UP without Shift (allows scan to be manually continued)
		// or SK2 on its own (allows Backlight to be triggered)
		if (((ev->keys.key == KEY_UP) && (BUTTONCHECK_DOWN(ev, BUTTON_SK2) == 0)
				&& (screenOperationMode[nonVolatileSettings.currentVFONumber] != VFO_SCREEN_OPERATION_DUAL_SCAN)) == false)
		{
			uiVFOModeStopScanning();
			keyboardReset();
			announceItem(PROMPT_SEQUENCE_CHANNEL_NAME_OR_VFO_FREQ, PROMPT_THRESHOLD_3);
			return;
		}
	}

	if (ev->events & FUNCTION_EVENT)
	{
		if (ev->function == FUNC_START_SCANNING)
		{
			initScan();
			setCurrentFreqToScanLimits();
			uiDataGlobal.Scan.active = true;
			return;
		}
		else if (ev->function == FUNC_REDRAW)
		{
			uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
			uiVFOModeUpdateScreen(0);
			return;
		}
	}

	if ((uiDataGlobal.reverseRepeater == false) && handleMonitorMode(ev))
	{
		return;
	}

	if (ev->events & BUTTON_EVENT)
	{

#if ! defined(PLATFORM_RD5R)
		// Stop the scan if any button is pressed.
		if (uiDataGlobal.Scan.active && BUTTONCHECK_DOWN(ev, BUTTON_ORANGE))
		{
			uiVFOModeStopScanning();
			return;
		}
#endif

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
				tsSetManualOverride(((Channel_t)nonVolatileSettings.currentVFONumber), (dmrMonitorCapturedTS + 1));
			}

			if (trxTalkGroupOrPcId != tg)
			{
				trxTalkGroupOrPcId = tg;
				settingsSet(nonVolatileSettings.overrideTG, trxTalkGroupOrPcId);
			}

			currentChannelData->txColor = trxGetDMRColourCode();// Set the CC to the current CC, which may have been determined by the CC finding algorithm in C6000.c

			announceItem(PROMPT_SEQUENCE_CONTACT_TG_OR_PC,PROMPT_THRESHOLD_NEVER_PLAY_IMMEDIATELY);

			uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
			uiVFOModeUpdateScreen(0);
			soundSetMelody(MELODY_ACK_BEEP);
			return;
		}

		if ((uiDataGlobal.reverseRepeater == false) && (BUTTONCHECK_DOWN(ev, BUTTON_SK1) && BUTTONCHECK_DOWN(ev, BUTTON_SK2)))
		{
			trxSetFrequency(currentChannelData->txFreq, currentChannelData->rxFreq, DMR_MODE_DMO);// Swap Tx and Rx freqs but force DMR Active
			uiDataGlobal.reverseRepeater = true;
			uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
			uiVFOModeUpdateScreen(0);
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
				uiVFOModeUpdateScreen(0);
			}

			return;
		}
		// Display channel settings (CTCSS, Squelch) while SK1 is pressed
		else if ((uiDataGlobal.displayChannelSettings == false) && BUTTONCHECK_DOWN(ev, BUTTON_SK1))
		{
			int prevQSODisp = uiDataGlobal.displayQSOStatePrev;

			uiDataGlobal.displayChannelSettings = true;
			uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
			uiVFOModeUpdateScreen(0);
			uiDataGlobal.displayQSOStatePrev = prevQSODisp;
			return;
		}
		else if ((uiDataGlobal.displayChannelSettings == true) && BUTTONCHECK_DOWN(ev, BUTTON_SK1) == 0)
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

			uiVFOModeUpdateScreen(0);
			return;
		}

#if !defined(PLATFORM_RD5R)
		if (BUTTONCHECK_SHORTUP(ev, BUTTON_ORANGE))
		{
			if (BUTTONCHECK_DOWN(ev, BUTTON_SK2))
			{
				announceItem(PROMPT_SEQUENCE_BATTERY, AUDIO_PROMPT_MODE_VOICE_LEVEL_1);
			}
			else
			{
				menuSystemPushNewMenu(UI_VFO_QUICK_MENU);

				// Trick to beep (AudioAssist), since ORANGE button doesn't produce any beep event
				ev->keys.event |= KEY_MOD_UP;
				ev->keys.key = 127;
				menuVFOExitStatus |= (MENU_STATUS_LIST_TYPE | MENU_STATUS_FORCE_FIRST);
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
			if (BUTTONCHECK_DOWN(ev, BUTTON_SK2))
			{
				menuSystemPushNewMenu(MENU_CHANNEL_DETAILS);
				freqEnterReset();
				return;
			}
			else
			{
				if (uiDataGlobal.FreqEnter.index == 0)
				{
					menuSystemPushNewMenu(MENU_MAIN_MENU);
					return;
				}
			}
		}

		if (uiDataGlobal.FreqEnter.index == 0)
		{
			if (KEYCHECK_SHORTUP(ev->keys, KEY_HASH))
			{
				if (BUTTONCHECK_DOWN(ev, BUTTON_SK2))
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
			else if (KEYCHECK_SHORTUP(ev->keys, KEY_STAR))
			{
				if (BUTTONCHECK_DOWN(ev, BUTTON_SK2))
				{
					if (trxGetMode() == RADIO_MODE_ANALOG)
					{
						currentChannelData->chMode = RADIO_MODE_DIGITAL;
						checkAndFixIndexInRxGroup();
						loadChannelData();
						updateTrxID();
						menuVFOExitStatus |= MENU_STATUS_FORCE_FIRST;
					}
					else
					{
						currentChannelData->chMode = RADIO_MODE_ANALOG;

						trxSetModeAndBandwidth(currentChannelData->chMode, ((currentChannelData->flag4 & 0x02) == 0x02));
					}

					announceItem(PROMPT_SEQUENCE_MODE, PROMPT_THRESHOLD_3);
					uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
				}
				else
				{
					if (trxGetMode() == RADIO_MODE_DIGITAL)
					{
						// Toggle TimeSlot
						trxSetDMRTimeSlot(1 - trxGetDMRTimeSlot());
						tsSetManualOverride(((Channel_t)nonVolatileSettings.currentVFONumber), (trxGetDMRTimeSlot() + 1));

						if ((nonVolatileSettings.overrideTG == 0) && (currentContactData.reserve1 & 0x01) == 0x00)
						{
							tsSetContactHasBeenOverriden(((Channel_t)nonVolatileSettings.currentVFONumber), true);
						}

						disableAudioAmp(AUDIO_AMP_MODE_RF);
						clearActiveDMRID();
						lastHeardClearLastID();
						uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
						uiVFOModeUpdateScreen(0);

						if (trxGetDMRTimeSlot() == 0)
						{
							menuVFOExitStatus |= MENU_STATUS_FORCE_FIRST;
						}
						announceItem(PROMPT_SEQUENCE_TS,PROMPT_THRESHOLD_3);
					}
					else
					{
						soundSetMelody(MELODY_ERROR_BEEP);
					}
				}
			}
			else if (KEYCHECK_LONGDOWN(ev->keys, KEY_STAR))
			{
				if (trxGetMode() == RADIO_MODE_DIGITAL)
				{
					tsSetManualOverride(((Channel_t)nonVolatileSettings.currentVFONumber), TS_NO_OVERRIDE);
					tsSetContactHasBeenOverriden(((Channel_t)nonVolatileSettings.currentVFONumber), false);

					// Check if this channel has an Rx Group
					if ((currentRxGroupData.name[0] != 0) &&
							(nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber] < currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup))
					{
						codeplugContactGetDataForIndex(currentRxGroupData.contacts[nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber]], &currentContactData);
					}
					else
					{
						codeplugContactGetDataForIndex(currentChannelData->contact, &currentContactData);
					}

					trxUpdateTsForCurrentChannelWithSpecifiedContact(&currentContactData);

					clearActiveDMRID();
					lastHeardClearLastID();
					uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
					uiVFOModeUpdateScreen(0);
				}
			}
			else if (KEYCHECK_SHORTUP(ev->keys, KEY_DOWN) || KEYCHECK_LONGDOWN_REPEAT(ev->keys, KEY_DOWN))
			{
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
				if (BUTTONCHECK_DOWN(ev, BUTTON_SK2))
				{
					// Don't permit to switch from RX/TX while scanning
					if ((screenOperationMode[nonVolatileSettings.currentVFONumber] != VFO_SCREEN_OPERATION_SCAN) ||
							(screenOperationMode[nonVolatileSettings.currentVFONumber] != VFO_SCREEN_OPERATION_DUAL_SCAN))
					{
						selectedFreq = VFO_SELECTED_FREQUENCY_INPUT_TX;
					}
				}
				else
				{
					stepFrequency(VFO_FREQ_STEP_TABLE[(currentChannelData->VFOflag5 >> 4)] * -1);
					uiVFOModeUpdateScreen(0);
					settingsSetVFODirty();
				}
			}
			else if (KEYCHECK_LONGDOWN(ev->keys, KEY_DOWN))
			{
				if (screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_SCAN)
				{
					screenOperationMode[nonVolatileSettings.currentVFONumber] = VFO_SCREEN_OPERATION_NORMAL;
					uiVFOModeStopScanning();
					return;
				}
			}
			else if (KEYCHECK_SHORTUP(ev->keys, KEY_UP) || KEYCHECK_LONGDOWN_REPEAT(ev->keys, KEY_UP))
			{
				handleUpKey(ev);
			}
			else if (KEYCHECK_LONGDOWN(ev->keys, KEY_UP) && (BUTTONCHECK_DOWN(ev, BUTTON_SK2) == 0))
			{
				if ((screenOperationMode[nonVolatileSettings.currentVFONumber] != VFO_SCREEN_OPERATION_SCAN) &&
						(screenOperationMode[nonVolatileSettings.currentVFONumber] != VFO_SCREEN_OPERATION_DUAL_SCAN))
				{
					initScan();
					return;
				}
				else
				{
					if (screenOperationMode[nonVolatileSettings.currentVFONumber] != VFO_SCREEN_OPERATION_DUAL_SCAN)
					{
						setCurrentFreqToScanLimits();
						if (uiDataGlobal.Scan.active == false)
						{
							uiDataGlobal.Scan.active = true;
							if (voicePromptsIsPlaying())
							{
								voicePromptsTerminate();
							}
							soundSetMelody(MELODY_KEY_LONG_BEEP);
						}
					}
				}
			}
			else if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
			{
				if (BUTTONCHECK_DOWN(ev, BUTTON_SK2) && (uiDataGlobal.tgBeforePcMode != 0))
				{
					settingsSet(nonVolatileSettings.overrideTG, uiDataGlobal.tgBeforePcMode);
					updateTrxID();
					uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;// Force redraw
					menuPrivateCallClear();
					uiVFOModeUpdateScreen(0);
					return;// The event has been handled
				}

#if defined(PLATFORM_GD77) || defined(PLATFORM_GD77S)
				if ((trxGetMode() == RADIO_MODE_DIGITAL) && (getAudioAmpStatus() & AUDIO_AMP_MODE_RF))
				{
					clearActiveDMRID();
				}
				menuVFOExitStatus |= MENU_STATUS_FORCE_FIRST;// Audible signal that the Channel screen has been selected
				menuSystemSetCurrentMenu(UI_CHANNEL_MODE);
#endif
				return;
			}
#if defined(PLATFORM_DM1801) || defined(PLATFORM_RD5R)
			else if (KEYCHECK_SHORTUP(ev->keys, KEY_VFO_MR))
			{
				if ((trxGetMode() == RADIO_MODE_DIGITAL) && (getAudioAmpStatus() & AUDIO_AMP_MODE_RF))
				{
					clearActiveDMRID();
				}
				menuVFOExitStatus |= MENU_STATUS_FORCE_FIRST;// Audible signal that the Channel screen has been selected
				menuSystemSetCurrentMenu(UI_CHANNEL_MODE);
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
				menuSystemPushNewMenu(UI_VFO_QUICK_MENU);

				// Trick to beep (AudioAssist), since ORANGE button doesn't produce any beep event
				ev->keys.event |= KEY_MOD_UP;
				ev->keys.key = 127;
				menuVFOExitStatus |= (MENU_STATUS_LIST_TYPE | MENU_STATUS_FORCE_FIRST);
				// End Trick
			}

			return;
		}
#endif
#if defined(PLATFORM_DM1801)
			else if (KEYCHECK_SHORTUP(ev->keys, KEY_A_B))
			{
				settingsSet(nonVolatileSettings.currentVFONumber, (1 - nonVolatileSettings.currentVFONumber));// Switch to other VFO
				currentChannelData = &settingsVFOChannel[nonVolatileSettings.currentVFONumber];
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
				menuSystemPopAllAndDisplayRootMenu(); // Force to set all TX/RX settings.
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
						uiUtilityRedrawHeaderOnly(uiVFOModeDualWatchIsScanning());
					}
				}
			}
			else if (KEYCHECK_PRESS(ev->keys, KEY_RIGHT))
			{
				if (BUTTONCHECK_DOWN(ev, BUTTON_SK2))
				{
					if (increasePowerLevel(false))
					{
						uiUtilityRedrawHeaderOnly(uiVFOModeDualWatchIsScanning());
					}
				}
				else
				{
					if (trxGetMode() == RADIO_MODE_DIGITAL)
					{
						if (currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup != 0)
						{
							if (currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup > 1)
							{
								if (nonVolatileSettings.overrideTG == 0)
								{
									settingsIncrement(nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber], 1);
									checkAndFixIndexInRxGroup();
								}

								if (nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber] == 0)
								{
									menuVFOExitStatus |= (MENU_STATUS_LIST_TYPE | MENU_STATUS_FORCE_FIRST);
								}
							}
							settingsSet(nonVolatileSettings.overrideTG, 0);// setting the override TG to 0 indicates the TG is not overridden
						}
						menuPrivateCallClear();
						updateTrxID();
						// We're in digital mode, RXing, and current talker is already at the top of last heard list,
						// hence immediately display complete contact/TG info on screen
						uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;//(isQSODataAvailableForCurrentTalker() ? QSO_DISPLAY_CALLER_DATA : QSO_DISPLAY_DEFAULT_SCREEN);
						if (isQSODataAvailableForCurrentTalker())
						{
							addTimerCallback(uiUtilityRenderQSODataAndUpdateScreen, 2000, true);
						}
						uiVFOModeUpdateScreen(0);
						announceItem(PROMPT_SEQUENCE_CONTACT_TG_OR_PC,PROMPT_THRESHOLD_3);
					}
					else
					{
						if(currentChannelData->sql == 0) //If we were using default squelch level
						{
							currentChannelData->sql = nonVolatileSettings.squelchDefaults[trxCurrentBand[TRX_RX_FREQ_BAND]];//start the adjustment from that point.
						}

						if (currentChannelData->sql < CODEPLUG_MAX_VARIABLE_SQUELCH)
						{
							currentChannelData->sql++;
						}

						announceItem(PROMPT_SQUENCE_SQUELCH,PROMPT_THRESHOLD_3);

						uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
						uiDataGlobal.displaySquelch = true;
						uiVFOModeUpdateScreen(0);
					}
				}
			}
			else if (KEYCHECK_PRESS(ev->keys, KEY_LEFT))
			{
				if (BUTTONCHECK_DOWN(ev, BUTTON_SK2))
				{
					if (decreasePowerLevel())
					{
						uiUtilityRedrawHeaderOnly(uiVFOModeDualWatchIsScanning());
					}

					if (trxGetPowerLevel() == 0)
					{
						menuVFOExitStatus |= (MENU_STATUS_LIST_TYPE | MENU_STATUS_FORCE_FIRST);
					}
				}
				else
				{
					if (trxGetMode() == RADIO_MODE_DIGITAL)
					{
						if (currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup != 0)
						{
							if (currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup > 1)
							{
								// To Do change TG in on same channel freq
								if (nonVolatileSettings.overrideTG == 0)
								{
									settingsDecrement(nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber], 1);
									if (nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber] < 0)
									{
										settingsSet(nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber],
												(int16_t) (currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup - 1));
									}

									if(nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber] == 0)
									{
										menuVFOExitStatus |= MENU_STATUS_FORCE_FIRST;
									}
								}
							}
							settingsSet(nonVolatileSettings.overrideTG, 0);// setting the override TG to 0 indicates the TG is not overridden
						}
						menuPrivateCallClear();
						updateTrxID();
						// We're in digital mode, RXing, and current talker is already at the top of last heard list,
						// hence immediately display complete contact/TG info on screen
						uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;//(isQSODataAvailableForCurrentTalker() ? QSO_DISPLAY_CALLER_DATA : QSO_DISPLAY_DEFAULT_SCREEN);
						if (isQSODataAvailableForCurrentTalker())
						{
							addTimerCallback(uiUtilityRenderQSODataAndUpdateScreen, 2000, true);
						}
						uiVFOModeUpdateScreen(0);
						announceItem(PROMPT_SEQUENCE_CONTACT_TG_OR_PC,PROMPT_THRESHOLD_3);
					}
					else
					{
						if(currentChannelData->sql == 0) //If we were using default squelch level
						{
							currentChannelData->sql = nonVolatileSettings.squelchDefaults[trxCurrentBand[TRX_RX_FREQ_BAND]];//start the adjustment from that point.
						}

						if (currentChannelData->sql > CODEPLUG_MIN_VARIABLE_SQUELCH)
						{
							currentChannelData->sql--;
						}

						announceItem(PROMPT_SQUENCE_SQUELCH,PROMPT_THRESHOLD_3);
						uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
						uiDataGlobal.displaySquelch = true;
						uiVFOModeUpdateScreen(0);
					}
				}
			}
		}
		else // (uiDataGlobal.FreqEnter.index == 0)
		{
			if (KEYCHECK_PRESS(ev->keys, KEY_LEFT))
			{
				uiDataGlobal.FreqEnter.index--;
				uiDataGlobal.FreqEnter.digits[uiDataGlobal.FreqEnter.index] = '-';
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
			}
			else if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
			{
				freqEnterReset();
				soundSetMelody(MELODY_NACK_BEEP);
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
				announceItem(PROMPT_SEQUENCE_CHANNEL_NAME_OR_VFO_FREQ, PROMPT_THRESHOLD_NEVER_PLAY_IMMEDIATELY);
			}
			else if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
			{
				if (screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_NORMAL)
				{
					int newFrequency = freqEnterRead(0, 8);

					if (trxGetBandFromFrequency(newFrequency) != -1)
					{
						updateFrequency(newFrequency, PROMPT_THRESHOLD_3);
						freqEnterReset();
					}
					else
					{
						menuVFOExitStatus |= MENU_STATUS_ERROR;
					}

					uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
				}
				else if (screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_SCAN)
				{
					if (uiDataGlobal.FreqEnter.index != 0)
					{
						menuVFOExitStatus |= MENU_STATUS_ERROR;
					}
				}
			}
		}

		if (uiDataGlobal.FreqEnter.index < ((screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_NORMAL) ? 8 : 12))
		{
			int keyval = menuGetKeypadKeyValue(ev, true);

			if ((keyval != 99) &&
					// Not first '0' digit in frequencies: we don't support < 100 MHz
					((((uiDataGlobal.FreqEnter.index == 0) && (keyval == 0)) == false) &&
							(((screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_SCAN) && (uiDataGlobal.FreqEnter.index == 6) && (keyval == 0)) == false)) &&
							(BUTTONCHECK_DOWN(ev, BUTTON_SK2) == 0))
			{
				voicePromptsInit();
				voicePromptsAppendPrompt(PROMPT_0 +  keyval);
				if ((uiDataGlobal.FreqEnter.index == 2) || (uiDataGlobal.FreqEnter.index == 8))
				{
					voicePromptsAppendPrompt(PROMPT_POINT);
				}
				voicePromptsPlay();

				uiDataGlobal.FreqEnter.digits[uiDataGlobal.FreqEnter.index] = (char) keyval + '0';
				uiDataGlobal.FreqEnter.index++;

				if (screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_NORMAL)
				{
					if (uiDataGlobal.FreqEnter.index == 8)
					{
						int newFreq = freqEnterRead(0, 8);

						if (trxGetBandFromFrequency(newFreq) != -1)
						{
							updateFrequency(newFreq, AUDIO_PROMPT_MODE_BEEP);
							freqEnterReset();
							soundSetMelody(MELODY_ACK_BEEP);
						}
						else
						{
							uiDataGlobal.FreqEnter.index--;
							uiDataGlobal.FreqEnter.digits[uiDataGlobal.FreqEnter.index] = '-';
							soundSetMelody(MELODY_ERROR_BEEP);
						}
					}
				}
				else if (screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_SCAN)
				{
					if (uiDataGlobal.FreqEnter.index == FREQ_ENTER_DIGITS_MAX)
					{
						int fStep = VFO_FREQ_STEP_TABLE[(currentChannelData->VFOflag5 >> 4)];
						int fLower = freqEnterRead(0, 6) * 100;
						int fUpper = freqEnterRead(6, 12) * 100;

						// Reorg min/max
						if (fLower > fUpper)
						{
							SAFE_SWAP(fLower, fUpper);
						}

						// At least on step diff
						if ((fUpper - fLower) < fStep)
						{
							fUpper = fLower + fStep;
						}

						// Refresh on every step if scan boundaries is equal to one frequency step.
						uiDataGlobal.Scan.refreshOnEveryStep = ((fUpper - fLower) <= fStep);

						if ((trxGetBandFromFrequency(fLower) != -1) && (trxGetBandFromFrequency(fUpper) != -1))
						{
							settingsSet(nonVolatileSettings.vfoScanLow[nonVolatileSettings.currentVFONumber], (uint32_t) fLower);
							settingsSet(nonVolatileSettings.vfoScanHigh[nonVolatileSettings.currentVFONumber], (uint32_t) fUpper);

							freqEnterReset();
							soundSetMelody(MELODY_ACK_BEEP);
						}
						else
						{
							uiDataGlobal.FreqEnter.index--;
							uiDataGlobal.FreqEnter.digits[uiDataGlobal.FreqEnter.index] = '-';
							soundSetMelody(MELODY_ERROR_BEEP);
						}
					}
				}

				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
			}
		}
	}
}


static void handleUpKey(uiEvent_t *ev)
{
	uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
	if (BUTTONCHECK_DOWN(ev, BUTTON_SK2))
	{
		// Don't permit to switch from RX/TX while scanning
		if ((screenOperationMode[nonVolatileSettings.currentVFONumber] != VFO_SCREEN_OPERATION_SCAN) &&
				(screenOperationMode[nonVolatileSettings.currentVFONumber] != VFO_SCREEN_OPERATION_DUAL_SCAN))
		{
			selectedFreq = VFO_SELECTED_FREQUENCY_INPUT_RX;
		}
	}
	else
	{
		if (uiDataGlobal.Scan.active)
		{
			stepFrequency(VFO_FREQ_STEP_TABLE[(currentChannelData->VFOflag5 >> 4)] * uiDataGlobal.Scan.direction);
			uiDataGlobal.Scan.timer = 500;
			uiDataGlobal.Scan.state = SCAN_SCANNING;
		}
		else
		{
			stepFrequency(VFO_FREQ_STEP_TABLE[(currentChannelData->VFOflag5 >> 4)]);
		}

		uiVFOModeUpdateScreen(0);
	}

	settingsSetVFODirty();
}

static void stepFrequency(int increment)
{
	int newTxFreq;
	int newRxFreq;

	if (selectedFreq == VFO_SELECTED_FREQUENCY_INPUT_TX)
	{
		newTxFreq  = currentChannelData->txFreq + increment;
		newRxFreq  = currentChannelData->rxFreq; // Needed later for the band limited checking
	}
	else
	{
		// VFO_SELECTED_FREQUENCY_INPUT_RX
		newRxFreq  = currentChannelData->rxFreq + increment;
		if (!uiDataGlobal.QuickMenu.tmpTxRxLockMode)
		{
			newTxFreq  = currentChannelData->txFreq + increment;
		}
		else
		{
			newTxFreq  = currentChannelData->txFreq;// Needed later for the band limited checking
		}
	}

	// Out of frequency in the current band, update freq to the next or prev band.
	if (trxGetBandFromFrequency(newRxFreq) == -1)
	{
		int band = trxGetNextOrPrevBandFromFrequency(newRxFreq, (increment > 0));

		if (band != -1)
		{
			newRxFreq = ((increment > 0) ? RADIO_HARDWARE_FREQUENCY_BANDS[band].minFreq : RADIO_HARDWARE_FREQUENCY_BANDS[band].maxFreq);
			newTxFreq = newRxFreq;
		}
		else
		{
			soundSetMelody(MELODY_ERROR_BEEP);
			return;
		}
	}

	if (trxGetBandFromFrequency(newRxFreq) != -1)
	{
		currentChannelData->txFreq = newTxFreq;
		currentChannelData->rxFreq = newRxFreq;

		trxSetFrequency(currentChannelData->rxFreq, currentChannelData->txFreq, DMR_MODE_AUTO);

		if ((uiDataGlobal.Scan.active == false) || (uiDataGlobal.Scan.active && (uiDataGlobal.Scan.state == SCAN_PAUSED)))
		{
			announceItem(PROMPT_SEQUENCE_VFO_FREQ_UPDATE, PROMPT_THRESHOLD_3);
		}
	}
	else
	{
		soundSetMelody(MELODY_ERROR_BEEP);
	}
}

// ---------------------------------------- Quick Menu functions -------------------------------------------------------------------
enum VFO_SCREEN_QUICK_MENU_ITEMS // The last item in the list is used so that we automatically get a total number of items in the list
{
#if defined(PLATFORM_GD77) || defined(PLATFORM_GD77S) || defined(PLATFORM_RD5R)
	VFO_SCREEN_QUICK_MENU_VFO_A_B = 0, VFO_SCREEN_QUICK_MENU_TX_SWAP_RX,
#elif defined(PLATFORM_DM1801)
	VFO_SCREEN_QUICK_MENU_TX_SWAP_RX = 0,
#endif
	VFO_SCREEN_QUICK_MENU_BOTH_TO_RX, VFO_SCREEN_QUICK_MENU_BOTH_TO_TX,
	VFO_SCREEN_QUICK_MENU_FILTER_FM,
	VFO_SCREEN_QUICK_MENU_FILTER_DMR,
	VFO_SCREEN_QUICK_MENU_DMR_CC_FILTER,
	VFO_SCREEN_QUICK_MENU_DMR_TS_FILTER,
	VFO_SCREEN_QUICK_MENU_VFO_TO_NEW, VFO_SCREEN_TONE_SCAN, VFO_SCREEN_DUAL_SCAN,
	VFO_SCREEN_QUICK_MENU_FREQ_BIND_MODE,
	NUM_VFO_SCREEN_QUICK_MENU_ITEMS
};

menuStatus_t uiVFOModeQuickMenu(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		if (quickmenuNewChannelHandled)
		{
			quickmenuNewChannelHandled = false;
			menuSystemPopAllAndDisplayRootMenu();
			return MENU_STATUS_SUCCESS;
		}

		uiDataGlobal.QuickMenu.tmpDmrDestinationFilterLevel = nonVolatileSettings.dmrDestinationFilter;
		uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel = nonVolatileSettings.dmrCcTsFilter;
		uiDataGlobal.QuickMenu.tmpAnalogFilterLevel = nonVolatileSettings.analogFilterLevel;
		uiDataGlobal.QuickMenu.tmpTxRxLockMode = nonVolatileSettings.bitfieldOptions & BIT_TX_RX_FREQ_LOCK;
		uiDataGlobal.QuickMenu.tmpVFONumber = nonVolatileSettings.currentVFONumber;
		uiDataGlobal.QuickMenu.tmpToneScanCSS = toneScanCSS;
		
		menuDataGlobal.endIndex = NUM_VFO_SCREEN_QUICK_MENU_ITEMS;

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
		menuQuickVFOExitStatus = MENU_STATUS_SUCCESS;

		if (ev->hasEvent)
		{
			handleQuickMenuEvent(ev);
		}
	}
	return menuQuickVFOExitStatus;
}

static bool validateNewChannel(void)
{
	quickmenuNewChannelHandled = true;

	if (uiDataGlobal.MessageBox.keyPressed == KEY_GREEN)
	{
		int16_t newChannelIndex;

		//look for empty channel
		for (newChannelIndex = CODEPLUG_CHANNELS_MIN; newChannelIndex <= CODEPLUG_CHANNELS_MAX; newChannelIndex++)
		{
			if (!codeplugAllChannelsIndexIsInUse(newChannelIndex))
			{
				break;
			}
		}

		if (newChannelIndex <= CODEPLUG_CONTACTS_MAX)
		{
			int currentTS = trxGetDMRTimeSlot();
			char nameBuf[17];

			uiDataGlobal.currentSelectedChannelNumber = newChannelIndex;

			memcpy(&channelScreenChannelData.rxFreq, &settingsVFOChannel[nonVolatileSettings.currentVFONumber].rxFreq, CODEPLUG_CHANNEL_DATA_STRUCT_SIZE - 16);// Don't copy the name of the vfo, which are in the first 16 bytes
			channelScreenChannelData.rxTone = currentChannelData->rxTone;
			channelScreenChannelData.txTone = currentChannelData->txTone;

			// Codeplug string aren't NULL terminated.
			snprintf(nameBuf, 17, "%s %d", currentLanguage->new_channel, newChannelIndex);
			memset(&channelScreenChannelData.name, 0xFF, sizeof(channelScreenChannelData.name));
			memcpy(&channelScreenChannelData.name, nameBuf, strlen(nameBuf));

			// change the TS on the new channel to whatever the radio is currently set to.
			if (currentTS != 0)
			{
				channelScreenChannelData.flag2 |= 0x40;// set TS 2 bit
			}
			else
			{
				channelScreenChannelData.flag2 &= 0xBF;// Clear TS 2 bit
			}

			codeplugChannelSaveDataForIndex(newChannelIndex, &channelScreenChannelData);
			codeplugAllChannelsIndexSetUsed(newChannelIndex); //Set channel index as valid

			// check if its real zone and or the virtual zone "All Channels" whose index is -1
			if (CODEPLUG_ZONE_IS_ALLCHANNELS(currentZone))
			{
				// All Channels virtual zone
				settingsSet(nonVolatileSettings.currentZone, (int16_t) (codeplugZonesGetCount() - 1));//set zone to all channels and channel index to free channel found

				settingsSet(nonVolatileSettings.currentChannelIndexInAllZone, newChannelIndex);// Change to the index of the new channel

				settingsSet(nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE], nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber]);
				currentZone.NOT_IN_CODEPLUGDATA_numChannelsInZone++;
			}
			else
			{
				if (codeplugZoneAddChannelToZoneAndSave(newChannelIndex, &currentZone))
				{
					settingsSet(nonVolatileSettings.currentChannelIndexInZone , currentZone.NOT_IN_CODEPLUGDATA_numChannelsInZone - 1);
				}
				else
				{
					nextKeyBeepMelody = (int *)MELODY_NACK_BEEP;
					return true;
				}
			}

			channelScreenChannelData.rxFreq = 0; // NOT SURE IF THIS IS NECESSARY... Flag to the Channel screen that the channel data is now invalid and needs to be reloaded
			uiDataGlobal.VoicePrompts.inhibitInitial = true;
			tsSetManualOverride(((Channel_t) CHANNEL_CHANNEL), (currentTS + 1)); //copy current TS

			// Just override TG/PC blindly, if not already set
			if (nonVolatileSettings.overrideTG == 0)
			{
				settingsSet(nonVolatileSettings.overrideTG, trxTalkGroupOrPcId);
			}

			menuSystemPopAllAndDisplaySpecificRootMenu(UI_CHANNEL_MODE, true);
			nextKeyBeepMelody = (int *)MELODY_ACK_BEEP;
			quickmenuNewChannelHandled = false; // Need to do this, as uiVFOModeQuickMenu() won't be re-entered on the next menu iteration
			return true;
		}

		nextKeyBeepMelody = (int *)MELODY_NACK_BEEP;
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
	int prompt;// For voice prompts

	ucClearBuf();
	menuDisplayTitle(currentLanguage->quick_menu);

	for(int i = -1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(NUM_VFO_SCREEN_QUICK_MENU_ITEMS, i);
		prompt = -1;// Prompt not used
		rightSideVar[0] = 0;
		rightSideConst = NULL;
		leftSide = NULL;

		switch(mNum)
		{
#if defined(PLATFORM_GD77) || defined(PLATFORM_GD77S) || defined(PLATFORM_RD5R)
			case VFO_SCREEN_QUICK_MENU_VFO_A_B:
				sprintf(rightSideVar, "VFO:%c", ((uiDataGlobal.QuickMenu.tmpVFONumber == 0) ? 'A' : 'B'));
				break;
#endif
			case VFO_SCREEN_QUICK_MENU_TX_SWAP_RX:
				prompt = PROMPT_VFO_EXCHANGE_TX_RX;
				strcpy(rightSideVar, "Tx <--> Rx");
				break;
			case VFO_SCREEN_QUICK_MENU_BOTH_TO_RX:
				prompt = PROMPT_VFO_COPY_RX_TO_TX;
				strcpy(rightSideVar, "Rx --> Tx");
				break;
			case VFO_SCREEN_QUICK_MENU_BOTH_TO_TX:
				prompt = PROMPT_VFO_COPY_TX_TO_RX;
				strcpy(rightSideVar, "Tx --> Rx");
				break;
			case VFO_SCREEN_QUICK_MENU_FILTER_FM:
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
			case VFO_SCREEN_QUICK_MENU_FILTER_DMR:
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
			case VFO_SCREEN_QUICK_MENU_DMR_CC_FILTER:
				leftSide = (char * const *)&currentLanguage->dmr_cc_filter;
				rightSideConst = (uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel & DMR_CC_FILTER_PATTERN)?(char * const *)&currentLanguage->on:(char * const *)&currentLanguage->off;
				break;
			case VFO_SCREEN_QUICK_MENU_DMR_TS_FILTER:
				leftSide = (char * const *)&currentLanguage->dmr_ts_filter;
				rightSideConst = (uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel & DMR_TS_FILTER_PATTERN)?(char * const *)&currentLanguage->on:(char * const *)&currentLanguage->off;
				break;
		    case VFO_SCREEN_QUICK_MENU_VFO_TO_NEW:
		    	rightSideConst = (char * const *)&currentLanguage->vfoToNewChannel;
		    	break;
			case VFO_SCREEN_TONE_SCAN:
				leftSide = (char * const *)&currentLanguage->tone_scan;
				if(trxGetMode() == RADIO_MODE_ANALOG)
				{
					const char *scanCSS[] = { currentLanguage->all, "CTCSS", "DCS", "iDCS" };
					snprintf(rightSideVar, bufferLen, "%s", scanCSS[uiDataGlobal.QuickMenu.tmpToneScanCSS]);

					if (uiDataGlobal.QuickMenu.tmpToneScanCSS == CSS_NONE)
					{
						rightSideConst = (char * const *)&currentLanguage->all;
					}
				}
				else
				{
					rightSideConst = (char * const *)&currentLanguage->n_a;
				}
				break;
			case VFO_SCREEN_DUAL_SCAN:
				rightSideConst = (char * const *)&currentLanguage->dual_watch;
				break;
			case VFO_SCREEN_QUICK_MENU_FREQ_BIND_MODE:
				leftSide = (char * const *)&currentLanguage->vfo_freq_bind_mode;
				rightSideConst = (!uiDataGlobal.QuickMenu.tmpTxRxLockMode)?(char * const *)&currentLanguage->on:(char * const *)&currentLanguage->off;
				break;
			default:
				strcpy(buf, "");
				break;
		}

		if (leftSide != NULL)
		{
			snprintf(buf, bufferLen, "%s:%s", *leftSide, (rightSideVar[0] ? rightSideVar : *rightSideConst));
		}
		else
		{
			snprintf(buf, bufferLen, "%s", ((rightSideVar[0] != 0) ? rightSideVar : *rightSideConst));
		}

		if (i == 0)
		{
			if (!isFirstRun)
			{
				voicePromptsInit();
			}

			if (prompt != -1)
			{
				voicePromptsAppendPrompt(prompt);
			}
			else
			{
				if (leftSide != NULL)
				{
					voicePromptsAppendLanguageString((const char * const *)leftSide);
				}

				if ((rightSideVar[0] != 0) && (rightSideConst == NULL))
				{
					voicePromptsAppendString(rightSideVar);
				}
				else
				{
					voicePromptsAppendLanguageString((const char * const *)rightSideConst);
				}
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
		uiVFOModeStopScanning();

		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
	{
#if defined(PLATFORM_GD77) || defined(PLATFORM_GD77S) || defined(PLATFORM_RD5R)
		if (nonVolatileSettings.currentVFONumber != uiDataGlobal.QuickMenu.tmpVFONumber)
		{
			settingsSet(nonVolatileSettings.currentVFONumber, uiDataGlobal.QuickMenu.tmpVFONumber);
			currentChannelData = &settingsVFOChannel[nonVolatileSettings.currentVFONumber];
		}
#endif
		toneScanCSS = uiDataGlobal.QuickMenu.tmpToneScanCSS;

		switch(menuDataGlobal.currentItemIndex)
		{
			case VFO_SCREEN_QUICK_MENU_TX_SWAP_RX:
			{
				int tmpFreq = currentChannelData->txFreq;
				currentChannelData->txFreq = currentChannelData->rxFreq;
				currentChannelData->rxFreq = tmpFreq;
				trxSetFrequency(currentChannelData->rxFreq, currentChannelData->txFreq, DMR_MODE_AUTO);
				announceItem(PROMPT_SEQUENCE_CHANNEL_NAME_OR_VFO_FREQ, PROMPT_THRESHOLD_3);
			}
			break;
			case VFO_SCREEN_QUICK_MENU_BOTH_TO_RX:
				currentChannelData->txFreq = currentChannelData->rxFreq;
				trxSetFrequency(currentChannelData->rxFreq, currentChannelData->txFreq, DMR_MODE_AUTO);
				announceItem(PROMPT_SEQUENCE_CHANNEL_NAME_OR_VFO_FREQ, PROMPT_THRESHOLD_3);
				break;
			case VFO_SCREEN_QUICK_MENU_BOTH_TO_TX:
				currentChannelData->rxFreq = currentChannelData->txFreq;
				trxSetFrequency(currentChannelData->rxFreq, currentChannelData->txFreq, DMR_MODE_AUTO);
				announceItem(PROMPT_SEQUENCE_CHANNEL_NAME_OR_VFO_FREQ, PROMPT_THRESHOLD_3);
				break;
			case VFO_SCREEN_QUICK_MENU_FILTER_FM:
				settingsSet(nonVolatileSettings.analogFilterLevel, (uint8_t) uiDataGlobal.QuickMenu.tmpAnalogFilterLevel);
				trxSetAnalogFilterLevel(nonVolatileSettings.analogFilterLevel);
				break;
			case VFO_SCREEN_QUICK_MENU_FILTER_DMR:
				settingsSet(nonVolatileSettings.dmrDestinationFilter, (uint8_t) uiDataGlobal.QuickMenu.tmpDmrDestinationFilterLevel);
				if (trxGetMode() == RADIO_MODE_DIGITAL)
				{
					init_digital_DMR_RX();
					disableAudioAmp(AUDIO_AMP_MODE_RF);
				}
				break;
			case VFO_SCREEN_QUICK_MENU_DMR_CC_FILTER:
			case VFO_SCREEN_QUICK_MENU_DMR_TS_FILTER:
				settingsSet(nonVolatileSettings.dmrCcTsFilter, (uint8_t) uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel);
				if (trxGetMode() == RADIO_MODE_DIGITAL)
				{
					init_digital_DMR_RX();
					disableAudioAmp(AUDIO_AMP_MODE_RF);
				}
				break;

			case VFO_SCREEN_QUICK_MENU_VFO_TO_NEW:
				if (quickmenuNewChannelHandled == false)
				{
					snprintf(uiDataGlobal.MessageBox.message, MESSAGEBOX_MESSAGE_LEN_MAX, "%s\n%s", currentLanguage->new_channel, currentLanguage->please_confirm);
					uiDataGlobal.MessageBox.type = MESSAGEBOX_TYPE_INFO;
					uiDataGlobal.MessageBox.decoration = MESSAGEBOX_DECORATION_FRAME;
					uiDataGlobal.MessageBox.buttons = MESSAGEBOX_BUTTONS_YESNO;
					uiDataGlobal.MessageBox.validatorCallback = validateNewChannel;
					menuSystemPushNewMenu(UI_MESSAGE_BOX);

					voicePromptsInit();
					voicePromptsAppendLanguageString(&currentLanguage->new_channel);
					voicePromptsAppendLanguageString(&currentLanguage->please_confirm);
					voicePromptsPlay();
				}
				return;
				break;
			case VFO_SCREEN_TONE_SCAN:
				if (trxGetMode() == RADIO_MODE_ANALOG)
				{
					trxSetAnalogFilterLevel(ANALOG_FILTER_CSS);
					bool cssTypesDiffer = false;
					uint16_t currentCSSType = (codeplugChannelToneIsCTCSS(currentChannelData->rxTone) ? CSS_CTCSS :
							(codeplugChannelToneIsDCS(currentChannelData->rxTone) ?
									((currentChannelData->rxTone & CODEPLUG_DCS_INVERTED_MASK) ? CSS_DCS_INVERTED : CSS_DCS) : CSS_NONE));

					// Check if the current CSS differs from the one set to scan.
					if ((currentChannelData->rxTone != CODEPLUG_CSS_NONE) && (toneScanCSS != CSS_NONE) && (toneScanCSS != currentCSSType))
					{
						cssTypesDiffer = true;
					}

					//                                          CTCSS or DCS         no CSS
					toneScanType = ((toneScanCSS != CSS_NONE) ? toneScanCSS : ((currentCSSType == CSS_NONE) ? CSS_CTCSS : currentCSSType));
					prevCSSTone = currentChannelData->rxTone;

					if ((currentChannelData->rxTone == CODEPLUG_CSS_NONE) || cssTypesDiffer)
					{
						// CSS type are different, start from index 0
						scanToneIndex = 0;
						currentChannelData->rxTone = cssGetTone(scanToneIndex, toneScanType);
					}
					else
					{
						// Try to get the CSS index from the current tone
						// If it fails, it will return index 0
						scanToneIndex = cssIndex(currentChannelData->rxTone, toneScanType);

						// Set the tone to the next one
						cssIncrement(&currentChannelData->rxTone, &scanToneIndex, &toneScanType, true, (toneScanCSS != CSS_NONE));
					}

					disableAudioAmp(AUDIO_AMP_MODE_RF);
					trxAT1846RxOff();
					trxSetRxCSS(currentChannelData->rxTone);
					trxAT1846RxOn();
					uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
					uiDataGlobal.Scan.toneActive = true;
					uiDataGlobal.Scan.timer = SCAN_TONE_INTERVAL;
					uiDataGlobal.Scan.refreshOnEveryStep = false;
					uiDataGlobal.Scan.timer = ((toneScanType == CSS_CTCSS) ? (SCAN_TONE_INTERVAL - (scanToneIndex * 2)) : SCAN_TONE_INTERVAL);
				}
				break;
			case VFO_SCREEN_DUAL_SCAN:
				uiDataGlobal.Scan.active = true;
				uiDataGlobal.Scan.timerReload = 135;// for Dual Watch, use a larger step time than normally scanning, and which does not synchronise with the DMR 30ms timeslots
				uiDataGlobal.Scan.timer = uiDataGlobal.Scan.timerReload;
				uiDataGlobal.Scan.refreshOnEveryStep = false;
				screenOperationMode[CHANNEL_VFO_A] = screenOperationMode[CHANNEL_VFO_B] = VFO_SCREEN_OPERATION_DUAL_SCAN;
				uiDataGlobal.VoicePrompts.inhibitInitial = true;
				uiDataGlobal.Scan.scanType = SCAN_TYPE_DUAL_WATCH;
				int currentPowerSavingLevel = rxPowerSavingGetLevel();
				if (currentPowerSavingLevel > 1)
				{
					rxPowerSavingSetLevel(currentPowerSavingLevel - 1);
				}
				break;
			case VFO_SCREEN_QUICK_MENU_FREQ_BIND_MODE:
				settingsSetOptionBit(BIT_TX_RX_FREQ_LOCK, uiDataGlobal.QuickMenu.tmpTxRxLockMode);
				break;
		}
		menuSystemPopPreviousMenu();
		return;
	}
#if defined(PLATFORM_GD77) || defined(PLATFORM_GD77S) || defined(PLATFORM_RD5R)
 #if defined(PLATFORM_GD77) || defined(PLATFORM_GD77S)
	else if (((ev->events & BUTTON_EVENT) && BUTTONCHECK_SHORTUP(ev, BUTTON_ORANGE)) && (menuDataGlobal.currentItemIndex == VFO_SCREEN_QUICK_MENU_VFO_A_B))
 #elif defined(PLATFORM_RD5R)
	else if (KEYCHECK_SHORTUP(ev->keys, KEY_VFO_MR) && (menuDataGlobal.currentItemIndex == VFO_SCREEN_QUICK_MENU_VFO_A_B))
 #endif
	{
		settingsSet(nonVolatileSettings.currentVFONumber, (1 - uiDataGlobal.QuickMenu.tmpVFONumber));// Switch to other VFO
		currentChannelData = &settingsVFOChannel[nonVolatileSettings.currentVFONumber];
		menuSystemPopPreviousMenu();
		if (nonVolatileSettings.currentVFONumber == 0)
		{
			// Trick to beep (AudioAssist), since ORANGE button doesn't produce any beep event
			ev->keys.event |= KEY_MOD_UP;
			ev->keys.key = 127;
			// End Trick

			menuQuickVFOExitStatus |= MENU_STATUS_FORCE_FIRST;
		}
		return;
	}
#endif
	else if (KEYCHECK_PRESS(ev->keys, KEY_RIGHT))
	{
		isDirty = true;
		switch(menuDataGlobal.currentItemIndex)
		{
#if defined(PLATFORM_GD77) || defined(PLATFORM_GD77S) || defined(PLATFORM_RD5R)
			case VFO_SCREEN_QUICK_MENU_VFO_A_B:
				if (uiDataGlobal.QuickMenu.tmpVFONumber == 0)
				{
					uiDataGlobal.QuickMenu.tmpVFONumber = 1;
				}
				break;
#endif
			case VFO_SCREEN_QUICK_MENU_FILTER_FM:
				if (uiDataGlobal.QuickMenu.tmpAnalogFilterLevel < NUM_ANALOG_FILTER_LEVELS - 1)
				{
					uiDataGlobal.QuickMenu.tmpAnalogFilterLevel++;
				}
				break;
			case VFO_SCREEN_QUICK_MENU_FILTER_DMR:
				if (uiDataGlobal.QuickMenu.tmpDmrDestinationFilterLevel < NUM_DMR_DESTINATION_FILTER_LEVELS - 1)
				{
					uiDataGlobal.QuickMenu.tmpDmrDestinationFilterLevel++;
				}
				break;
			case VFO_SCREEN_QUICK_MENU_DMR_CC_FILTER:
				if (!(uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel & DMR_CC_FILTER_PATTERN))
				{
					uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel |= DMR_CC_FILTER_PATTERN;
				}
				break;
			case VFO_SCREEN_QUICK_MENU_DMR_TS_FILTER:
				if (!(uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel & DMR_TS_FILTER_PATTERN))
				{
					uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel |= DMR_TS_FILTER_PATTERN;
				}
				break;
			case VFO_SCREEN_TONE_SCAN:
				if (trxGetMode() == RADIO_MODE_ANALOG)
				{
					if (uiDataGlobal.QuickMenu.tmpToneScanCSS < CSS_DCS_INVERTED)
					{
						uiDataGlobal.QuickMenu.tmpToneScanCSS++;
					}
				}
				break;
			case VFO_SCREEN_QUICK_MENU_FREQ_BIND_MODE:
				uiDataGlobal.QuickMenu.tmpTxRxLockMode = false;
				break;
			}
	}
	else if (KEYCHECK_PRESS(ev->keys, KEY_LEFT))
	{
		isDirty = true;
		switch(menuDataGlobal.currentItemIndex)
		{
#if defined(PLATFORM_GD77) || defined(PLATFORM_GD77S) || defined(PLATFORM_RD5R)
			case VFO_SCREEN_QUICK_MENU_VFO_A_B:
				if (uiDataGlobal.QuickMenu.tmpVFONumber == 1)
				{
					uiDataGlobal.QuickMenu.tmpVFONumber = 0;
				}
				menuQuickVFOExitStatus |= MENU_STATUS_FORCE_FIRST;
				break;
#endif
			case VFO_SCREEN_QUICK_MENU_FILTER_FM:
				if (uiDataGlobal.QuickMenu.tmpAnalogFilterLevel > ANALOG_FILTER_NONE)
				{
					uiDataGlobal.QuickMenu.tmpAnalogFilterLevel--;
				}
				break;
			case VFO_SCREEN_QUICK_MENU_FILTER_DMR:
				if (uiDataGlobal.QuickMenu.tmpDmrDestinationFilterLevel > DMR_DESTINATION_FILTER_NONE)
				{
					uiDataGlobal.QuickMenu.tmpDmrDestinationFilterLevel--;
				}
				break;
			case VFO_SCREEN_QUICK_MENU_DMR_CC_FILTER:
				if (uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel & DMR_CC_FILTER_PATTERN)
				{
					uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel &= ~DMR_CC_FILTER_PATTERN;
				}
				break;
			case VFO_SCREEN_QUICK_MENU_DMR_TS_FILTER:
				if (uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel & DMR_TS_FILTER_PATTERN)
				{
					uiDataGlobal.QuickMenu.tmpDmrCcTsFilterLevel &= ~DMR_TS_FILTER_PATTERN;
				}
				break;
			case VFO_SCREEN_TONE_SCAN:
				if (trxGetMode() == RADIO_MODE_ANALOG)
				{
					if (uiDataGlobal.QuickMenu.tmpToneScanCSS > CSS_NONE)
					{
						uiDataGlobal.QuickMenu.tmpToneScanCSS--;
					}
				}
				break;
			case VFO_SCREEN_QUICK_MENU_FREQ_BIND_MODE:
				uiDataGlobal.QuickMenu.tmpTxRxLockMode = true;
				break;
		}
	}
	else if (KEYCHECK_PRESS(ev->keys, KEY_DOWN))
	{
		isDirty = true;
		menuSystemMenuIncrement(&menuDataGlobal.currentItemIndex, NUM_VFO_SCREEN_QUICK_MENU_ITEMS);
		menuQuickVFOExitStatus |= MENU_STATUS_LIST_TYPE;
	}
	else if (KEYCHECK_PRESS(ev->keys, KEY_UP))
	{
		isDirty = true;
		menuSystemMenuDecrement(&menuDataGlobal.currentItemIndex, NUM_VFO_SCREEN_QUICK_MENU_ITEMS);
		menuQuickVFOExitStatus |= MENU_STATUS_LIST_TYPE;
	}

	if (isDirty)
	{
		updateQuickMenuScreen(false);
	}
}

bool uiVFOModeIsScanning(void)
{
	return (uiDataGlobal.Scan.toneActive || uiDataGlobal.Scan.active);
}

bool uiVFOModeDualWatchIsScanning()
{
	return ((menuSystemGetCurrentMenuNumber() == UI_VFO_MODE) && uiDataGlobal.Scan.active && (uiDataGlobal.Scan.state == SCAN_SCANNING) && (screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_DUAL_SCAN));
}

static void toneScan(void)
{
	if (getAudioAmpStatus() & AUDIO_AMP_MODE_RF)
	{
		currentChannelData->txTone = currentChannelData->rxTone;
		uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
		uiVFOModeUpdateScreen(0);
		prevCSSTone = (CODEPLUG_CSS_NONE - 1);
		uiDataGlobal.Scan.toneActive = false;
		return;
	}

	if (uiDataGlobal.Scan.timer > 0)
	{
		uiDataGlobal.Scan.timer--;
	}
	else
	{
		cssIncrement(&currentChannelData->rxTone, &scanToneIndex, &toneScanType, true, (toneScanCSS != CSS_NONE));
		trxAT1846RxOff();
		trxSetRxCSS(currentChannelData->rxTone);
		uiDataGlobal.Scan.timer = ((toneScanType == CSS_CTCSS) ? (SCAN_TONE_INTERVAL - (scanToneIndex * 2)) : SCAN_TONE_INTERVAL);
		trxAT1846RxOn();
		uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
		uiVFOModeUpdateScreen(0);
	}
}

static void updateTrxID(void)
{
	if (nonVolatileSettings.overrideTG != 0)
	{
		trxTalkGroupOrPcId = nonVolatileSettings.overrideTG;
	}
	else
	{
		//tsSetManualOverride(((Channel_t)nonVolatileSettings.currentVFONumber), TS_NO_OVERRIDE);

		// Check if this channel has an Rx Group
		if ((currentRxGroupData.name[0] != 0) && (nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber] < currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup))
		{
			codeplugContactGetDataForIndex(currentRxGroupData.contacts[nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber]], &currentContactData);
		}
		else
		{
			codeplugContactGetDataForIndex(currentChannelData->contact, &currentContactData);
		}

		trxTalkGroupOrPcId = currentContactData.tgNumber;
		if (currentContactData.callType == CONTACT_CALLTYPE_PC)
		{
			trxTalkGroupOrPcId |= (PC_CALL_FLAG << 24);
		}

		tsSetContactHasBeenOverriden(((Channel_t)nonVolatileSettings.currentVFONumber), false);

		trxUpdateTsForCurrentChannelWithSpecifiedContact(&currentContactData);
	}
	lastHeardClearLastID();
	menuPrivateCallClear();
}

static void setCurrentFreqToScanLimits(void)
{
	if((currentChannelData->rxFreq < nonVolatileSettings.vfoScanLow[nonVolatileSettings.currentVFONumber]) ||
			(currentChannelData->rxFreq > nonVolatileSettings.vfoScanHigh[nonVolatileSettings.currentVFONumber]))    //if we are not already inside the Low and High Limit freqs then move to the low limit.
	{
		int offset = currentChannelData->txFreq - currentChannelData->rxFreq;

		currentChannelData->rxFreq = nonVolatileSettings.vfoScanLow[nonVolatileSettings.currentVFONumber];
		currentChannelData->txFreq = currentChannelData->rxFreq + offset;
		trxSetFrequency(currentChannelData->rxFreq, currentChannelData->txFreq, DMR_MODE_AUTO);
		announceItem(PROMPT_SEQUENCE_CHANNEL_NAME_OR_VFO_FREQ, PROMPT_THRESHOLD_3);
	}
}

static void initScan(void)
{
	if (screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_DUAL_SCAN)
	{
		return;
	}

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

	screenOperationMode[nonVolatileSettings.currentVFONumber] = VFO_SCREEN_OPERATION_SCAN;
	uiDataGlobal.Scan.direction = 1;

	// If scan limits have not been defined. Set them to the current Rx freq .. +1MHz
	if ((nonVolatileSettings.vfoScanLow[nonVolatileSettings.currentVFONumber] == 0) || (nonVolatileSettings.vfoScanHigh[nonVolatileSettings.currentVFONumber] == 0))
	{
		int limitDown = currentChannelData->rxFreq;
		int limitUp = currentChannelData->rxFreq + 100000;

		// If the limitUp in not valid, set it to the next band's minFreq
		if (trxGetBandFromFrequency(limitUp) == -1)
		{
			int band = trxGetNextOrPrevBandFromFrequency(limitUp, true);

			if (band != -1)
			{
				limitUp = RADIO_HARDWARE_FREQUENCY_BANDS[band].minFreq;
			}
		}

		settingsSet(nonVolatileSettings.vfoScanLow[nonVolatileSettings.currentVFONumber], SAFE_MIN(limitUp, limitDown));
		settingsSet(nonVolatileSettings.vfoScanHigh[nonVolatileSettings.currentVFONumber], SAFE_MAX(limitUp, limitDown));
	}

	// Refresh on every step if scan boundaries is equal to one frequency step.
	uiDataGlobal.Scan.refreshOnEveryStep = ((nonVolatileSettings.vfoScanHigh[nonVolatileSettings.currentVFONumber] - nonVolatileSettings.vfoScanLow[nonVolatileSettings.currentVFONumber]) <= VFO_FREQ_STEP_TABLE[(currentChannelData->VFOflag5 >> 4)]);

	//clear all nuisance delete channels at start of scanning
	for(int i = 0; i < MAX_ZONE_SCAN_NUISANCE_CHANNELS; i++)
	{
		uiDataGlobal.Scan.nuisanceDelete[i] = -1;
	}
	uiDataGlobal.Scan.nuisanceDeleteIndex = 0;

	selectedFreq = VFO_SELECTED_FREQUENCY_INPUT_RX;

	uiDataGlobal.Scan.timer = 500;
	uiDataGlobal.Scan.state = SCAN_SCANNING;
	menuSystemPopAllAndDisplaySpecificRootMenu(UI_VFO_MODE, true);
}

static void scanning(void)
{
	static bool scanPaused = false;
	static bool voicePromptsAnnounced = true;

	if (!rxPowerSavingIsRxOn())
	{
		uiDataGlobal.Scan.timerReload = 10000;
		uiDataGlobal.Scan.timer = 0;
		return;
	}

	//After initial settling time
	if((uiDataGlobal.Scan.state == SCAN_SCANNING) && (uiDataGlobal.Scan.timer > SCAN_SKIP_CHANNEL_INTERVAL) && (uiDataGlobal.Scan.timer < (uiDataGlobal.Scan.timerReload - SCAN_FREQ_CHANGE_SETTLING_INTERVAL)))
	{
		// Test for presence of RF Carrier.

		// In FM mode the dmr slot_state will always be DMR_STATE_IDLE
		if (slot_state != DMR_STATE_IDLE)
		{
			announceItem(PROMPT_SEQUENCE_CHANNEL_NAME_OR_VFO_FREQ,
					((nonVolatileSettings.scanModePause == SCAN_MODE_STOP) ? AUDIO_PROMPT_MODE_VOICE_LEVEL_3 : PROMPT_THRESHOLD_NEVER_PLAY_IMMEDIATELY));

			if (nonVolatileSettings.scanModePause == SCAN_MODE_STOP)
			{
				uiVFOModeStopScanning();
				// Just update the header (to prevent hidden mode)
				ucClearRows(0, 2, false);
				uiUtilityRenderHeader(false);
				ucRenderRows(0, 2);
				return;
			}
			else
			{
				uiDataGlobal.Scan.state = SCAN_PAUSED;
				uiDataGlobal.Scan.timer = nonVolatileSettings.scanDelay * 1000;
				scanPaused = true;
				voicePromptsAnnounced = false;
			}
		}
		else
		{
			if(trxCarrierDetected())
			{
				announceItem(PROMPT_SEQUENCE_CHANNEL_NAME_OR_VFO_FREQ,
						((nonVolatileSettings.scanModePause == SCAN_MODE_STOP) ? AUDIO_PROMPT_MODE_VOICE_LEVEL_3 : PROMPT_THRESHOLD_NEVER_PLAY_IMMEDIATELY));

				if (nonVolatileSettings.scanModePause == SCAN_MODE_STOP)
				{
					uiVFOModeStopScanning();
					// Just update the header (to prevent hidden mode)
					ucClearRows(0, 2, false);
					uiUtilityRenderHeader(false);
					ucRenderRows(0, 2);
					return;
				}
				else
				{
					uiDataGlobal.Scan.timer = SCAN_SHORT_PAUSE_TIME; //start short delay to allow full detection of signal
					uiDataGlobal.Scan.state = SCAN_SHORT_PAUSED; //state 1 = pause and test for valid signal that produces audio
					scanPaused = true;
					voicePromptsAnnounced = false;

					// Force screen redraw in Analog mode, Dual Watch scanning
					if (screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_DUAL_SCAN)
					{
						uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
					}
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

			if (voicePromptsAnnounced == false)
			{
				if (nonVolatileSettings.audioPromptMode > AUDIO_PROMPT_MODE_VOICE_LEVEL_2)
				{
					voicePromptsPlay();
				}
				voicePromptsAnnounced = true;
			}
		}
	}

	if(uiDataGlobal.Scan.timer > 0)
	{
		uiDataGlobal.Scan.timer--;
	}
	else
	{
		// We are in Dual Watch scanning mode
		if (screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_DUAL_SCAN)
		{
			// Select and set the next VFO
			//
			// Note: nonVolatileSettings.currentVFONumber is not changed using settingsSet(), to prevent crazy EEPROM
			//       writes. uiVFOModeStopScanning() is doing this, when the scanning process ends (for any reason).
			nonVolatileSettings.currentVFONumber = (1 - nonVolatileSettings.currentVFONumber);
			currentChannelData = &settingsVFOChannel[nonVolatileSettings.currentVFONumber];

			currentChannelData->libreDMR_Power = 0x00;// Force channel to the Master power

			trxSetFrequency(currentChannelData->rxFreq, currentChannelData->txFreq, DMR_MODE_AUTO);

			//Need to load the Rx group if specified even if TG is currently overridden as we may need it later when the left or right button is pressed
			if (currentChannelData->rxGroupList != 0)
			{
				if (currentChannelData->rxGroupList != lastLoadedRxGroup)
				{
					if (codeplugRxGroupGetDataForIndex(currentChannelData->rxGroupList, &currentRxGroupData))
					{
						lastLoadedRxGroup = currentChannelData->rxGroupList;
					}
					else
					{
						lastLoadedRxGroup = -1;
					}
				}
			}
			else
			{
				memset(&currentRxGroupData, 0xFF, sizeof(struct_codeplugRxGroup_t));// If the VFO doesnt have an Rx Group ( TG List) the global var needs to be cleared, otherwise it contains the data from the previous screen e.g. Channel screen
				lastLoadedRxGroup = -1;
			}

			loadChannelData();

			if ((trxGetMode() == RADIO_MODE_DIGITAL) && (trxDMRModeRx != DMR_MODE_SFR) && (uiDataGlobal.Scan.stepTimeMilliseconds < SCAN_DMR_SIMPLEX_MIN_INTERVAL))
			{
				uiDataGlobal.Scan.timerReload = SCAN_DMR_SIMPLEX_MIN_INTERVAL;
			}
			else
			{
				uiDataGlobal.Scan.timerReload = uiDataGlobal.Scan.stepTimeMilliseconds;
			}

			if (scanPaused)
			{
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN; // Force screen redraw on scan resume
				scanPaused = false;
			}
		}
		else // Frequency scanning mode
		{
			uiEvent_t tmpEvent = { .buttons = 0, .keys = NO_KEYCODE, .rotary = 0, .function = 0, .events = NO_EVENT, .hasEvent = 0, .time = 0 };
			int fStep = VFO_FREQ_STEP_TABLE[(currentChannelData->VFOflag5 >> 4)];

			if (uiDataGlobal.Scan.direction == 1)
			{
				if(currentChannelData->rxFreq + fStep <= nonVolatileSettings.vfoScanHigh[nonVolatileSettings.currentVFONumber])
				{
					handleUpKey(&tmpEvent);
				}
				else
				{
					int offset = currentChannelData->txFreq - currentChannelData->rxFreq;

					currentChannelData->rxFreq = nonVolatileSettings.vfoScanLow[nonVolatileSettings.currentVFONumber];
					currentChannelData->txFreq = currentChannelData->rxFreq + offset;
					trxSetFrequency(currentChannelData->rxFreq, currentChannelData->txFreq, DMR_MODE_AUTO);
				}
			}
			else
			{
				if(currentChannelData->rxFreq + fStep >= nonVolatileSettings.vfoScanLow[nonVolatileSettings.currentVFONumber])
				{
					handleUpKey(&tmpEvent);
				}
				else
				{
					int offset = currentChannelData->txFreq - currentChannelData->rxFreq;
					currentChannelData->rxFreq = nonVolatileSettings.vfoScanHigh[nonVolatileSettings.currentVFONumber];
					currentChannelData->txFreq = currentChannelData->rxFreq+offset;
					trxSetFrequency(currentChannelData->rxFreq, currentChannelData->txFreq, DMR_MODE_AUTO);
				}
			}

			if (uiDataGlobal.Scan.refreshOnEveryStep)
			{
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
			}
		}

		uiDataGlobal.Scan.timer = uiDataGlobal.Scan.timerReload;
		uiDataGlobal.Scan.state = SCAN_SCANNING; // Settling and test for carrier presence.

		if (screenOperationMode[nonVolatileSettings.currentVFONumber] == VFO_SCREEN_OPERATION_SCAN)
		{
			//check all nuisance delete entries and skip channel if there is a match
			for(int i = 0; i < MAX_ZONE_SCAN_NUISANCE_CHANNELS; i++)
			{
				if (uiDataGlobal.Scan.nuisanceDelete[i] == -1)
				{
					break;
				}
				else
				{
					if(uiDataGlobal.Scan.nuisanceDelete[i] == currentChannelData->rxFreq)
					{
						uiDataGlobal.Scan.timer = SCAN_SKIP_CHANNEL_INTERVAL;
						break;
					}
				}
			}
		}
	}
}
