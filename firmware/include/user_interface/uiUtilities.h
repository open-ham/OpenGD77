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
#ifndef _OPENGD77_UIUTILITIES_H_
#define _OPENGD77_UIUTILITIES_H_

#include "user_interface/uiGlobals.h"
#include "user_interface/menuSystem.h"
#include "functions/settings.h"

#if defined(PLATFORM_GD77S)
#define ANNOUNCE_STATIC
#else
#define ANNOUNCE_STATIC static
#endif

#if defined(PLATFORM_GD77S)
// Displaying ANNOUNCE_STATIC here is a bit overkill but it makes things clearer.
ANNOUNCE_STATIC void announceRadioMode(bool voicePromptWasPlaying);
ANNOUNCE_STATIC void announceZoneName(bool voicePromptWasPlaying);
ANNOUNCE_STATIC void announceContactNameTgOrPc(bool voicePromptWasPlaying);
ANNOUNCE_STATIC void announcePowerLevel(bool voicePromptWasPlaying);
ANNOUNCE_STATIC void announceBatteryPercentage(void);
ANNOUNCE_STATIC void announceTS(void);
ANNOUNCE_STATIC void announceCC(void);
ANNOUNCE_STATIC void announceChannelName(bool voicePromptWasPlaying);
ANNOUNCE_STATIC void announceFrequency(void);
ANNOUNCE_STATIC void announceVFOChannelName(void);
ANNOUNCE_STATIC void announceVFOAndFrequency(bool announceVFOName);
ANNOUNCE_STATIC void announceSquelchLevel(bool voicePromptWasPlaying);

#endif

typedef enum
{
	DIRECTION_NONE = 0U,
	DIRECTION_TRANSMIT,
	DIRECTION_RECEIVE
} Direction_t;

typedef enum
{
	PROMPT_SEQUENCE_CHANNEL_NAME_OR_VFO_FREQ_AND_MODE,
	PROMPT_SEQUENCE_CHANNEL_NAME_AND_CONTACT_OR_VFO_FREQ_AND_MODE,
	PROMPT_SEQUENCE_CHANNEL_NAME_OR_VFO_FREQ,
	PROMPT_SEQUENCE_VFO_FREQ_UPDATE,
	PROMPT_SEQUENCE_ZONE,
	PROMPT_SEQUENCE_MODE,
	PROMPT_SEQUENCE_CONTACT_TG_OR_PC,
	PROMPT_SEQUENCE_TS,
	PROMPT_SEQUENCE_CC,
	PROMPT_SEQUENCE_POWER,
	PROMPT_SEQUENCE_BATTERY,
	PROMPT_SQUENCE_SQUELCH,
	PROMPT_SEQUENCE_TEMPERATURE,
	NUM_PROMPT_SEQUENCES
} voicePromptItem_t;

typedef enum
{
	DISPLAY_INFO_CONTACT_INVERTED = 0U,
	DISPLAY_INFO_CONTACT,
	DISPLAY_INFO_CONTACT_OVERRIDE_FRAME,
	DISPLAY_INFO_CHANNEL,
	DISPLAY_INFO_SQUELCH,
	DISPLAY_INFO_TONE_AND_SQUELCH,
	DISPLAY_INFO_SQUELCH_CLEAR_AREA,
	DISPLAY_INFO_TX_TIMER,
	DISPLAY_INFO_ZONE
} displayInformation_t;

typedef struct
{
	uint32_t 			entries;
	uint8_t  			contactLength;
	int32_t  			slices[ID_SLICES]; // [0] is min availabel ID, [ID_SLICES - 1] is max available ID
	uint32_t 			IDsPerSlice;
} dmrIDsCache_t;



#define TS_NO_OVERRIDE  0
void tsSetManualOverride(Channel_t chan, int8_t ts);
void tsSetFromContactOverride(Channel_t chan, struct_codeplugContact_t *contact);
int8_t tsGetManualOverride(Channel_t chan);
int8_t tsGetManualOverrideFromCurrentChannel(void);
bool tsIsManualOverridden(Channel_t chan);
void tsSetContactHasBeenOverriden(Channel_t chan, bool isOverriden);
bool tsIsContactHasBeenOverridden(Channel_t chan);
bool tsIsContactHasBeenOverriddenFromCurrentChannel(void);

bool isQSODataAvailableForCurrentTalker(void);
int alignFrequencyToStep(int freq, int step);
char *chomp(char *str);
int32_t getFirstSpacePos(char *str);
void dmrIDCacheInit(void);
bool dmrIDLookup(int targetId, dmrIdDataStruct_t *foundRecord);
bool contactIDLookup(uint32_t id, int calltype, char *buffer);
void uiUtilityRenderQSOData(void);
void uiUtilityRenderHeader(bool isVFODualWatchScanning);
void uiUtilityRedrawHeaderOnly(bool isVFODualWatchScanning);
LinkItem_t *lastheardFindInList(uint32_t id);
void lastheardInitList(void);
bool lastHeardListUpdate(uint8_t *dmrDataBuffer, bool forceOnHotspot);
void lastHeardClearLastID(void);
int getRSSIdBm(void);
void uiUtilityDrawRSSIBarGraph(void);
void uiUtilityDrawFMMicLevelBarGraph(void);
void uiUtilityDrawDMRMicLevelBarGraph(void);
void setOverrideTGorPC(int tgOrPc, bool privateCall);
void uiUtilityDisplayFrequency(uint8_t y, bool isTX, bool hasFocus, uint32_t frequency, bool displayVFOChannel, bool isScanMode, uint8_t dualWatchVFO);

uint16_t cssGetTone(int32_t index, CSSTypes_t type);
int cssIndex(uint16_t tone, CSSTypes_t type);
void cssIncrement(uint16_t *tone, int32_t *index, CSSTypes_t *type, bool loop, bool stayInCSSType);
void cssDecrement(uint16_t *tone, int32_t *index, CSSTypes_t *type);


size_t snprintDCS(char *s, size_t n, uint16_t code, bool inverted);

void freqEnterReset(void);
int freqEnterRead(int startDigit, int endDigit);

int getBatteryPercentage(void);
void getBatteryVoltage(int *volts, int *mvolts);
bool decreasePowerLevel(void);
bool increasePowerLevel(bool allowFullPower);

void announceChar(char ch);

void buildCSSCodeVoicePrompts(uint16_t code, CSSTypes_t cssType, Direction_t direction, bool announceType);
void announceCSSCode(uint16_t code, CSSTypes_t cssType, Direction_t direction, bool announceType, audioPromptThreshold_t immediateAnnounceThreshold);

void announceItem(voicePromptItem_t item, audioPromptThreshold_t immediateAnnouceThreshold);
void promptsPlayNotAfterTx(void);
void playNextSettingSequence(void);
void uiUtilityBuildTgOrPCDisplayName(char *nameBuf, int bufferLen);
void acceptPrivateCall(int id, int timeslot);
bool repeatVoicePromptOnSK1(uiEvent_t *ev);
bool handleMonitorMode(uiEvent_t *ev);
void uiUtilityDisplayInformation(const char *str, displayInformation_t line, int8_t yOverride);
void uiUtilityRenderQSODataAndUpdateScreen(void);

// QuickKeys
void saveQuickkeyMenuIndex(char key, uint8_t menuId, uint8_t entryId, uint8_t function);
void saveQuickkeyMenuLongValue(char key, uint8_t menuId, uint16_t entryiI);
void saveQuickkeyContactIndex(char key, uint16_t contactId);

void cssIncrement(uint16_t *tone, int32_t *index, CSSTypes_t *type, bool loop, bool stayInCSSType);
uint16_t cssGetTone(int32_t index, CSSTypes_t type);

bool uiShowQuickKeysChoices(char *buf, const int bufferLen, const char *menuTitle);

// DTMF contact sequences
void dtmfSequenceReset(void);
bool dtmfSequenceIsKeying(void);
void dtmfSequencePrepare(uint8_t *seq, bool autoStart);
void dtmfSequenceStart(void);
void dtmfSequenceStop(void);
void dtmfSequenceTick(bool popPreviousMenuOnEnding);

void resetOriginalSettingsData(void);

#endif
