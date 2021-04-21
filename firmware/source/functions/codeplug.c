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

#include <stdint.h>
#include <stdio.h>
#include "functions/codeplug.h"
#include "hardware/EEPROM.h"
#include "hardware/SPI_Flash.h"
#include "functions/trx.h"
#include "usb/usb_com.h"
#include "user_interface/uiLocalisation.h"


const int CODEPLUG_ADDR_EX_ZONE_BASIC = 0x8000;
const int CODEPLUG_ADDR_EX_ZONE_INUSE_PACKED_DATA = 0x8010;
#define CODEPLUG_EX_ZONE_INUSE_PACKED_DATA_SIZE  32
const int CODEPLUG_ADDR_EX_ZONE_LIST = 0x8030;

const int CODEPLUG_ZONE_MAX_COUNT = 250;
const int CODEPLUG_ADDR_CHANNEL_EEPROM = 0x3790;
const int CODEPLUG_ADDR_CHANNEL_HEADER_EEPROM = 0x3780; // CODEPLUG_ADDR_CHANNEL_EEPROM - 16
const int CODEPLUG_ADDR_CHANNEL_FLASH = 0x7B1C0;
const int CODEPLUG_ADDR_CHANNEL_HEADER_FLASH = 0x7B1B0; // CODEPLUG_ADDR_CHANNEL_FLASH - 16

const int CODEPLUG_ADDR_SIGNALLING_DTMF = 0x1400;
const int CODEPLUG_ADDR_SIGNALLING_DTMF_DURATIONS = (CODEPLUG_ADDR_SIGNALLING_DTMF + 0x72); // offset to grab the DTMF durations
const int CODEPLUG_SIGNALLING_DTMF_DURATIONS_SIZE = 4;

const int CODEPLUG_ADDR_RX_GROUP_LEN = 0x8D620;  // 76 TG lists
const int CODEPLUG_ADDR_RX_GROUP = 0x8D6A0;//

const int CODEPLUG_ADDR_CONTACTS = 0x87620;

const int CODEPLUG_ADDR_DTMF_CONTACTS = 0x02f88;

const int CODEPLUG_ADDR_USER_DMRID = 0x00E8;
const int CODEPLUG_ADDR_USER_CALLSIGN = 0x00E0;

const int CODEPLUG_ADDR_GENERAL_SETTINGS = 0x00E0;

const int CODEPLUG_ADDR_BOOT_INTRO_SCREEN = 0x7518;// 0x01 = Chars 0x00 = Picture
const int CODEPLUG_ADDR_BOOT_PASSWORD_ENABLE= 0x7519;// 0x00 = password disabled 0x01 = password enable
const int CODEPLUG_ADDR_BOOT_PASSWORD_AREA = 0x751C;// Seems to be 3 bytes coded as BCD e.f. 0x12 0x34 0x56
const int CODEPLUG_BOOT_PASSWORD_LEN = 3;
const int CODEPLUG_ADDR_BOOT_LINE1 = 0x7540;
const int CODEPLUG_ADDR_BOOT_LINE2 = 0x7550;
const int CODEPLUG_ADDR_VFO_A_CHANNEL = 0x7590;

int codeplugChannelsPerZone = 16;

const int VFO_FREQ_STEP_TABLE[8] = {250,500,625,1000,1250,2500,3000,5000};

const int CODEPLUG_MAX_VARIABLE_SQUELCH = 21;
const int CODEPLUG_MIN_VARIABLE_SQUELCH = 1;

const int CODEPLUG_MIN_PER_CHANNEL_POWER  = 1;

const int CODEPLUG_ADDR_DEVICE_INFO = 0x80;
const int CODEPLUG_ADDR_DEVICE_INFO_READ_SIZE = 96;// (sizeof struct_codeplugDeviceInfo_t)

const int CODEPLUG_ADDR_BOOT_PASSWORD_PIN = 0x7518;

static uint16_t allChannelsTotalNumOfChannels = 0;
static uint16_t allChannelsHighestChannelIndex = 0;

typedef struct
{
	uint32_t tgOrPCNum;
	uint16_t index;
} codeplugContactCache_t;


typedef struct
{
	uint8_t index;
} codeplugDTMFContactCache_t;

typedef struct
{
	int numTGContacts;
	int numPCContacts;
	int numALLContacts;
	int numDTMFContacts;
	codeplugContactCache_t contactsLookupCache[CODEPLUG_CONTACTS_MAX];
	codeplugDTMFContactCache_t contactsDTMFLookupCache[CODEPLUG_DTMF_CONTACTS_MAX];
} codeplugContactsCache_t;

__attribute__((section(".data.$RAM2"))) codeplugContactsCache_t codeplugContactsCache;

__attribute__((section(".data.$RAM2"))) uint8_t codeplugRXGroupCache[CODEPLUG_RX_GROUPLIST_MAX];
__attribute__((section(".data.$RAM2"))) uint8_t codeplugAllChannelsCache[128];
__attribute__((section(".data.$RAM2"))) uint8_t codeplugZonesInUseCache[CODEPLUG_EX_ZONE_INUSE_PACKED_DATA_SIZE];
__attribute__((section(".data.$RAM2"))) uint16_t quickKeysCache[CODEPLUG_QUICKKEYS_SIZE];


static bool codeplugContactGetReserve1ByteForIndex(int index, struct_codeplugContact_t *contact);

uint32_t byteSwap32(uint32_t n)
{
    return ((((n) & 0x000000FFU) << 24U) | (((n) & 0x0000FF00U) << 8U) | (((n) & 0x00FF0000U) >> 8U) | (((n) & 0xFF000000U) >> 24U));// from usb_misc.h
}

uint32_t byteSwap16(uint16_t n)
{
    return  (((n) & 0x00FFU) << 8U) | (((n) & 0xFF00U) >> 8U);
}

// BCD encoding to integer conversion
uint32_t bcd2int(uint32_t i)
{
    int result = 0;
    int multiplier = 1;
    while (i)
    {
        result += (i & 0x0f) * multiplier;
        multiplier *= 10;
        i = i >> 4;
    }
    return result;
}

// Needed to convert the legacy DMR ID data which uses BCD encoding for the DMR ID numbers
int int2bcd(int i)
{
    int result = 0;
    int shift = 0;

    while (i)
    {
        result +=  (i % 10) << shift;
        i = i / 10;
        shift += 4;
    }
    return result;
}

// Binary-coded Octal encoding to integer conversion
// DCS codes are stored in the codeplug as binary-coded octal
uint16_t bco2int(uint16_t i)
{
    uint16_t result = 0;
    uint16_t multiplier = 1;
    while (i)
    {
        result += (i & 0x0f) * multiplier;
        multiplier *= 8;
        i = i >> 4;
    }
    return result;
}

uint16_t int2bco(uint16_t i)
{
    uint16_t result = 0;
    uint16_t shift = 0;

    while (i)
    {
        result += (i % 8) << shift;
        i = i / 8;
        shift += 4;
    }
    return result;
}

uint16_t bcd2uint16(uint16_t i)
{
    int result = 0;
    int multiplier = 1;
    while (i)
    {
        result += (i & 0x0f) * multiplier;
        multiplier *= 10;
        i = i >> 4;
    }
    return result;
}

bool codeplugChannelToneIsCTCSS(uint16_t tone)
{
	return ((tone != CODEPLUG_CSS_NONE) && !(tone & CODEPLUG_DCS_FLAGS_MASK));
}

bool codeplugChannelToneIsDCS(uint16_t tone)
{
	// Could be improved to validate the rest of the field
	return ((tone != CODEPLUG_CSS_NONE) && (tone & CODEPLUG_DCS_FLAGS_MASK));
}

// Converts a codeplug coded-squelch system value to int (with flags if DCS)
uint16_t codeplugCSSToInt(uint16_t css)
{
	if (codeplugChannelToneIsCTCSS(css))
	{
		return bcd2int(css);
	}
	else if (codeplugChannelToneIsDCS(css))
	{
		return bco2int(css & ~CODEPLUG_DCS_FLAGS_MASK) | (css & CODEPLUG_DCS_FLAGS_MASK);
	}
	return css;
}

// Converts an int (with flags if DCS) to codeplug coded-squelch system value
uint16_t codeplugIntToCSS(uint16_t i)
{
	if (codeplugChannelToneIsCTCSS(i))
	{
		return int2bcd(i);
	}
	else if (codeplugChannelToneIsDCS(i))
	{
		return int2bco(i & ~CODEPLUG_DCS_FLAGS_MASK) | (i & CODEPLUG_DCS_FLAGS_MASK);
	}
	return i;
}

void codeplugUtilConvertBufToString(char *codeplugBuf, char *outBuf, int len)
{
	for(int i = 0; i < len; i++)
	{
		if (codeplugBuf[i] == 0xff)
		{
			codeplugBuf[i] = 0;
		}
		outBuf[i] = codeplugBuf[i];
	}
	outBuf[len] = 0;
}

void codeplugUtilConvertStringToBuf(char *inBuf, char *outBuf, int len)
{
	memset(outBuf,0xff,len);
	for (int i = 0; i < len; i++)
	{
		if (inBuf[i] == 0x00)
		{
			break;
		}
		outBuf[i] = inBuf[i];
	}
}

static void codeplugZonesInitCache(void)
{
	EEPROM_Read(CODEPLUG_ADDR_EX_ZONE_INUSE_PACKED_DATA, (uint8_t *)&codeplugZonesInUseCache, CODEPLUG_EX_ZONE_INUSE_PACKED_DATA_SIZE);
}

int codeplugZonesGetCount(void)
{
	int numZones = 1;// Add one extra zone to allow for the special 'All Channels' Zone

	for(int i = 0; i < CODEPLUG_EX_ZONE_INUSE_PACKED_DATA_SIZE; i++)
	{
		numZones += __builtin_popcount(codeplugZonesInUseCache[i]);
	}

	return numZones;
}

bool codeplugZoneGetDataForNumber(int zoneNum, struct_codeplugZone_t *returnBuf)
{
	if (zoneNum == (codeplugZonesGetCount() - 1)) //special case: return a special Zone called 'All Channels'
	{
		int nameLen = SAFE_MIN(((int)sizeof(returnBuf->name)), ((int)strlen(currentLanguage->all_channels)));

		// Codeplug name is 0xff filled, codeplugUtilConvertBufToString() handles the conversion
		memset(returnBuf->name, 0xff, sizeof(returnBuf->name));
		memcpy(returnBuf->name, currentLanguage->all_channels, nameLen);

		// set all channels to zero, All Channels is handled separately
		memset(returnBuf->channels, 0, codeplugChannelsPerZone);

		returnBuf->NOT_IN_CODEPLUGDATA_numChannelsInZone = allChannelsTotalNumOfChannels;
		returnBuf->NOT_IN_CODEPLUGDATA_highestIndex = allChannelsHighestChannelIndex;
		returnBuf->NOT_IN_CODEPLUGDATA_indexNumber = -1;// Set as -1 as this is not a real zone. Its the "All Channels" zone
		return true;
	}
	else
	{
		// Need to find the index into the Zones data for the specific Zone number.
		// Because the Zones data is not guaranteed to be packed by the CPS (though we should attempt to make the CPS always pack the Zones)
		int count = -1;// Need to start counting at -1 because the Zone number is zero indexed
		int foundIndex = -1;

		// Go though each byte in the In Use table
		for(int i = 0; i < CODEPLUG_EX_ZONE_INUSE_PACKED_DATA_SIZE; i++)
		{
			// Go though each binary bit, counting them one by one
			for(int j = 0; j < 8; j++)
			{
				if (((codeplugZonesInUseCache[i] >> j) & 0x01) == 0x01)
				{
					count++;

					if (count == zoneNum)
					{
						// found it. So save the index before we exit the "for" loops
						foundIndex = (i * 8) + j;
						break;// Will break out of this loop, but the outer loop breaks becuase it also checks for foundIndex
					}
				}
			}
		}

		if (foundIndex != -1)
		{
			// Save this in case we need to add channels to a zone and hence need the index number so it can be saved back to the codeplug memory
			returnBuf->NOT_IN_CODEPLUGDATA_indexNumber = foundIndex;

			// IMPORTANT. Write size is different from the size of the data, because it the zone struct contains properties not in the codeplug data
			EEPROM_Read(CODEPLUG_ADDR_EX_ZONE_LIST + (foundIndex * (16 + (sizeof(uint16_t) * codeplugChannelsPerZone))),
					(uint8_t *)returnBuf, ((codeplugChannelsPerZone == 16) ? CODEPLUG_ZONE_DATA_ORIGINAL_STRUCT_SIZE : CODEPLUG_ZONE_DATA_OPENGD77_STRUCT_SIZE));


			for(int i = 0; i < codeplugChannelsPerZone; i++)
			{
				// Empty channels seem to be filled with zeros, and zone could be full of channels.
				if ((returnBuf->channels[i] == 0) || (i == (codeplugChannelsPerZone - 1)))
				{
					returnBuf->NOT_IN_CODEPLUGDATA_highestIndex = returnBuf->NOT_IN_CODEPLUGDATA_numChannelsInZone = (i + ((returnBuf->channels[i] == 0) ? 0 : 1));
					return true;
				}
			}
		}

		memset(returnBuf->channels, 0, codeplugChannelsPerZone);
		returnBuf->NOT_IN_CODEPLUGDATA_highestIndex = returnBuf->NOT_IN_CODEPLUGDATA_numChannelsInZone = 0;
		returnBuf->NOT_IN_CODEPLUGDATA_indexNumber = -2; // we could not use '-1' on error, as -1 is All Channel zone
	}

	return false;
}

bool codeplugZoneAddChannelToZoneAndSave(int channelIndex, struct_codeplugZone_t *zoneBuf)
{
	if ((zoneBuf->NOT_IN_CODEPLUGDATA_numChannelsInZone <= codeplugChannelsPerZone) && (zoneBuf->NOT_IN_CODEPLUGDATA_indexNumber != -1))
	{
		zoneBuf->channels[zoneBuf->NOT_IN_CODEPLUGDATA_numChannelsInZone++] = channelIndex;// add channel to zone, and increment numb channels in zone
		zoneBuf->NOT_IN_CODEPLUGDATA_highestIndex = zoneBuf->NOT_IN_CODEPLUGDATA_numChannelsInZone;

		// IMPORTANT. Write size is different from the size of the data, because it the zone struct contains properties not in the codeplug data
		return EEPROM_Write(CODEPLUG_ADDR_EX_ZONE_LIST + (zoneBuf->NOT_IN_CODEPLUGDATA_indexNumber * (16 + (sizeof(uint16_t) * codeplugChannelsPerZone))),
				(uint8_t *)zoneBuf, ((codeplugChannelsPerZone == 16) ? CODEPLUG_ZONE_DATA_ORIGINAL_STRUCT_SIZE : CODEPLUG_ZONE_DATA_OPENGD77_STRUCT_SIZE));
	}
	else
	{
		return false;
	}
}

static uint16_t codeplugAllChannelsGetCount(void)
{
	uint16_t c = 0;

	//                         v-- This is a bit verbose, but it does make it clear
	for (uint16_t i = 0; i < ((CODEPLUG_CHANNELS_BANKS_MAX * CODEPLUG_CHANNELS_PER_BANK) / 8); i++)
	{
		c += __builtin_popcount(codeplugAllChannelsCache[i]);
	}

	return c;
}

static bool codeplugAllChannelsReadHeaderBank(int channelBank, uint8_t *bitArray)
{
	if(channelBank == 0)
	{
		return EEPROM_Read(CODEPLUG_ADDR_CHANNEL_HEADER_EEPROM, bitArray, 16);
	}

	return SPI_Flash_read(CODEPLUG_ADDR_CHANNEL_HEADER_FLASH + ((channelBank - 1) *
			(CODEPLUG_CHANNELS_PER_BANK * CODEPLUG_CHANNEL_DATA_STRUCT_SIZE + 16)), bitArray, 16);
}

bool codeplugAllChannelsIndexIsInUse(int index)
{
	if ((index >= CODEPLUG_CHANNELS_MIN) && (index <= CODEPLUG_CHANNELS_MAX))
	{
		index--;
		return (((codeplugAllChannelsCache[index / 8] >> (index % 8)) & 0x01) != 0);
	}

	return false;
}

void codeplugAllChannelsIndexSetUsed(int index)
{
	if ((index >= CODEPLUG_CHANNELS_MIN) && (index <= CODEPLUG_CHANNELS_MAX))
	{
		index--;
		int channelBank = (index / CODEPLUG_CHANNELS_PER_BANK);
		int byteno = (index % CODEPLUG_CHANNELS_PER_BANK) / 8;
		int cacheOffset = index / 8;

		codeplugAllChannelsCache[cacheOffset] |= (1 << (index % 8));

		if(channelBank == 0)
		{
			EEPROM_Write(CODEPLUG_ADDR_CHANNEL_HEADER_EEPROM + byteno, &codeplugAllChannelsCache[cacheOffset], 1);
		}
		else
		{
			SPI_Flash_write((CODEPLUG_ADDR_CHANNEL_HEADER_FLASH + ((channelBank - 1) *
					(CODEPLUG_CHANNELS_PER_BANK * CODEPLUG_CHANNEL_DATA_STRUCT_SIZE + 16))) + byteno, &codeplugAllChannelsCache[cacheOffset], 1);
		}

		allChannelsTotalNumOfChannels++;
		if ((index + 1) > allChannelsHighestChannelIndex)
		{
			allChannelsHighestChannelIndex = (index + 1);
		}
	}
}

void codeplugAllChannelsInitCache(void)
{
	// There are 8 banks
	for (uint16_t bank = 0; bank < CODEPLUG_CHANNELS_BANKS_MAX; bank++)
	{
		// Of 128 channels
		codeplugAllChannelsReadHeaderBank(bank, &codeplugAllChannelsCache[bank * 16]);
	}

	allChannelsHighestChannelIndex = 0;
	for (uint16_t index = CODEPLUG_CHANNELS_MAX; index >= CODEPLUG_CHANNELS_MIN ; index--)
	{
		if (codeplugAllChannelsIndexIsInUse(index))
		{
			allChannelsHighestChannelIndex = index;
			break;
		}
	}
}

void codeplugChannelGetDataWithOffsetAndLengthForIndex(int index, struct_codeplugChannel_t *channelBuf, uint8_t offset, int length)
{
	// lower 128 channels are in EEPROM. Remaining channels are in Flash ! (What a mess...)
	index--; // I think the channel index numbers start from 1 not zero.
	if (index < 128)
	{
		EEPROM_Read((CODEPLUG_ADDR_CHANNEL_EEPROM + index * CODEPLUG_CHANNEL_DATA_STRUCT_SIZE) + offset, ((uint8_t *)channelBuf) + offset, length);
	}
	else
	{
		int flashReadPos = CODEPLUG_ADDR_CHANNEL_FLASH;

		index -= 128;// First 128 channels are in the EEPROM, so subtract 128 from the number when looking in the Flash

		// Every 128 bytes there seem to be 16 bytes gaps. I don't know why,bits since 16*8 = 128 bits, its likely they are flag bytes to indicate which channel in the next block are in use
		flashReadPos += 16 * (index / 128);// we just need to skip over that these flag bits when calculating the position of the channel data in memory

		SPI_Flash_read((flashReadPos + index * CODEPLUG_CHANNEL_DATA_STRUCT_SIZE) + offset, ((uint8_t *)channelBuf) + offset, length);
	}
}

void codeplugChannelGetDataForIndex(int index, struct_codeplugChannel_t *channelBuf)
{
	// Read the whole channel
	codeplugChannelGetDataWithOffsetAndLengthForIndex(index, channelBuf, 0, CODEPLUG_CHANNEL_DATA_STRUCT_SIZE);

	channelBuf->chMode = (channelBuf->chMode == 0) ? RADIO_MODE_ANALOG : RADIO_MODE_DIGITAL;
	// Convert legacy codeplug tx and rx freq values into normal integers
	channelBuf->txFreq = bcd2int(channelBuf->txFreq);
	channelBuf->rxFreq = bcd2int(channelBuf->rxFreq);
	channelBuf->txTone = codeplugCSSToInt(channelBuf->txTone);
	channelBuf->rxTone = codeplugCSSToInt(channelBuf->rxTone);
	channelBuf->NOT_IN_CODEPLUG_flag = 0x00;

	// Sanity check the sql value, because its not used by the official firmware and may contain random value e.g. 255
	if (channelBuf->sql > 21)
	{
		channelBuf->sql = 10;
	}
	/* 2020.10.27 vk3kyy - I don't think this is necessary as the function which loads the contact treats index = 0 as a special case and always loads TG 9
	 *
	// Sanity check the digital contact and set it to 1 is its not been assigned, even for FM channels, as the user could switch to DMR on this channel
	if (channelBuf->contact == 0)
	{
		channelBuf->contact = 1;
	}*/
}

bool codeplugChannelSaveDataForIndex(int index, struct_codeplugChannel_t *channelBuf)
{
	bool retVal = true;

	channelBuf->chMode = (channelBuf->chMode == RADIO_MODE_ANALOG) ? 0 : 1;
	// Convert normal integers into legacy codeplug tx and rx freq values
	channelBuf->txFreq = int2bcd(channelBuf->txFreq);
	channelBuf->rxFreq = int2bcd(channelBuf->rxFreq);
	channelBuf->txTone = codeplugIntToCSS(channelBuf->txTone);
	channelBuf->rxTone = codeplugIntToCSS(channelBuf->rxTone);

	// lower 128 channels are in EEPROM. Remaining channels are in Flash ! (What a mess...)
	index--; // I think the channel index numbers start from 1 not zero.
	if (index < 128)
	{
		retVal = EEPROM_Write(CODEPLUG_ADDR_CHANNEL_EEPROM + index * CODEPLUG_CHANNEL_DATA_STRUCT_SIZE, (uint8_t *)channelBuf, CODEPLUG_CHANNEL_DATA_STRUCT_SIZE);
	}
	else
	{
		int flashWritePos = CODEPLUG_ADDR_CHANNEL_FLASH;
		int flashSector;
		int flashEndSector;
		int bytesToWriteInCurrentSector = CODEPLUG_CHANNEL_DATA_STRUCT_SIZE;

		index -= 128;// First 128 channels are in the EEPOM, so subtract 128 from the number when looking in the Flash

		// Every 128 bytes there seem to be 16 bytes gaps. I don't know why,bits since 16*8 = 128 bits, its likely they are flag bytes to indicate which channel in the next block are in use
		flashWritePos += 16 * (index / 128);// we just need to skip over that these flag bits when calculating the position of the channel data in memory
		flashWritePos += index * CODEPLUG_CHANNEL_DATA_STRUCT_SIZE;// go to the position of the specific index

		flashSector 	= flashWritePos / 4096;
		flashEndSector 	= (flashWritePos + CODEPLUG_CHANNEL_DATA_STRUCT_SIZE) / 4096;

		if (flashSector != flashEndSector)
		{
			bytesToWriteInCurrentSector = (flashEndSector * 4096) - flashWritePos;
		}

		SPI_Flash_read(flashSector * 4096, SPI_Flash_sectorbuffer, 4096);
		uint8_t *writePos = SPI_Flash_sectorbuffer + flashWritePos - (flashSector * 4096);
		memcpy(writePos, channelBuf, bytesToWriteInCurrentSector);

		retVal = SPI_Flash_eraseSector(flashSector * 4096);
		if (!retVal)
		{
			return false;
		}

		for (int i = 0; i < 16; i++)
		{
			retVal = SPI_Flash_writePage(flashSector * 4096 + i * 256, SPI_Flash_sectorbuffer + i * 256);
			if (!retVal)
			{
				return false;
			}
		}

		if (flashSector != flashEndSector)
		{
			uint8_t *channelBufPusOffset = (uint8_t *)channelBuf + bytesToWriteInCurrentSector;
			bytesToWriteInCurrentSector = CODEPLUG_CHANNEL_DATA_STRUCT_SIZE - bytesToWriteInCurrentSector;

			SPI_Flash_read(flashEndSector * 4096, SPI_Flash_sectorbuffer, 4096);
			memcpy(SPI_Flash_sectorbuffer, (uint8_t *)channelBufPusOffset, bytesToWriteInCurrentSector);

			retVal = SPI_Flash_eraseSector(flashEndSector * 4096);

			if (!retVal)
			{
				return false;
			}
			for (int i = 0; i < 16; i++)
			{
				retVal = SPI_Flash_writePage(flashEndSector * 4096 + i * 256, SPI_Flash_sectorbuffer + i * 256);

				if (!retVal)
				{
					return false;
				}
			}

		}
	}

	// Need to restore the values back to what we need for the operation of the firmware rather than the BCD values the codeplug uses

	channelBuf->chMode = (channelBuf->chMode == 0) ? RADIO_MODE_ANALOG : RADIO_MODE_DIGITAL;
	// Convert the the legacy codeplug tx and rx freq values into normal integers
	channelBuf->txFreq = bcd2int(channelBuf->txFreq);
	channelBuf->rxFreq = bcd2int(channelBuf->rxFreq);
	channelBuf->txTone = codeplugCSSToInt(channelBuf->txTone);
	channelBuf->rxTone = codeplugCSSToInt(channelBuf->rxTone);

	return retVal;
}

static void codeplugRxGroupInitCache(void)
{
	SPI_Flash_read(CODEPLUG_ADDR_RX_GROUP_LEN, (uint8_t*) &codeplugRXGroupCache[0], CODEPLUG_RX_GROUPLIST_MAX);
}

bool codeplugRxGroupGetDataForIndex(int index, struct_codeplugRxGroup_t *rxGroupBuf)
{
	int i = 0;
	struct_codeplugContact_t contactData;

	if ((index >= 1) && (index <= CODEPLUG_RX_GROUPLIST_MAX))
	{
		index--; // Index numbers start from 1 not zero

		if (codeplugRXGroupCache[index] > 0)
		{
			// Not our struct contains an extra property to hold the number of TGs in the group
			SPI_Flash_read(CODEPLUG_ADDR_RX_GROUP + (index * CODEPLUG_RXGROUP_DATA_STRUCT_SIZE), (uint8_t *) rxGroupBuf, CODEPLUG_RXGROUP_DATA_STRUCT_SIZE);

			for (i = 0; i < 32; i++)
			{
				codeplugContactGetDataForIndex(rxGroupBuf->contacts[i], &contactData);
				rxGroupBuf->NOT_IN_CODEPLUG_contactsTG[i] = contactData.tgNumber;
				// Empty groups seem to be filled with zeros
				if (rxGroupBuf->contacts[i] == 0)
				{
					break;
				}
			}

			rxGroupBuf->NOT_IN_CODEPLUG_numTGsInGroup = i;
			return true;
		}
	}

	rxGroupBuf->name[0] = 0;
	rxGroupBuf->NOT_IN_CODEPLUG_numTGsInGroup = 0;
	return false;
}

int codeplugDTMFContactsGetCount(void)
{
	return codeplugContactsCache.numDTMFContacts;
}

int codeplugContactsGetCount(int callType) // 0:TG 1:PC 2:ALL
{
	switch (callType)
	{
		case CONTACT_CALLTYPE_TG:
			return codeplugContactsCache.numTGContacts;
			break;
		case CONTACT_CALLTYPE_PC:
			return codeplugContactsCache.numPCContacts;
			break;
		case CONTACT_CALLTYPE_ALL:
			return codeplugContactsCache.numALLContacts;
			break;
	}

	return 0; // Should not happen
}

int codeplugDTMFContactGetDataForNumber(int number, struct_codeplugDTMFContact_t *contact)
{
	if ((number >= CODEPLUG_DTMF_CONTACTS_MIN) && (number <= CODEPLUG_DTMF_CONTACTS_MAX))
	{
		codeplugDTMFContactGetDataForIndex(codeplugContactsCache.contactsDTMFLookupCache[number - 1].index, contact);
		return number;
	}

	return 0;
}

int codeplugContactGetDataForNumberInType(int number, int callType, struct_codeplugContact_t *contact)
{
	int pos = 0;
	int numContacts = codeplugContactsCache.numTGContacts + codeplugContactsCache.numALLContacts + codeplugContactsCache.numPCContacts;

	for (int i = 0; i < numContacts; i++)
	{
		if ((codeplugContactsCache.contactsLookupCache[i].tgOrPCNum >> 24) == callType)
		{
			number--;
		}

		if (number == 0)
		{
			if (codeplugContactGetDataForIndex(codeplugContactsCache.contactsLookupCache[i].index, contact))
			{
				pos = i + 1;
				break;
			}
		}
	}

	return pos;
}

// optionalTS: 0 = no TS checking, 1..2 = TS
int codeplugContactIndexByTGorPC(int tgorpc, int callType, struct_codeplugContact_t *contact, uint8_t optionalTS)
{
	int numContacts = codeplugContactsCache.numTGContacts + codeplugContactsCache.numALLContacts + codeplugContactsCache.numPCContacts;
	int firstMatch = -1;

	for (int i = 0; i < numContacts; i++)
	{
		if (((codeplugContactsCache.contactsLookupCache[i].tgOrPCNum & 0xFFFFFF) == tgorpc) &&
				((codeplugContactsCache.contactsLookupCache[i].tgOrPCNum >> 24) == callType))
		{
			// Check for the contact TS override
			if (optionalTS > 0)
			{
				// Just read the reserve1 byte for now
				codeplugContactGetReserve1ByteForIndex(codeplugContactsCache.contactsLookupCache[i].index, contact);

				if (((contact->reserve1 & 0x01) == 0x00) && (((contact->reserve1 & 0x02) >> 1) == (optionalTS - 1)))
				{
					codeplugContactGetDataForIndex(codeplugContactsCache.contactsLookupCache[i].index, contact);
					return i;
				}
				else
				{
					if (firstMatch < 0)
					{
						firstMatch = i;
					}
				}
			}
			else
			{
				codeplugContactGetDataForIndex(codeplugContactsCache.contactsLookupCache[i].index, contact);
				return i;
			}
		}
	}

	if (firstMatch >= 0)
	{
		codeplugContactGetDataForIndex(codeplugContactsCache.contactsLookupCache[firstMatch].index, contact);
		return firstMatch;
	}

	return -1;
}

bool codeplugContactsContainsPC(uint32_t pc)
{
	int numContacts =  codeplugContactsCache.numTGContacts + codeplugContactsCache.numALLContacts + codeplugContactsCache.numPCContacts;
	pc = pc & 0x00FFFFFF;
	pc = pc | (CONTACT_CALLTYPE_PC << 24);

	for (int i = 0; i < numContacts; i++)
	{
		if (codeplugContactsCache.contactsLookupCache[i].tgOrPCNum == pc)
		{
			return true;
		}
	}
	return false;
}

static void codeplugInitContactsCache(void)
{
	struct_codeplugContact_t contact;
	uint8_t                  c;
	int codeplugNumContacts = 0;
	codeplugContactsCache.numTGContacts = 0;
	codeplugContactsCache.numPCContacts = 0;
	codeplugContactsCache.numALLContacts = 0;
	codeplugContactsCache.numDTMFContacts = 0;

	for(int i = 0; i < CODEPLUG_CONTACTS_MAX; i++)
	{
		if (SPI_Flash_read((CODEPLUG_ADDR_CONTACTS + (i * CODEPLUG_CONTACT_DATA_SIZE)), (uint8_t *)&contact, 16 + 4 + 1))// Name + TG/ID + Call type
		{
			if (contact.name[0] != 0xFF)
			{
				codeplugContactsCache.contactsLookupCache[codeplugNumContacts].tgOrPCNum = bcd2int(byteSwap32(contact.tgNumber));
				codeplugContactsCache.contactsLookupCache[codeplugNumContacts].index = i + 1;// Contacts are numbered from 1 to 1024
				codeplugContactsCache.contactsLookupCache[codeplugNumContacts].tgOrPCNum |= (contact.callType << 24);// Store the call type in the upper byte
				if (contact.callType == CONTACT_CALLTYPE_PC)
				{
					codeplugContactsCache.numPCContacts++;
				}
				else if (contact.callType == CONTACT_CALLTYPE_TG)
				{
					codeplugContactsCache.numTGContacts++;
				}
				else if (contact.callType == CONTACT_CALLTYPE_ALL)
				{
					codeplugContactsCache.numALLContacts++;
				}
				codeplugNumContacts++;
			}
		}
	}

	for (int i = 0; i < CODEPLUG_DTMF_CONTACTS_MAX; i++)
	{
		if (EEPROM_Read(CODEPLUG_ADDR_DTMF_CONTACTS + (i * CODEPLUG_DTMF_CONTACT_DATA_STRUCT_SIZE), (uint8_t *)&c, 1))
		{
			if (c != 0xFF)
			{
				codeplugContactsCache.contactsDTMFLookupCache[codeplugContactsCache.numDTMFContacts++].index = i + 1; // Contacts are numbered from 1 to 32
			}
		}
	}
}

void codeplugContactsCacheUpdateOrInsertContactAt(int index, struct_codeplugContact_t *contact)
{
	int numContacts =  codeplugContactsCache.numTGContacts + codeplugContactsCache.numALLContacts + codeplugContactsCache.numPCContacts;
	int numContactsMinus1 = numContacts - 1;

	for(int i = 0; i < numContacts; i++)
	{
		// Check if the contact is already in the cache, and is being modified
		if (codeplugContactsCache.contactsLookupCache[i].index == index)
		{
			uint8_t callType = codeplugContactsCache.contactsLookupCache[i].tgOrPCNum >> 24;// get call type from cache

			if (callType != contact->callType)
			{
				switch (callType)
				{
					case CONTACT_CALLTYPE_TG:
						codeplugContactsCache.numTGContacts--;
						break;
					case CONTACT_CALLTYPE_PC:
						codeplugContactsCache.numPCContacts--;
						break;
					case CONTACT_CALLTYPE_ALL:
						codeplugContactsCache.numALLContacts--;
						break;
				}

				switch (contact->callType)
				{
					case CONTACT_CALLTYPE_TG:
						codeplugContactsCache.numTGContacts++;
						break;
					case CONTACT_CALLTYPE_PC:
						codeplugContactsCache.numPCContacts++;
						break;
					case CONTACT_CALLTYPE_ALL:
						codeplugContactsCache.numALLContacts++;
						break;
				}
			}
			//update the
			codeplugContactsCache.contactsLookupCache[i].tgOrPCNum = bcd2int(byteSwap32(contact->tgNumber));
			codeplugContactsCache.contactsLookupCache[i].tgOrPCNum |= (contact->callType << 24);// Store the call type in the upper byte

			return;
		}
		else
		{
			if((i < numContactsMinus1) && (codeplugContactsCache.contactsLookupCache[i].index < index) && (codeplugContactsCache.contactsLookupCache[i + 1].index > index))
			{
				if (contact->callType == CONTACT_CALLTYPE_PC)
				{
					codeplugContactsCache.numPCContacts++;
				}
				else if (contact->callType == CONTACT_CALLTYPE_TG)
				{
					codeplugContactsCache.numTGContacts++;
				}
				else if (contact->callType == CONTACT_CALLTYPE_ALL)
				{
					codeplugContactsCache.numALLContacts++;
				}

				numContacts++;// Total contacts increases by 1

				// Note . Need to use memmove as the source and destination overlap.
				memmove(&codeplugContactsCache.contactsLookupCache[i + 2], &codeplugContactsCache.contactsLookupCache[i + 1], (numContacts - 2 - i) * sizeof(codeplugContactCache_t));

				codeplugContactsCache.contactsLookupCache[i + 1].tgOrPCNum = bcd2int(byteSwap32(contact->tgNumber));
				codeplugContactsCache.contactsLookupCache[i + 1].index = index;// Contacts are numbered from 1 to 1024
				codeplugContactsCache.contactsLookupCache[i + 1].tgOrPCNum |= (contact->callType << 24);// Store the call type in the upper byte
				return;
			}
		}
	}

	// Did not find the index in the cache or a gap between 2 existing indexes. So the new contact needs to be added to the end of the cache

	if (contact->callType == CONTACT_CALLTYPE_PC)
	{
		codeplugContactsCache.numPCContacts++;
	}
	else if (contact->callType == CONTACT_CALLTYPE_TG)
	{
		codeplugContactsCache.numTGContacts++;
	}
	else if (contact->callType == CONTACT_CALLTYPE_ALL)
	{
		codeplugContactsCache.numALLContacts++;
	}

	// Note. We can use numContacts as the the index as the array is zero indexed but the number of contacts is starts from 1
	// Hence is already in some ways pre incremented in terms of being an array index
	codeplugContactsCache.contactsLookupCache[numContacts].tgOrPCNum = bcd2int(byteSwap32(contact->tgNumber));
	codeplugContactsCache.contactsLookupCache[numContacts].index = index;// Contacts are numbered from 1 to 1024
	codeplugContactsCache.contactsLookupCache[numContacts].tgOrPCNum |= (contact->callType << 24);// Store the call type in the upper byte
}

void codeplugContactsCacheRemoveContactAt(int index)
{
	int numContacts = codeplugContactsCache.numTGContacts + codeplugContactsCache.numALLContacts + codeplugContactsCache.numPCContacts;
	for(int i = 0; i < numContacts; i++)
	{
		if(codeplugContactsCache.contactsLookupCache[i].index == index)
		{
			uint8_t callType = codeplugContactsCache.contactsLookupCache[i].tgOrPCNum >> 24;

			if (callType == CONTACT_CALLTYPE_PC)
			{
				codeplugContactsCache.numPCContacts--;
			}
			else if (callType == CONTACT_CALLTYPE_TG)
			{
				codeplugContactsCache.numTGContacts--;
			}
			else if (callType == CONTACT_CALLTYPE_ALL)
			{
				codeplugContactsCache.numALLContacts--;
			}
			// Note memcpy should work here, because memcpy normally copys from the lowest memory location upwards
			memcpy(&codeplugContactsCache.contactsLookupCache[i], &codeplugContactsCache.contactsLookupCache[i + 1], (numContacts - 1 - i) * sizeof(codeplugContactCache_t));
			return;
		}
	}
}

int codeplugContactGetFreeIndex(void)
{
	int numContacts = codeplugContactsCache.numTGContacts + codeplugContactsCache.numALLContacts + codeplugContactsCache.numPCContacts;
	int lastIndex = 0;
	int i;

	for (i = 0; i < numContacts; i++)
	{
		if (codeplugContactsCache.contactsLookupCache[i].index != lastIndex + 1)
		{
			return lastIndex + 1;
		}
		lastIndex = codeplugContactsCache.contactsLookupCache[i].index;
	}

	if (i < CODEPLUG_CONTACTS_MAX)
	{
		return codeplugContactsCache.contactsLookupCache[i - 1].index + 1;
	}

	return 0;
}

bool codeplugDTMFContactGetDataForIndex(int index, struct_codeplugDTMFContact_t *contact)
{
	if ((codeplugContactsCache.numDTMFContacts > 0) &&  (index >= CODEPLUG_DTMF_CONTACTS_MIN) && (index <= CODEPLUG_DTMF_CONTACTS_MAX))
	{
		index--;
		if(EEPROM_Read(CODEPLUG_ADDR_DTMF_CONTACTS + (index * CODEPLUG_DTMF_CONTACT_DATA_STRUCT_SIZE), (uint8_t *)contact, CODEPLUG_DTMF_CONTACT_DATA_STRUCT_SIZE))
		{
			return true;
		}
	}

	memset(contact, 0xff, CODEPLUG_DTMF_CONTACT_DATA_STRUCT_SIZE);
	return false;
}

static bool codeplugContactGetReserve1ByteForIndex(int index, struct_codeplugContact_t *contact)
{
	if (((codeplugContactsCache.numTGContacts > 0) || (codeplugContactsCache.numPCContacts > 0) || (codeplugContactsCache.numALLContacts > 0)) &&
			(index >= CODEPLUG_CONTACTS_MIN) && (index <= CODEPLUG_CONTACTS_MAX))
	{
		index--;
		SPI_Flash_read(CODEPLUG_ADDR_CONTACTS + (index * CODEPLUG_CONTACT_DATA_SIZE) + 23 , (uint8_t *)contact + 23, 1);
		return true;
	}
	return false;
}

bool codeplugContactGetDataForIndex(int index, struct_codeplugContact_t *contact)
{
	char buf[17];

	if (((codeplugContactsCache.numTGContacts > 0) || (codeplugContactsCache.numPCContacts > 0) || (codeplugContactsCache.numALLContacts > 0)) &&
			(index >= CODEPLUG_CONTACTS_MIN) && (index <= CODEPLUG_CONTACTS_MAX))
	{
		index--;
		SPI_Flash_read(CODEPLUG_ADDR_CONTACTS + index * CODEPLUG_CONTACT_DATA_SIZE, (uint8_t *)contact, CODEPLUG_CONTACT_DATA_SIZE);
		contact->NOT_IN_CODEPLUGDATA_indexNumber = index + 1;
		contact->tgNumber = bcd2int(byteSwap32(contact->tgNumber));
		return true;
	}

	// If an invalid contact number has been requested, return a TG 9 contact
	contact->tgNumber = 9;
	contact->callType = CONTACT_CALLTYPE_TG;
	contact->reserve1 = 0xff;
	contact->NOT_IN_CODEPLUGDATA_indexNumber = -1;
	snprintf(buf, 17, "%s 9", currentLanguage->tg);
	codeplugUtilConvertStringToBuf(buf, contact->name, 16);
	return false;
}

int codeplugContactSaveDataForIndex(int index, struct_codeplugContact_t *contact)
{
	int retVal;
	int flashWritePos = CODEPLUG_ADDR_CONTACTS;
	int flashSector;
	int flashEndSector;
	int bytesToWriteInCurrentSector = CODEPLUG_CONTACT_DATA_SIZE;

	index--;
	contact->tgNumber = byteSwap32(int2bcd(contact->tgNumber));


	flashWritePos += index * CODEPLUG_CONTACT_DATA_SIZE;// go to the position of the specific index

	flashSector 	= flashWritePos / 4096;
	flashEndSector 	= (flashWritePos + CODEPLUG_CONTACT_DATA_SIZE) / 4096;

	if (flashSector != flashEndSector)
	{
		bytesToWriteInCurrentSector = (flashEndSector * 4096) - flashWritePos;
	}

	SPI_Flash_read(flashSector * 4096, SPI_Flash_sectorbuffer, 4096);
	uint8_t *writePos = SPI_Flash_sectorbuffer + flashWritePos - (flashSector * 4096);
	memcpy(writePos, contact, bytesToWriteInCurrentSector);

	retVal = SPI_Flash_eraseSector(flashSector * 4096);
	if (!retVal)
	{
		return false;
	}

	for (int i = 0; i < 16; i++)
	{
		retVal = SPI_Flash_writePage(flashSector * 4096 + i * 256, SPI_Flash_sectorbuffer + i * 256);
		if (!retVal)
		{
			return false;
		}
	}

	if (flashSector != flashEndSector)
	{
		uint8_t *channelBufPusOffset = (uint8_t *)contact + bytesToWriteInCurrentSector;
		bytesToWriteInCurrentSector = CODEPLUG_CONTACT_DATA_SIZE - bytesToWriteInCurrentSector;

		SPI_Flash_read(flashEndSector * 4096, SPI_Flash_sectorbuffer, 4096);
		memcpy(SPI_Flash_sectorbuffer, (uint8_t *)channelBufPusOffset, bytesToWriteInCurrentSector);

		retVal = SPI_Flash_eraseSector(flashEndSector * 4096);

		if (!retVal)
		{
			return false;
		}
		for (int i = 0; i < 16; i++)
		{
			retVal = SPI_Flash_writePage(flashEndSector * 4096 + i * 256, SPI_Flash_sectorbuffer + i * 256);

			if (!retVal)
			{
				return false;
			}
		}

	}
	if ((contact->name[0] == 0xff) || (contact->callType == 0xFF))
	{
		codeplugContactsCacheRemoveContactAt(index + 1);// index was decremented at the start of the function
	}
	else
	{
		codeplugContactsCacheUpdateOrInsertContactAt(index + 1, contact);
		//initCodeplugContactsCache();// Update the cache
	}
	return retVal;
}

bool codeplugContactGetRXGroup(int index)
{
	struct_codeplugRxGroup_t rxGroupBuf;
	int i;

	for (i = 1; i <= CODEPLUG_RX_GROUPLIST_MAX; i++)
	{
		if (codeplugRxGroupGetDataForIndex(i, &rxGroupBuf))
		{
			for (int j = 0; j < 32; j++)
			{
				if (rxGroupBuf.contacts[j] == index)
				{
					return true;
				}
			}
		}
	}
	return false;
}

int codeplugGetUserDMRID(void)
{
	int dmrId;
	EEPROM_Read(CODEPLUG_ADDR_USER_DMRID, (uint8_t *)&dmrId, 4);
	return bcd2int(byteSwap32(dmrId));
}

void codeplugSetUserDMRID(uint32_t dmrId)
{
	dmrId = byteSwap32(int2bcd(dmrId));
	EEPROM_Write(CODEPLUG_ADDR_USER_DMRID, (uint8_t *)&dmrId, 4);
}

// Max length the user can enter is 8. Hence buf must be 16 chars to allow for the termination
void codeplugGetRadioName(char *buf)
{
	memset(buf, 0, 9);
	EEPROM_Read(CODEPLUG_ADDR_USER_CALLSIGN, (uint8_t *)buf, 8);
	codeplugUtilConvertBufToString(buf, buf, 8);
}

// Max length the user can enter is 16. Hence buf must be 17 chars to allow for the termination
void codeplugGetBootScreenData(char *line1, char *line2, uint8_t *displayType)
{
	memset(line1, 0, 17);
	memset(line2, 0, 17);

	EEPROM_Read(CODEPLUG_ADDR_BOOT_LINE1, (uint8_t *)line1, 16);
	codeplugUtilConvertBufToString(line1, line1, 16);
	EEPROM_Read(CODEPLUG_ADDR_BOOT_LINE2, (uint8_t *)line2, 16);
	codeplugUtilConvertBufToString(line2, line2, 16);

	EEPROM_Read(CODEPLUG_ADDR_BOOT_INTRO_SCREEN, displayType, 1);// read the display type
}

void codeplugGetVFO_ChannelData(struct_codeplugChannel_t *vfoBuf, Channel_t VFONumber)
{
	EEPROM_Read(CODEPLUG_ADDR_VFO_A_CHANNEL + (CODEPLUG_CHANNEL_DATA_STRUCT_SIZE * (int)VFONumber), (uint8_t *)vfoBuf, CODEPLUG_CHANNEL_DATA_STRUCT_SIZE);

	// Convert the the legacy codeplug tx and rx freq values into normal integers
	vfoBuf->chMode = (vfoBuf->chMode == 0) ? RADIO_MODE_ANALOG : RADIO_MODE_DIGITAL;
	vfoBuf->txFreq = bcd2int(vfoBuf->txFreq);
	vfoBuf->rxFreq = bcd2int(vfoBuf->rxFreq);
	vfoBuf->txTone = codeplugCSSToInt(vfoBuf->txTone);
	vfoBuf->rxTone = codeplugCSSToInt(vfoBuf->rxTone);
	vfoBuf->NOT_IN_CODEPLUG_flag = ((VFONumber == CHANNEL_VFO_A) ? 0x01 : ((VFONumber == CHANNEL_VFO_B) ? 0x03 : 0x00));
}

void codeplugSetVFO_ChannelData(struct_codeplugChannel_t *vfoBuf, Channel_t VFONumber)
{
	struct_codeplugChannel_t tmpChannel;

	memcpy(&tmpChannel, vfoBuf, CODEPLUG_CHANNEL_DATA_STRUCT_SIZE);// save current VFO data as we need to modify
	tmpChannel.chMode = (vfoBuf->chMode == RADIO_MODE_ANALOG) ? 0 : 1;
	tmpChannel.txFreq = int2bcd(vfoBuf->txFreq);
	tmpChannel.rxFreq = int2bcd(vfoBuf->rxFreq);
	tmpChannel.txTone = codeplugIntToCSS(vfoBuf->txTone);
	tmpChannel.rxTone = codeplugIntToCSS(vfoBuf->rxTone);
	EEPROM_Write(CODEPLUG_ADDR_VFO_A_CHANNEL + (CODEPLUG_CHANNEL_DATA_STRUCT_SIZE * (int)VFONumber), (uint8_t *)&tmpChannel, CODEPLUG_CHANNEL_DATA_STRUCT_SIZE);
}

void codeplugInitChannelsPerZone(void)
{
	uint8_t buf[16];
	// 0x806F is the last byte of the name of the second Zone if 16 channels per zone
	// And because the CPS terminates the zone name with 0xFF, this will be 0xFF if its a 16 channel per zone schema
	// If the zone has 80 channels per zone this address will be the upper byte of the channel number and because the max channels is 1024
	// this value can never me 0xff if its a channel number

	// Note. I tried to read just 1 byte but it crashed. So I am now reading 16 bytes and checking the last one
	EEPROM_Read(0x8060, (uint8_t *)buf, 16);
	if ((buf[15] >= 0x00) && (buf[15] <= 0x04))
	{
		codeplugChannelsPerZone = 80;// Must be the new 80 channel per zone format
	}
}

typedef struct
{
	int dataType;
	int dataLength;
} codeplugCustomDataBlockHeader_t;

bool codeplugGetOpenGD77CustomData(codeplugCustomDataType_t dataType, uint8_t *dataBuf)
{
	uint8_t tmpBuf[12];
	int dataHeaderAddress = 12;
	const int MAX_BLOCK_ADDRESS = 0x10000;

	SPI_Flash_read(0, tmpBuf, 12);

	if (memcmp("OpenGD77", tmpBuf, 8) == 0)
	{
		codeplugCustomDataBlockHeader_t blockHeader;
		do
		{
			SPI_Flash_read(dataHeaderAddress, (uint8_t *)&blockHeader, sizeof(codeplugCustomDataBlockHeader_t));
			if (blockHeader.dataType == dataType)
			{
				SPI_Flash_read(dataHeaderAddress + sizeof(codeplugCustomDataBlockHeader_t), dataBuf,blockHeader.dataLength);
				return true;
			}
			dataHeaderAddress += sizeof(codeplugCustomDataBlockHeader_t) + blockHeader.dataLength;

		} while (dataHeaderAddress < MAX_BLOCK_ADDRESS);

	}
	return false;
}


bool codeplugGetGeneralSettings(struct_codeplugGeneralSettings_t *generalSettingsBuffer)
{
	return EEPROM_Read(CODEPLUG_ADDR_GENERAL_SETTINGS, (uint8_t *)generalSettingsBuffer, CODEPLUG_GENERAL_SETTINGS_DATA_STRUCT_SIZE);
}

bool codeplugGetSignallingDTMF(struct_codeplugSignalling_DTMF_t *signallingDTMFBuffer)
{
	return EEPROM_Read(CODEPLUG_ADDR_SIGNALLING_DTMF, (uint8_t *)signallingDTMFBuffer, CODEPLUG_SIGNALLING_DTMF_DATA_STRUCT_SIZE);
}

bool codeplugGetSignallingDTMFDurations(struct_codeplugSignalling_DTMFDurations_t *signallingDTMFDurationsBuffer)
{
	if (EEPROM_Read(CODEPLUG_ADDR_SIGNALLING_DTMF_DURATIONS, (uint8_t *)signallingDTMFDurationsBuffer, SIGNALLING_DTMF_DURATIONS_DATA_STRUCT_SIZE))
	{
		// Default codeplug value was 128(0x80). Override any value above 1000ms
		if (signallingDTMFDurationsBuffer->libreDMR_Tail > 10)
		{
			signallingDTMFDurationsBuffer->libreDMR_Tail = 5;
		}

		return true;
	}
	else
	{
		// Avoid division-by-zero
		signallingDTMFDurationsBuffer->rate = 2; // 250ms/250ms
	}

	return false;
}


bool codeplugGetDeviceInfo(struct_codeplugDeviceInfo_t *deviceInfoBuffer)
{
	bool readOK = EEPROM_Read(CODEPLUG_ADDR_DEVICE_INFO, (uint8_t *)deviceInfoBuffer, sizeof(struct_codeplugDeviceInfo_t));//CODEPLUG_ADDR_DEVICE_INFO_READ_SIZE);
	if (readOK)
	{
		deviceInfoBuffer->minUHFFreq = bcd2uint16(deviceInfoBuffer->minUHFFreq);
		deviceInfoBuffer->maxUHFFreq = bcd2uint16(deviceInfoBuffer->maxUHFFreq);

		deviceInfoBuffer->minVHFFreq = bcd2uint16(deviceInfoBuffer->minVHFFreq);
		deviceInfoBuffer->maxVHFFreq = bcd2uint16(deviceInfoBuffer->maxVHFFreq);

		return true;
	}
	return false;
}

static void codeplugQuickKeyInitCache(void)
{
	EEPROM_Read(CODEPLUG_ADDR_QUICKKEYS, (uint8_t *)&quickKeysCache, (CODEPLUG_QUICKKEYS_SIZE * sizeof(uint16_t)));
}

uint16_t codeplugGetQuickkeyFunctionID(char key)
{
	uint16_t functionId = 0;

	if ((key >= '0') && (key <= '9'))
	{
		key = key - '0';
		functionId = quickKeysCache[(int) key];
	}

	return functionId;
}

static bool quickkeyIsEmpty(char key, uint16_t functionId)
{
	// Wants to clear a slot, so consider it empty
	if (functionId == 0x8000)
	{
		return true;
	}

	uint16_t data = codeplugGetQuickkeyFunctionID(key);

	if ((data == 0x8000) ||
			(((data & 0x8000) == 0) && ((data < CODEPLUG_CONTACTS_MIN) || (data > CODEPLUG_CONTACTS_MAX))))
	{
		return true;
	}

	return false;
}

bool codeplugSetQuickkeyFunctionID(char key, uint16_t functionId)
{
	// Only permit to store QuickKey in empty slots
	if (quickkeyIsEmpty(key, functionId) && (key >= '0' && key <= '9'))
	{
		key = key - '0';
		EEPROM_Write(CODEPLUG_ADDR_QUICKKEYS + (sizeof(uint16_t) * (int)key), (uint8_t *)&functionId, sizeof(uint16_t));
		quickKeysCache[(int) key] = functionId;

		return true;
	}

	return false;
}

int codeplugGetRepeaterWakeAttempts(void)
{
	return 4;// Hard coded. In the future we may read this from the codeplug.
}

void codeplugInitCaches(void)
{
	codeplugInitContactsCache();

	codeplugAllChannelsInitCache();
	allChannelsTotalNumOfChannels = codeplugAllChannelsGetCount();

	codeplugZonesInitCache();
	codeplugRxGroupInitCache();
	codeplugQuickKeyInitCache();
}

// Returns pin length or 0 if no pin. Pin code is passed as pointer to int32_t
int codeplugGetPasswordPin(int32_t *pinCode)
{
	int pinLength = 0;
	uint8_t buf[6];

	if (EEPROM_Read(CODEPLUG_ADDR_BOOT_PASSWORD_PIN + 1, (uint8_t *)buf, 6))
	{
		if ((buf[0] & 0x01) == 0)
		{
			return pinLength;
		}

		uint8_t nibble;
		*pinCode = 0;
		for(int i = 0 ; i < 6 ; i++)
		{
			nibble = buf[(i/2)+3] >> ((1-(i % 2)) * 4) & 0x0F;
			if (nibble != 0x0f)
			{
				*pinCode = (*pinCode * 10) + nibble;
				pinLength++;
			}
			else
			{
				break;
			}
		}
	}
	return pinLength;
}
