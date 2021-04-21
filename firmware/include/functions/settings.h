/*
 * Copyright (C)2019 	Roger Clark, VK3KYY / G4KYF
 * 				and		Kai Ludwig, DG4KLU
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

#ifndef _OPENGD77_SETTINGS_H_
#define _OPENGD77_SETTINGS_H_

#include "functions/codeplug.h"
#include "functions/trx.h"

enum USB_MODE { USB_MODE_CPS, USB_MODE_HOTSPOT, USB_MODE_DEBUG };
enum SETTINGS_UI_MODE { SETTINGS_CHANNEL_MODE = 0, SETTINGS_VFO_A_MODE, SETTINGS_VFO_B_MODE };
enum BACKLIGHT_MODE { BACKLIGHT_MODE_AUTO = 0, BACKLIGHT_MODE_SQUELCH, BACKLIGHT_MODE_MANUAL, BACKLIGHT_MODE_BUTTONS, BACKLIGHT_MODE_NONE };
enum HOTSPOT_TYPE { HOTSPOT_TYPE_OFF = 0, HOTSPOT_TYPE_MMDVM, HOTSPOT_TYPE_BLUEDV };
enum CONTACT_DISPLAY_PRIO { CONTACT_DISPLAY_PRIO_CC_DB_TA = 0, CONTACT_DISPLAY_PRIO_DB_CC_TA, CONTACT_DISPLAY_PRIO_TA_CC_DB, CONTACT_DISPLAY_PRIO_TA_DB_CC };
enum SCAN_MODE { SCAN_MODE_HOLD = 0, SCAN_MODE_PAUSE, SCAN_MODE_STOP };
enum SPLIT_CONTACT { SPLIT_CONTACT_SINGLE_LINE_ONLY = 0, SPLIT_CONTACT_ON_TWO_LINES, SPLIT_CONTACT_AUTO };
enum ALLOW_PRIVATE_CALLS_MODE { ALLOW_PRIVATE_CALLS_OFF = 0, ALLOW_PRIVATE_CALLS_ON, ALLOW_PRIVATE_CALLS_PTT };//, ALLOW_PRIVATE_CALLS_AUTO };
enum BAND_LIMITS_ENUM { BAND_LIMITS_NONE = 0 , BAND_LIMITS_ON_LEGACY_DEFAULT, BAND_LIMITS_FROM_CPS };
enum INFO_ON_SCREEN { INFO_ON_SCREEN_OFF = 0x00, INFO_ON_SCREEN_TS = 0x01, INFO_ON_SCREEN_PWR = 0x02, INFO_ON_SCREEN_BOTH = 0x03 };

extern const int ECO_LEVEL_MAX;
extern const uint8_t BEEP_TX_NONE;
extern const uint8_t BEEP_TX_START;
extern const uint8_t BEEP_TX_STOP;

extern int settingsCurrentChannelNumber;
extern int *nextKeyBeepMelody;
extern struct_codeplugChannel_t settingsVFOChannel[2];
extern struct_codeplugGeneralSettings_t settingsCodeplugGeneralSettings;

typedef enum
{
	BIT_INVERSE_VIDEO               = (1 << 0),
	BIT_PTT_LATCH                   = (1 << 1),
	BIT_TRANSMIT_TALKER_ALIAS       = (1 << 2),
	BIT_BATTERY_VOLTAGE_IN_HEADER   = (1 << 3),
	BIT_SETTINGS_UPDATED            = (1 << 4),
	BIT_TX_RX_FREQ_LOCK             = (1 << 5),
	BIT_ALL_LEDS_DISABLED           = (1 << 6)
} bitfieldOptions_t;

typedef struct
{
	int 			magicNumber;
	uint32_t		overrideTG;
	uint32_t		vfoScanLow[2]; // low frequency for VFO Scanning
	uint32_t		vfoScanHigh[2]; // High frequency for VFO Scanning
	int16_t			currentChannelIndexInZone;
	int16_t			currentChannelIndexInAllZone;
	int16_t			currentIndexInTRxGroupList[3]; // Current Channel, VFO A and VFO B
	int16_t			currentZone;
	uint16_t		keypadTimerLong;
	uint16_t		keypadTimerRepeat;
	uint16_t		userPower;
	uint16_t		bitfieldOptions; // see bitfieldOptions_t
	uint8_t			txPowerLevel;
	uint8_t			txTimeoutBeepX5Secs;
	uint8_t			beepVolumeDivider;
	uint8_t			beepOptions;
	uint8_t			micGainDMR;
	uint8_t			micGainFM;
	uint8_t			backlightMode; // see BACKLIGHT_MODE enum
	uint8_t			backLightTimeout; // 0 = never timeout. 1 - 255 time in seconds
	int8_t			displayContrast;
	int8_t			displayBacklightPercentage;
	int8_t			displayBacklightPercentageOff; // backlight level when "off"
	uint8_t			initialMenuNumber;
	uint8_t			extendedInfosOnScreen;
	uint8_t			txFreqLimited;
	uint8_t			scanModePause;
	uint8_t			scanDelay;
	uint8_t			scanStepTime;
	uint8_t			squelchDefaults[RADIO_BANDS_TOTAL_NUM]; // VHF, 200Mhz and UHF
	uint8_t			currentVFONumber;
	uint16_t		tsManualOverride;
	uint8_t			dmrDestinationFilter;
	uint8_t			dmrCaptureTimeout;
	uint8_t			dmrCcTsFilter;
	uint8_t			analogFilterLevel;
	uint8_t			languageIndex;
	uint8_t			hotspotType;
	uint8_t    		privateCalls;
	uint8_t			contactDisplayPriority;
	uint8_t			splitContact;
	uint8_t			voxThreshold; // 0: disabled
	uint8_t			voxTailUnits; // 500ms units
	uint8_t			audioPromptMode;
	int8_t			temperatureCalibration;// Units of 0.5 deg C
	uint8_t			ecoLevel;// Power saving / economy level
} settingsStruct_t;

typedef enum DMR_DESTINATION_FILTER_TYPE
{
	DMR_DESTINATION_FILTER_NONE = 0,
	DMR_DESTINATION_FILTER_TG,
	DMR_DESTINATION_FILTER_DC,
	DMR_DESTINATION_FILTER_RXG,
	NUM_DMR_DESTINATION_FILTER_LEVELS
} dmrDestinationFilter_t;

// Bit patterns
#define DMR_CC_FILTER_PATTERN  0x01
#define DMR_TS_FILTER_PATTERN  0x02

// NOTE. THIS ENUM IS USED AS BIT PATTERNS, DO NOT CHANGE THE DEFINED VALUES WITHOUT UPDATING ANY CODE WHICH USES THIS ENUM
typedef enum DMR_CCTS_FILTER_TYPE
{
	DMR_CCTS_FILTER_NONE = 0,
	DMR_CCTS_FILTER_CC = DMR_CC_FILTER_PATTERN,
	DMR_CCTS_FILTER_TS = DMR_TS_FILTER_PATTERN,
	DMR_CCTS_FILTER_CC_TS = DMR_CC_FILTER_PATTERN | DMR_TS_FILTER_PATTERN,
	NUM_DMR_CCTS_FILTER_LEVELS
} dmrCcTsFilter_t;

typedef enum ANALOG_FILTER_TYPE
{
	ANALOG_FILTER_NONE = 0,
	ANALOG_FILTER_CSS,
	NUM_ANALOG_FILTER_LEVELS
} analogFilter_t;

typedef enum AUDIO_PROMPT_MODE
{
	AUDIO_PROMPT_MODE_SILENT = 0,
	AUDIO_PROMPT_MODE_BEEP,
	AUDIO_PROMPT_MODE_VOICE_LEVEL_1,
	AUDIO_PROMPT_MODE_VOICE_LEVEL_2,
	AUDIO_PROMPT_MODE_VOICE_LEVEL_3 ,
	NUM_AUDIO_PROMPT_MODES
} audioPromptMode_t;

typedef enum PROMPT_AUTOPLAY_THRESHOLD
{
	PROMPT_THRESHOLD_1 = AUDIO_PROMPT_MODE_VOICE_LEVEL_1,
	PROMPT_THRESHOLD_2,
	PROMPT_THRESHOLD_3,
	PROMPT_THRESHOLD_NEVER_PLAY_IMMEDIATELY
} audioPromptThreshold_t;

typedef struct
{
	bool 	isEnabled;
	int 	DMRTimeout;
	int 	savedRadioMode;
	uint8_t savedSquelch;
	int 	savedDMRCcTsFilter;
	int 	savedDMRDestinationFilter;
	int 	savedDMRCc;
	int 	savedDMRTs;
} monitorModeSettingsStruct_t;

extern settingsStruct_t nonVolatileSettings;
extern struct_codeplugChannel_t *currentChannelData;
extern struct_codeplugChannel_t channelScreenChannelData;
extern struct_codeplugContact_t contactListContactData;
extern struct_codeplugDTMFContact_t contactListDTMFContactData;
extern int contactListContactIndex;
extern int settingsUsbMode;
extern monitorModeSettingsStruct_t monitorModeData;

// Do not use the following settingsSet<TYPE>(...) functions, use settingsSet() instead
void settingsSetBOOL(bool *s, bool v);
void settingsSetINT8(int8_t *s, int8_t v);
void settingsSetUINT8(uint8_t *s, uint8_t v);
void settingsSetINT16(int16_t *s, int16_t v);
void settingsSetUINT16(uint16_t *s, uint16_t v);
void settingsSetINT32(int32_t *s, int32_t v);
void settingsSetUINT32(uint32_t *s, uint32_t v);

// Do not use the following settingsInc/Dec<TYPE>(...) functions, use settingsIncrement/Decrement() instead
void settingsIncINT8(int8_t *s, int8_t v);
void settingsIncUINT8(uint8_t *s, uint8_t v);
void settingsIncINT16(int16_t *s, int16_t v);
void settingsIncUINT16(uint16_t *s, uint16_t v);
void settingsIncINT32(int32_t *s, int32_t v);
void settingsIncUINT32(uint32_t *s, uint32_t v);

void settingsDecINT8(int8_t *s, int8_t v);
void settingsDecUINT8(uint8_t *s, uint8_t v);
void settingsDecINT16(int16_t *s, int16_t v);
void settingsDecUINT16(uint16_t *s, uint16_t v);
void settingsDecINT32(int32_t *s, int32_t v);
void settingsDecUINT32(uint32_t *s, uint32_t v);

// Workaround for Eclipse's CDT parser angriness because it doesn't support C11 yet
#ifdef __CDT_PARSER__
#define settingsSet(S, V) do { /* It uses C11's _Generic() at compile time */ S = V; } while(0)
#else
#define settingsSet(S, V) _Generic((S),   \
	bool:     settingsSetBOOL,            \
	int8_t:   settingsSetINT8,            \
	uint8_t:  settingsSetUINT8,           \
	int16_t:  settingsSetINT16,           \
	uint16_t: settingsSetUINT16,          \
	int32_t:  settingsSetINT32,           \
	uint32_t: settingsSetUINT32           \
	)(&S, V)
#endif

#ifdef __CDT_PARSER__
#define settingsIncrement(S, V) do { /* It uses C11's _Generic() at compile time */ S += V; } while(0)
#else
#define settingsIncrement(S, V) _Generic((S),   \
	int8_t:   settingsIncINT8,            \
	uint8_t:  settingsIncUINT8,           \
	int16_t:  settingsIncINT16,           \
	uint16_t: settingsIncUINT16,          \
	int32_t:  settingsIncINT32,           \
	uint32_t: settingsIncUINT32           \
	)(&S, V)
#endif

#ifdef __CDT_PARSER__
#define settingsDecrement(S, V) do { /* It uses C11's _Generic() at compile time */ S -= V; } while(0)
#else
#define settingsDecrement(S, V) _Generic((S),   \
	int8_t:   settingsDecINT8,            \
	uint8_t:  settingsDecUINT8,           \
	int16_t:  settingsDecINT16,           \
	uint16_t: settingsDecUINT16,          \
	int32_t:  settingsDecINT32,           \
	uint32_t: settingsDecUINT32           \
	)(&S, V)
#endif

void settingsSetOptionBit(bitfieldOptions_t bit, bool set);
bool settingsIsOptionBitSet(bitfieldOptions_t bit);

void settingsSetDirty(void);
void settingsSetVFODirty(void);
void settingsSaveIfNeeded(bool immediately);
bool settingsSaveSettings(bool includeVFOs);
bool settingsLoadSettings(void);
void settingsRestoreDefaultSettings(void);
void settingsEraseCustomContent(void);
void settingsInitVFOChannel(int vfoNumber);
void enableVoicePromptsIfLoaded(void);
int settingsGetScanStepTimeMilliseconds(void);

#endif
