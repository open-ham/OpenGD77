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

#include <math.h>
#include "hardware/EEPROM.h"
#include "user_interface/menuSystem.h"
#include "user_interface/uiUtilities.h"
#include "user_interface/uiLocalisation.h"
#include "hardware/HR-C6000.h"
#include "functions/settings.h"
#include "hardware/SPI_Flash.h"
#include "functions/ticks.h"
#include "functions/trx.h"


static __attribute__((section(".data.$RAM2"))) LinkItem_t callsList[NUM_LASTHEARD_STORED];

static const int DMRID_MEMORY_STORAGE_START = 0x30000;
static const int DMRID_HEADER_LENGTH = 0x0C;
static dmrIDsCache_t dmrIDsCache;
static voicePromptItem_t voicePromptSequenceState = PROMPT_SEQUENCE_CHANNEL_NAME_OR_VFO_FREQ;
static uint32_t lastTG = 0;

volatile uint32_t lastID = 0;// This needs to be volatile as lastHeardClearLastID() is called from an ISR
LinkItem_t *LinkHead = callsList;
static void announceChannelNameOrVFOFrequency(bool voicePromptWasPlaying, bool announceVFOName);

DECLARE_SMETER_ARRAY(rssiMeterHeaderBar, DISPLAY_SIZE_X);

// Set TS manual override
// chan: CHANNEL_VFO_A, CHANNEL_VFO_B, CHANNEL_CHANNEL
// ts: 1, 2, TS_NO_OVERRIDE
void tsSetManualOverride(Channel_t chan, int8_t ts)
{
	uint16_t tsOverride = nonVolatileSettings.tsManualOverride;

	// Clear TS override for given channel
	tsOverride &= ~(0x03 << (2 * ((int8_t)chan)));
	if (ts != TS_NO_OVERRIDE)
	{
		// Set TS override for given channel
		tsOverride |= (ts << (2 * ((int8_t)chan)));
	}

	settingsSet(nonVolatileSettings.tsManualOverride, tsOverride);
}

// Set TS manual override from contact TS override
// chan: CHANNEL_VFO_A, CHANNEL_VFO_B, CHANNEL_CHANNEL
// contact: apply TS override from contact setting
void tsSetFromContactOverride(Channel_t chan, struct_codeplugContact_t *contact)
{
	if ((contact->reserve1 & 0x01) == 0x00)
	{
		tsSetManualOverride(chan, (((contact->reserve1 & 0x02) >> 1) + 1));
	}
	else
	{
		tsSetManualOverride(chan, TS_NO_OVERRIDE);
	}
}

// Get TS override value
// chan: CHANNEL_VFO_A, CHANNEL_VFO_B, CHANNEL_CHANNEL
// returns (TS + 1, 0 no override)
int8_t tsGetManualOverride(Channel_t chan)
{
	return (nonVolatileSettings.tsManualOverride >> (2 * (int8_t)chan)) & 0x03;
}

// Get manually overridden TS, if any, from currentChannelData
// returns (TS + 1, 0 no override)
int8_t tsGetManualOverrideFromCurrentChannel(void)
{
	Channel_t chan = (((currentChannelData->NOT_IN_CODEPLUG_flag & 0x01) == 0x01) ?
			(((currentChannelData->NOT_IN_CODEPLUG_flag & 0x02) == 0x02) ? CHANNEL_VFO_B : CHANNEL_VFO_A) : CHANNEL_CHANNEL);

	return tsGetManualOverride(chan);
}

// Check if TS is overrode
// chan: CHANNEL_VFO_A, CHANNEL_VFO_B, CHANNEL_CHANNEL
// returns true on overrode for the specified channel
bool tsIsManualOverridden(Channel_t chan)
{
	return (nonVolatileSettings.tsManualOverride & (0x03 << (2 * ((int8_t)chan))));
}

// Keep track of an override when the selected contact has a TS override set
// chan: CHANNEL_VFO_A, CHANNEL_VFO_B, CHANNEL_CHANNEL
void tsSetContactHasBeenOverriden(Channel_t chan, bool isOverriden)
{
	uint16_t tsOverride = nonVolatileSettings.tsManualOverride;

	if (isOverriden)
	{
		tsOverride |= (1 << ((3 * 2) + ((int8_t)chan)));
	}
	else
	{
		tsOverride &= ~(1 << ((3 * 2) + ((int8_t)chan)));
	}

	settingsSet(nonVolatileSettings.tsManualOverride, tsOverride);
}

// Get TS override status, of a selected contact which have a TS override set
// chan: CHANNEL_VFO_A, CHANNEL_VFO_B, CHANNEL_CHANNEL
bool tsIsContactHasBeenOverriddenFromCurrentChannel(void)
{
	Channel_t chan = (((currentChannelData->NOT_IN_CODEPLUG_flag & 0x01) == 0x01) ?
			(((currentChannelData->NOT_IN_CODEPLUG_flag & 0x02) == 0x02) ? CHANNEL_VFO_B : CHANNEL_VFO_A) : CHANNEL_CHANNEL);

	return tsIsContactHasBeenOverridden(chan);
}

// Get manual TS override of the selected contact (which has a TS override set)
// chan: CHANNEL_VFO_A, CHANNEL_VFO_B, CHANNEL_CHANNEL
bool tsIsContactHasBeenOverridden(Channel_t chan)
{
	return (nonVolatileSettings.tsManualOverride >> ((3 * 2) + (int8_t)chan)) & 0x01;
}

bool isQSODataAvailableForCurrentTalker(void)
{
	LinkItem_t *item = NULL;
	uint32_t rxID = HRC6000GetReceivedSrcId();

	// We're in digital mode, RXing, and current talker is already at the top of last heard list,
	// hence immediately display complete contact/TG info on screen
	if ((trxTransmissionEnabled == false) && ((trxGetMode() == RADIO_MODE_DIGITAL) && (rxID != 0) && (HRC6000GetReceivedTgOrPcId() != 0)) &&
			(getAudioAmpStatus() & AUDIO_AMP_MODE_RF)
			&& checkTalkGroupFilter() &&
			(((item = lastheardFindInList(rxID)) != NULL) && (item == LinkHead)))
	{
		return true;
	}

	return false;
}


int alignFrequencyToStep(int freq, int step)
{
	int r = freq % step;

	return (r ? freq + (step - r) : freq);
}

/*
 * Remove space at the end of the array, and return pointer to first non space character
 */
char *chomp(char *str)
{
	char *sp = str, *ep = str;

	while (*ep != '\0')
	{
		ep++;
	}

	// Spaces at the end
	while (ep > str)
	{
		if (*ep == '\0')
		{
		}
		else if (*ep == ' ')
		{
			*ep = '\0';
		}
		else
		{
			break;
		}

		ep--;
	}

	// Spaces at the beginning
	while (*sp == ' ')
	{
		sp++;
	}

	return sp;
}

int32_t getFirstSpacePos(char *str)
{
	char *p = str;

	while(*p != '\0')
	{
		if (*p == ' ')
		{
			return (p - str);
		}

		p++;
	}

	return -1;
}

void lastheardInitList(void)
{
	LinkHead = callsList;

	for(int i = 0; i < NUM_LASTHEARD_STORED; i++)
	{
		callsList[i].id = 0;
		callsList[i].talkGroupOrPcId = 0;
		callsList[i].contact[0] = 0;
		callsList[i].talkgroup[0] = 0;
		callsList[i].talkerAlias[0] = 0;
		callsList[i].locator[0] = 0;
		callsList[i].time = 0;

		if (i == 0)
		{
			callsList[i].prev = NULL;
		}
		else
		{
			callsList[i].prev = &callsList[i - 1];
		}

		if (i < (NUM_LASTHEARD_STORED - 1))
		{
			callsList[i].next = &callsList[i + 1];
		}
		else
		{
			callsList[i].next = NULL;
		}
	}
}

LinkItem_t *lastheardFindInList(uint32_t id)
{
    LinkItem_t *item = LinkHead;

    while (item->next != NULL)
    {
        if (item->id == id)
        {
            // found it
            return item;
        }
        item = item->next;
    }
    return NULL;
}

static uint8_t *coordsToMaidenhead(double longitude, double latitude)
{
	static uint8_t maidenhead[15];
	double l, l2;
	uint8_t c;

	l = longitude;

	for (uint8_t i = 0; i < 2; i++)
	{
		l = l / ((i == 0) ? 20.0 : 10.0) + 9.0;
		c = (uint8_t) l;
		maidenhead[0 + i] = c + 'A';
		l2 = c;
		l -= l2;
		l *= 10.0;
		c = (uint8_t) l;
		maidenhead[2 + i] = c + '0';
		l2 = c;
		l -= l2;
		l *= 24.0;
		c = (uint8_t) l;
		maidenhead[4 + i] = c + 'A';

#if 0
		if (extended)
		{
			l2 = c;
			l -= l2;
			l *= 10.0;
			c = (uint8_t) l;
			maidenhead[6 + i] = c + '0';
			l2 = c;
			l -= l2;
			l *= 24.0;
			c = (uint8_t) l;
			maidenhead[8 + i] = c + (extended ? 'A' : 'a');
			l2 = c;
			l -= l2;
			l *= 10.0;
			c = (uint8_t) l;
			maidenhead[10 + i] = c + '0';
			l2 = c;
			l -= l2;
			l *= 24.0;
			c = (uint8_t) l;
			maidenhead[12 + i] = c + (extended ? 'A' : 'a');
		}
#endif

		l = latitude;
	}

#if 0
	maidenhead[extended ? 14 : 6] = '\0';
#else
	maidenhead[6] = '\0';
#endif

	return &maidenhead[0];
}

static uint8_t *decodeGPSPosition(uint8_t *data)
{
#if 0
	uint8_t errorI = (data[2U] & 0x0E) >> 1U;
	const char* error;
	switch (errorI) {
	case 0U:
		error = "< 2m";
		break;
	case 1U:
		error = "< 20m";
		break;
	case 2U:
		error = "< 200m";
		break;
	case 3U:
		error = "< 2km";
		break;
	case 4U:
		error = "< 20km";
		break;
	case 5U:
		error = "< 200km";
		break;
	case 6U:
		error = "> 200km";
		break;
	default:
		error = "not known";
		break;
	}
#endif

	int32_t longitudeI = ((data[2U] & 0x01U) << 31) | (data[3U] << 23) | (data[4U] << 15) | (data[5U] << 7);
	longitudeI >>= 7;

	int32_t latitudeI = (data[6U] << 24) | (data[7U] << 16) | (data[8U] << 8);
	latitudeI >>= 8;

	float longitude = 360.0F / 33554432.0F;	// 360/2^25 steps
	float latitude  = 180.0F / 16777216.0F;	// 180/2^24 steps

	longitude *= (float)longitudeI;
	latitude  *= (float)latitudeI;

	return (coordsToMaidenhead(longitude, latitude));
}

static uint8_t *decodeTA(uint8_t *TA)
{
	uint8_t *b;
	uint8_t c;
	int8_t j;
    uint8_t i, t1, t2;
    static uint8_t buffer[32];
    uint8_t *talkerAlias = TA;
    uint8_t TAformat = (talkerAlias[0] >> 6U) & 0x03U;
    uint8_t TAsize   = (talkerAlias[0] >> 1U) & 0x1FU;

    switch (TAformat)
    {
		case 0U:		// 7 bit
			memset(&buffer, 0, sizeof(buffer));
			b = &talkerAlias[0];
			t1 = 0U; t2 = 0U; c = 0U;

			for (i = 0U; (i < 32U) && (t2 < TAsize); i++)
			{
				for (j = 7; j >= 0; j--)
				{
					c = (c << 1U) | (b[i] >> j);

					if (++t1 == 7U)
					{
						if (i > 0U)
						{
							buffer[t2++] = c & 0x7FU;
						}

						t1 = 0U;
						c = 0U;
					}
				}
			}
			buffer[TAsize] = 0;
			break;

		case 1U:		// ISO 8 bit
		case 2U:		// UTF8
			memcpy(&buffer, talkerAlias + 1U, sizeof(buffer));
			break;

		case 3U:		// UTF16 poor man's conversion
			t2=0;
			memset(&buffer, 0, sizeof(buffer));
			for (i = 0U; (i < 15U) && (t2 < TAsize); i++)
			{
				if (talkerAlias[2U * i + 1U] == 0)
				{
					buffer[t2++] = talkerAlias[2U * i + 2U];
				}
				else
				{
					buffer[t2++] = '?';
				}
			}
			buffer[TAsize] = 0;
			break;
    }

	return &buffer[0];
}

void lastHeardClearLastID(void)
{
	lastID = 0;
}

static void updateLHItem(LinkItem_t *item)
{
	static const int bufferLen = 33; // displayChannelNameOrRxFrequency() use 6x8 font
	char buffer[bufferLen];// buffer passed to the DMR ID lookup function, needs to be large enough to hold worst case text length that is returned. Currently 16+1
	dmrIdDataStruct_t currentRec;

	if ((item->talkGroupOrPcId >> 24) == PC_CALL_FLAG)
	{
		// Its a Private call
		switch (nonVolatileSettings.contactDisplayPriority)
		{
			case CONTACT_DISPLAY_PRIO_CC_DB_TA:
			case CONTACT_DISPLAY_PRIO_TA_CC_DB:
				if (contactIDLookup(item->id, CONTACT_CALLTYPE_PC, buffer) == true)
				{
					snprintf(item->contact, 17, "%s", buffer);
				}
				else
				{
					dmrIDLookup(item->id, &currentRec);
					snprintf(item->contact, 17, "%s", currentRec.text);
				}
				break;

			case CONTACT_DISPLAY_PRIO_DB_CC_TA:
			case CONTACT_DISPLAY_PRIO_TA_DB_CC:
				if (dmrIDLookup(item->id, &currentRec) == true)
				{
					snprintf(item->contact, 17, "%s", currentRec.text);
				}
				else
				{
					if (contactIDLookup(item->id, CONTACT_CALLTYPE_PC, buffer) == true)
					{
						snprintf(item->contact, 17, "%s", buffer);
					}
					else
					{
						snprintf(item->contact, 17, "%s", currentRec.text);
					}
				}
				break;
		}

		if (item->talkGroupOrPcId != (trxDMRID | (PC_CALL_FLAG << 24)))
		{
			if (contactIDLookup(item->talkGroupOrPcId & 0x00FFFFFF, CONTACT_CALLTYPE_PC, buffer) == true)
			{
				snprintf(item->talkgroup, 17, "%s", buffer);
			}
			else
			{
				dmrIDLookup(item->talkGroupOrPcId & 0x00FFFFFF, &currentRec);
				snprintf(item->talkgroup, 17, "%s", currentRec.text);
			}
		}
	}
	else
	{
		// TalkGroup
		if (contactIDLookup(item->talkGroupOrPcId, CONTACT_CALLTYPE_TG, buffer) == true)
		{
			snprintf(item->talkgroup, 17, "%s", buffer);
		}
		else
		{
			snprintf(item->talkgroup, 17, "%s %d", currentLanguage->tg, (item->talkGroupOrPcId & 0x00FFFFFF));
		}

		switch (nonVolatileSettings.contactDisplayPriority)
		{
			case CONTACT_DISPLAY_PRIO_CC_DB_TA:
			case CONTACT_DISPLAY_PRIO_TA_CC_DB:
				if (contactIDLookup(item->id, CONTACT_CALLTYPE_PC, buffer) == true)
				{
					snprintf(item->contact, 21, "%s", buffer);
				}
				else
				{
					dmrIDLookup((item->id & 0x00FFFFFF), &currentRec);
					snprintf(item->contact, 21, "%s", currentRec.text);
				}
				break;

			case CONTACT_DISPLAY_PRIO_DB_CC_TA:
			case CONTACT_DISPLAY_PRIO_TA_DB_CC:
				if (dmrIDLookup((item->id & 0x00FFFFFF), &currentRec) == true)
				{
					snprintf(item->contact, 21, "%s", currentRec.text);
				}
				else
				{
					if (contactIDLookup(item->id, CONTACT_CALLTYPE_PC, buffer) == true)
					{
						snprintf(item->contact, 21, "%s", buffer);
					}
					else
					{
						snprintf(item->contact, 21, "%s", currentRec.text);
					}
				}
				break;
		}
	}
}

bool lastHeardListUpdate(uint8_t *dmrDataBuffer, bool forceOnHotspot)
{
	static uint8_t bufferTA[32];
	static uint8_t blocksTA = 0x00;
	bool retVal = false;
	uint32_t talkGroupOrPcId = (dmrDataBuffer[0] << 24) + (dmrDataBuffer[3] << 16) + (dmrDataBuffer[4] << 8) + (dmrDataBuffer[5] << 0);
	static bool overrideTA = false;

	if ((HRC6000GetReceivedTgOrPcId() != 0) || forceOnHotspot)
	{
		if (dmrDataBuffer[0] == TG_CALL_FLAG || dmrDataBuffer[0] == PC_CALL_FLAG)
		{
			uint32_t id = (dmrDataBuffer[6] << 16) + (dmrDataBuffer[7] << 8) + (dmrDataBuffer[8] << 0);

			if (id != lastID)
			{
				memset(bufferTA, 0, 32);// Clear any TA data in TA buffer (used for decode)
				blocksTA = 0x00;
				overrideTA = false;

				retVal = true;// something has changed
				lastID = id;

				LinkItem_t *item = lastheardFindInList(id);

				// Already in the list
				if (item != NULL)
				{
					if (item->talkGroupOrPcId != talkGroupOrPcId)
					{
						item->talkGroupOrPcId = talkGroupOrPcId; // update the TG in case they changed TG
						updateLHItem(item);
					}

					item->time = fw_millis();
					lastTG = talkGroupOrPcId;

					if (item == LinkHead)
					{
						uiDataGlobal.displayQSOState = QSO_DISPLAY_CALLER_DATA;// flag that the display needs to update
						return true;// already at top of the list
					}
					else
					{
						// not at top of the list
						// Move this item to the top of the list
						LinkItem_t *next = item->next;
						LinkItem_t *prev = item->prev;

						// set the previous item to skip this item and link to 'items' next item.
						prev->next = next;

						if (item->next != NULL)
						{
							// not the last in the list
							next->prev = prev;// backwards link the next item to the item before us in the list.
						}

						item->next = LinkHead;// link our next item to the item at the head of the list

						LinkHead->prev = item;// backwards link the hold head item to the item moving to the top of the list.

						item->prev = NULL;// change the items prev to NULL now we are at teh top of the list
						LinkHead = item;// Change the global for the head of the link to the item that is to be at the top of the list.
						if (item->talkGroupOrPcId != 0)
						{
							uiDataGlobal.displayQSOState = QSO_DISPLAY_CALLER_DATA;// flag that the display needs to update
						}
					}
				}
				else
				{
					// Not in the list
					item = LinkHead;// setup to traverse the list from the top.

					if (uiDataGlobal.lastHeardCount < NUM_LASTHEARD_STORED)
					{
						uiDataGlobal.lastHeardCount++;
					}

					// need to use the last item in the list as the new item at the top of the list.
					// find last item in the list
					while(item->next != NULL)
					{
						item = item->next;
					}
					//item is now the last

					(item->prev)->next = NULL;// make the previous item the last

					LinkHead->prev = item;// set the current head item to back reference this item.
					item->next = LinkHead;// set this items next to the current head
					LinkHead = item;// Make this item the new head

					item->id = id;
					item->talkGroupOrPcId = talkGroupOrPcId;
					item->time = fw_millis();
					item->receivedTS = (dmrMonitorCapturedTS != -1) ? dmrMonitorCapturedTS : trxGetDMRTimeSlot();
					lastTG = talkGroupOrPcId;

					memset(item->contact, 0, sizeof(item->contact)); // Clear contact's datas
					memset(item->talkgroup, 0, sizeof(item->talkgroup));
					memset(item->talkerAlias, 0, sizeof(item->talkerAlias));
					memset(item->locator, 0, sizeof(item->locator));

					updateLHItem(item);

					if (item->talkGroupOrPcId != 0)
					{
						uiDataGlobal.displayQSOState = QSO_DISPLAY_CALLER_DATA;// flag that the display needs to update
					}
				}
			}
			else // update TG even if the DMRID did not change
			{
				LinkItem_t *item = lastheardFindInList(id);

				if (lastTG != talkGroupOrPcId)
				{
					if (item != NULL)
					{
						// Already in the list
						item->talkGroupOrPcId = talkGroupOrPcId;// update the TG in case they changed TG
						updateLHItem(item);
						item->time = fw_millis();
					}

					lastTG = talkGroupOrPcId;
					memset(bufferTA, 0, 32);// Clear any TA data in TA buffer (used for decode)
					blocksTA = 0x00;
					overrideTA = false;
					retVal = true;// something has changed
				}

				item->receivedTS = (dmrMonitorCapturedTS != -1) ? dmrMonitorCapturedTS : trxGetDMRTimeSlot();// Always update this in case the TS changed.
			}
		}
		else
		{
			// Data contains the Talker Alias Data
			uint8_t blockID = (forceOnHotspot ? dmrDataBuffer[0] : DMR_frame_buffer[0]) - 4;

			// ID 0x04..0x07: TA
			if (blockID < 4)
			{

				// Already stored first byte in block TA Header has changed, lets clear other blocks too
				if ((blockID == 0) && ((blocksTA & (1 << blockID)) != 0) &&
						(bufferTA[0] != (forceOnHotspot ? dmrDataBuffer[2] : DMR_frame_buffer[2])))
				{
					blocksTA &= ~(1 << 0);

					// Clear all other blocks if they're already stored
					if ((blocksTA & (1 << 1)) != 0)
					{
						blocksTA &= ~(1 << 1);
						memset(bufferTA + 7, 0, 7); // Clear 2nd TA block
					}
					if ((blocksTA & (1 << 2)) != 0)
					{
						blocksTA &= ~(1 << 2);
						memset(bufferTA + 14, 0, 7); // Clear 3rd TA block
					}
					if ((blocksTA & (1 << 3)) != 0)
					{
						blocksTA &= ~(1 << 3);
						memset(bufferTA + 21, 0, 7); // Clear 4th TA block
					}
					overrideTA = true;
				}

				// We don't already have this TA block
				if ((blocksTA & (1 << blockID)) == 0)
				{
					static const uint8_t blockLen = 7;
					uint32_t blockOffset = blockID * blockLen;

					blocksTA |= (1 << blockID);

					if ((blockOffset + blockLen) < sizeof(bufferTA))
					{
						memcpy(bufferTA + blockOffset, (void *)(forceOnHotspot ? &dmrDataBuffer[2] : &DMR_frame_buffer[2]), blockLen);

						// Format and length infos are available, we can decode now
						if (bufferTA[0] != 0x0)
						{
							uint8_t *decodedTA;

							if ((decodedTA = decodeTA(&bufferTA[0])) != NULL)
							{
								// TAs doesn't match, update contact and screen.
								if (overrideTA || (strlen((const char *)decodedTA) > strlen((const char *)&LinkHead->talkerAlias)))
								{
									memcpy(&LinkHead->talkerAlias, decodedTA, 31);// Brandmeister seems to send callsign as 6 chars only

									if ((blocksTA & (1 << 1)) != 0) // we already received the 2nd TA block, check for 'DMR ID:'
									{
										char *p = NULL;

										// Get rid of 'DMR ID:xxxxxxx' part of the TA, sent by BM
										if (((p = strstr(&LinkHead->talkerAlias[0], "DMR ID:")) != NULL) || ((p = strstr(&LinkHead->talkerAlias[0], "DMR I")) != NULL))
										{
											*p = 0;
										}
									}

									overrideTA = false;
									uiDataGlobal.displayQSOState = QSO_DISPLAY_CALLER_DATA;
								}
							}
						}
					}
				}
			}
			else if (blockID == 4) // ID 0x08: GPS
			{
				uint8_t *locator = decodeGPSPosition((uint8_t *)(forceOnHotspot ? &dmrDataBuffer[0] : &DMR_frame_buffer[0]));

				if (strncmp((char *)&LinkHead->locator, (char *)locator, 7) != 0)
				{
					memcpy(&LinkHead->locator, locator, 7);
					uiDataGlobal.displayQSOState = QSO_DISPLAY_CALLER_DATA_UPDATE;
				}
			}
		}
	}

	return retVal;
}


static void dmrIDReadContactInFlash(uint32_t contactOffset, uint8_t *data, uint32_t len)
{
	SPI_Flash_read((DMRID_MEMORY_STORAGE_START + DMRID_HEADER_LENGTH) + contactOffset, data, len);
}


void dmrIDCacheInit(void)
{
	uint8_t headerBuf[32];

	memset(&dmrIDsCache, 0, sizeof(dmrIDsCache_t));
	memset(&headerBuf, 0, sizeof(headerBuf));

	SPI_Flash_read(DMRID_MEMORY_STORAGE_START, headerBuf, DMRID_HEADER_LENGTH);

	if ((headerBuf[0] != 'I') || (headerBuf[1] != 'D') || (headerBuf[2] != '-'))
	{
		return;
	}

	dmrIDsCache.entries = ((uint32_t)headerBuf[8] | (uint32_t)headerBuf[9] << 8 | (uint32_t)headerBuf[10] << 16 | (uint32_t)headerBuf[11] << 24);
	dmrIDsCache.contactLength = (uint8_t)headerBuf[3] - 0x4a;

	if (dmrIDsCache.entries > 0)
	{
		dmrIdDataStruct_t dmrIDContact;

		// Set Min and Max IDs boundaries
		// First available ID
		dmrIDReadContactInFlash(0, (uint8_t *)&dmrIDContact, 4U);
		dmrIDsCache.slices[0] = dmrIDContact.id;

		// Last available ID
		dmrIDReadContactInFlash((dmrIDsCache.contactLength * (dmrIDsCache.entries - 1)), (uint8_t *)&dmrIDContact, 4U);
		dmrIDsCache.slices[ID_SLICES - 1] = dmrIDContact.id;

		if (dmrIDsCache.entries > MIN_ENTRIES_BEFORE_USING_SLICES)
		{
			dmrIDsCache.IDsPerSlice = dmrIDsCache.entries / (ID_SLICES - 1);

			for (uint8_t i = 0; i < (ID_SLICES - 2); i++)
			{
				dmrIDReadContactInFlash((dmrIDsCache.contactLength * ((dmrIDsCache.IDsPerSlice * i) + dmrIDsCache.IDsPerSlice)), (uint8_t *)&dmrIDContact, 4U);
				dmrIDsCache.slices[i + 1] = dmrIDContact.id;
			}
		}
	}
}

bool dmrIDLookup(int targetId, dmrIdDataStruct_t *foundRecord)
{
	int targetIdBCD = int2bcd(targetId);

	if ((dmrIDsCache.entries > 0) && (targetIdBCD >= dmrIDsCache.slices[0]) && (targetIdBCD <= dmrIDsCache.slices[ID_SLICES - 1]))
	{
		uint32_t startPos = 0;
		uint32_t endPos = dmrIDsCache.entries - 1;
		uint32_t curPos;

		// Contact's text length == (dmrIDsCache.contactLength - 4U) aren't NULL terminated,
		// so clearing the whole destination array is mandatory
		memset(foundRecord->text, 0, sizeof(foundRecord->text));

		if (dmrIDsCache.entries > MIN_ENTRIES_BEFORE_USING_SLICES) // Use slices
		{
			for (uint8_t i = 0; i < ID_SLICES - 1; i++)
			{
				// Check if ID is in slices boundaries, with a special case for the last slice as [ID_SLICES - 1] is the last ID
				if ((targetIdBCD >= dmrIDsCache.slices[i]) &&
						((i == ID_SLICES - 2) ? (targetIdBCD <= dmrIDsCache.slices[i + 1]) : (targetIdBCD < dmrIDsCache.slices[i + 1])))
				{
					// targetID is the min slice limit, don't go further
					if (targetIdBCD == dmrIDsCache.slices[i])
					{
						foundRecord->id = dmrIDsCache.slices[i];
						dmrIDReadContactInFlash((dmrIDsCache.contactLength * (dmrIDsCache.IDsPerSlice * i)) + 4U, (uint8_t *)foundRecord + 4U, (dmrIDsCache.contactLength - 4U));

						return true;
					}

					startPos = dmrIDsCache.IDsPerSlice * i;
					endPos = (i == ID_SLICES - 2) ? (dmrIDsCache.entries - 1) : dmrIDsCache.IDsPerSlice * (i + 1);

					break;
				}
			}
		}
		else // Not enough contact to use slices
		{
			bool isMin;

			// Check if targetID is equal to the first or the last in the IDs list
			if ((isMin = (targetIdBCD == dmrIDsCache.slices[0])) || (targetIdBCD == dmrIDsCache.slices[ID_SLICES - 1]))
			{
				foundRecord->id = dmrIDsCache.slices[(isMin ? 0 : (ID_SLICES - 1))];
				dmrIDReadContactInFlash((dmrIDsCache.contactLength * (dmrIDsCache.IDsPerSlice * (isMin ? 0 : (ID_SLICES - 1)))) + 4U, (uint8_t *)foundRecord + 4U, (dmrIDsCache.contactLength - 4U));

				return true;
			}
		}

		// Look for the ID now
		while (startPos <= endPos)
		{
			curPos = (startPos + endPos) >> 1;

			dmrIDReadContactInFlash((dmrIDsCache.contactLength * curPos), (uint8_t *)foundRecord, 4U);

			if (foundRecord->id < targetIdBCD)
			{
				startPos = curPos + 1;
			}
			else
			{
				if (foundRecord->id > targetIdBCD)
				{
					endPos = curPos - 1;
				}
				else
				{
					dmrIDReadContactInFlash((dmrIDsCache.contactLength * curPos) + 4U, (uint8_t *)foundRecord + 4U, (dmrIDsCache.contactLength - 4U));
					return true;
				}
			}
		}
	}

	snprintf(foundRecord->text, 20, "ID:%d", targetId);
	return false;
}

bool contactIDLookup(uint32_t id, int calltype, char *buffer)
{
	struct_codeplugContact_t contact;
	int8_t manTS = tsGetManualOverrideFromCurrentChannel();

	int contactIndex = codeplugContactIndexByTGorPC((id & 0x00FFFFFF), calltype, &contact, (manTS ? manTS : (trxGetDMRTimeSlot() + 1)));
	if (contactIndex != -1)
	{
		codeplugUtilConvertBufToString(contact.name, buffer, 16);
		return true;
	}

	return false;
}

static void displayChannelNameOrRxFrequency(char *buffer, size_t maxLen)
{
	if (menuSystemGetCurrentMenuNumber() == UI_CHANNEL_MODE)
	{
		codeplugUtilConvertBufToString(currentChannelData->name, buffer, 16);
	}
	else
	{
		int val_before_dp = currentChannelData->rxFreq / 100000;
		int val_after_dp = currentChannelData->rxFreq - val_before_dp * 100000;
		snprintf(buffer, maxLen, "%d.%05d MHz", val_before_dp, val_after_dp);
	}

	uiUtilityDisplayInformation(buffer, DISPLAY_INFO_ZONE, -1);
}

static void displaySplitOrSpanText(uint8_t y, char *text)
{
	if (text != NULL)
	{
		uint8_t len = strlen(text);

		if (len == 0)
		{
			return;
		}
		else if (len <= 16)
		{
			ucPrintCentered(y, text, FONT_SIZE_3);
		}
		else
		{
			uint8_t nLines = len / 21 + (((len % 21) != 0) ? 1 : 0);

			if (nLines > 2)
			{
				nLines = 2;
				len = 42; // 2 lines max.
			}

			if (nLines > 1)
			{
				char buffer[43]; // 2 * 21 chars + NULL

				memcpy(buffer, text, len + 1);

				char *p = buffer + 20;

				// Find a space backward
				while ((*p != ' ') && (p > buffer))
				{
					p--;
				}

				uint8_t rest = (uint8_t)((buffer + strlen(buffer)) - p) - ((*p == ' ') ? 1 : 0);

				// rest is too long, just split the line in two chunks
				if (rest > 21)
				{
					char c = buffer[21];

					buffer[21] = 0;

					ucPrintCentered(y, chomp(buffer), FONT_SIZE_1); // 2 pixels are saved, could center

					buffer[21] = c;
					buffer[42] = 0;

					ucPrintCentered(y + 8, chomp(buffer + 21), FONT_SIZE_1);
				}
				else
				{
					*p = 0;

					ucPrintCentered(y, chomp(buffer), FONT_SIZE_1);
					ucPrintCentered(y + 8, chomp(p + 1), FONT_SIZE_1);
				}
			}
			else // One line of 21 chars max
			{
				ucPrintCentered(y
#if ! defined(PLATFORM_RD5R)
						+ 4
#endif
						, text, FONT_SIZE_1);
			}
		}
	}
}


/*
 * Try to extract callsign and extra text from TA or DMR ID data, then display that on
 * two lines, if possible.
 * We don't care if extra text is larger than 16 chars, ucPrint*() functions cut them.
 *.
 */
static void displayContactTextInfos(char *text, size_t maxLen, bool isFromTalkerAlias)
{
	char buffer[37]; // Max: TA 27 (in 7bit format) + ' [' + 6 (Maidenhead)  + ']' + NULL

	if (strlen(text) >= 5 && isFromTalkerAlias) // if it's Talker Alias and there is more text than just the callsign, split across 2 lines
	{
		char    *pbuf;
		int32_t  cpos;

		// User prefers to not span the TA info over two lines, check it that could fit
		if ((nonVolatileSettings.splitContact == SPLIT_CONTACT_SINGLE_LINE_ONLY) ||
				((nonVolatileSettings.splitContact == SPLIT_CONTACT_AUTO) && (strlen(text) <= 16)))
		{
			memcpy(buffer, text, 16);
			buffer[16] = 0;

			uiUtilityDisplayInformation(chomp(buffer), DISPLAY_INFO_CHANNEL, -1);
			displayChannelNameOrRxFrequency(buffer, (sizeof(buffer) / sizeof(buffer[0])));
			return;
		}

		if ((cpos = getFirstSpacePos(text)) != -1)
		{
			// Callsign found
			memcpy(buffer, text, cpos);
			buffer[cpos] = 0;

			uiUtilityDisplayInformation(chomp(buffer), DISPLAY_INFO_CHANNEL, -1);

			memcpy(buffer, text + (cpos + 1), (maxLen - (cpos + 1)));
			buffer[(strlen(text) - (cpos + 1))] = 0;

			pbuf = chomp(buffer);

			if (strlen(pbuf))
			{
				displaySplitOrSpanText(DISPLAY_Y_POS_CHANNEL_SECOND_LINE, pbuf);
			}
			else
			{
				displayChannelNameOrRxFrequency(buffer, (sizeof(buffer) / sizeof(buffer[0])));
			}
		}
		else
		{
			// No space found, use a chainsaw
			memcpy(buffer, text, 16);
			buffer[16] = 0;

			uiUtilityDisplayInformation(chomp(buffer), DISPLAY_INFO_CHANNEL, -1);

			memcpy(buffer, text + 16, (maxLen - 16));
			buffer[(strlen(text) - 16)] = 0;

			pbuf = chomp(buffer);

			if (strlen(pbuf))
			{
				displaySplitOrSpanText(DISPLAY_Y_POS_CHANNEL_SECOND_LINE, pbuf);
			}
			else
			{
				displayChannelNameOrRxFrequency(buffer, (sizeof(buffer) / sizeof(buffer[0])));
			}
		}
	}
	else
	{
		memcpy(buffer, text, 16);
		buffer[16] = 0;

		uiUtilityDisplayInformation(chomp(buffer), DISPLAY_INFO_CHANNEL, -1);
		displayChannelNameOrRxFrequency(buffer, (sizeof(buffer) / sizeof(buffer[0])));
	}
}

void uiUtilityDisplayInformation(const char *str, displayInformation_t line, int8_t yOverride)
{
	bool inverted = false;

	switch (line)
	{
		case DISPLAY_INFO_CONTACT_INVERTED:
#if defined(PLATFORM_RD5R)
			ucFillRect(0, DISPLAY_Y_POS_CONTACT + 1, DISPLAY_SIZE_X, MENU_ENTRY_HEIGHT, false);
#else
			ucClearRows(2, 4, true);
#endif
			inverted = true;
		case DISPLAY_INFO_CONTACT:
			ucPrintCore(0, ((yOverride == -1) ? (DISPLAY_Y_POS_CONTACT + V_OFFSET) : yOverride), str, FONT_SIZE_3, TEXT_ALIGN_CENTER, inverted);
			break;

		case DISPLAY_INFO_CONTACT_OVERRIDE_FRAME:
			ucDrawRect(0, ((yOverride == -1) ? DISPLAY_Y_POS_CONTACT : yOverride), DISPLAY_SIZE_X, OVERRIDE_FRAME_HEIGHT, true);
			break;

		case DISPLAY_INFO_CHANNEL:
			ucPrintCentered(((yOverride == -1) ? DISPLAY_Y_POS_CHANNEL_FIRST_LINE : yOverride), str, FONT_SIZE_3);
			break;

		case DISPLAY_INFO_SQUELCH:
			{
				static const int xbar = 74; // 128 - (51 /* max squelch px */ + 3);
				int sLen = (strlen(str) * 8);

				// Center squelch word between col0 and bargraph, if possible.
				ucPrintAt(0 + ((sLen) < xbar - 2 ? (((xbar - 2) - (sLen)) >> 1) : 0), DISPLAY_Y_POS_SQUELCH_BAR, str, FONT_SIZE_3);

				int bargraph = 1 + ((currentChannelData->sql - 1) * 5) / 2;
				ucDrawRect(xbar - 2, DISPLAY_Y_POS_SQUELCH_BAR, 55, SQUELCH_BAR_H + 4, true);
				ucFillRect(xbar, DISPLAY_Y_POS_SQUELCH_BAR + 2, bargraph, SQUELCH_BAR_H, false);
			}
			break;

		case DISPLAY_INFO_TONE_AND_SQUELCH:
			{
				char buf[24];
				int pos = 0;
				if (trxGetMode() == RADIO_MODE_ANALOG)
				{
					pos += snprintf(buf + pos, 24 - pos, "Rx:");
					if (codeplugChannelToneIsCTCSS(currentChannelData->rxTone))
					{
						pos += snprintf(buf + pos, 24 - pos, "%d.%dHz", currentChannelData->rxTone / 10 , currentChannelData->rxTone % 10);
					}
					else if (codeplugChannelToneIsDCS(currentChannelData->rxTone))
					{
						pos += snprintDCS(buf + pos, 24 - pos, currentChannelData->rxTone & 0777, (currentChannelData->rxTone & CODEPLUG_DCS_INVERTED_MASK));
					}
					else
					{
						pos += snprintf(buf + pos, 24 - pos, "%s", currentLanguage->none);
					}
					pos += snprintf(buf + pos, 24 - pos, "|Tx:");

					if (codeplugChannelToneIsCTCSS(currentChannelData->txTone))
					{
						pos += snprintf(buf + pos, 24 - pos, "%d.%dHz", currentChannelData->txTone / 10 , currentChannelData->txTone % 10);
					}
					else if (codeplugChannelToneIsDCS(currentChannelData->txTone))
					{
						pos += snprintDCS(buf + pos, 24 - pos, currentChannelData->txTone & 0777, (currentChannelData->txTone & CODEPLUG_DCS_INVERTED_MASK));
					}
					else
					{
						pos += snprintf(buf + pos, 24 - pos, "%s", currentLanguage->none);
					}

					ucPrintCentered(DISPLAY_Y_POS_CSS_INFO, buf, FONT_SIZE_1);
					snprintf(buf, 24, "SQL:%d%%", 5 * (((currentChannelData->sql == 0) ? nonVolatileSettings.squelchDefaults[trxCurrentBand[TRX_RX_FREQ_BAND]] : currentChannelData->sql)-1));
					ucPrintCentered(DISPLAY_Y_POS_SQL_INFO, buf, FONT_SIZE_1);
				}
			}
			break;

		case DISPLAY_INFO_SQUELCH_CLEAR_AREA:
#if defined(PLATFORM_RD5R)
			ucFillRect(0, DISPLAY_Y_POS_SQUELCH_BAR, DISPLAY_SIZE_X, 9, true);
#else
			ucClearRows(2, 4, false);
#endif
			break;

		case DISPLAY_INFO_TX_TIMER:
			ucPrintCentered(DISPLAY_Y_POS_TX_TIMER, str, FONT_SIZE_4);
			break;

		case DISPLAY_INFO_ZONE:
			ucPrintCentered(DISPLAY_Y_POS_ZONE, str, FONT_SIZE_1);
			break;
	}
}

void uiUtilityRenderQSODataAndUpdateScreen(void)
{
	if (isQSODataAvailableForCurrentTalker())
	{
		ucClearBuf();
		uiUtilityRenderHeader(false);
		uiUtilityRenderQSOData();
		ucRender();
	}
}

void uiUtilityRenderQSOData(void)
{
	uiDataGlobal.receivedPcId = 0x00; //reset the received PcId

	/*
	 * Note.
	 * When using Brandmeister reflectors. TalkGroups can be used to select reflectors e.g. TG 4009, and TG 5000 to check the connnection
	 * Under these conditions Brandmeister seems to respond with a message via a private call even if the command was sent as a TalkGroup,
	 * and this caused the Private Message acceptance system to operate.
	 * Brandmeister seems respond on the same ID as the keyed TG, so the code
	 * (LinkHead->id & 0xFFFFFF) != (trxTalkGroupOrPcId & 0xFFFFFF)  is designed to stop the Private call system tiggering in these instances
	 *
	 * FYI. Brandmeister seems to respond with a TG value of the users on ID number,
	 * but I thought it was safer to disregard any PC's from IDs the same as the current TG
	 * rather than testing if the TG is the user's ID, though that may work as well.
	 */
	if (HRC6000GetReceivedTgOrPcId() != 0)
	{
		if ((LinkHead->talkGroupOrPcId >> 24) == PC_CALL_FLAG) // &&  (LinkHead->id & 0xFFFFFF) != (trxTalkGroupOrPcId & 0xFFFFFF))
		{
			// Its a Private call
			ucPrintCentered(16, LinkHead->contact, FONT_SIZE_3);

			ucPrintCentered(DISPLAY_Y_POS_CHANNEL_FIRST_LINE, currentLanguage->private_call, FONT_SIZE_3);

			if (LinkHead->talkGroupOrPcId != (trxDMRID | (PC_CALL_FLAG << 24)))
			{
				uiUtilityDisplayInformation(LinkHead->talkgroup, DISPLAY_INFO_ZONE, -1);
				ucPrintAt(1, DISPLAY_Y_POS_ZONE, "=>", FONT_SIZE_1);
			}
		}
		else
		{
			// Group call
			bool different = (((LinkHead->talkGroupOrPcId & 0xFFFFFF) != trxTalkGroupOrPcId ) ||
					(((trxDMRModeRx != DMR_MODE_DMO) && (dmrMonitorCapturedTS != -1)) && (dmrMonitorCapturedTS != trxGetDMRTimeSlot())) ||
					(trxGetDMRColourCode() != currentChannelData->txColor));

			uiUtilityDisplayInformation(LinkHead->talkgroup, different ? DISPLAY_INFO_CONTACT_INVERTED : DISPLAY_INFO_CONTACT, -1);

			// If voice prompt feedback is enabled. Play a short beep to indicate the inverse video display showing the TG / TS / CC does not match the current Tx config
			if (different && nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_2)
			{
				soundSetMelody(MELODY_RX_TGTSCC_WARNING_BEEP);
			}

			switch (nonVolatileSettings.contactDisplayPriority)
			{
				case CONTACT_DISPLAY_PRIO_CC_DB_TA:
				case CONTACT_DISPLAY_PRIO_DB_CC_TA:
					// No contact found in codeplug and DMRIDs, use TA as fallback, if any.
					if ((strncmp(LinkHead->contact, "ID:", 3) == 0) && (LinkHead->talkerAlias[0] != 0x00))
					{
						if (LinkHead->locator[0] != 0)
						{
							char bufferTA[37]; // TA + ' [' + Maidenhead + ']' + NULL

							memset(bufferTA, 0, sizeof(bufferTA));
							snprintf(bufferTA, 37, "%s [%s]", LinkHead->talkerAlias, LinkHead->locator);
							displayContactTextInfos(bufferTA, sizeof(bufferTA), true);
						}
						else
						{
							displayContactTextInfos(LinkHead->talkerAlias, sizeof(LinkHead->talkerAlias), !(nonVolatileSettings.splitContact == SPLIT_CONTACT_SINGLE_LINE_ONLY));
						}
					}
					else
					{
						displayContactTextInfos(LinkHead->contact, sizeof(LinkHead->contact), !(nonVolatileSettings.splitContact == SPLIT_CONTACT_SINGLE_LINE_ONLY));
					}
					break;

				case CONTACT_DISPLAY_PRIO_TA_CC_DB:
				case CONTACT_DISPLAY_PRIO_TA_DB_CC:
					// Talker Alias have the priority here
					if (LinkHead->talkerAlias[0] != 0x00)
					{
						if (LinkHead->locator[0] != 0)
						{
							char bufferTA[37]; // TA + ' [' + Maidenhead + ']' + NULL

							memset(bufferTA, 0, sizeof(bufferTA));
							snprintf(bufferTA, 37, "%s [%s]", LinkHead->talkerAlias, LinkHead->locator);
							displayContactTextInfos(bufferTA, sizeof(bufferTA), true);
						}
						else
						{
							displayContactTextInfos(LinkHead->talkerAlias, sizeof(LinkHead->talkerAlias), !(nonVolatileSettings.splitContact == SPLIT_CONTACT_SINGLE_LINE_ONLY));
						}
					}
					else // No TA, then use the one extracted from Codeplug or DMRIdDB
					{
						displayContactTextInfos(LinkHead->contact, sizeof(LinkHead->contact), !(nonVolatileSettings.splitContact == SPLIT_CONTACT_SINGLE_LINE_ONLY));
					}
					break;
			}
		}
	}
}

void uiUtilityRenderHeader(bool isVFODualWatchScanning)
{
	const int MODE_TEXT_X_OFFSET = 1;
	const int FILTER_TEXT_X_OFFSET = 25;
	static const int bufferLen = 17;
	char buffer[bufferLen];
	static bool scanBlinkPhase = true;
	static uint32_t blinkTime = 0;
	int powerLevel = trxGetPowerLevel();
	bool isPerChannelPower = (currentChannelData->libreDMR_Power != 0x00);

	if (isVFODualWatchScanning == false)
	{
		if (!trxTransmissionEnabled)
		{
			uiUtilityDrawRSSIBarGraph();
		}
		else
		{
			if (trxGetMode() == RADIO_MODE_DIGITAL)
			{
				uiUtilityDrawDMRMicLevelBarGraph();
			}
		}
	}

	if (uiDataGlobal.Scan.active || uiDataGlobal.Scan.toneActive)
	{
		int blinkPeriod = 1000;
		if (scanBlinkPhase)
		{
			blinkPeriod = 500;
		}

		if ((fw_millis() - blinkTime) > blinkPeriod)
		{
			blinkTime = fw_millis();
			scanBlinkPhase = !scanBlinkPhase;
		}
	}
	else
	{
		scanBlinkPhase = false;
	}

	switch(trxGetMode())
	{
		case RADIO_MODE_ANALOG:
			if (isVFODualWatchScanning)
			{
				strcpy(buffer, "[DW]");
			}
			else
			{
				strcpy(buffer, "FM");
				if (!trxGetBandwidthIs25kHz())
				{
					strcat(buffer,"N");
				}
			}

			ucPrintCore(MODE_TEXT_X_OFFSET, DISPLAY_Y_POS_HEADER, buffer, ((nonVolatileSettings.hotspotType != HOTSPOT_TYPE_OFF) ? FONT_SIZE_1_BOLD : FONT_SIZE_1), TEXT_ALIGN_LEFT, scanBlinkPhase);

			if ((monitorModeData.isEnabled == false) &&
					((currentChannelData->txTone != CODEPLUG_CSS_NONE) || (currentChannelData->rxTone != CODEPLUG_CSS_NONE)))
			{
				bool cssTextInverted = (trxGetAnalogFilterLevel() == ANALOG_FILTER_NONE);//(nonVolatileSettings.analogFilterLevel == ANALOG_FILTER_NONE);

				if (currentChannelData->txTone != CODEPLUG_CSS_NONE)
				{
					strcpy(buffer, (codeplugChannelToneIsDCS(currentChannelData->txTone) ? "DT" : "CT"));
				}
				else // tx and/or rx tones are enabled, no need to check for this
				{
					strcpy(buffer, (codeplugChannelToneIsDCS(currentChannelData->rxTone) ? "D" : "C"));
				}

				// There is no room to display if rxTone is CTCSS or DCS, when txTone is set.
				if (currentChannelData->rxTone != CODEPLUG_CSS_NONE)
				{
					strcat(buffer, "R");
				}

				if (cssTextInverted)
				{
					// Inverted rectangle width is fixed size, large enough to fit 3 characters
					ucFillRect((FILTER_TEXT_X_OFFSET - 2), DISPLAY_Y_POS_HEADER - 1, (18 + 3), 9, false);
				}

				// DCS chars are centered in their H space
				ucPrintCore((FILTER_TEXT_X_OFFSET + (9 /* halt of 3 chars */)) - ((strlen(buffer) * 6) >> 1),
						DISPLAY_Y_POS_HEADER, buffer, FONT_SIZE_1, TEXT_ALIGN_LEFT, cssTextInverted);
			}
			break;

		case RADIO_MODE_DIGITAL:
			if (settingsUsbMode != USB_MODE_HOTSPOT)
			{
				bool contactTSActive = false;
				bool tsManOverride = false;

				if (nonVolatileSettings.extendedInfosOnScreen & (INFO_ON_SCREEN_TS & INFO_ON_SCREEN_BOTH))
				{
					contactTSActive = ((nonVolatileSettings.overrideTG == 0) && ((currentContactData.reserve1 & 0x01) == 0x00));
					tsManOverride = (contactTSActive ? tsIsContactHasBeenOverriddenFromCurrentChannel() : (tsGetManualOverrideFromCurrentChannel() != 0));
				}

				if (isVFODualWatchScanning == false)
				{
					if (!scanBlinkPhase && (nonVolatileSettings.dmrDestinationFilter > DMR_DESTINATION_FILTER_NONE))
					{
						ucFillRect(0, DISPLAY_Y_POS_HEADER - 1, 20, 9, false);
					}
				}

				if (!scanBlinkPhase)
				{
					bool isInverted = isVFODualWatchScanning ? false : (scanBlinkPhase ^ (nonVolatileSettings.dmrDestinationFilter > DMR_DESTINATION_FILTER_NONE));
					ucPrintCore(MODE_TEXT_X_OFFSET, DISPLAY_Y_POS_HEADER, isVFODualWatchScanning ? "[DW]" : "DMR", ((nonVolatileSettings.hotspotType != HOTSPOT_TYPE_OFF) ? FONT_SIZE_1_BOLD : FONT_SIZE_1), TEXT_ALIGN_LEFT, isInverted);
				}

				if (isVFODualWatchScanning == false)
				{
					bool tsInverted = false;

					snprintf(buffer, bufferLen, "%s%d", contactTSActive ? "cS" : currentLanguage->ts, trxGetDMRTimeSlot() + 1);

					if (!(nonVolatileSettings.dmrCcTsFilter & DMR_TS_FILTER_PATTERN))
					{
						ucFillRect(FILTER_TEXT_X_OFFSET - 2, DISPLAY_Y_POS_HEADER - 1, 21, 9, false);
						tsInverted = true;
					}
					ucPrintCore(FILTER_TEXT_X_OFFSET, DISPLAY_Y_POS_HEADER, buffer, (tsManOverride ? FONT_SIZE_1_BOLD : FONT_SIZE_1), TEXT_ALIGN_LEFT, tsInverted);
				}
			}
			break;
	}

	sprintf(buffer,"%s%s", POWER_LEVELS[powerLevel], POWER_LEVEL_UNITS[powerLevel]);
	ucPrintCore(0, DISPLAY_Y_POS_HEADER, buffer,
			((isPerChannelPower && (nonVolatileSettings.extendedInfosOnScreen & (INFO_ON_SCREEN_PWR & INFO_ON_SCREEN_BOTH))) ? FONT_SIZE_1_BOLD : FONT_SIZE_1), TEXT_ALIGN_CENTER, false);

	// In hotspot mode the CC is show as part of the rest of the display and in Analog mode the CC is meaningless
	if((isVFODualWatchScanning == false) && (((settingsUsbMode == USB_MODE_HOTSPOT) || (trxGetMode() == RADIO_MODE_ANALOG)) == false))
	{
		const int COLOR_CODE_X_POSITION = 84;
		int ccode = trxGetDMRColourCode();
		bool isNotFilteringCC = !(nonVolatileSettings.dmrCcTsFilter & DMR_CC_FILTER_PATTERN);

		snprintf(buffer, bufferLen, "C%d", ccode);

		if (isNotFilteringCC)
		{
			ucFillRect(COLOR_CODE_X_POSITION - 1, DISPLAY_Y_POS_HEADER - 1, 13 + ((ccode > 9) * 6), 9, false);
		}

		ucPrintCore(COLOR_CODE_X_POSITION, DISPLAY_Y_POS_HEADER, buffer, FONT_SIZE_1, TEXT_ALIGN_LEFT, isNotFilteringCC);
	}

	// Display battery percentage/voltage
	if (nonVolatileSettings.bitfieldOptions & BIT_BATTERY_VOLTAGE_IN_HEADER)
	{
		int volts, mvolts;
		int16_t xV = (DISPLAY_SIZE_X - ((3 * 6) + 3));

		getBatteryVoltage(&volts, &mvolts);

		snprintf(buffer, bufferLen, "%1d", volts);
		ucPrintCore(xV, DISPLAY_Y_POS_HEADER, buffer, FONT_SIZE_1, TEXT_ALIGN_LEFT, false);

		ucDrawRect(xV + 6, DISPLAY_Y_POS_HEADER + 5, 2, 2, true);

		snprintf(buffer, bufferLen, "%1dV", mvolts);
		ucPrintCore(xV + 6 + 3, DISPLAY_Y_POS_HEADER, buffer, FONT_SIZE_1, TEXT_ALIGN_LEFT, false);
	}
	else
	{
		snprintf(buffer, bufferLen, "%d%%", getBatteryPercentage());
		ucPrintCore(0, DISPLAY_Y_POS_HEADER, buffer, FONT_SIZE_1, TEXT_ALIGN_RIGHT, false);// Display battery percentage at the right
	}
}

void uiUtilityRedrawHeaderOnly(bool isVFODualWatchScanning)
{
#if defined(PLATFORM_RD5R)
	ucClearRows(0, 1, false);
#else
	ucClearRows(0, 2, false);
#endif
	uiUtilityRenderHeader(isVFODualWatchScanning);
	ucRenderRows(0, 2);
}

int getRSSIdBm(void)
{
	int dBm = 0;

	if (trxCurrentBand[TRX_RX_FREQ_BAND] == RADIO_BAND_UHF)
	{
		// Use fixed point maths to scale the RSSI value to dBm, based on data from VK4JWT and VK7ZJA
		dBm = -151 + trxRxSignal;// Note no the RSSI value on UHF does not need to be scaled like it does on VHF
	}
	else
	{
		// VHF
		// Use fixed point maths to scale the RSSI value to dBm, based on data from VK4JWT and VK7ZJA
		dBm = -164 + ((trxRxSignal * 32) / 27);
	}

	return dBm;
}

static void drawHeaderBar(int *barWidth, int16_t barHeight)
{
	*barWidth = CLAMP(*barWidth, 0, DISPLAY_SIZE_X);

	if (*barWidth)
	{
		ucFillRect(0, DISPLAY_Y_POS_BAR, *barWidth, barHeight, false);
	}

	// Clear the end of the bar area, if needed
	if (*barWidth < DISPLAY_SIZE_X)
	{
		ucFillRect(*barWidth, DISPLAY_Y_POS_BAR, (DISPLAY_SIZE_X - *barWidth), barHeight, true);
	}
}

void uiUtilityDrawRSSIBarGraph(void)
{
	int rssi = getRSSIdBm();

	if ((rssi > SMETER_S9) && (trxGetMode() == RADIO_MODE_ANALOG))
	{
		// In Analog mode, the max RSSI value from the hardware is over S9+60.
		// So scale this to fit in the last 30% of the display
		rssi = ((rssi - SMETER_S9) / 5) + SMETER_S9;

		// in Digital mode. The maximum signal is around S9+10 dB.
		// So no scaling is required, as the full scale value is approximately S9+10dB
	}

	// Scale the entire bar by 2.
	// Because above S9 the values are scaled to 1/5. This results in the signal below S9 being doubled in scale
	// Signals above S9 the scales is compressed to 2/5.
	rssi = (rssi - SMETER_S0) * 2;

	int barWidth = ((rssi * rssiMeterHeaderBarNumUnits) / rssiMeterHeaderBarDivider);

	drawHeaderBar(&barWidth, 4);

#if 0 // Commented for now, maybe an option later.
	int xPos = 0;
	int currentMode = trxGetMode();
	for (uint8_t i = 1; ((i < 10) && (xPos <= barWidth)); i += 2)
	{
		if ((i <= 9) || (currentMode == RADIO_MODE_DIGITAL))
		{
			xPos = rssiMeterHeaderBar[i];
		}
		else
		{
			xPos = ((rssiMeterHeaderBar[i] - rssiMeterHeaderBar[9]) / 5) + rssiMeterHeaderBar[9];
		}
		xPos *= 2;

		ucDrawFastVLine(xPos, (DISPLAY_Y_POS_BAR + 1), 2, false);
	}
#endif
}

void uiUtilityDrawFMMicLevelBarGraph(void)
{
	trxReadVoxAndMicStrength();

	uint8_t micdB = (trxTxMic >> 1); // trxTxMic is in 0.5dB unit, displaying 50dB .. 100dB
	// display from 50dB to 100dB, span over 128pix
	int barWidth = ((uint16_t)(((float)DISPLAY_SIZE_X / 50.0) * ((float)micdB - 50.0)));
	drawHeaderBar(&barWidth, 3);
}

void uiUtilityDrawDMRMicLevelBarGraph(void)
{
	int barWidth = ((uint16_t)(sqrt(micAudioSamplesTotal) * 1.5));
	drawHeaderBar(&barWidth, 3);
}

void setOverrideTGorPC(int tgOrPc, bool privateCall)
{
	uiDataGlobal.tgBeforePcMode = 0;
	settingsSet(nonVolatileSettings.overrideTG, (uint32_t) tgOrPc);
	if (privateCall == true)
	{
		// Private Call

		if ((trxTalkGroupOrPcId >> 24) != PC_CALL_FLAG)
		{
			// if the current Tx TG is a TalkGroup then save it so it can be restored after the end of the private call
			uiDataGlobal.tgBeforePcMode = trxTalkGroupOrPcId;
		}
		settingsSet(nonVolatileSettings.overrideTG, (nonVolatileSettings.overrideTG | (PC_CALL_FLAG << 24)));
	}
}

void uiUtilityDisplayFrequency(uint8_t y, bool isTX, bool hasFocus, uint32_t frequency, bool displayVFOChannel, bool isScanMode, uint8_t dualWatchVFO)
{
	static const int bufferLen = 17;
	char buffer[bufferLen];
	int val_before_dp = frequency / 100000;
	int val_after_dp = frequency - val_before_dp * 100000;

	// Focus + direction
	snprintf(buffer, bufferLen, "%c%c", ((hasFocus && !isScanMode)? '>' : ' '), (isTX ? 'T' : 'R'));

	ucPrintAt(0, y, buffer, FONT_SIZE_3);
	// VFO
	if (displayVFOChannel)
	{
		ucPrintAt(16, y + VFO_LETTER_Y_OFFSET, (((dualWatchVFO == 0) && (nonVolatileSettings.currentVFONumber == 0)) || (dualWatchVFO == 1)) ? "A" : "B", FONT_SIZE_1);
	}
	// Frequency
	snprintf(buffer, bufferLen, "%d.%05d", val_before_dp, val_after_dp);
	ucPrintAt(FREQUENCY_X_POS, y, buffer, FONT_SIZE_3);
	ucPrintAt(DISPLAY_SIZE_X - (3 * 8), y, "MHz", FONT_SIZE_3);
}

size_t snprintDCS(char *s, size_t n, uint16_t code, bool inverted)
{
	return snprintf(s, n, "D%03o%c", code, (inverted ? 'I' : 'N'));
}

void freqEnterReset(void)
{
	memset(uiDataGlobal.FreqEnter.digits, '-', FREQ_ENTER_DIGITS_MAX);
	uiDataGlobal.FreqEnter.index = 0;
}

int freqEnterRead(int startDigit, int endDigit)
{
	int result = 0;

	if (((startDigit >= 0) && (startDigit <= FREQ_ENTER_DIGITS_MAX)) && ((endDigit >= 0) && (endDigit <= FREQ_ENTER_DIGITS_MAX)))
	{
		for (int i = startDigit; i < endDigit; i++)
		{
			result = result * 10;
			if ((uiDataGlobal.FreqEnter.digits[i] >= '0') && (uiDataGlobal.FreqEnter.digits[i] <= '9'))
			{
				result = result + uiDataGlobal.FreqEnter.digits[i] - '0';
			}
		}
	}

	return result;
}

int getBatteryPercentage(void)
{
	return SAFE_MAX(0, SAFE_MIN(((int)(((averageBatteryVoltage - CUTOFF_VOLTAGE_UPPER_HYST) * 100) / (BATTERY_MAX_VOLTAGE - CUTOFF_VOLTAGE_UPPER_HYST))), 100));
}

void getBatteryVoltage(int *volts, int *mvolts)
{
	*volts = (int)(averageBatteryVoltage / 10);
	*mvolts = (int)(averageBatteryVoltage - (*volts * 10));
}

bool increasePowerLevel(bool allowFullPower)
{
	bool powerHasChanged = false;

	if (currentChannelData->libreDMR_Power != 0x00)
	{
		if (currentChannelData->libreDMR_Power < (MAX_POWER_SETTING_NUM - 1 + CODEPLUG_MIN_PER_CHANNEL_POWER) + (allowFullPower?1:0))
		{
			currentChannelData->libreDMR_Power++;
			trxSetPowerFromLevel(currentChannelData->libreDMR_Power - 1);
			powerHasChanged = true;
		}
	}
	else
	{
		if (nonVolatileSettings.txPowerLevel < (MAX_POWER_SETTING_NUM - 1 + (allowFullPower?1:0)))
		{
			settingsIncrement(nonVolatileSettings.txPowerLevel, 1);
			trxSetPowerFromLevel(nonVolatileSettings.txPowerLevel);
			powerHasChanged = true;
		}
	}

	announceItem(PROMPT_SEQUENCE_POWER, PROMPT_THRESHOLD_3);

	return powerHasChanged;
}

bool decreasePowerLevel(void)
{
	bool powerHasChanged = false;

	if (currentChannelData->libreDMR_Power != 0x00)
	{
		if (currentChannelData->libreDMR_Power > CODEPLUG_MIN_PER_CHANNEL_POWER)
		{
			currentChannelData->libreDMR_Power--;
			trxSetPowerFromLevel(currentChannelData->libreDMR_Power - 1);
			powerHasChanged = true;
		}
	}
	else
	{
		if (nonVolatileSettings.txPowerLevel > 0)
		{
			settingsDecrement(nonVolatileSettings.txPowerLevel, 1);
			trxSetPowerFromLevel(nonVolatileSettings.txPowerLevel);
			powerHasChanged = true;
		}
	}

	announceItem(PROMPT_SEQUENCE_POWER, PROMPT_THRESHOLD_3);

	return powerHasChanged;
}

ANNOUNCE_STATIC void announceRadioMode(bool voicePromptWasPlaying)
{
	if (!voicePromptWasPlaying)
	{
		voicePromptsAppendLanguageString(&currentLanguage->mode);
	}
	voicePromptsAppendPrompt( (trxGetMode() == RADIO_MODE_DIGITAL) ? PROMPT_DMR : PROMPT_FM);
}

ANNOUNCE_STATIC void announceZoneName(bool voicePromptWasPlaying)
{
	if (!voicePromptWasPlaying)
	{
		voicePromptsAppendLanguageString(&currentLanguage->zone);
	}
	voicePromptsAppendString(currentZone.name);
}

ANNOUNCE_STATIC void announceContactNameTgOrPc(bool voicePromptWasPlaying)
{
	if (nonVolatileSettings.overrideTG == 0)
	{
		if (!voicePromptWasPlaying)
		{
			voicePromptsAppendLanguageString(&currentLanguage->contact);
		}
		char nameBuf[17];
		codeplugUtilConvertBufToString(currentContactData.name, nameBuf, 16);
		voicePromptsAppendString(nameBuf);
	}
	else
	{
		char buf[17];
		itoa(nonVolatileSettings.overrideTG & 0xFFFFFF, buf, 10);
		if ((nonVolatileSettings.overrideTG >> 24) == PC_CALL_FLAG)
		{
			if (!voicePromptWasPlaying)
			{
				voicePromptsAppendLanguageString(&currentLanguage->private_call);
			}
			voicePromptsAppendString("ID");
		}
		else
		{
			voicePromptsAppendPrompt(PROMPT_TALKGROUP);
		}
		voicePromptsAppendString(buf);
	}
}

ANNOUNCE_STATIC void announcePowerLevel(bool voicePromptWasPlaying)
{
	int powerLevel = trxGetPowerLevel();

	if (!voicePromptWasPlaying)
	{
		voicePromptsAppendPrompt(PROMPT_POWER);
	}

	if (powerLevel < 9)
	{
		voicePromptsAppendString((char *)POWER_LEVELS[powerLevel]);
		switch(powerLevel)
		{
			case 0://50mW
			case 1://250mW
			case 2://500mW
			case 3://750mW
				voicePromptsAppendPrompt(PROMPT_MILLIWATTS);
				break;
			case 4://1W
				voicePromptsAppendPrompt(PROMPT_WATT);
				break;
			default:
				voicePromptsAppendPrompt(PROMPT_WATTS);
			break;
		}
	}
	else
	{
		voicePromptsAppendLanguageString(&currentLanguage->user_power);
	}
}

ANNOUNCE_STATIC void announceTemperature(bool voicePromptWasPlaying)
{
	char buffer[17];
	int temperature = getTemperature();
	if (!voicePromptWasPlaying)
	{
		voicePromptsAppendLanguageString(&currentLanguage->temperature);
	}
	snprintf(buffer, 17, "%d.%1d", (temperature / 10), (temperature % 10));
	voicePromptsAppendString(buffer);
	voicePromptsAppendLanguageString(&currentLanguage->celcius);
}

ANNOUNCE_STATIC void announceBatteryVoltage(void)
{
	char buffer[17];
	int volts, mvolts;

	voicePromptsAppendLanguageString(&currentLanguage->battery);
	getBatteryVoltage(&volts,  &mvolts);
	snprintf(buffer, 17, " %1d.%1d", volts, mvolts);
	voicePromptsAppendString(buffer);
	voicePromptsAppendPrompt(PROMPT_VOLTS);
}

ANNOUNCE_STATIC void announceBatteryPercentage(void)
{
	voicePromptsAppendLanguageString(&currentLanguage->battery);
	voicePromptsAppendInteger(getBatteryPercentage());
	voicePromptsAppendPrompt(PROMPT_PERCENT);
}

ANNOUNCE_STATIC void announceTS(void)
{
	voicePromptsAppendPrompt(PROMPT_TIMESLOT);
	voicePromptsAppendInteger(trxGetDMRTimeSlot() + 1);
}

ANNOUNCE_STATIC void announceCC(void)
{
	voicePromptsAppendLanguageString(&currentLanguage->colour_code);
	voicePromptsAppendInteger(trxGetDMRColourCode());
}

ANNOUNCE_STATIC void announceChannelName(bool voicePromptWasPlaying)
{
	char voiceBuf[17];
	codeplugUtilConvertBufToString(channelScreenChannelData.name, voiceBuf, 16);

	if (!voicePromptWasPlaying)
	{
		voicePromptsAppendPrompt(PROMPT_CHANNEL);
	}

	voicePromptsAppendString(voiceBuf);
}

static void removeUnnecessaryZerosFromVoicePrompts(char *str)
{
	const int NUM_DECIMAL_PLACES = 1;
	int len = strlen(str);
	for(int i = len; i > 2; i--)
	{
		if ((str[i - 1] != '0') || (str[i - (NUM_DECIMAL_PLACES + 1)] == '.'))
		{
			str[i] = 0;
			return;
		}
	}
}

ANNOUNCE_STATIC void announceFrequency(void)
{
	char buffer[17];
	bool duplex = (currentChannelData->txFreq != currentChannelData->rxFreq);

	if (duplex)
	{
		voicePromptsAppendPrompt(PROMPT_RECEIVE);
	}
	int val_before_dp = currentChannelData->rxFreq / 100000;
	int val_after_dp = currentChannelData->rxFreq - val_before_dp * 100000;
	snprintf(buffer, 17, "%d.%05d", val_before_dp, val_after_dp);
	removeUnnecessaryZerosFromVoicePrompts(buffer);
	voicePromptsAppendString(buffer);
	voicePromptsAppendPrompt(PROMPT_MEGAHERTZ);

	if (duplex)
	{
		voicePromptsAppendPrompt(PROMPT_TRANSMIT);
		val_before_dp = currentChannelData->txFreq / 100000;
		val_after_dp = currentChannelData->txFreq - val_before_dp * 100000;
		snprintf(buffer, 17, "%d.%05d", val_before_dp, val_after_dp);
		removeUnnecessaryZerosFromVoicePrompts(buffer);
		voicePromptsAppendString(buffer);
		voicePromptsAppendPrompt(PROMPT_MEGAHERTZ);
	}
}

ANNOUNCE_STATIC void announceVFOChannelName(void)
{
	voicePromptsAppendPrompt(PROMPT_VFO);
	voicePromptsAppendString((nonVolatileSettings.currentVFONumber == 0) ? "A" : "B");
	voicePromptsAppendPrompt(PROMPT_SILENCE);
}

ANNOUNCE_STATIC void announceVFOAndFrequency(bool announceVFOName)
{
	if (announceVFOName)
	{
		announceVFOChannelName();
	}
	announceFrequency();
}

ANNOUNCE_STATIC void announceSquelchLevel(bool voicePromptWasPlaying)
{
	static const int BUFFER_LEN = 8;
	char buf[BUFFER_LEN];

	if (!voicePromptWasPlaying)
	{
		voicePromptsAppendLanguageString(&currentLanguage->squelch);
	}

	snprintf(buf, BUFFER_LEN, "%d%%", 5 * (((currentChannelData->sql == 0) ? nonVolatileSettings.squelchDefaults[trxCurrentBand[TRX_RX_FREQ_BAND]] : currentChannelData->sql)-1));
	voicePromptsAppendString(buf);
}

void announceChar(char ch)
{
	if (nonVolatileSettings.audioPromptMode < AUDIO_PROMPT_MODE_VOICE_LEVEL_1)
	{
		return;
	}

	char buf[2] = {ch, 0};

	voicePromptsInit();
	voicePromptsAppendString(buf);
	voicePromptsPlay();
}

void buildCSSCodeVoicePrompts(uint16_t code, CSSTypes_t cssType, Direction_t direction, bool announceType)
{
	static const int BUFFER_LEN = 6;
	char buf[BUFFER_LEN];

	switch(direction)
	{
		case DIRECTION_RECEIVE:
			voicePromptsAppendString("RX");
			break;
		case DIRECTION_TRANSMIT:
			voicePromptsAppendString("TX");
			break;
		default:
			break;
	}

	voicePromptsAppendPrompt(PROMPT_SILENCE);

	switch (cssType)
	{
		case CSS_NONE:
			voicePromptsAppendString("CSS");
			voicePromptsAppendPrompt(PROMPT_SILENCE);
			voicePromptsAppendLanguageString(&currentLanguage->none);
			break;
		case CSS_CTCSS:
			if (announceType)
			{
				voicePromptsAppendString("CTCSS");
				voicePromptsAppendPrompt(PROMPT_SILENCE);
			}
			snprintf(buf, BUFFER_LEN, "%d.%d", code / 10, code % 10);
			voicePromptsAppendString(buf);
			voicePromptsAppendPrompt(PROMPT_HERTZ);
			break;
		case CSS_DCS:
		case CSS_DCS_INVERTED:
			if (announceType)
			{
				voicePromptsAppendString("DCS");
				voicePromptsAppendPrompt(PROMPT_SILENCE);
			}
			snprintf(buf, BUFFER_LEN, "D%03o%c", code & 0777, (code & CODEPLUG_DCS_INVERTED_MASK) ? 'I' : 'N');
			voicePromptsAppendString(buf);
			break;
	}
}


void announceCSSCode(uint16_t code, CSSTypes_t cssType, Direction_t direction, bool announceType, audioPromptThreshold_t immediateAnnounceThreshold)
{
	if (nonVolatileSettings.audioPromptMode < AUDIO_PROMPT_MODE_VOICE_LEVEL_1)
	{
		return;
	}

	bool voicePromptWasPlaying = voicePromptsIsPlaying();

	voicePromptsInit();

	buildCSSCodeVoicePrompts(code, cssType, direction, announceType);

	// Follow-on when voicePromptWasPlaying is enabled on voice prompt level 2 and above
	// Prompts are voiced immediately on voice prompt level 3
	if ((voicePromptWasPlaying && (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_2)) ||
			(nonVolatileSettings.audioPromptMode >= immediateAnnounceThreshold))
	{
		voicePromptsPlay();
	}
}

void playNextSettingSequence(void)
{
	voicePromptSequenceState++;

	if (voicePromptSequenceState == NUM_PROMPT_SEQUENCES)
	{
		voicePromptSequenceState = 0;
	}

	announceItem(voicePromptSequenceState, PROMPT_THRESHOLD_3);
}

static void announceChannelNameOrVFOFrequency(bool voicePromptWasPlaying, bool announceVFOName)
{
	if (menuSystemGetCurrentMenuNumber() == UI_CHANNEL_MODE)
	{
		announceChannelName(voicePromptWasPlaying);
	}
	else
	{
		announceVFOAndFrequency(announceVFOName);
	}
}

void announceItem(voicePromptItem_t item, audioPromptThreshold_t immediateAnnounceThreshold)
{
	if (nonVolatileSettings.audioPromptMode < AUDIO_PROMPT_MODE_VOICE_LEVEL_1)
	{
		return;
	}
	bool voicePromptWasPlaying = voicePromptsIsPlaying();

	voicePromptSequenceState = item;

	voicePromptsInit();

	switch(voicePromptSequenceState)
	{
		case PROMPT_SEQUENCE_CHANNEL_NAME_OR_VFO_FREQ:
		case PROMPT_SEQUENCE_CHANNEL_NAME_OR_VFO_FREQ_AND_MODE:
		case PROMPT_SEQUENCE_CHANNEL_NAME_AND_CONTACT_OR_VFO_FREQ_AND_MODE:
		case PROMPT_SEQUENCE_VFO_FREQ_UPDATE:
			announceChannelNameOrVFOFrequency(voicePromptWasPlaying, (voicePromptSequenceState != PROMPT_SEQUENCE_VFO_FREQ_UPDATE));
			if (voicePromptSequenceState == PROMPT_SEQUENCE_CHANNEL_NAME_OR_VFO_FREQ)
			{
				break;
			}
			if (voicePromptSequenceState == PROMPT_SEQUENCE_VFO_FREQ_UPDATE)
			{
				announceVFOChannelName();
			}
			announceRadioMode(voicePromptWasPlaying);
			if (voicePromptSequenceState == PROMPT_SEQUENCE_CHANNEL_NAME_OR_VFO_FREQ_AND_MODE)
			{
				break;
			}
			if (trxGetMode() == RADIO_MODE_DIGITAL)
			{
				announceContactNameTgOrPc(false);// false = force the title "Contact" to be played to always separate the Channel name announcement from the Contact name
			}
			break;
		case PROMPT_SEQUENCE_ZONE:
			announceZoneName(voicePromptWasPlaying);
			break;
		case PROMPT_SEQUENCE_MODE:
			announceRadioMode(voicePromptWasPlaying);
			break;
		case PROMPT_SEQUENCE_CONTACT_TG_OR_PC:
			announceContactNameTgOrPc(voicePromptWasPlaying);
			break;
		case PROMPT_SEQUENCE_TS:
			announceTS();
			break;
		case PROMPT_SEQUENCE_CC:
			announceCC();
			break;
		case PROMPT_SEQUENCE_POWER:
			announcePowerLevel(voicePromptWasPlaying);
			break;
		case PROMPT_SEQUENCE_BATTERY:
			if (nonVolatileSettings.bitfieldOptions & BIT_BATTERY_VOLTAGE_IN_HEADER)
			{
				announceBatteryVoltage();
			}
			else
			{
				announceBatteryPercentage();
			}
			break;
		case PROMPT_SQUENCE_SQUELCH:
			announceSquelchLevel(voicePromptWasPlaying);
			break;
		case PROMPT_SEQUENCE_TEMPERATURE:
			announceTemperature(voicePromptWasPlaying);
			break;

		default:
			break;
	}
	// Follow-on when voicePromptWasPlaying is enabled on voice prompt level 2 and above
	// Prompts are voiced immediately on voice prompt level 3
	if ((voicePromptWasPlaying && (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_2)) ||
			(nonVolatileSettings.audioPromptMode >= immediateAnnounceThreshold))
	{
		voicePromptsPlay();
	}
}

void promptsPlayNotAfterTx(void)
{
	if (menuSystemGetPreviouslyPushedMenuNumber() != UI_TX_SCREEN)
	{
		voicePromptsPlay();
	}
}

void uiUtilityBuildTgOrPCDisplayName(char *nameBuf, int bufferLen)
{
	int contactIndex;
	struct_codeplugContact_t contact;
	uint32_t id = (trxTalkGroupOrPcId & 0x00FFFFFF);
	int8_t manTS = tsGetManualOverrideFromCurrentChannel();

	if ((trxTalkGroupOrPcId >> 24) == TG_CALL_FLAG)
	{
		contactIndex = codeplugContactIndexByTGorPC(id, CONTACT_CALLTYPE_TG, &contact, (manTS ? manTS : (trxGetDMRTimeSlot() + 1)));
		if (contactIndex == -1)
		{
			snprintf(nameBuf, bufferLen, "%s %d", currentLanguage->tg, (trxTalkGroupOrPcId & 0x00FFFFFF));
		}
		else
		{
			codeplugUtilConvertBufToString(contact.name, nameBuf, 16);
		}
	}
	else
	{
		contactIndex = codeplugContactIndexByTGorPC(id, CONTACT_CALLTYPE_PC, &contact, (manTS ? manTS : (trxGetDMRTimeSlot() + 1)));
		if (contactIndex == -1)
		{
			dmrIdDataStruct_t currentRec;
			if (dmrIDLookup(id, &currentRec))
			{
				strncpy(nameBuf, currentRec.text, bufferLen);
			}
			else
			{
				// check LastHeard for TA data.
				LinkItem_t *item = lastheardFindInList(id);
				if ((item != NULL) && (strlen(item->talkerAlias) != 0))
				{
					strncpy(nameBuf, item->talkerAlias, bufferLen);
				}
				else
				{
					snprintf(nameBuf, bufferLen, "ID:%d", id);
				}
			}
		}
		else
		{
			codeplugUtilConvertBufToString(contact.name, nameBuf, 16);
		}
	}
}

void acceptPrivateCall(int id, int timeslot)
{
	uiDataGlobal.PrivateCall.state = PRIVATE_CALL;
	uiDataGlobal.PrivateCall.lastID = (id & 0xffffff);
	uiDataGlobal.receivedPcId = 0x00;

	setOverrideTGorPC(uiDataGlobal.PrivateCall.lastID, true);

#if !defined(PLATFORM_GD77S)
	if (timeslot != trxGetDMRTimeSlot())
	{
		trxSetDMRTimeSlot(timeslot);
		tsSetManualOverride(((menuSystemGetRootMenuNumber() == UI_CHANNEL_MODE) ? CHANNEL_CHANNEL : (CHANNEL_VFO_A + nonVolatileSettings.currentVFONumber)), (timeslot + 1));
	}
#else
	(void)timeslot;
#endif


	announceItem(PROMPT_SEQUENCE_CONTACT_TG_OR_PC,PROMPT_THRESHOLD_3);
}

bool repeatVoicePromptOnSK1(uiEvent_t *ev)
{
	if (BUTTONCHECK_SHORTUP(ev, BUTTON_SK1) && (ev->keys.key == 0))
	{
		if (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1)
		{
			int currentMenu = menuSystemGetCurrentMenuNumber();

			if ((((currentMenu == UI_CHANNEL_MODE) && (uiDataGlobal.Scan.active && (uiDataGlobal.Scan.state != SCAN_PAUSED))) ||
					((currentMenu == UI_VFO_MODE) && ((uiDataGlobal.Scan.active && (uiDataGlobal.Scan.state != SCAN_PAUSED)) || uiDataGlobal.Scan.toneActive))) == false)
			{

				if (!voicePromptsIsPlaying())
				{
					voicePromptsPlay();
				}
				else
				{
					voicePromptsTerminate();
				}
			}
		}

		return true;
	}

	return false;
}

bool handleMonitorMode(uiEvent_t *ev)
{
	// Time by which a DMR signal should have been decoded, including locking on to a DMR signal on a different CC
	const int DMR_MODE_CC_DETECT_TIME_MS = 250;// Normally it seems to take about 125mS to detect DMR even if the CC is incorrect.

	if (monitorModeData.isEnabled)
	{
#if defined(PLATFORM_GD77S)
		// PLATFORM_GD77S
		if ((BUTTONCHECK_DOWN(ev, BUTTON_SK1) == false) || (BUTTONCHECK_DOWN(ev, BUTTON_SK2) == false) || BUTTONCHECK_DOWN(ev, BUTTON_ORANGE) || (ev->events & ROTARY_EVENT))
#else
		if ((BUTTONCHECK_DOWN(ev, BUTTON_SK2) == false) || (ev->keys.key != 0) || BUTTONCHECK_DOWN(ev, BUTTON_SK1)
#if !defined(PLATFORM_RD5R)
				|| BUTTONCHECK_DOWN(ev, BUTTON_ORANGE)
#endif
		)
#endif
		{
			bool wasRadioModeWasChanged = (trxGetMode() != monitorModeData.savedRadioMode);
			if (wasRadioModeWasChanged)
			{
				trxSetModeAndBandwidth(currentChannelData->chMode, ((currentChannelData->flag4 & 0x02) == 0x02));
				currentChannelData->sql = monitorModeData.savedSquelch;
			}
			switch (monitorModeData.savedRadioMode)
			{
				case RADIO_MODE_ANALOG:
					currentChannelData->sql = monitorModeData.savedSquelch;
					trxSetRxCSS(currentChannelData->rxTone);
					break;
				case RADIO_MODE_DIGITAL:
					nonVolatileSettings.dmrCcTsFilter = monitorModeData.savedDMRCcTsFilter;
					nonVolatileSettings.dmrDestinationFilter = monitorModeData.savedDMRDestinationFilter;
					trxSetDMRColourCode(monitorModeData.savedDMRCc);
					trxSetDMRTimeSlot(monitorModeData.savedDMRTs);
					break;
			}
			monitorModeData.isEnabled = false;
			headerRowIsDirty = true;
			return true;
		}
	}
	else
	{
#if defined(PLATFORM_GD77S)
		if (BUTTONCHECK_LONGDOWN(ev, BUTTON_SK1) && BUTTONCHECK_LONGDOWN(ev, BUTTON_SK2))
		{
			if (voicePromptsIsPlaying())
			{
				voicePromptsTerminate();
			}
#else
		if (BUTTONCHECK_EXTRALONGDOWN(ev, BUTTON_SK2))
		{
#endif
			monitorModeData.savedRadioMode = trxGetMode();
			monitorModeData.savedSquelch = currentChannelData->sql;

			switch (monitorModeData.savedRadioMode)
			{
				case RADIO_MODE_ANALOG:
					monitorModeData.savedSquelch = currentChannelData->sql;
					currentChannelData->sql = CODEPLUG_MIN_VARIABLE_SQUELCH;
					trxSetRxCSS(CODEPLUG_CSS_NONE);
					break;
				case RADIO_MODE_DIGITAL:
					monitorModeData.savedDMRCcTsFilter = nonVolatileSettings.dmrCcTsFilter;
					monitorModeData.savedDMRDestinationFilter = nonVolatileSettings.dmrDestinationFilter;
					monitorModeData.savedDMRCc = trxGetDMRColourCode();
					monitorModeData.savedDMRTs = trxGetDMRTimeSlot();

					// Temporary override DMR filtering
					nonVolatileSettings.dmrCcTsFilter = DMR_CCTS_FILTER_NONE;
					nonVolatileSettings.dmrDestinationFilter = DMR_DESTINATION_FILTER_NONE;
					monitorModeData.DMRTimeout = DMR_MODE_CC_DETECT_TIME_MS;
					break;
			}
			monitorModeData.isEnabled = true;
			headerRowIsDirty = true;
			return true;
		}
	}
	return false;
}

// Helper function that manages the returned value from the codeplug quickkey code
static bool setQuickkeyFunctionID(char key, uint16_t functionId, bool silent)
{
	if (
#if defined(PLATFORM_RD5R)
			// '5' is reserved for torch on RD-5R
			(key != '5') &&
#endif
			codeplugSetQuickkeyFunctionID(key, functionId))
	{
		if (silent == false)
		{
			nextKeyBeepMelody = (int *)MELODY_ACK_BEEP;
		}
		return true;
	}

	nextKeyBeepMelody = (int *)MELODY_ERROR_BEEP;
	return false;
}

void saveQuickkeyMenuIndex(char key, uint8_t menuId, uint8_t entryId, uint8_t function)
{
	uint16_t functionID;

	functionID = QUICKKEY_MENUVALUE(menuId, entryId, function);
	if (setQuickkeyFunctionID(key, functionID, false))
	{
		menuDataGlobal.menuOptionsTimeout = -1;// Flag to indicate that a QuickKey has just been set.
	}
}

void saveQuickkeyMenuLongValue(char key, uint8_t menuId, uint16_t entryId)
{
	uint16_t functionID;

	functionID = QUICKKEY_MENULONGVALUE(menuId, entryId);
	setQuickkeyFunctionID(key, functionID, ((menuId == 0) && (entryId == 0)));
}

void saveQuickkeyContactIndex(char key, uint16_t contactId)
{
	setQuickkeyFunctionID(key, QUICKKEY_CONTACTVALUE(contactId), false);
}

// Returns the index in either the CTCSS or DCS list of the tone (or closest match)
int cssIndex(uint16_t tone, CSSTypes_t type)
{
	switch (type)
	{
		case CSS_CTCSS:
			for (int i = 0; i < TRX_NUM_CTCSS; i++)
			{
				if (TRX_CTCSSTones[i] >= tone)
				{
					return i;
				}
			}
			break;
		case CSS_DCS:
		case CSS_DCS_INVERTED:
			tone &= 0777;
			for (int i = 0; i < TRX_NUM_DCS; i++)
			{
				if (TRX_DCSCodes[i] >= tone)
				{
					return i;
				}
			}
			break;
		case CSS_NONE:
			break;
	}
	return 0;
}

uint16_t cssGetTone(int32_t index, CSSTypes_t type)
{
	if (index >= 0)
	{
		switch (type)
		{
			case CSS_CTCSS:
				if (index < TRX_NUM_CTCSS)
				{
					return TRX_CTCSSTones[index];
				}
				break;
			case CSS_DCS:
				if (index < TRX_NUM_DCS)
				{
					return (TRX_DCSCodes[index] | 0x8000);
				}
				break;
			case CSS_DCS_INVERTED:
				if (index < TRX_NUM_DCS)
				{
					return (TRX_DCSCodes[index] | 0xC000);
				}
				break;
			case CSS_NONE:
				break;
		}
	}
	return TRX_CTCSSTones[0];
}

void cssIncrement(uint16_t *tone, int32_t *index, CSSTypes_t *type, bool loop, bool stayInCSSType)
{
	(*index)++;
	switch (*type)
	{
		case CSS_CTCSS:
			if (*index >= TRX_NUM_CTCSS)
			{
				if (stayInCSSType)
				{
					*index = 0;
				}
				else
				{
					*type = CSS_DCS;
					*index = 0;
					*tone = TRX_DCSCodes[*index] | 0x8000;
					return;
				}
			}
			*tone = TRX_CTCSSTones[*index];
			break;
		case CSS_DCS:
			if (*index >= TRX_NUM_DCS)
			{
				if (stayInCSSType)
				{
					*index = 0;
				}
				else
				{
					*type = CSS_DCS_INVERTED;
					*index = 0;
					*tone = TRX_DCSCodes[*index] | 0xC000;
					return;
				}
			}
			*tone = TRX_DCSCodes[*index] | 0x8000;
			break;
		case CSS_DCS_INVERTED:
			if (*index >= TRX_NUM_DCS)
			{
				if (stayInCSSType)
				{
					*index = 0;
				}
				else
				{
					if (loop)
					{
						*type = CSS_CTCSS;
						*index = 0;
						*tone = TRX_CTCSSTones[*index];
						return;
					}
					*index = TRX_NUM_DCS - 1;
				}
			}
			*tone = TRX_DCSCodes[*index] | 0xC000;
			break;
		case CSS_NONE:
			*type = CSS_CTCSS;
			*index = 0;
			*tone = TRX_CTCSSTones[*index];
			break;
	}
	return;
}

void cssDecrement(uint16_t *tone, int32_t *index, CSSTypes_t *type)
{
	(*index)--;
	switch (*type)
	{
		case CSS_CTCSS:
			if (*index < 0)
			{
				*type = CSS_NONE;
				*index = 0;
				*tone = CODEPLUG_CSS_NONE;
				return;
			}
			*tone = TRX_CTCSSTones[*index];
			break;
		case CSS_DCS:
			if (*index < 0)
			{
				*type = CSS_CTCSS;
				*index = TRX_NUM_CTCSS - 1;
				*tone = TRX_CTCSSTones[*index];
				return;
			}
			*tone = TRX_DCSCodes[*index] | 0x8000;
			break;
		case CSS_DCS_INVERTED:
			if (*index < 0)
			{
				*type = CSS_DCS;
				*index = (TRX_NUM_DCS - 1);
				*tone = TRX_DCSCodes[*index] | 0x8000;
				return;
			}
			*tone = TRX_DCSCodes[*index] | 0xC000;
			break;
		case CSS_NONE:
			*index = 0;
			*tone = CODEPLUG_CSS_NONE;
			break;
	}
}

bool uiShowQuickKeysChoices(char *buf, const int bufferLen, const char *menuTitle)
{
	bool settingOption = (menuDataGlobal.menuOptionsSetQuickkey != 0) || (menuDataGlobal.menuOptionsTimeout > 0);

	if (menuDataGlobal.menuOptionsSetQuickkey != 0)
	{
		snprintf(buf, bufferLen, "%s %c", currentLanguage->set_quickkey, menuDataGlobal.menuOptionsSetQuickkey);
		menuDisplayTitle(buf);
		ucDrawChoice(CHOICES_OKARROWS, true);

		if (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1)
		{
			voicePromptsInit();
			voicePromptsAppendLanguageString(&currentLanguage->set_quickkey);
			voicePromptsAppendPrompt(PROMPT_0 + (menuDataGlobal.menuOptionsSetQuickkey - '0'));
		}
	}
	else if (settingOption == false)
	{
		menuDisplayTitle(menuTitle);
	}

	return settingOption;
}


// --- DTMF contact list playback ---

static uint32_t dtmfGetToneDuration(uint32_t duration)
{
	bool starOrHash = ((uiDataGlobal.DTMFContactList.buffer[uiDataGlobal.DTMFContactList.poPtr] == 14) || (uiDataGlobal.DTMFContactList.buffer[uiDataGlobal.DTMFContactList.poPtr] == 15));

	/*
	 * https://www.sigidwiki.com/wiki/Dual_Tone_Multi_Frequency_(DTMF):
	 *    Standard Whelen timing is 40ms tone, 20ms space, where standard Motorola rate is 250ms tone, 250ms space.
	 *    Federal Signal ranges from 35ms tone 5ms space to 1000ms tone 1000ms space.
	 *    Genave Superfast rate is 20ms tone 20ms space. Genave claims their decoders can even respond to 20ms tone 5ms space.
	 *
	 *
	 * ETSI: https://www.etsi.org/deliver/etsi_es/201200_201299/20123502/01.01.01_60/es_20123502v010101p.pdf
	 * 4.2.4 Signal timing
	 *    4.2.4.1 Tone duration
	 *      Where the DTMF signalling tone duration is controlled automatically by the transmitter, the duration of any individual
	 *      DTMF tone combination sent shall not be less than 65 ms. The time shall be measured from the time when the tone
	 *      reaches 90 % of its steady-state value, until it has dropped to 90 % of its steady-state value.
	 *
	 *         NOTE: For correct operation of supplementary services such as SCWID (Spontaneous Call Waiting
	 *               Identification) and ADSI (Analogue Display Services Interface), DTMF tone bursts should not be longer
	 *               than 90 ms.
	 *
	 *    4.2.4.2 Pause duration
	 *       Where the DTMF signalling pause duration is controlled automatically by the transmitter the duration of the pause
	 *       between any individual DTMF tone combination shall not be less than 65 ms. The time shall be measured from the time
	 *       when the tone has dropped to 10 % of its steady-state value, until it has risen to 10 % of its steady-state value.
	 *
	 *         NOTE: In order to ensure correct reception of all the digits in a network address sequence, some networks may
	 *               require a sufficient pause after the last DTMF digit signalled and before normal transmission starts.
	 */

	// First digit
	if ((uiDataGlobal.DTMFContactList.poPtr == 0) && (uiDataGlobal.DTMFContactList.durations.fstDur > 0))
	{
		/*
		 * First digit duration:
		 *    - Example 1： "DTMF rate" is set to 10 digits per second (duration is 50 milliseconds).
		 *        The first digit time is set to 100 milliseconds. Thus, the actual length of the first digit duration is 150 milliseconds.
		 *        However, if the launch starts with a "*" or "#" tone, the intercom will compare the duration with "* and #" and whichever
		 *        is longer for both.
		 *    - Example 2： "DTMF rate" is set to 10 digits per second (duration is 50 milliseconds).
		 *        The first digit time is set to 100 milliseconds. "* And # tone" is set to 500 milliseconds.
		 *        Thus, the actual length of the first "*" or "#" tone is 550 milliseconds.
		 */
		return ((starOrHash ? (uiDataGlobal.DTMFContactList.durations.otherDur * 100) : (uiDataGlobal.DTMFContactList.durations.fstDur * 100)) + duration);
	}

	/*
	 * '*' '#' Duration:
	 *    - Example 1： "DTMF rate" is set to 10 digits per second (duration is 50 milliseconds).
	 *        "* And # tone" is set to 500 milliseconds. Thus, the actual length of "* and # sounds" is 550 milliseconds.
	 *        However, if the launch starts with * and # sounds, the intercom compares the duration of the pitch with
	 *        the "first digit time" and uses the longer one of the two.
	 *    - Example 2： "DTMF rate" is set to 10 digits per second (duration is 50 milliseconds).
	 *        The first digit time is set to 100 milliseconds. "* And # tone" is set to 500 milliseconds.
	 *        Therefore, the actual number of the first digit * or # is 550 milliseconds.
	 */
	return ((starOrHash ? (uiDataGlobal.DTMFContactList.durations.otherDur * 100) : 0) + duration);
}


static void dtmfProcess(void)
{
	if (uiDataGlobal.DTMFContactList.poLen == 0U)
	{
		return;
	}

	if (PITCounter > uiDataGlobal.DTMFContactList.nextPeriod)
	{
		uint32_t duration = (1000 / (uiDataGlobal.DTMFContactList.durations.rate * 2));

		if (uiDataGlobal.DTMFContactList.buffer[uiDataGlobal.DTMFContactList.poPtr] != 0xFFU)
		{
			// Set voice channel (and tone), accordingly to the next inTone state
			if (uiDataGlobal.DTMFContactList.inTone == false)
			{
				trxSetDTMF(uiDataGlobal.DTMFContactList.buffer[uiDataGlobal.DTMFContactList.poPtr]);
				trxSelectVoiceChannel(AT1846_VOICE_CHANNEL_DTMF);
			}
			else
			{
				trxSelectVoiceChannel(AT1846_VOICE_CHANNEL_NONE);
			}

			uiDataGlobal.DTMFContactList.inTone = !uiDataGlobal.DTMFContactList.inTone;
		}
		else
		{
			// Pause after last digit
			if (uiDataGlobal.DTMFContactList.inTone)
			{
				uiDataGlobal.DTMFContactList.inTone = false;
				trxSelectVoiceChannel(AT1846_VOICE_CHANNEL_NONE);
				uiDataGlobal.DTMFContactList.nextPeriod = PITCounter + ((duration + (uiDataGlobal.DTMFContactList.durations.libreDMR_Tail * 100)) * 10U);
				return;
			}
		}

		if (uiDataGlobal.DTMFContactList.inTone)
		{
			// Move forward in the sequence, set tone duration
			uiDataGlobal.DTMFContactList.nextPeriod = PITCounter + (dtmfGetToneDuration(duration) * 10U);
			uiDataGlobal.DTMFContactList.poPtr++;
		}
		else
		{
			// No next character, last iteration pause has already been processed.
			// Move the pointer (offset) beyond the end of the sequence (handled in the next statement)
			if (uiDataGlobal.DTMFContactList.buffer[uiDataGlobal.DTMFContactList.poPtr] == 0xFFU)
			{
				uiDataGlobal.DTMFContactList.poPtr++;
			}
			else
			{
				// Set pause time in-between tone duration
				uiDataGlobal.DTMFContactList.nextPeriod = PITCounter + (duration * 10U);
			}
		}

		if (uiDataGlobal.DTMFContactList.poPtr > uiDataGlobal.DTMFContactList.poLen)
		{
			uiDataGlobal.DTMFContactList.poPtr = 0U;
			uiDataGlobal.DTMFContactList.poLen = 0U;
		}
	}
}

void dtmfSequenceReset(void)
{
	uiDataGlobal.DTMFContactList.poLen = 0U;
	uiDataGlobal.DTMFContactList.poPtr = 0U;
	uiDataGlobal.DTMFContactList.isKeying = false;
}

bool dtmfSequenceIsKeying(void)
{
	return uiDataGlobal.DTMFContactList.isKeying;
}

void dtmfSequencePrepare(uint8_t *seq, bool autoStart)
{
	uint8_t len = 16U;

	dtmfSequenceReset();

	memcpy(uiDataGlobal.DTMFContactList.buffer, seq, 16);
	uiDataGlobal.DTMFContactList.buffer[16] = 0xFFU;

	// non empty
	if (uiDataGlobal.DTMFContactList.buffer[0] != 0xFFU)
	{
		// Find the sequence length
		for (uint8_t i = 0; i < 16; i++)
		{
			if (uiDataGlobal.DTMFContactList.buffer[i] == 0xFFU)
			{
				len = i;
				break;
			}
		}

		uiDataGlobal.DTMFContactList.poLen = len;
		uiDataGlobal.DTMFContactList.isKeying = (autoStart ? (len > 0) : false);
	}
}

void dtmfSequenceStart(void)
{
	if (uiDataGlobal.DTMFContactList.isKeying == false)
	{
		uiDataGlobal.DTMFContactList.isKeying = (uiDataGlobal.DTMFContactList.poLen > 0);
	}
}

void dtmfSequenceStop(void)
{
	uiDataGlobal.DTMFContactList.poLen = 0U;
}

void dtmfSequenceTick(bool popPreviousMenuOnEnding)
{
	if (uiDataGlobal.DTMFContactList.isKeying)
	{
		if (!trxTransmissionEnabled)
		{
			// Start TX DTMF, prepare for ANALOG
			if (trxGetMode() != RADIO_MODE_ANALOG)
			{
				trxSetModeAndBandwidth(RADIO_MODE_ANALOG, false);
				trxSetTxCSS(CODEPLUG_CSS_NONE);
			}

			trxEnableTransmission();

			trxSelectVoiceChannel(AT1846_VOICE_CHANNEL_NONE);
			enableAudioAmp(AUDIO_AMP_MODE_RF);
			GPIO_PinWrite(GPIO_RX_audio_mux, Pin_RX_audio_mux, 1);
			uiDataGlobal.DTMFContactList.inTone = false;
			uiDataGlobal.DTMFContactList.nextPeriod = PITCounter + ((uiDataGlobal.DTMFContactList.durations.fstDigitDly * 100) * 10U); // Sequence preamble
		}

		// DTMF has been TXed, restore DIGITAL/ANALOG
		if (uiDataGlobal.DTMFContactList.poLen == 0U)
		{
			trxDisableTransmission();

			if (trxTransmissionEnabled)
			{
				// Stop TXing;
				trxTransmissionEnabled = false;
				trxSetRX();
				LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);

				trxSelectVoiceChannel(AT1846_VOICE_CHANNEL_MIC);
				disableAudioAmp(AUDIO_AMP_MODE_RF);
				if (currentChannelData->chMode == RADIO_MODE_ANALOG)
				{
					trxSetModeAndBandwidth(currentChannelData->chMode, ((currentChannelData->flag4 & 0x02) == 0x02));
					trxSetTxCSS(currentChannelData->txTone);
				}
				else
				{
					trxSetModeAndBandwidth(currentChannelData->chMode, false);// bandwidth false = 12.5Khz as DMR uses 12.5kHz
					trxSetDMRColourCode(currentChannelData->txColor);
				}
			}

			uiDataGlobal.DTMFContactList.isKeying = false;

			if (popPreviousMenuOnEnding)
			{
				menuSystemPopPreviousMenu();
			}

			return;
		}

		if (uiDataGlobal.DTMFContactList.poLen > 0U)
		{
			dtmfProcess();
		}
	}
}

void resetOriginalSettingsData(void)
{
	originalNonVolatileSettings.magicNumber = 0xDEADBEEF;
}
