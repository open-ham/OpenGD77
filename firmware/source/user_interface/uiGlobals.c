/*
 * Copyright (C)2019-2020 Roger Clark. VK3KYY / G4KYF
 *                        Daniel Caujolle-Bert, F1RMB
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

#include "user_interface/uiGlobals.h"

const int DBM_LEVELS[16] = {
		SMETER_S0, SMETER_S1, SMETER_S2, SMETER_S3, SMETER_S4, SMETER_S5, SMETER_S6, SMETER_S7, SMETER_S8,
		SMETER_S9, SMETER_S9_10, SMETER_S9_20, SMETER_S9_30, SMETER_S9_40, SMETER_S9_50, SMETER_S9_60
};


const char *POWER_LEVELS[]                  = { "50", "250", "500", "750", "1", "2", "3", "4", "5", "+W-"};
const char *POWER_LEVEL_UNITS[]             = { "mW", "mW",  "mW",  "mW",  "W", "W", "W", "W", "W", ""};
const char *DMR_DESTINATION_FILTER_LEVELS[] = { "TG", "Ct", "RxG" };
const char *ANALOG_FILTER_LEVELS[]          = { "CTCSS|DCS" };


uiDataGlobal_t uiDataGlobal =
{
		.receivedPcId 	              = 0x00, // No current Private call awaiting acceptance
		.tgBeforePcMode 	          = 0,    // No TG saved, prior to a Private call being accepted.
		.displayQSOState              = QSO_DISPLAY_DEFAULT_SCREEN,
		.displayQSOStatePrev          = QSO_DISPLAY_DEFAULT_SCREEN,
		.displaySquelch               = false,
		.isDisplayingQSOData          = false,
		.displayChannelSettings       = false,
		.reverseRepeater              = false,
		.currentSelectedChannelNumber = 0,
		.currentSelectedContactIndex  = 0,
		.lastHeardCount               = 0,

		.Scan =
		{
				.timer                  = 0,
				.timerReload			= 30,
				.direction              = 1,
				.availableChannelsCount = 0,
				.nuisanceDeleteIndex    = 0,
				.nuisanceDelete         = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
				.state                  = SCAN_SCANNING,
				.active                 = false,
				.toneActive             = false,
				.refreshOnEveryStep     = false,
				.lastIteration          = false,
				.scanType               = SCAN_TYPE_NORMAL_STEP,
				.stepTimeMilliseconds   = 0
		},

		.QuickMenu =
		{
				.tmpDmrDestinationFilterLevel = 0,
				.tmpDmrCcTsFilterLevel        = 0,
				.tmpAnalogFilterLevel         = 0,
				.tmpTxRxLockMode              = 0,
				.tmpToneScanCSS               = CSS_NONE,
				.tmpVFONumber                 = 0
		},

		.FreqEnter =
		{
				.index  = 0,
				.digits = { '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-' }
		},

		.PrivateCall =
		{
				.lastID = 0,
				.state  = PRIVATE_CALL_NOT_IN_CALL
		},

		.VoicePrompts =
		{
				.inhibitInitial =
#if defined(PLATFORM_GD77S)
									true
#else
									false // Used to indicate whether the voice prompts should be reloaded with the channel name or VFO freq
#endif
		},

		.MessageBox =
		{
				.type              = MESSAGEBOX_TYPE_UNDEF,
				.decoration        = MESSAGEBOX_DECORATION_NONE,
				.buttons           = MESSAGEBOX_BUTTONS_NONE,
				.pinLength         = 4,
				.message           = { 0 },
				.keyPressed        = 0,
				.validatorCallback = NULL
		},

		.DTMFContactList =
		{
				.nextPeriod = 0U,
				.isKeying   = false,
				.buffer     = { 0xff },
				.poLen      = 0U,
				.poPtr      = 0U,
				.durations  = { 0, 0, 0, 0, 0 },
				.inTone     = false
		}

};


settingsStruct_t originalNonVolatileSettings =
{
		.magicNumber = 0xDEADBEEF
};

struct_codeplugZone_t currentZone;
__attribute__((section(".data.$RAM2"))) struct_codeplugRxGroup_t currentRxGroupData;
int lastLoadedRxGroup = -1;// to show data for which RxGroupIndex has been loaded into    currentRxGroupData
struct_codeplugContact_t currentContactData;

bool PTTToggledDown = false; // PTT toggle feature

