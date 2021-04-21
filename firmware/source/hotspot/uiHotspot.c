/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
 *
 * Using some code ported from MMDVM_HS by Andy CA6JAU
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
#include <ctype.h>
#include "functions/calibration.h"
#include "hotspot/uiHotspot.h"
#include "user_interface/menuSystem.h"
#include "user_interface/uiUtilities.h"
#include "user_interface/uiLocalisation.h"
#include "hotspot/DMREmbeddedData.h"
#include "hotspot/DMRFullLC.h"
#include "hotspot/DMRShortLC.h"
#include "hotspot/DMRSlotType.h"
#include "hotspot/QR1676.h"
#include "hardware/HR-C6000.h"
#include "functions/settings.h"
#include "functions/sound.h"
#include "functions/ticks.h"
#include "functions/trx.h"
#include "usb/usb_com.h"
#include "functions/rxPowerSaving.h"

// Uncomment the following to enable demo screen, access it with function events
//#define DEMO_SCREEN

/*
                                Problems with MD-390 on the same frequency
                                ------------------------------------------

 Using a Dual Hat:
M: 2020-01-07 08:46:23.337 DMR Slot 2, received RF voice header from F1RMB to 5000
M: 2020-01-07 08:46:24.143 DMR Slot 2, received RF end of voice transmission from F1RMB to 5000, 0.7 seconds, BER: 0.4%
M: 2020-01-07 08:46:24.644 DMR Slot 2, RF user 12935424 rejected

 Using OpenHD77 HS
 PC 5000 sent from a GD-77
M: 2020-01-07 09:51:55.467 DMR Slot 2, received RF voice header from F1RMB to 5000
M: 2020-01-07 09:51:56.727 DMR Slot 2, received RF end of voice transmission from F1RMB to 5000, 1.1 seconds, BER: 0.3%
M: 2020-01-07 09:52:00.068 DMR Slot 2, received network voice header from 5000 to TG 9
M: 2020-01-07 09:52:03.428 DMR Slot 2, received network end of voice transmission from 5000 to TG 9, 3.5 seconds, 0% packet loss, BER: 0.0%

 Its echo (data sent from the MD-390, by itself):
M: 2020-01-07 09:52:07.300 DMR Slot 2, received RF voice header from F1RMB to 5000
M: 2020-01-07 09:52:09.312 DMR Slot 2, RF voice transmission lost from F1RMB to 5000, 0.0 seconds, BER: 0.0%
M: 2020-01-07 09:52:11.856 DMR Slot 2, received network voice header from 5000 to TG 9
M: 2020-01-07 09:52:15.246 DMR Slot 2, received network end of voice transmission from 5000 to TG 9, 3.5 seconds, 0% packet loss, BER: 0.0%


	 There is a problem if you have a MD-390, GPS enabled, listening on the same frequency (even without different DMRId, I checked !).
	 It send invalid data from time to time (that's not critical, it's simply rejected), but once it heard a PC or TG call (from other
	 transceiver), it will keep repeating that, instead of invalid data.

	 After investigations, it's turns out if you enable 'GPS System' (even with correct configuration, like
	 GC xxx999 for BM), it will send such weird or echo data each GPS interval.

 ** Long story short: turn off GPS stuff on your MD-390 if it's listening local transmissions from other transceivers. **

 */


// Uncomment this to enable all sendDebug*() functions. You will see the results in the MMDVMHost log file.
//#define MMDVM_SEND_DEBUG


#define MMDVM_FRAME_START   0xE0U

#define MMDVM_GET_VERSION   0x00U
#define MMDVM_GET_STATUS    0x01U
#define MMDVM_SET_CONFIG    0x02U
#define MMDVM_SET_MODE      0x03U
#define MMDVM_SET_FREQ      0x04U
#define MMDVM_CAL_DATA      0x08U
#define MMDVM_RSSI_DATA     0x09U
#define MMDVM_SEND_CWID     0x0AU

#define MMDVM_DSTAR_HEADER  0x10U
#define MMDVM_DSTAR_DATA    0x11U
#define MMDVM_DSTAR_LOST    0x12U
#define MMDVM_DSTAR_EOT     0x13U

#define MMDVM_DMR_DATA1     0x18U
#define MMDVM_DMR_LOST1     0x19U
#define MMDVM_DMR_DATA2     0x1AU
#define MMDVM_DMR_LOST2     0x1BU
#define MMDVM_DMR_SHORTLC   0x1CU
#define MMDVM_DMR_START     0x1DU
#define MMDVM_DMR_ABORT     0x1EU

#define MMDVM_YSF_DATA      0x20U
#define MMDVM_YSF_LOST      0x21U

#define MMDVM_P25_HDR       0x30U
#define MMDVM_P25_LDU       0x31U
#define MMDVM_P25_LOST      0x32U

#define MMDVM_NXDN_DATA     0x40U
#define MMDVM_NXDN_LOST     0x41U

#define MMDVM_POCSAG_DATA   0x50U

#define MMDVM_ACK           0x70U
#define MMDVM_NAK           0x7FU

#define MMDVM_SERIAL        0x80U
#define MMDVM_TRANSPARENT   0x90U
#define MMDVM_QSO_INFO      0x91U

#define MMDVM_DEBUG1        0xF1U
#define MMDVM_DEBUG2        0xF2U
#define MMDVM_DEBUG3        0xF3U
#define MMDVM_DEBUG4        0xF4U
#define MMDVM_DEBUG5        0xF5U

#define PROTOCOL_VERSION    1U

#define MMDVM_HEADER_LENGTH 4U

#define HOTSPOT_VERSION_STRING "OpenGD77_HS v0.1.6"
#define concat(a, b) a " GitID #" b ""
static const char HARDWARE[] = concat(HOTSPOT_VERSION_STRING, GITVERSION);

// Fake TA and QSO Info (send by MMDVMHost's CAST display driver)
static char mmdvmQSOInfoIP[22] = {0}; // use 6x8 font; 21 char long
static char overriddenLCTA[2 * 9] = {0}; // 2 LC frame only (enough to store callsign)
static bool overriddenLCAvailable = false;
static uint8_t overriddenBlocksTA = 0x00;

static const uint8_t MMDVM_VOICE_SYNC_PATTERN = 0x20U;

static const int EMBEDDED_DATA_OFFSET = 13U;
static const int TX_BUFFER_MIN_BEFORE_TRANSMISSION = 4;

static const uint8_t START_FRAME_PATTERN[]  = { 0xFF,0x57,0xD7,0x5D,0xF5,0xD9 };
static const uint8_t END_FRAME_PATTERN[]    = { 0x5D,0x7F,0x77,0xFD,0x75,0x79 };
//static const uint32_t HOTSPOT_BUFFER_LENGTH = 0xA0;

static uint32_t freq_rx = 0;
static uint32_t freq_tx = 0;
static uint8_t colorCode = 1;
static uint8_t rf_power = 255;
static uint32_t tx_delay = 0;
static uint32_t savedTGorPC;
static uint8_t hotspotTxLC[9];
static bool startedEmbeddedSearch = false;

// USB TX read/write positions and count
volatile uint16_t usbComSendBufWritePosition = 0;
volatile uint16_t usbComSendBufReadPosition = 0;
volatile uint16_t usbComSendBufCount = 0;

// RF data read/write positions and count
volatile uint32_t rfFrameBufReadIdx = 0;
volatile uint32_t rfFrameBufWriteIdx = 0;
volatile uint32_t rfFrameBufCount = 0;

static uint8_t lastRxState = HOTSPOT_RX_IDLE;
static const int TX_BUFFERING_TIMEOUT = 5000;// 500mS

static int timeoutCounter;
static uint8_t savedLibreDMR_Power;
static int savedPowerLevel = -1;// no power level saved yet
static int hotspotPowerLevel = 0;// no power level saved yet

static uint32_t mmdvmHostLastActiveTime = 0; // store last activity time (ms)
static const uint32_t MMDVMHOST_TIMEOUT = 20000; // 20s timeout (MMDVMHost mode only, there is no timeout for BlueDV)
static bool mmdvmHostIsConnected = false;

static bool displayFWVersion;
static uint8_t currentRxCommandState;

static DMRLC_T rxedDMR_LC; // used to stored LC info from RXed frames

extern LinkItem_t *LinkHead;


static volatile enum
{
	MMDVMHOST_RX_READY,
	MMDVMHOST_RX_BUSY,
	MMDVMHOST_RX_ERROR
} MMDVMHostRxState;

typedef enum
{
	STATE_IDLE      = 0,
	STATE_DSTAR     = 1,
	STATE_DMR       = 2,
	STATE_YSF       = 3,
	STATE_P25       = 4,
	STATE_NXDN      = 5,
	STATE_POCSAG    = 6,

	// Dummy states start at 90
	STATE_DMRDMO1K  = 92,
	STATE_RSSICAL   = 96,
	STATE_CWID      = 97,
	STATE_DMRCAL    = 98,
	STATE_DSTARCAL  = 99,
	STATE_INTCAL    = 100,
	STATE_POCSAGCAL = 101
} MMDVM_STATE;

static volatile MMDVM_STATE modemState = STATE_IDLE;


typedef enum
{
	HOTSPOT_STATE_NOT_CONNECTED,
	HOTSPOT_STATE_INITIALISE,
	HOTSPOT_STATE_RX_START,
	HOTSPOT_STATE_RX_PROCESS,
	HOTSPOT_STATE_RX_END,
	HOTSPOT_STATE_TX_START_BUFFERING,
	HOTSPOT_STATE_TRANSMITTING,
	HOTSPOT_STATE_TX_SHUTDOWN
} HOTSPOT_STATE;

static volatile HOTSPOT_STATE hotspotState = HOTSPOT_STATE_NOT_CONNECTED;

//const int USB_SERIAL_TX_RETRIES = 2;
static const uint8_t VOICE_LC_SYNC_FULL[]       = { 0x04U, 0x6DU, 0x5DU, 0x7FU, 0x77U, 0xFDU, 0x75U, 0x7EU, 0x30U };
static const uint8_t TERMINATOR_LC_SYNC_FULL[]  = { 0x04U, 0xADU, 0x5DU, 0x7FU, 0x77U, 0xFDU, 0x75U, 0x79U, 0x60U };

static const uint8_t LC_SYNC_MASK_FULL[]        = { 0x0FU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xF0U };


static const uint8_t DMR_AUDIO_SEQ_SYNC[6][7]   = {
		{ 0x07U, 0xF0U, 0x00U, 0x00U, 0x00U, 0x0FU, 0xD0U },  // seq 0 NOT USED AS THIS IS THE SYNC
		{ 0x01U, 0x30U, 0x00U, 0x00U, 0x00U, 0x09U, 0x10U },  // seq 1
		{ 0x01U, 0x70U, 0x00U, 0x00U, 0x00U, 0x07U, 0x40U },  // seq 2
		{ 0x01U, 0x70U, 0x00U, 0x00U, 0x00U, 0x07U, 0x40U },  // seq 3
		{ 0x01U, 0x50U, 0x00U, 0x00U, 0x00U, 0x00U, 0x70U },  // seq 4
		{ 0x01U, 0x10U, 0x00U, 0x00U, 0x00U, 0x0EU, 0x20U }   // seq 5
};

static const uint8_t DMR_AUDIO_SEQ_MASK[]       = { 0x0FU, 0xF0U, 0x00U, 0x00U, 0x00U, 0x0FU, 0xF0U };
static const uint8_t DMR_EMBED_SEQ_MASK[]       = { 0x00U, 0x0FU, 0xFFU, 0xFFU, 0xFFU, 0xF0U, 0x00U };


// CWID related
static const struct
{
	uint8_t  c;
	uint32_t pattern;
	uint8_t  length;
} CW_SYMBOL_LIST[] = {
		{'A', 0xB8000000U, 8U},
		{'B', 0xEA800000U, 12U},
		{'C', 0xEBA00000U, 14U},
		{'D', 0xEA000000U, 10U},
		{'E', 0x80000000U, 4U},
		{'F', 0xAE800000U, 12U},
		{'G', 0xEE800000U, 12U},
		{'H', 0xAA000000U, 10U},
		{'I', 0xA0000000U, 6U},
		{'J', 0xBBB80000U, 16U},
		{'K', 0xEB800000U, 12U},
		{'L', 0xBA800000U, 12U},
		{'M', 0xEE000000U, 10U},
		{'N', 0xE8000000U, 8U},
		{'O', 0xEEE00000U, 14U},
		{'P', 0xBBA00000U, 14U},
		{'Q', 0xEEB80000U, 16U},
		{'R', 0xBA000000U, 10U},
		{'S', 0xA8000000U, 8U},
		{'T', 0xE0000000U, 6U},
		{'U', 0xAE000000U, 10U},
		{'V', 0xAB800000U, 12U},
		{'W', 0xBB800000U, 12U},
		{'X', 0xEAE00000U, 14U},
		{'Y', 0xEBB80000U, 16U},
		{'Z', 0xEEA00000U, 14U},
		{'1', 0xBBBB8000U, 20U},
		{'2', 0xAEEE0000U, 18U},
		{'3', 0xABB80000U, 16U},
		{'4', 0xAAE00000U, 14U},
		{'5', 0xAA800000U, 12U},
		{'6', 0xEAA00000U, 14U},
		{'7', 0xEEA80000U, 16U},
		{'8', 0xEEEA0000U, 18U},
		{'9', 0xEEEE8000U, 20U},
		{'0', 0xEEEEE000U, 22U},
		{'/', 0xEAE80000U, 16U},
		{'?', 0xAEEA0000U, 18U},
		{',', 0xEEAEE000U, 22U},
		{'-', 0xEAAE0000U, 18U},
		{'=', 0xEAB80000U, 16U},
		{'.', 0xBAEB8000U, 20U},
		{' ', 0x00000000U, 4U},
		{0U,  0x00000000U, 0U}
};

static const uint8_t BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

static const uint32_t cwDOTDuration = (10U * 60U); // 60ms per DOT
static uint32_t cwNextPeriod = 0;

static uint8_t cwBuffer[64U];
static uint16_t cwpoLen;
static uint16_t cwpoPtr;
static bool cwKeying = false;
static uint8_t savedDMRDestinationFilter = 0xFF; // 0xFF value means unset
static uint8_t savedDMRCcTsFilter = 0xFF; // 0xFF value means unset

// End of CWID related


static void updateScreen(uint8_t rxState);
static bool handleEvent(uiEvent_t *ev);
static void hotspotExit(void);
static void hotspotStateMachine(void);
static void processUSBDataQueue(void);
static void handleHotspotRequest(void);
static void cwReset(void);
static void cwProcess(void);

#if defined(MMDVM_SEND_DEBUG)
static void sendDebug1(const char *text);
static void sendDebug2(const char *text, int16_t n1);
static void sendDebug3(const char *text, int16_t n1, int16_t n2);
static void sendDebug4(const char *text, int16_t n1, int16_t n2, int16_t n3);
static void sendDebug5(const char *text, int16_t n1, int16_t n2, int16_t n3, int16_t n4);
#endif

#if defined(DEMO_SCREEN)
bool demoScreen = false;
#endif

menuStatus_t menuHotspotMode(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		rxPowerSavingSetLevel(0);// disable power saving

		// DMR filter level isn't saved yet (cycling power OFF/ON quickly can corrupt
		// this value otherwise, as menuHotspotMode(true) could be called twice.
		if (savedDMRDestinationFilter == 0xFF)
		{
			// Override DMR filtering
			savedDMRDestinationFilter = nonVolatileSettings.dmrDestinationFilter;
			savedDMRCcTsFilter = nonVolatileSettings.dmrCcTsFilter;
			settingsSet(nonVolatileSettings.dmrDestinationFilter, DMR_DESTINATION_FILTER_NONE);
			settingsSet(nonVolatileSettings.dmrCcTsFilter, DMR_CCTS_FILTER_CC_TS);
		}

		hotspotState = HOTSPOT_STATE_NOT_CONNECTED;

		// Do not user per channel power settings
		savedLibreDMR_Power = currentChannelData->libreDMR_Power;
		currentChannelData->libreDMR_Power = 0;

		savedTGorPC = trxTalkGroupOrPcId;// Save the current TG or PC
		trxTalkGroupOrPcId = 0;
		mmdvmHostIsConnected = false;
		currentRxCommandState = HOTSPOT_RX_UNKNOWN;
		displayFWVersion = false;
		overriddenLCTA[0] = 0;
		overriddenBlocksTA = 0x0;
		overriddenLCAvailable = false;
		cwKeying = false;
		cwReset();

		memset(&rxedDMR_LC, 0, sizeof(DMRLC_T));// clear automatic variable

		// Clear RF buffers
		rfFrameBufCount = 0;
		rfFrameBufReadIdx = 0;
		rfFrameBufWriteIdx = 0;
		for (uint8_t i = 0; i < HOTSPOT_BUFFER_COUNT; i++)
		{
			memset((void *)&audioAndHotspotDataBuffer.hotspotBuffer[i], 0, HOTSPOT_BUFFER_SIZE);
		}

		// Clear USB TX buffers
		usbComSendBufWritePosition = 0;
		usbComSendBufReadPosition = 0;
		usbComSendBufCount = 0;
		memset(&usbComSendBuf, 0, sizeof(usbComSendBuf));

		trxSetModeAndBandwidth(RADIO_MODE_DIGITAL, false);// hotspot mode is for DMR i.e Digital mode

		if (freq_tx == 0)
		{
			freq_tx = 43000000;
		}

		if (freq_rx == 0)
		{
			freq_rx = 43000000;
		}

		tx_delay = 0;

		MMDVMHostRxState = MMDVMHOST_RX_READY; // We have not sent anything to MMDVMHost, so it can't be busy yet.

		// Set CC, QRG and power, in case hotspot menu has left then re-enter.
		trxSetDMRColourCode(colorCode);

		if (trxCheckFrequencyInAmateurBand(freq_rx) && trxCheckFrequencyInAmateurBand(freq_tx))
		{
			trxSetFrequency(freq_rx, freq_tx, DMR_MODE_DMO);
		}

		if (rf_power != 255)
		{
			if (rf_power < 50)
			{
				hotspotPowerLevel = rf_power / 12;
			}
			else
			{
				hotspotPowerLevel = (rf_power / 50) + 3;
			}

			trxSetPowerFromLevel(hotspotPowerLevel);
		}

		ucClearBuf();
		ucPrintCentered(0, "Hotspot", FONT_SIZE_3);
		ucPrintCentered(32, "Waiting for", FONT_SIZE_3);
		ucPrintCentered(48, "Pi-Star", FONT_SIZE_3);
		ucRender();

		displayLightTrigger(false);
	}
	else
	{

#if defined(PLATFORM_GD77S)
		uiChannelModeHeartBeatActivityForGD77S(ev);
#endif

		if (ev->hasEvent)
		{
			if (handleEvent(ev) == false)
			{
				return MENU_STATUS_SUCCESS;
			}
		}
	}

#if defined(DEMO_SCREEN)
	if (! demoScreen)
	{
#endif
		processUSBDataQueue();
		if (com_request == 1)
		{
			handleHotspotRequest();
			com_request = 0;
		}
		hotspotStateMachine();

		// CW beaconing
		if (cwKeying)
		{
			if (cwpoLen > 0U)
			{
				cwProcess();
			}
		}
#if defined(DEMO_SCREEN)
	}
#endif

	return MENU_STATUS_SUCCESS;
}

void menuHotspotRestoreSettings(void)
{
	if (savedDMRDestinationFilter != 0xFF)
	{
		settingsSet(nonVolatileSettings.dmrDestinationFilter, savedDMRDestinationFilter);
		savedDMRDestinationFilter = 0xFF; // Unset saved DMR destination filter level

		settingsSet(nonVolatileSettings.dmrCcTsFilter, savedDMRCcTsFilter);
		savedDMRCcTsFilter = 0xFF; // Unset saved CC TS filter level
	}
}

static void displayContactInfo(uint8_t y, char *text, size_t maxLen)
{
	char buffer[37]; // Max: TA 27 (in 7bit format) + ' [' + 6 (Maidenhead)  + ']' + NULL

	if (strlen(text) >= 5)
	{
		int32_t  cpos;

		if ((cpos = getFirstSpacePos(text)) != -1)
		{
			// Callsign found
			memcpy(buffer, text, cpos);
			buffer[cpos] = 0;
			ucPrintCentered(y, chomp(buffer), FONT_SIZE_3);
		}
		else
		{
			// No space found, use a chainsaw
			memcpy(buffer, text, 16);
			buffer[16] = 0;

			ucPrintCentered(y, chomp(buffer), FONT_SIZE_3);
		}
	}
	else
	{
		memcpy(buffer, text, strlen(text));
		buffer[strlen(text)] = 0;
		ucPrintCentered(y, chomp(buffer), FONT_SIZE_3);
	}
}

static void updateContactLine(uint8_t y)
{
	if ((LinkHead->talkGroupOrPcId >> 24) == PC_CALL_FLAG) // Its a Private call
	{
		ucPrintCentered(y, LinkHead->contact, FONT_SIZE_3);
	}
	else // Group call
	{
		switch (nonVolatileSettings.contactDisplayPriority)
		{
			case CONTACT_DISPLAY_PRIO_CC_DB_TA:
			case CONTACT_DISPLAY_PRIO_DB_CC_TA:
				// No contact found is codeplug and DMRIDs, use TA as fallback, if any.
				if ((strncmp(LinkHead->contact, "ID:", 3) == 0) && (LinkHead->talkerAlias[0] != 0x00))
				{
					displayContactInfo(y, LinkHead->talkerAlias, sizeof(LinkHead->talkerAlias));
				}
				else
				{
					displayContactInfo(y, LinkHead->contact, sizeof(LinkHead->contact));
				}
				break;

			case CONTACT_DISPLAY_PRIO_TA_CC_DB:
			case CONTACT_DISPLAY_PRIO_TA_DB_CC:
				// Talker Alias have the priority here
				if (LinkHead->talkerAlias[0] != 0x00)
				{
					displayContactInfo(y, LinkHead->talkerAlias, sizeof(LinkHead->talkerAlias));
				}
				else // No TA, then use the one extracted from Codeplug or DMRIdDB
				{
					displayContactInfo(y, LinkHead->contact, sizeof(LinkHead->contact));
				}
				break;
		}
	}
}

static void updateScreen(uint8_t rxCommandState)
{
	int val_before_dp;
	int val_after_dp;
	static const int bufferLen = 17;
	char buffer[22U]; // set to 22 due to FW info

	currentRxCommandState = rxCommandState;

	ucClearBuf();
	ucPrintAt(4, 4, "DMR", FONT_SIZE_1);
	ucPrintCentered(0, "Hotspot", FONT_SIZE_3);

	snprintf(buffer, bufferLen, "%d%%", getBatteryPercentage());

	ucPrintAt(DISPLAY_SIZE_X - (strlen(buffer) * 6) - 4, 4, buffer, FONT_SIZE_1);

	if (trxTransmissionEnabled)
	{
		if (displayFWVersion)
		{
			snprintf(buffer, 22U, "%s", &HOTSPOT_VERSION_STRING[12]);
			ucPrintCentered(16 + 4, buffer, FONT_SIZE_1);
		}
		else
		{
			if (cwKeying)
			{
				sprintf(buffer, "%s", "<Tx CW ID>");
				ucPrintCentered(16, buffer, FONT_SIZE_3);
			}
			else
			{
				updateContactLine(16);
			}
		}

		if (cwKeying)
		{
			buffer[0] = 0;
		}
		else
		{
			if ((trxTalkGroupOrPcId & 0xFF000000) == 0)
			{
				snprintf(buffer, bufferLen, "%s %d", currentLanguage->tg, trxTalkGroupOrPcId & 0x00FFFFFF);
			}
			else
			{
				snprintf(buffer, bufferLen, "%s %d", currentLanguage->pc, trxTalkGroupOrPcId & 0x00FFFFFF);
			}
		}

		ucPrintCentered(32, buffer, FONT_SIZE_3);

		val_before_dp = freq_tx / 100000;
		val_after_dp = freq_tx - val_before_dp * 100000;
		sprintf(buffer, "T %d.%05d MHz", val_before_dp, val_after_dp);
	}
	else
	{
		if (rxCommandState == HOTSPOT_RX_START  || rxCommandState == HOTSPOT_RX_START_LATE)
		{
			dmrIdDataStruct_t currentRec;

			if (displayFWVersion)
			{
				snprintf(buffer, 22U, "%s", &HOTSPOT_VERSION_STRING[12]);
			}
			else
			{
				if (dmrIDLookup(rxedDMR_LC.srcId, &currentRec) == true)
				{
					snprintf(buffer, bufferLen, "%s", currentRec.text);
				}
				else
				{
					snprintf(buffer, bufferLen, "ID: %d", rxedDMR_LC.srcId);
				}
			}

			ucPrintCentered(16 + (displayFWVersion ? 4 : 0), buffer, (displayFWVersion ? FONT_SIZE_1 : FONT_SIZE_3));

			if (rxedDMR_LC.FLCO == 0)
			{
				snprintf(buffer, bufferLen, "%s %d", currentLanguage->tg, rxedDMR_LC.dstId);
			}
			else
			{
				snprintf(buffer, bufferLen, "%s %d", currentLanguage->pc, rxedDMR_LC.dstId);
			}

			ucPrintCentered(32, buffer, FONT_SIZE_3);
		}
		else
		{

			if (displayFWVersion)
			{
				snprintf(buffer, 22U, "%s", &HOTSPOT_VERSION_STRING[12]);
				ucPrintCentered(16 + 4, buffer, FONT_SIZE_1);
			}
			else
			{
				if (modemState == STATE_POCSAG)
				{
					ucPrintCentered(16, "<POCSAG>", FONT_SIZE_3);
				}
				else
				{
					if (strlen(mmdvmQSOInfoIP))
					{
						ucPrintCentered(16 + 4, mmdvmQSOInfoIP, FONT_SIZE_1);
					}
				}
			}

			snprintf(buffer, bufferLen, "CC:%d", trxGetDMRColourCode());//, trxGetDMRTimeSlot()+1) ;

			ucPrintCore(0, 32, buffer, FONT_SIZE_3, TEXT_ALIGN_LEFT, false);

			sprintf(buffer,"%s%s",POWER_LEVELS[hotspotPowerLevel],POWER_LEVEL_UNITS[hotspotPowerLevel]);
			ucPrintCore(0, 32, buffer, FONT_SIZE_3, TEXT_ALIGN_RIGHT, false);
		}
		val_before_dp = freq_rx / 100000;
		val_after_dp = freq_rx - val_before_dp * 100000;
		snprintf(buffer, bufferLen, "R %d.%05d MHz", val_before_dp, val_after_dp);
	}

	ucPrintCentered(48, buffer, FONT_SIZE_3);
	ucRender();

	displayLightTrigger(false);
}

static bool handleEvent(uiEvent_t *ev)
{
	displayLightTrigger(ev->hasEvent);

#if defined(DEMO_SCREEN)
	if (ev->events & FUNCTION_EVENT)
	{
		demoScreen = ((ev->function >= 90) && (ev->function < 100));

		switch (ev->function)
		{
			case 90:
				sprintf(mmdvmQSOInfoIP, "%s", "192.168.100.10");
				settingsUsbMode = USB_MODE_CPS;
				break;
			case 91:
				trxTalkGroupOrPcId = 98977;
				LinkHead->talkGroupOrPcId = 0;
				sprintf(LinkHead->contact, "%s", "VK3KYY");
				trxTransmissionEnabled = true;
				updateScreen(HOTSPOT_STATE_TX_START_BUFFERING);
				break;
			case 92:
				rxedDMR_LC.FLCO = 0;
				rxedDMR_LC.srcId = 5053238;
				rxedDMR_LC.dstId = 91;
				trxTransmissionEnabled = false;
				updateScreen(HOTSPOT_RX_START);
				break;
			case 93:
				modemState = STATE_POCSAG;
				trxTransmissionEnabled = false;
				updateScreen(HOTSPOT_RX_IDLE);
				break;
		}
	}
#endif

	if (KEYCHECK_SHORTUP(ev->keys, KEY_RED)
#if defined(PLATFORM_GD77S)
			// There is no RED key on the GD-77S, use Orange button to exit the hotspot mode
			|| ((ev->events & BUTTON_EVENT) && (ev->buttons & BUTTON_ORANGE))
#endif
	)
	{
		// Do not permit to leave HS in MMDVMHost mode, otherwise that will mess up the communication
		// and MMDVMHost won't recover from that, sometimes.
		// Anyway, in MMDVMHost mode, there is a timeout after MMDVMHost stop responding (or went to shutdown).
		if (nonVolatileSettings.hotspotType == HOTSPOT_TYPE_BLUEDV)
		{
			hotspotExit();
			return false;
		}
	}

	if (ev->events & BUTTON_EVENT)
	{
		// Display HS FW version
		if ((displayFWVersion == false) && (ev->buttons == BUTTON_SK1))
		{
			uint8_t prevRxCmd = currentRxCommandState;

			displayFWVersion = true;
			updateScreen(currentRxCommandState);
			currentRxCommandState = prevRxCmd;
			return true;
		}
		else if (displayFWVersion && ((ev->buttons & BUTTON_SK1) == 0))
		{
			displayFWVersion = false;
			updateScreen(currentRxCommandState);
			return true;
		}
	}

	return true;
}

static void hotspotExit(void)
{
	trxDisableTransmission();
	if (trxTransmissionEnabled)
	{
		trxTransmissionEnabled = false;
		//trxSetRX();

		LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);

		if (cwKeying)
		{
			cwReset();
			cwKeying = false;
		}
	}

	currentChannelData->libreDMR_Power = savedLibreDMR_Power;

	trxTalkGroupOrPcId = savedTGorPC;// restore the current TG or PC
	if (savedPowerLevel != -1)
	{
		trxSetPowerFromLevel(savedPowerLevel);
	}

	trxDMRID = codeplugGetUserDMRID();
	settingsUsbMode = USB_MODE_CPS;
	mmdvmHostIsConnected = false;

	menuHotspotRestoreSettings();
	menuSystemPopAllAndDisplayRootMenu();
}

// Queue system is a single byte header containing the length of the item, followed by the data
// if the block won't fit in the space between the current write location and the end of the buffer,
// a zero is written to the length for that block and the data and its length byte is put at the beginning of the buffer
static void enqueueUSBData(uint8_t *data, uint8_t length)
{
	if ((usbComSendBufWritePosition + (length + 1)) > (COM_BUFFER_SIZE - 1))
	{
		usbComSendBuf[usbComSendBufWritePosition] = 0xFF; // flag that the data block won't fit and will be put at the start of the buffer
		usbComSendBufWritePosition = 0;
	}

	usbComSendBuf[usbComSendBufWritePosition] = length;
	memcpy(&usbComSendBuf[usbComSendBufWritePosition + 1], data, length);
	usbComSendBufWritePosition += (length + 1);
	usbComSendBufCount++;

	if (length <= 1)
	{
		// Blah
	}

}

static void processUSBDataQueue(void)
{
	if (usbComSendBufCount > 0)
	{
		if (usbComSendBuf[usbComSendBufReadPosition] == 0xFF) // End marker
		{
			usbComSendBuf[usbComSendBufReadPosition] = 0;
			usbComSendBufReadPosition = 0;
		}

		uint8_t len = usbComSendBuf[usbComSendBufReadPosition] + 1;

		if (len < (3U + 1U)) // the shortest MMDVM frame length (3U = DMRLost)
		{
			usbComSendBufCount--;
		}
		else
		{
			usb_status_t status = USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, &usbComSendBuf[usbComSendBufReadPosition + 1], usbComSendBuf[usbComSendBufReadPosition]);

			if (status == kStatus_USB_Success)
			{
				usbComSendBufReadPosition += len;

				if (usbComSendBufReadPosition >= (COM_BUFFER_SIZE - 1)) // reaching the end of the buffer
				{
					usbComSendBufReadPosition = 0;
				}

				usbComSendBufCount--;
			}
			else
			{
				// USB Send Fail
			}
		}
	}
}

static void swapWithFakeTA(uint8_t *lc)
{
	if ((lc[0] >= FLCO_TALKER_ALIAS_HEADER) && (lc[0] < FLCO_TALKER_ALIAS_BLOCK2))
	{
		uint8_t blockID = lc[0] - 4;

		if ((overriddenBlocksTA & (1 << blockID)) != 0U)
		{
			// Clear the bit as it was consumed
			overriddenBlocksTA &= ~(1 << blockID);

			memcpy(lc, overriddenLCTA + (blockID * 9), 9);
		}
	}
}

static void setRSSIToFrame(uint8_t *frameData)
{
	frameData[DMR_FRAME_LENGTH_BYTES + MMDVM_HEADER_LENGTH]      = (trxRxSignal >> 8) & 0xFFU;
	frameData[DMR_FRAME_LENGTH_BYTES + MMDVM_HEADER_LENGTH + 1U] = (trxRxSignal >> 0) & 0xFFU;
}

static void hotspotSendVoiceFrame(volatile const uint8_t *receivedDMRDataAndAudio)
{
	uint8_t frameData[DMR_FRAME_LENGTH_BYTES + MMDVM_HEADER_LENGTH + 2U] = {MMDVM_FRAME_START, (DMR_FRAME_LENGTH_BYTES + MMDVM_HEADER_LENGTH + 2U), MMDVM_DMR_DATA2};
	uint8_t embData[DMR_FRAME_LENGTH_BYTES];
	int i;
	int sequenceNumber = receivedDMRDataAndAudio[27 + 0x0c + 1] - 1;

	// copy the audio sections
	memcpy(frameData + MMDVM_HEADER_LENGTH, (uint8_t *)receivedDMRDataAndAudio + 0x0C, 14);
	memcpy(frameData + MMDVM_HEADER_LENGTH + EMBEDDED_DATA_OFFSET + 6, (uint8_t *)receivedDMRDataAndAudio + 0x0C + EMBEDDED_DATA_OFFSET, 14);

	if (sequenceNumber == 0)
	{
		frameData[3] = MMDVM_VOICE_SYNC_PATTERN;// sequence 0
		for (i = 0U; i < 7U; i++)
		{
			frameData[i + EMBEDDED_DATA_OFFSET + MMDVM_HEADER_LENGTH] = (frameData[i + EMBEDDED_DATA_OFFSET + MMDVM_HEADER_LENGTH] & ~SYNC_MASK[i]) | MS_SOURCED_AUDIO_SYNC[i];
		}
	}
	else
	{
		frameData[3] = sequenceNumber;
		DMREmbeddedData_getData(embData, sequenceNumber);
		for (i = 0U; i < 7U; i++)
		{
			frameData[i + EMBEDDED_DATA_OFFSET + MMDVM_HEADER_LENGTH] = (frameData[i + EMBEDDED_DATA_OFFSET + MMDVM_HEADER_LENGTH] & ~DMR_AUDIO_SEQ_MASK[i]) | DMR_AUDIO_SEQ_SYNC[sequenceNumber][i];
			frameData[i + EMBEDDED_DATA_OFFSET + MMDVM_HEADER_LENGTH] = (frameData[i + EMBEDDED_DATA_OFFSET + MMDVM_HEADER_LENGTH] & ~DMR_EMBED_SEQ_MASK[i]) | embData[i + EMBEDDED_DATA_OFFSET];
		}
	}

	// Add RSSI into frame
	setRSSIToFrame(frameData);

	enqueueUSBData(frameData, frameData[1U]);
}

static void sendVoiceHeaderLC_Frame(volatile const uint8_t *receivedDMRDataAndAudio)
{
	uint8_t frameData[DMR_FRAME_LENGTH_BYTES + MMDVM_HEADER_LENGTH + 2U] = {MMDVM_FRAME_START, (DMR_FRAME_LENGTH_BYTES + MMDVM_HEADER_LENGTH + 2U), MMDVM_DMR_DATA2, DMR_SYNC_DATA | DT_VOICE_LC_HEADER};

	DMRLC_T lc;

	memset(&lc, 0, sizeof(DMRLC_T));// clear automatic variable

	lc.srcId = (receivedDMRDataAndAudio[6] << 16) + (receivedDMRDataAndAudio[7] << 8) + (receivedDMRDataAndAudio[8] << 0);
	lc.dstId = (receivedDMRDataAndAudio[3] << 16) + (receivedDMRDataAndAudio[4] << 8) + (receivedDMRDataAndAudio[5] << 0);
	lc.FLCO = receivedDMRDataAndAudio[0];// Private or group call

	if ((lc.srcId == 0) || (lc.dstId == 0))
	{
		return;
	}

	// Encode the src and dst Ids etc
	if (!DMRFullLC_encode(&lc, frameData + MMDVM_HEADER_LENGTH, DT_VOICE_LC_HEADER)) // Encode the src and dst Ids etc
	{
		return;
	}

	rxedDMR_LC = lc;

	DMREmbeddedData_setLC(&lc);
	for (uint8_t i = 0U; i < 8U; i++)
	{
		frameData[i + 12U + MMDVM_HEADER_LENGTH] = (frameData[i + 12U + MMDVM_HEADER_LENGTH] & ~LC_SYNC_MASK_FULL[i]) | VOICE_LC_SYNC_FULL[i];
	}

	// Add RSSI into frame
	setRSSIToFrame(frameData);

	enqueueUSBData(frameData, frameData[1U]);
}

static void sendTerminator_LC_Frame(volatile const uint8_t *receivedDMRDataAndAudio)
{
	uint8_t frameData[DMR_FRAME_LENGTH_BYTES + MMDVM_HEADER_LENGTH + 2U] = {MMDVM_FRAME_START, (DMR_FRAME_LENGTH_BYTES + MMDVM_HEADER_LENGTH + 2U), MMDVM_DMR_DATA2, DMR_SYNC_DATA | DT_TERMINATOR_WITH_LC};
	DMRLC_T lc;

	memset(&lc, 0, sizeof(DMRLC_T));// clear automatic variable
	lc.srcId = (receivedDMRDataAndAudio[6] << 16) + (receivedDMRDataAndAudio[7] << 8) + (receivedDMRDataAndAudio[8] << 0);
	lc.dstId = (receivedDMRDataAndAudio[3] << 16) + (receivedDMRDataAndAudio[4] << 8) + (receivedDMRDataAndAudio[5] << 0);

	if ((lc.srcId == 0) || (lc.dstId == 0))
	{
		return;
	}

	// Encode the src and dst Ids etc
	if (!DMRFullLC_encode(&lc, frameData + MMDVM_HEADER_LENGTH, DT_TERMINATOR_WITH_LC))
	{
		return;
	}

	for (uint8_t i = 0U; i < 8U; i++)
	{
		frameData[i + 12U + MMDVM_HEADER_LENGTH] = (frameData[i + 12U + MMDVM_HEADER_LENGTH] & ~LC_SYNC_MASK_FULL[i]) | TERMINATOR_LC_SYNC_FULL[i];
	}

	// Add RSSI into frame
	setRSSIToFrame(frameData);

	enqueueUSBData(frameData, frameData[1U]);
}

void hotspotRxFrameHandler(uint8_t* frameBuf)
{
	taskENTER_CRITICAL();
	memcpy((uint8_t *)&audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufWriteIdx], frameBuf, 27 + 0x0c  + 2);// 27 audio + 0x0c header + 2 hotspot signalling bytes
	rfFrameBufCount++;
	rfFrameBufWriteIdx = ((rfFrameBufWriteIdx + 1) % HOTSPOT_BUFFER_COUNT);
	taskEXIT_CRITICAL();
}

static bool getEmbeddedData(volatile const uint8_t *com_requestbuffer)
{
	int             lcss;
	unsigned char   DMREMB[2U];
	// the following is used for fake TA
	static uint32_t oldTrxDMRID = 0;
	static uint8_t  rawDataCount = 0;
	static uint8_t  fakeTABlockID = FLCO_TALKER_ALIAS_HEADER;
	static bool     sendFakeTA = true;

	DMREMB[0U]  = (com_requestbuffer[MMDVM_HEADER_LENGTH + 13U] << 4) & 0xF0U;
	DMREMB[0U] |= (com_requestbuffer[MMDVM_HEADER_LENGTH + 14U] >> 4) & 0x0FU;
	DMREMB[1U]  = (com_requestbuffer[MMDVM_HEADER_LENGTH + 18U] << 4) & 0xF0U;
	DMREMB[1U] |= (com_requestbuffer[MMDVM_HEADER_LENGTH + 19U] >> 4) & 0x0FU;
	CQR1676_decode(DMREMB);

//	m_colorCode = (DMREMB[0U] >> 4) & 0x0FU;
//	m_PI        = (DMREMB[0U] & 0x08U) == 0x08U;

	lcss = (DMREMB[0U] >> 1) & 0x03U;

	if (startedEmbeddedSearch == false)
	{
		DMREmbeddedData_initEmbeddedDataBuffers();
		startedEmbeddedSearch = true;
	}

	if (DMREmbeddedData_addData((uint8_t *)com_requestbuffer + MMDVM_HEADER_LENGTH, lcss))
	{
		//DMRLC_T lc;

		bool res = DMREmbeddedData_getRawData(hotspotTxLC);

		if (res)
		{
			if (overriddenLCAvailable) // We can send fake talker aliases.
			{
				if (trxDMRID != oldTrxDMRID)
				{
					// reset counters
					rawDataCount = 0;
					sendFakeTA = true;
					fakeTABlockID = FLCO_TALKER_ALIAS_HEADER;
					oldTrxDMRID = trxDMRID;
				}

				if (sendFakeTA)
				{
					if ((hotspotTxLC[0] >= FLCO_TALKER_ALIAS_HEADER) && (hotspotTxLC[0] <= FLCO_TALKER_ALIAS_BLOCK3))
					{
						sendFakeTA = false;
						rawDataCount = 0;
						fakeTABlockID = FLCO_TALKER_ALIAS_HEADER;
						overriddenLCAvailable = false;
					}
					else
					{
						rawDataCount++;

						if (rawDataCount > 4)
						{
							hotspotTxLC[0] = fakeTABlockID;

							swapWithFakeTA(&hotspotTxLC[0]);

							// Update LH with fake TA
							lastHeardListUpdate(hotspotTxLC, true);

							fakeTABlockID++;

							// We just send 2 fake blocks, as it only contains callsign
							if (fakeTABlockID == FLCO_TALKER_ALIAS_BLOCK2)
							{
								rawDataCount = 0;
								sendFakeTA = false;
								fakeTABlockID = FLCO_TALKER_ALIAS_HEADER;
								oldTrxDMRID = trxDMRID;
								overriddenLCAvailable = false;
							}
						}
					}
				}
			}
			else
			{
				lastHeardListUpdate(hotspotTxLC, true);
			}
		}

		/*
		int flco = DMREmbeddedData_getFLCO();
		switch (flco)
		{
			case FLCO_GROUP:
				DMREmbeddedData_getLC(&lc);
				//SEGGER_RTT_printf(0, "Emb Group  FID:%d FLCO:%d PF:%d R:%d dstId:%d src:Id:%d options:0x%02x\n",lc.FID,lc.FLCO,lc.PF,lc.R,lc.dstId,lc.srcId,lc.options);
				//displayDataBytes(hotspotTxLC,9);
				break;
			case FLCO_USER_USER:
				DMREmbeddedData_getLC(&lc);
				//SEGGER_RTT_printf(0, "Emb User  FID:%d FLCO:%d PF:%d R:%d dstId:%d src:Id:%d options:0x%02x\n",lc.FID,lc.FLCO,lc.PF,lc.R,lc.dstId,lc.srcId,lc.options);
				//displayDataBytes(hotspotTxLC,9);
				break;

			case FLCO_GPS_INFO:
				//SEGGER_RTT_printf(0, "Emb GPS\n");
				//displayDataBytes(hotspotTxLC,9);
				break;

			case FLCO_TALKER_ALIAS_HEADER:
				//SEGGER_RTT_printf(0, "Emb FLCO_TALKER_ALIAS_HEADER\n");
				//displayDataBytes(hotspotTxLC,9);
				break;

			case FLCO_TALKER_ALIAS_BLOCK1:
				//SEGGER_RTT_printf(0, "Emb FLCO_TALKER_ALIAS_BLOCK1\n");
				//displayDataBytes(hotspotTxLC,9);
				break;

			case FLCO_TALKER_ALIAS_BLOCK2:
				//SEGGER_RTT_printf(0, "Emb FLCO_TALKER_ALIAS_BLOCK2\n");
				//displayDataBytes(hotspotTxLC,9);
				break;

			case FLCO_TALKER_ALIAS_BLOCK3:
				//SEGGER_RTT_printf(0, "Emb FLCO_TALKER_ALIAS_BLOCK3\n");
				//displayDataBytes(hotspotTxLC,9);
				break;

			default:
				//SEGGER_RTT_printf(0, "Emb UNKNOWN TYPE\n");
				break;
		}
		*/
		startedEmbeddedSearch = false;
	}

	return false;
}

static void storeNetFrame(volatile const uint8_t *com_requestbuffer)
{
	bool foundEmbedded;

	if (memcmp((uint8_t *)&com_requestbuffer[18], END_FRAME_PATTERN, 6) == 0)
	{
		return;
	}

	if (memcmp((uint8_t *)&com_requestbuffer[18], START_FRAME_PATTERN, 6) == 0)
	{
		return;
	}

	foundEmbedded = getEmbeddedData(com_requestbuffer);

	if (	(foundEmbedded || (nonVolatileSettings.hotspotType == HOTSPOT_TYPE_BLUEDV))
			&&
			(hotspotTxLC[0] == TG_CALL_FLAG || hotspotTxLC[0] == PC_CALL_FLAG) &&
			(hotspotState != HOTSPOT_STATE_TX_START_BUFFERING && hotspotState != HOTSPOT_STATE_TRANSMITTING))
	{
		timeoutCounter = TX_BUFFERING_TIMEOUT;// set buffering timeout
		hotspotState = HOTSPOT_STATE_TX_START_BUFFERING;
	}

	if (hotspotState == HOTSPOT_STATE_TRANSMITTING ||
		hotspotState == HOTSPOT_STATE_TX_SHUTDOWN  ||
		hotspotState == HOTSPOT_STATE_TX_START_BUFFERING)
	{
		if (wavbuffer_count >= HOTSPOT_BUFFER_COUNT)
		{
			// Buffer overflow
		}

		taskENTER_CRITICAL();
		memcpy((uint8_t *)&audioAndHotspotDataBuffer.hotspotBuffer[wavbuffer_write_idx][0x0C], (uint8_t *)com_requestbuffer + 4, 13);//copy the first 13, whole bytes of audio
		audioAndHotspotDataBuffer.hotspotBuffer[wavbuffer_write_idx][0x0C + 13] = (com_requestbuffer[17] & 0xF0) | (com_requestbuffer[23] & 0x0F);
		memcpy((uint8_t *)&audioAndHotspotDataBuffer.hotspotBuffer[wavbuffer_write_idx][0x0C + 14], (uint8_t *)&com_requestbuffer[24], 13);//copy the last 13, whole bytes of audio

		memcpy((uint8_t *)&audioAndHotspotDataBuffer.hotspotBuffer[wavbuffer_write_idx], hotspotTxLC, 9);// copy the current LC into the data (mainly for use with the embedded data);
		wavbuffer_count++;
		wavbuffer_write_idx = ((wavbuffer_write_idx + 1) % HOTSPOT_BUFFER_COUNT);
		taskEXIT_CRITICAL();
	}

}

static uint8_t hotspotModeReceiveNetFrame(volatile const uint8_t *com_requestbuffer, uint8_t timeSlot)
{
	DMRLC_T lc;

	if (!mmdvmHostIsConnected)
	{
		hotspotState = HOTSPOT_STATE_INITIALISE;
		mmdvmHostIsConnected = true;
		updateScreen(HOTSPOT_RX_IDLE);
	}

	lc.srcId = 0;// zero these values as they are checked later in the function, but only updated if the data type is DT_VOICE_LC_HEADER
	lc.dstId = 0;

	DMRFullLC_decode((uint8_t *)com_requestbuffer + MMDVM_HEADER_LENGTH, DT_VOICE_LC_HEADER, &lc);// Need to decode the frame to get the source and destination
	/*	DMRSlotType_decode(com_requestbuffer + MMDVM_HEADER_LENGTH,&colorCode,&dataType);
	//SEGGER_RTT_printf(0, "SlotType:$d %d\n",dataType,colorCode);
	switch(dataType)
	{

		case DT_VOICE_LC_HEADER:
			//SEGGER_RTT_printf(0, "DT_VOICE_LC_HEADER\n");
			break;
		case DT_TERMINATOR_WITH_LC:
			//SEGGER_RTT_printf(0, "DT_TERMINATOR_WITH_LC\n");
			//trxTalkGroupOrPcId  = 0;
			//trxDMRID = 0;
			break;

		case DT_VOICE_PI_HEADER:
			//SEGGER_RTT_printf(0, "DT_VOICE_PI_HEADER\n");
			break;
		case DT_CSBK:
			//SEGGER_RTT_printf(0, "DT_CSBK\n");
			break;
		case DT_DATA_HEADER:
			//SEGGER_RTT_printf(0, "DT_DATA_HEADER\n",);
			break;
		case DT_RATE_12_DATA:
			//SEGGER_RTT_printf(0, "DT_RATE_12_DATA\n");
			break;
		case DT_RATE_34_DATA:
			//SEGGER_RTT_printf(0, "DT_RATE_34_DATA\n",);
			break;
		case DT_IDLE:
			//SEGGER_RTT_printf(0, "DT_IDLE\n");
			break;
		case DT_RATE_1_DATA:
			//SEGGER_RTT_printf(0, "DT_RATE_1_DATA\n");
			break;
		default:
			//SEGGER_RTT_printf(0, "Frame dataType %d\n",dataType);
			break;
	}*/

	// update the src and destination ID's if valid
	if 	(lc.srcId != 0 && lc.dstId != 0)
	{
		trxTalkGroupOrPcId  = lc.dstId | (lc.FLCO << 24);
		trxDMRID = lc.srcId;

		if (hotspotState != HOTSPOT_STATE_TX_START_BUFFERING)
		{
			memcpy(hotspotTxLC, lc.rawData, 9);//Hotspot uses LC Data bytes rather than the src and dst ID's for the embed data

			lastHeardListUpdate(hotspotTxLC, true);

			// the Src and Dst Id's have been sent, and we are in RX mode then an incoming Net normally arrives next
			timeoutCounter = TX_BUFFERING_TIMEOUT;
			hotspotState = HOTSPOT_STATE_TX_START_BUFFERING;
		}
	}
	else
	{
		storeNetFrame(com_requestbuffer);
	}

	return 0U;
}

#if defined(MMDVM_SEND_DEBUG)
static void sendDebug1(const char *text)
{
	uint8_t buf[130U];

	buf[0U] = MMDVM_FRAME_START;
	buf[1U] = 0U;
	buf[2U] = MMDVM_DEBUG1;

	uint8_t count = 3U;
	for (uint8_t i = 0U; text[i] != '\0'; i++, count++)
		buf[count] = text[i];

	buf[1U] = count;

	enqueueUSBData(buf, buf[1U]);
}

static void sendDebug2(const char *text, int16_t n1)
{
	uint8_t buf[130U];

	buf[0U] = MMDVM_FRAME_START;
	buf[1U] = 0U;
	buf[2U] = MMDVM_DEBUG2;

	uint8_t count = 3U;
	for (uint8_t i = 0U; text[i] != '\0'; i++, count++)
		buf[count] = text[i];

	buf[count++] = (n1 >> 8) & 0xFF;
	buf[count++] = (n1 >> 0) & 0xFF;

	buf[1U] = count;

	enqueueUSBData(buf, buf[1U]);
}

static void sendDebug3(const char *text, int16_t n1, int16_t n2)
{
	uint8_t buf[130U];

	buf[0U] = MMDVM_FRAME_START;
	buf[1U] = 0U;
	buf[2U] = MMDVM_DEBUG3;

	uint8_t count = 3U;
	for (uint8_t i = 0U; text[i] != '\0'; i++, count++)
		buf[count] = text[i];

	buf[count++] = (n1 >> 8) & 0xFF;
	buf[count++] = (n1 >> 0) & 0xFF;

	buf[count++] = (n2 >> 8) & 0xFF;
	buf[count++] = (n2 >> 0) & 0xFF;

	buf[1U] = count;

	enqueueUSBData(buf, buf[1U]);
}

static void sendDebug4(const char *text, int16_t n1, int16_t n2, int16_t n3)
{
	uint8_t buf[130U];

	buf[0U] = MMDVM_FRAME_START;
	buf[1U] = 0U;
	buf[2U] = MMDVM_DEBUG4;

	uint8_t count = 3U;
	for (uint8_t i = 0U; text[i] != '\0'; i++, count++)
		buf[count] = text[i];

	buf[count++] = (n1 >> 8) & 0xFF;
	buf[count++] = (n1 >> 0) & 0xFF;

	buf[count++] = (n2 >> 8) & 0xFF;
	buf[count++] = (n2 >> 0) & 0xFF;

	buf[count++] = (n3 >> 8) & 0xFF;
	buf[count++] = (n3 >> 0) & 0xFF;

	buf[1U] = count;

	enqueueUSBData(buf, buf[1U]);
}

static void sendDebug5(const char *text, int16_t n1, int16_t n2, int16_t n3, int16_t n4)
{
	uint8_t buf[130U];

	buf[0U] = MMDVM_FRAME_START;
	buf[1U] = 0U;
	buf[2U] = MMDVM_DEBUG5;

	uint8_t count = 3U;
	for (uint8_t i = 0U; text[i] != '\0'; i++, count++)
		buf[count] = text[i];

	buf[count++] = (n1 >> 8) & 0xFF;
	buf[count++] = (n1 >> 0) & 0xFF;

	buf[count++] = (n2 >> 8) & 0xFF;
	buf[count++] = (n2 >> 0) & 0xFF;

	buf[count++] = (n3 >> 8) & 0xFF;
	buf[count++] = (n3 >> 0) & 0xFF;

	buf[count++] = (n4 >> 8) & 0xFF;
	buf[count++] = (n4 >> 0) & 0xFF;

	buf[1U] = count;

	enqueueUSBData(buf, buf[1U]);
}
#endif

static void sendDMRLost(void)
{
	uint8_t buf[3U];

	buf[0U] = MMDVM_FRAME_START;
	buf[1U] = 3U;
	buf[2U] = MMDVM_DMR_LOST2;

	enqueueUSBData(buf, buf[1U]);
}

static void sendACK(void)
{
	uint8_t buf[4];

	buf[0U] = MMDVM_FRAME_START;
	buf[1U] = 4U;
	buf[2U] = MMDVM_ACK;
	buf[3U] = com_requestbuffer[2U];

	enqueueUSBData(buf, buf[1U]);
}

static void sendNAK(uint8_t err)
{
	uint8_t buf[5];

	buf[0U] = MMDVM_FRAME_START;
	buf[1U] = 5U;
	buf[2U] = MMDVM_NAK;
	buf[3U] = com_requestbuffer[2U];
	buf[4U] = err;

	enqueueUSBData(buf, buf[1U]);
}

static void hotspotStateMachine(void)
{
	static uint32_t rxFrameTime = 0;

	switch(hotspotState)
	{
		case HOTSPOT_STATE_NOT_CONNECTED:
			// do nothing
			lastRxState = HOTSPOT_RX_UNKNOWN;

			// force immediate shutdown of Tx if we get here and the tx is on for some reason.
			if (trxTransmissionEnabled)
			{
				trxTransmissionEnabled = false;
				trxDisableTransmission();
			}

			rfFrameBufCount = 0;
			if (mmdvmHostIsConnected)
			{
				hotspotState = HOTSPOT_STATE_INITIALISE;
			}
			else
			{
				if ((nonVolatileSettings.hotspotType == HOTSPOT_TYPE_MMDVM) &&
						((fw_millis() - mmdvmHostLastActiveTime) > MMDVMHOST_TIMEOUT))
				{
					wavbuffer_count = 0;

					hotspotExit();
					break;
				}
			}
			break;

		case HOTSPOT_STATE_INITIALISE:
			wavbuffer_read_idx = 0;
			wavbuffer_write_idx = 0;
			wavbuffer_count = 0;
			rfFrameBufCount = 0;

			overriddenLCAvailable = false;

			hotspotState = HOTSPOT_STATE_RX_START;
			break;

		case HOTSPOT_STATE_RX_START:
			// force immediate shutdown of Tx if we get here and the tx is on for some reason.
			if (trxTransmissionEnabled)
			{
				trxTransmissionEnabled = false;
				trxDisableTransmission();
			}

			wavbuffer_read_idx = 0;
			wavbuffer_write_idx = 0;
			wavbuffer_count = 0;
			rxFrameTime = fw_millis();

			hotspotState = HOTSPOT_STATE_RX_PROCESS;
			break;

		case HOTSPOT_STATE_RX_PROCESS:
			if (mmdvmHostIsConnected)
			{
				// No activity from MMDVMHost
				if ((nonVolatileSettings.hotspotType == HOTSPOT_TYPE_MMDVM) &&
						((fw_millis() - mmdvmHostLastActiveTime) > MMDVMHOST_TIMEOUT))
				{
					mmdvmHostIsConnected = false;
					hotspotState = HOTSPOT_STATE_NOT_CONNECTED;
					rfFrameBufCount = 0;
					wavbuffer_count = 0;

					hotspotExit();
					break;
				}
			}
			else
			{
				hotspotState = HOTSPOT_STATE_NOT_CONNECTED;
				rfFrameBufCount = 0;
				wavbuffer_count = 0;

				if (trxTransmissionEnabled)
				{
					trxTransmissionEnabled = false;
					trxDisableTransmission();
				}

				updateScreen(HOTSPOT_RX_IDLE);
				break;
			}

			if (rfFrameBufCount > 0)
			{
				// We have pending data in RF side, but don't process it when MMDVMHost
				// set the hotspot in POCSAG mode. Just trash it.
				if (modemState == STATE_POCSAG)
				{
					memset((void *)&audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx], 0, HOTSPOT_BUFFER_SIZE);
				}

				if (MMDVMHostRxState == MMDVMHOST_RX_READY)
				{
					uint8_t rx_command = audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx][27 + 0x0c];

					switch(rx_command)
					{
						case HOTSPOT_RX_IDLE:
							break;

						case HOTSPOT_RX_START:
							sendVoiceHeaderLC_Frame(audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx]);
							updateScreen(rx_command);
							lastRxState = HOTSPOT_RX_START;
							rxFrameTime = fw_millis();
							break;

						case HOTSPOT_RX_START_LATE:
							sendVoiceHeaderLC_Frame(audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx]);
							updateScreen(rx_command);
							lastRxState = HOTSPOT_RX_START_LATE;
							rxFrameTime = fw_millis();
							break;

						case HOTSPOT_RX_AUDIO_FRAME:
							hotspotSendVoiceFrame(audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx]);
							lastRxState = HOTSPOT_RX_AUDIO_FRAME;
							rxFrameTime = fw_millis();
							break;

						case HOTSPOT_RX_STOP:
							updateScreen(rx_command);
							sendTerminator_LC_Frame(audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx]);
							lastRxState = HOTSPOT_RX_STOP;
							hotspotState = HOTSPOT_STATE_RX_END;
							break;

						case HOTSPOT_RX_IDLE_OR_REPEAT:
							lastRxState = HOTSPOT_RX_IDLE_OR_REPEAT;
							/*
										switch(lastRxState)
										{
											case HOTSPOT_RX_START:
												sendVoiceHeaderLC_Frame(audioAndHotspotDataBuffer.hotspotBuffer[wavbuffer_read_idx]);
												break;
											case HOTSPOT_RX_STOP:
												sendTerminator_LC_Frame(audioAndHotspotDataBuffer.hotspotBuffer[wavbuffer_read_idx]);
												break;
											default:
												//SEGGER_RTT_printf(0, "ERROR: Unkown HOTSPOT_RX_IDLE_OR_REPEAT\n");
												break;
										}
							 */
							break;

						default:
							lastRxState = HOTSPOT_RX_UNKNOWN;
							break;
					}

					memset((void *)&audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx], 0, HOTSPOT_BUFFER_SIZE);
					rfFrameBufReadIdx = ((rfFrameBufReadIdx + 1) % HOTSPOT_BUFFER_COUNT);

					if (rfFrameBufCount > 0)
					{
						rfFrameBufCount--;
					}
				}
				else
				{
					// Rx Error NAK
					hotspotState = HOTSPOT_STATE_RX_END;
				}
			}
			else
			{
				// Timeout: no RF data for too long
				if (((lastRxState == HOTSPOT_RX_AUDIO_FRAME) || (lastRxState == HOTSPOT_RX_START)) &&
						((fw_millis() - rxFrameTime) > 300)) // 300ms
				{
					sendDMRLost();
					updateScreen(HOTSPOT_RX_IDLE);
					lastRxState = HOTSPOT_RX_STOP;
					hotspotState = HOTSPOT_STATE_RX_END;
					rfFrameBufCount = 0;
					//rfFrameBufReadIdx = 0;
					//wavbuffer_count = 0;
					return;
				}
			}
			break;

		case HOTSPOT_STATE_RX_END:
			hotspotState = HOTSPOT_STATE_RX_START;
			break;

		case HOTSPOT_STATE_TX_START_BUFFERING:
			// If MMDVMHost tells us to go back to idle. (receiving)
			if (modemState == STATE_IDLE)
			{
				//modemState = STATE_DMR;
				//wavbuffer_read_idx = 0;
				//wavbuffer_write_idx = 0;
				//wavbuffer_count = 0;
				rfFrameBufCount = 0;
				lastRxState = HOTSPOT_RX_IDLE;
				hotspotState = HOTSPOT_STATE_TX_SHUTDOWN;
				mmdvmHostIsConnected = false;
				trxTransmissionEnabled = false;
				trxDisableTransmission();
				updateScreen(HOTSPOT_RX_IDLE);

			}
			else
			{
				if (wavbuffer_count > TX_BUFFER_MIN_BEFORE_TRANSMISSION)
				{
					if (cwKeying == false)
					{
						hotspotState = HOTSPOT_STATE_TRANSMITTING;
						trxEnableTransmission();
						updateScreen(HOTSPOT_RX_IDLE);
					}
				}
				else
				{
					if (--timeoutCounter == 0)
					{
						sendDMRLost();
						hotspotState = HOTSPOT_STATE_INITIALISE;
					}
				}
			}
			break;

		case HOTSPOT_STATE_TRANSMITTING:
			// Stop transmitting when there is no data in the buffer or if MMDVMHost sends the idle command
			if (wavbuffer_count == 0 || modemState == STATE_IDLE)
			{
				hotspotState = HOTSPOT_STATE_TX_SHUTDOWN;
				//trxTransmissionEnabled = false;
			}
			break;

		case HOTSPOT_STATE_TX_SHUTDOWN:
			overriddenLCAvailable = false;
			if (txstopdelay > 0)
			{
				txstopdelay--;
				if (wavbuffer_count > 0)
				{
					// restart
					trxEnableTransmission();
					timeoutCounter = TX_BUFFERING_TIMEOUT;
					hotspotState = HOTSPOT_STATE_TX_START_BUFFERING;
				}
			}
			else
			{
				if ((trxIsTransmitting) ||
						((modemState == STATE_IDLE) && trxTransmissionEnabled)) // MMDVMHost asked to go back to IDLE (mostly on shutdown)
				{
					trxTransmissionEnabled = false;
					trxDisableTransmission();
					hotspotState = HOTSPOT_STATE_RX_START;
					updateScreen(HOTSPOT_RX_IDLE);

					/*
						wavbuffer_read_idx=0;
						wavbuffer_write_idx=0;
						wavbuffer_count=0;
					 */
				}
			}
			break;
	}
}

static uint8_t setFreq(volatile const uint8_t* data, uint8_t length)
{
	if (length < 9U)
	{
		return 4U;
	}

	// satellite frequencies banned frequency ranges
	const int BAN1_MIN  = 14580000;
	const int BAN1_MAX  = 14600000;
	const int BAN2_MIN  = 43500000;
	const int BAN2_MAX  = 43800000;
	uint32_t fRx, fTx;

	hotspotState = HOTSPOT_STATE_INITIALISE;

	if (!mmdvmHostIsConnected)
	{
		mmdvmHostIsConnected = true;
		updateScreen(HOTSPOT_RX_IDLE);
	}

	// Very old MMDVMHost, set full power
	if (length == 9U)
	{
		rf_power = 255U;
	}
	// Current MMDVMHost, set power from MMDVM.ini
	if (length >= 10U)
	{
		rf_power = data[9U];// 255 = max power
	}

	fRx = (data[1U] << 0 | data[2U] << 8  | data[3U] << 16 | data[4U] << 24) / 10;
	fTx = (data[5U] << 0 | data[6U] << 8  | data[7U] << 16 | data[8U] << 24) / 10;

	if ((fTx >= BAN1_MIN && fTx <= BAN1_MAX) || (fTx >= BAN2_MIN && fTx <= BAN2_MAX))
	{
		return 4U;// invalid frequency
	}

	if (trxCheckFrequencyInAmateurBand(fRx) && trxCheckFrequencyInAmateurBand(fTx))
	{
		freq_rx = fRx;
		freq_tx = fTx;
		trxSetFrequency(freq_rx, freq_tx, DMR_MODE_DMO);// Override the default assumptions about DMR mode based on frequency
	}
	else
	{
		return 4U;// invalid frequency
	}

	hotspotPowerLevel = nonVolatileSettings.txPowerLevel;
	// If the power level sent by MMDVMHost is 255 it means the user has left the setting at 100% and potentially does not realise that there is even a setting for this
	// As the GD-77 can't be run at full power, this power level will be ignored and instead the level specified for normal operation will be used.
	if (rf_power != 255)
	{
		savedPowerLevel = nonVolatileSettings.txPowerLevel;

		if (rf_power < 50)
		{
			hotspotPowerLevel = rf_power / 12;
		}
		else
		{
			hotspotPowerLevel = (rf_power / 50) + 3;
		}
		trxSetPowerFromLevel(hotspotPowerLevel);
	}

  return 0U;
}

static bool hasRXOverflow(void)
{
	return ((HOTSPOT_BUFFER_SIZE - rfFrameBufCount) <= 0);
}

static bool hasTXOverflow(void)
{
	return ((HOTSPOT_BUFFER_COUNT - wavbuffer_count) <= 0);
}

static void getStatus(void)
{
	uint8_t buf[16];

	// Send all sorts of interesting internal values
	buf[0U]  = MMDVM_FRAME_START;
	buf[1U]  = 13U;
	buf[2U]  = MMDVM_GET_STATUS;
	buf[3U]  = (0x02U | 0x20U); // DMR and POCSAG enabled
	buf[4U]  = modemState;
	buf[5U]  = ( ((hotspotState == HOTSPOT_STATE_TX_START_BUFFERING) ||
					(hotspotState == HOTSPOT_STATE_TRANSMITTING) ||
					(hotspotState == HOTSPOT_STATE_TX_SHUTDOWN)) ||
					cwKeying ) ? 0x01U : 0x00U;

	if (hasRXOverflow())
	{
		buf[5U] |= 0x04U;
	}

	if (hasTXOverflow())
	{
		buf[5U] |= 0x08U;
	}

	buf[6U]  = 0U; // No DSTAR space

	buf[7U]  = 10U; // DMR Simplex
	buf[8U]  = (HOTSPOT_BUFFER_COUNT - wavbuffer_count); // DMR space

	buf[9U]  = 0U; // No YSF space
	buf[10U] = 0U; // No P25 space
	buf[11U] = 0U; // no NXDN space
	buf[12U] = 1U; // virtual space for POCSAG

	if (!mmdvmHostIsConnected)
	{
		hotspotState = HOTSPOT_STATE_INITIALISE;
		mmdvmHostIsConnected = true;
		updateScreen(HOTSPOT_RX_IDLE);
	}

	enqueueUSBData(buf, buf[1U]);
}

static uint8_t setConfig(volatile const uint8_t *data, uint8_t length)
{
	if (length < 13U)
	{
		return 4U;
	}

	uint32_t tempTXDelay = (data[2U] * 100); // MMDVMHost send in 10ms units, we use 0.1ms PIT counter unit

	if (tempTXDelay > 10000) // 1s limit
	{
		return 4U;
	}

	tx_delay = tempTXDelay;

	// Only supported mode are DMR, CWID, POCSAG and IDLE
	switch (data[3U])
	{
		case STATE_IDLE:
		case STATE_DMR:
		case STATE_CWID:
		case STATE_POCSAG:
			break;

		default:
			return 4U;
			break;
	}

	modemState = (MMDVM_STATE)data[3U];

	uint8_t tmpColorCode = data[6U];

	if (tmpColorCode > 15U)
	{
		return 4U;
	}

	colorCode = tmpColorCode;
	trxSetDMRColourCode(colorCode);

	/* To Do
	 m_cwIdTXLevel = data[5U]>>2;
     uint8_t dmrTXLevel    = data[10U];
     io.setDeviations(dstarTXLevel, dmrTXLevel, ysfTXLevel, p25TXLevel, nxdnTXLevel, pocsagTXLevel, ysfLoDev);
     dmrDMOTX.setTXDelay(txDelay);
     */

	if ((modemState == STATE_DMR) && (hotspotState == HOTSPOT_STATE_NOT_CONNECTED))
	{
		hotspotState = HOTSPOT_STATE_INITIALISE;

		if (!mmdvmHostIsConnected)
		{
			mmdvmHostIsConnected = true;
			updateScreen(HOTSPOT_RX_IDLE);
		}
	}

	return 0U;
}

static uint8_t setMode(volatile const uint8_t* data, uint8_t length)
{
	if (length < 1U)
	{
		return 4U;
	}

	if (modemState == data[0U])
	{
		return 0U;
	}

	// Only supported mode are DMR, CWID, POCSAG and IDLE
	switch (data[0U])
	{
		case STATE_IDLE:
		case STATE_DMR:
		case STATE_CWID:
		case STATE_POCSAG:
			break;

		default:
			return 4U;
			break;
	}

	modemState = data[0U];
#if 0
	// MMDVMHost seems to send setMode commands longer than 1 byte. This seems wrong according to the spec, so we ignore those.
	if (data[0U] == STATE_IDLE || (length==1 && data[0U] == STATE_DMR) || data[0U] != STATE_POCSAG)
	{
		modemState = data[0U];
	}
#endif

#if 0
	// MMDVHost on the PC seems to send mode DMR when the transmitter should be turned on and IDLE when it should be turned off.
	switch(modemState)
	{
		case STATE_IDLE:
			//enableTransmission(false);
			break;
		case STATE_DMR:
			//enableTransmission(true);
			break;
		default:
			break;
	}
#endif

	if (!mmdvmHostIsConnected)
	{
		hotspotState = HOTSPOT_STATE_INITIALISE;
		mmdvmHostIsConnected = true;
		updateScreen(HOTSPOT_RX_IDLE);
	}

	return 0U;
}

static void getVersion(void)
{
	char    buffer[80];
	uint8_t buf[128];
	uint8_t count = 0U;

	buf[0U]  = MMDVM_FRAME_START;
	buf[1U]  = 0U;
	buf[2U]  = MMDVM_GET_VERSION;
	buf[3U]  = PROTOCOL_VERSION;

	count = 4U;

	snprintf(buffer, sizeof(buffer), "%s (Radio:%s, Mode:%s)", HARDWARE,
#if defined(PLATFORM_GD77)
			"GD-77"
#elif defined(PLATFORM_GD77S)
			"GD-77S"
#elif defined(PLATFORM_DM1801)
			"DM-1801"
#elif defined(PLATFORM_RD5R)
			"RD-5R"
#else
			"Unknown"
#endif
			,(nonVolatileSettings.hotspotType == HOTSPOT_TYPE_MMDVM ? "MMDVM" : "BlueDV"));

	for (uint8_t i = 0U; buffer[i] != 0x00U; i++, count++)
	{
		buf[count] = buffer[i];
	}

	buf[1U] = count;

	if (!mmdvmHostIsConnected)
	{
		hotspotState = HOTSPOT_STATE_INITIALISE;
		mmdvmHostIsConnected = true;
		updateScreen(HOTSPOT_RX_IDLE);
	}

	enqueueUSBData(buf, buf[1U]);
}

static uint8_t handleDMRShortLC(volatile const uint8_t *data, uint8_t length)
{
	////	uint8_t LCBuf[5];
	////	DMRShortLC_decode((uint8_t *) com_requestbuffer + 3U,LCBuf);

	if (!mmdvmHostIsConnected)
	{
		hotspotState = HOTSPOT_STATE_INITIALISE;
		mmdvmHostIsConnected = true;
		updateScreen(HOTSPOT_RX_IDLE);
	}

	return 0U;
}

static uint8_t setQSOInfo(volatile const uint8_t *data, uint8_t length)
{
	if (length < (MMDVM_HEADER_LENGTH + 1U))
	{
		return 4U;
	}

	if (data[3U] == 250U) // IP info from MMDVMHost's CAST display driver
	{
		char buf[26U]; // MMDVMHost use an array of 25U
		char *pBuf = buf;
		char *pIface;

		memcpy(&buf, (char *)(data + MMDVM_HEADER_LENGTH), (length - MMDVM_HEADER_LENGTH));
		buf[(length - MMDVM_HEADER_LENGTH)] = 0;

		// Get rid of the interface name as it could be too large to fit in the screen.
		if ((pIface = strchr(pBuf, ':')) != NULL)
		{
			pBuf = pIface + 1;
		}

		snprintf(mmdvmQSOInfoIP, 22, "%s", pBuf);

		updateScreen(currentRxCommandState);

		return 0U;
	}
	else if ((data[3U] != STATE_DMR) && (data[3U] != STATE_POCSAG)) // We just want DMR and POCSAG QSO info
	{
		return 4U;
	}

	if (data[3U] == STATE_DMR)
	{
		if (length != 47U) // Check for QSO Info frame length
		{
			return 5U;
		}

		// Source and destination are both fitted in 20U arrays, in MMDVMHost
		// We just store use and store source info
		char QSOInfo[21U]; // QSO infos are array[20]
		char source[21U];
		char *p;
		uint8_t len;

		memcpy(&source, (char *)(data + 5U), 20U);
		source[20U] = 0;

		memset(&QSOInfo, 0, sizeof(QSOInfo));
		sprintf(QSOInfo, "%s", chomp(source));

		len = strlen(QSOInfo);

		// Keep the callsign only.
		if ((p = strchr(QSOInfo, ' ')) != NULL)
		{
			*p = 0;

			// zeroing
			p++;
			for (uint8_t i = 0; i < (len - (p - QSOInfo)); i++)
			{
				*(p + i) = 0;
			}

			len = strlen(QSOInfo);
		}

		// Non empty string, check if it's not numerical (TG/PC), as it will be ignored
		if (len > 0)
		{
			bool onlyDigits = true;

			for (uint8_t i = 0; i < len; i++)
			{
				if (isalpha(QSOInfo[i]))
				{
					onlyDigits = false;
					break;
				}
			}

			if (onlyDigits)
			{
				overriddenLCAvailable = false;
				return 0U;
			}

			// Build fake TA (2 blocks max)
			memset(&overriddenLCTA, 0, sizeof(overriddenLCTA));

			overriddenLCTA[0]  = 0x04;
			overriddenLCTA[2]  = (0x01 << 6) | (len << 1);
			overriddenLCTA[9]  = 0x05;

			char *p = QSOInfo;
			for (uint8_t i = 0; i < 2; i++) // 2 blocks only, that enough for store a callsign
			{
				memcpy(&overriddenLCTA[(i * 9) + ((i == 0) ? 3 : 2)], p, ((i == 0) ? 6 : 7));

				p += ((i == 0) ? 6 : 7);
			}

			overriddenBlocksTA = 0x03; // 2 blocks are now available
		}

		overriddenLCAvailable = (len > 0);
	}
	else if (data[3U] == STATE_POCSAG)
	{
		if ((length - 11U) > (17U + 4U))
		{
			for (int8_t i = 0; i < (((length - 11U) / (17U + 4)) - 1); i++)
			{
				sendACK();
			}
		}
	}

	return 0U;
}

static void cwProcess(void)
{
	if (cwpoLen == 0U)
	{
		return;
	}

	if (PITCounter > cwNextPeriod)
	{
		cwNextPeriod = PITCounter + cwDOTDuration;

		bool b = READ_BIT1(cwBuffer, cwpoPtr);
		static bool lastValue = true;

		if (lastValue != b)
		{
			lastValue = b;

			if (b)
			{
				trxSetTone1(880);
			}
			else
			{
				trxSetTone1(0);
			}
		}

		cwpoPtr++;

		if (cwpoPtr >= cwpoLen)
		{
			cwpoPtr = 0U;
			cwpoLen = 0U;
			return;
		}
	}
}

static void cwReset(void)
{
	cwpoLen = 0U;
	cwpoPtr = 0U;
}

static uint8_t handleCWID(volatile const uint8_t *data, uint8_t length)
{
	cwReset();
	memset(cwBuffer, 0x00U, sizeof(cwBuffer));

	cwpoLen = 6U; // Silence at the beginning
	cwpoPtr = 0U;

	for (uint8_t i = 0U; i < length; i++)
	{
		for (uint8_t j = 0U; CW_SYMBOL_LIST[j].c != 0U; j++)
		{
			if (CW_SYMBOL_LIST[j].c == data[i])
			{
				uint32_t MASK = 0x80000000U;

				for (uint8_t k = 0U; k < CW_SYMBOL_LIST[j].length; k++, cwpoLen++, MASK >>= 1)
				{
					bool b = (CW_SYMBOL_LIST[j].pattern & MASK) == MASK;

					WRITE_BIT1(cwBuffer, cwpoLen, b);

					if (cwpoLen >= ((sizeof(cwBuffer) * 8) - 3U)) // Will overflow otherwise
					{
						cwpoLen = 0U;
						return 4U;
					}
				}

				break;
			}
		}
	}

	// An empty message
	if (cwpoLen == 6U)
	{
		cwpoLen = 0U;
		return 4U;
	}

	// Silence at the end
	cwpoLen += 3U;

	return 0U;
}

#if 0
static uint8_t handlePOCSAG(volatile const uint8_t *data, uint8_t length)
{
	return 0U;
}
#endif

static void handleHotspotRequest(void)
{
	if (com_requestbuffer[0] == MMDVM_FRAME_START)
	{
		uint8_t err = 2U;

		mmdvmHostLastActiveTime = fw_millis();

		switch(com_requestbuffer[2U])
		{
			case MMDVM_GET_STATUS:
				getStatus();
				break;

			case MMDVM_GET_VERSION:
				getVersion();
				break;

			case MMDVM_SET_CONFIG:
				err = setConfig(com_requestbuffer + 3U, com_requestbuffer[1U] - 3U);
				if (err == 0U)
				{
					sendACK();
					updateScreen(HOTSPOT_RX_IDLE);
				}
				else
				{
					sendNAK(err);
				}
				break;

			case MMDVM_SET_MODE:
				{
					MMDVM_STATE prevState = modemState;

					err = setMode(com_requestbuffer + 3U, com_requestbuffer[1U] - 3U);

					if (((prevState == STATE_POCSAG) && (modemState != STATE_POCSAG)) ||
							((prevState != STATE_POCSAG) && (modemState == STATE_POCSAG)))
					{
						updateScreen(HOTSPOT_RX_IDLE); // refresh screen
					}

					if (err == 0U)
					{
						sendACK();
					}
					else
					{
						sendNAK(err);
					}
				}
				break;

			case MMDVM_SET_FREQ:
				err = setFreq(com_requestbuffer + 3U, com_requestbuffer[1U] - 3U);
				if (err == 0U)
				{
					sendACK();
					updateScreen(HOTSPOT_RX_IDLE);
				}
				else
				{
					sendNAK(err);
				}
				break;

			case MMDVM_CAL_DATA:
				sendNAK(err);
				break;

			case MMDVM_SEND_CWID:
				err = 5U;
				if (modemState == STATE_IDLE)
				{
					err = handleCWID(com_requestbuffer + 3U, com_requestbuffer[1U] - 3U);
				}

				if (err == 0U)
				{
					cwKeying = true;
					cwNextPeriod = PITCounter + tx_delay;
					sendACK();
				}
				else
				{
					sendNAK(err);
				}
				break;

			case MMDVM_DSTAR_HEADER:
			case MMDVM_DSTAR_DATA:
			case MMDVM_DSTAR_EOT:
				sendNAK(err);
				break;

			case MMDVM_DMR_DATA1: // We are a simplex hotspot, no TS1 support
				//hotspotModeReceiveNetFrame((uint8_t *)com_requestbuffer, 1U);
				sendNAK(err);
				break;

			case MMDVM_DMR_DATA2:
				// It seems BlueDV under Windows forget to set correct mode
				// when it connect a TG with an already running QSO. So we force
				// the modemState and init the HS.
				if ((nonVolatileSettings.hotspotType == HOTSPOT_TYPE_BLUEDV) && (modemState == STATE_IDLE))
				{
					modemState = STATE_DMR;
				}

				err = hotspotModeReceiveNetFrame(com_requestbuffer, 2U);
				if (err == 0U)
				{
					sendACK();
				}
				else
				{
					sendNAK(err);
				}
				break;

			case MMDVM_DMR_START: // Only for duplex
				sendACK();
				break;

			case MMDVM_DMR_SHORTLC:
				err = handleDMRShortLC(com_requestbuffer, com_requestbuffer[1U]);
				if (err == 0U)
				{
					sendACK();
				}
				else
				{
					sendNAK(err);
				}
				break;

			case MMDVM_DMR_ABORT: // Only for duplex
				sendACK();
				break;

#if 0  // Serial passthrough (a.k.a Nextion serial port), unhandled
			case MMDVM_SERIAL:
				//sendACK();
				break;
#endif

			case MMDVM_YSF_DATA:
				sendNAK(err);
				break;

			case MMDVM_P25_HDR:
			case MMDVM_P25_LDU:
				sendNAK(err);
				break;

			case MMDVM_NXDN_DATA:
				sendNAK(err);
				break;

			case MMDVM_POCSAG_DATA:
				if ((modemState == STATE_IDLE) || (modemState == STATE_POCSAG))
				{
					//err = handlePOCSAG(com_requestbuffer + 3U, com_requestbuffer[1U] - 3U);
					err = 0U;
				}

				if (err == 0U)
				{
					sendACK(); // We don't send pages, but POCSAG can be enabled in Pi-Star
				}
				else
				{
					sendNAK(err);
				}
				break;

			case MMDVM_TRANSPARENT: // Do nothing, stay silent
				//sendNAK(err);
				break;

			case MMDVM_QSO_INFO:
				err = setQSOInfo(com_requestbuffer, com_requestbuffer[1U]);
				if ((err == 0U) || (err == 4U) /* non DMR mode, ignored */)
				{
					sendACK();
				}
				else
				{
					sendNAK(err);
				}
				break;

			default:
				sendNAK(1U);
				break;
		}

		com_requestbuffer[0U] = 0xFFU; // Invalidate this one
	}
	else
	{
		// Invalid MMDVM header byte
	}

	if ((uiDataGlobal.displayQSOState == QSO_DISPLAY_CALLER_DATA) || (uiDataGlobal.displayQSOState == QSO_DISPLAY_CALLER_DATA_UPDATE))
	{
		updateScreen(currentRxCommandState);
		uiDataGlobal.displayQSOState = QSO_DISPLAY_IDLE;
	}

	if (cwKeying)
	{
		if (!trxTransmissionEnabled)
		{
			// Start TX CWID, prepare for ANALOG
			if (trxGetMode() != RADIO_MODE_ANALOG)
			{
				trxSetModeAndBandwidth(RADIO_MODE_ANALOG, false);
				trxSetTxCSS(CODEPLUG_CSS_NONE);
				trxSetTone1(0);
			}

			trxEnableTransmission();

			trxSelectVoiceChannel(AT1846_VOICE_CHANNEL_TONE1);
			enableAudioAmp(AUDIO_AMP_MODE_RF);
			GPIO_PinWrite(GPIO_RX_audio_mux, Pin_RX_audio_mux, 1);

			updateScreen(HOTSPOT_RX_IDLE);
		}

		// CWID has been TXed, restore DIGITAL
		if (cwpoLen == 0U)
		{
			trxDisableTransmission();

			if (trxTransmissionEnabled)
			{
				// Stop TXing;
				trxTransmissionEnabled = false;
				//trxSetRX();
				LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);

				if (trxGetMode() == RADIO_MODE_ANALOG)
				{
					trxSetModeAndBandwidth(RADIO_MODE_DIGITAL, false);
					trxSelectVoiceChannel(AT1846_VOICE_CHANNEL_MIC);
					disableAudioAmp(AUDIO_AMP_MODE_RF);
				}
			}

			cwKeying = false;
			updateScreen(HOTSPOT_RX_IDLE);
		}
	}

}
