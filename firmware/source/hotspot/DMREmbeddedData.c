/*
 *   Copyright (C) 2015,2016,2017 by Jonathan Naylor G4KLX
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <string.h>
//#include "hotspot/DMREmbeddedData.h"
#include "hotspot/CRC.h"
#include "hotspot/dmrDefines.h"
#include "hotspot/dmrUtils.h"
#include "hotspot/DMRLC.h"
#include "hotspot/Hamming.h"

enum LC_STATE
{
	LCS_NONE,
	LCS_FIRST,
	LCS_SECOND,
	LCS_THIRD
} m_state;

static bool 	DMREmbeddedData_m_raw[128];
static bool 	DMREmbeddedData_m_data[72];
static int		DMREmbeddedData_m_FLCO;
static bool		DMREmbeddedData_m_valid;

static void DMREmbeddedData_decodeEmbeddedData(void);
static void DMREmbeddedData_encodeEmbeddedData(void);

void DMREmbeddedData_initEmbeddedDataBuffers(void)
{
	memset(DMREmbeddedData_m_raw, 0, sizeof(DMREmbeddedData_m_raw));
	memset(DMREmbeddedData_m_data, 0, sizeof(DMREmbeddedData_m_data));
	DMREmbeddedData_m_FLCO = 0;
	DMREmbeddedData_m_valid = false;
}

// Add LC data (which may consist of 4 blocks) to the data store
bool DMREmbeddedData_addData(const unsigned char *data, unsigned char lcss)
{
	bool rawData[40U];
	dmrUtils_byteToBitsBE(data[14U], rawData + 0U);
	dmrUtils_byteToBitsBE(data[15U], rawData + 8U);
	dmrUtils_byteToBitsBE(data[16U], rawData + 16U);
	dmrUtils_byteToBitsBE(data[17U], rawData + 24U);
	dmrUtils_byteToBitsBE(data[18U], rawData + 32U);

	// Is this the first block of a 4 block embedded LC ?
	if (lcss == 1U)
	{
		for (unsigned int a = 0U; a < 32U; a++)
		{
			DMREmbeddedData_m_raw[a] = rawData[a + 4U];
		}
		// Show we are ready for the next LC block
		m_state = LCS_FIRST;
		DMREmbeddedData_m_valid = false;

		return false;
	}

	// Is this the 2nd block of a 4 block embedded LC ?
	if (lcss == 3U && m_state == LCS_FIRST)
	{
		for (unsigned int a = 0U; a < 32U; a++)
			DMREmbeddedData_m_raw[a + 32U] = rawData[a + 4U];

		// Show we are ready for the next LC block
		m_state = LCS_SECOND;

		return false;
	}

	// Is this the 3rd block of a 4 block embedded LC ?
	if (lcss == 3U && m_state == LCS_SECOND)
	{
		for (unsigned int a = 0U; a < 32U; a++)
			DMREmbeddedData_m_raw[a + 64U] = rawData[a + 4U];

		// Show we are ready for the final LC block
		m_state = LCS_THIRD;

		return false;
	}

	// Is this the final block of a 4 block embedded LC ?
	if (lcss == 2U && m_state == LCS_THIRD)
	{
		for (unsigned int a = 0U; a < 32U; a++)
			DMREmbeddedData_m_raw[a + 96U] = rawData[a + 4U];

		// Show that we're not ready for any more data
		m_state = LCS_NONE;

		// Process the complete data block
		DMREmbeddedData_decodeEmbeddedData();
		if (DMREmbeddedData_m_valid)
		{
			DMREmbeddedData_encodeEmbeddedData();
		}
		return DMREmbeddedData_m_valid;
	}

	return false;
}

void DMREmbeddedData_setLC(const DMRLC_T * lc)
{

	DMRLC_getDataFromBits(DMREmbeddedData_m_data,lc);
	DMREmbeddedData_m_FLCO  = lc->FLCO;
	DMREmbeddedData_m_valid = true;

	DMREmbeddedData_encodeEmbeddedData();
}

void DMREmbeddedData_encodeEmbeddedData(void)
{
	unsigned int crc;
	CRC_encodeFiveBit(DMREmbeddedData_m_data, &crc);

	bool data[128U];
	memset(data, 0x00U, 128U * sizeof(bool));

	data[106U] = (crc & 0x01U) == 0x01U;
	data[90U]  = (crc & 0x02U) == 0x02U;
	data[74U]  = (crc & 0x04U) == 0x04U;
	data[58U]  = (crc & 0x08U) == 0x08U;
	data[42U]  = (crc & 0x10U) == 0x10U;

	unsigned int b = 0U;
	for (unsigned int a = 0U; a < 11U; a++, b++)
		data[a] = DMREmbeddedData_m_data[b];
	for (unsigned int a = 16U; a < 27U; a++, b++)
		data[a] = DMREmbeddedData_m_data[b];
	for (unsigned int a = 32U; a < 42U; a++, b++)
		data[a] = DMREmbeddedData_m_data[b];
	for (unsigned int a = 48U; a < 58U; a++, b++)
		data[a] = DMREmbeddedData_m_data[b];
	for (unsigned int a = 64U; a < 74U; a++, b++)
		data[a] = DMREmbeddedData_m_data[b];
	for (unsigned int a = 80U; a < 90U; a++, b++)
		data[a] = DMREmbeddedData_m_data[b];
	for (unsigned int a = 96U; a < 106U; a++, b++)
		data[a] = DMREmbeddedData_m_data[b];

	// Hamming (16,11,4) check each row except the last one
	for (unsigned int a = 0U; a < 112U; a += 16U)
		Hamming_encode16114(data + a);

	// Add the parity bits for each column
	for (unsigned int a = 0U; a < 16U; a++)
		data[a + 112U] = data[a + 0U] ^ data[a + 16U] ^ data[a + 32U] ^ data[a + 48U] ^ data[a + 64U] ^ data[a + 80U] ^ data[a + 96U];

	// The data is packed downwards in columns
	b = 0U;
	for (unsigned int a = 0U; a < 128U; a++) {
		DMREmbeddedData_m_raw[a] = data[b];
		b += 16U;
		if (b > 127U)
			b -= 127U;
	}
}

unsigned char DMREmbeddedData_getData(unsigned char *data, unsigned char n)
{
	memset(data, 0, DMR_FRAME_LENGTH_BYTES);//clear

	if ((n >= 1U) && (n < 5U))
	{
		n--;

		bool bits[40U];
		memset(bits, 0x00U, 40U * sizeof(bool));
		memcpy(bits + 4U, DMREmbeddedData_m_raw + n * 32U, 32U * sizeof(bool));

		unsigned char bytes[5U];
		dmrUtils_bitsToByteBE(bits + 0U,  &bytes[0U]);
		dmrUtils_bitsToByteBE(bits + 8U,  &bytes[1U]);
		dmrUtils_bitsToByteBE(bits + 16U, &bytes[2U]);
		dmrUtils_bitsToByteBE(bits + 24U, &bytes[3U]);
		dmrUtils_bitsToByteBE(bits + 32U, &bytes[4U]);

		data[14U] = (data[14U] & 0xF0U) | (bytes[0U] & 0x0FU);
		data[15U] = bytes[1U];
		data[16U] = bytes[2U];
		data[17U] = bytes[3U];
		data[18U] = (data[18U] & 0x0FU) | (bytes[4U] & 0xF0U);

		switch (n)
		{
			case 0U:
				return 1U;
			case 3U:
				return 2U;
			default:
				return 3U;
		}
	}

	data[14U] &= 0xF0U;
	data[15U]  = 0x00U;
	data[16U]  = 0x00U;
	data[17U]  = 0x00U;
	data[18U] &= 0x0FU;

	return 0U;
}

// Unpack and error check an embedded LC
static void DMREmbeddedData_decodeEmbeddedData(void)
{
	// The data is unpacked downwards in columns
	bool data[128U];
	memset(data, 0x00U, 128U * sizeof(bool));

	unsigned int b = 0U;
	for (unsigned int a = 0U; a < 128U; a++)
	{
		data[b] = DMREmbeddedData_m_raw[a];
		b += 16U;
		if (b > 127U)
			b -= 127U;
	}

	// Hamming (16,11,4) check each row except the last one
	for (unsigned int a = 0U; a < 112U; a += 16U)
	{
		if (!Hamming_decode16114(data + a))
			return;
	}

	// Check the parity bits
	for (unsigned int a = 0U; a < 16U; a++)
	{
		bool parity = data[a + 0U] ^ data[a + 16U] ^ data[a + 32U] ^ data[a + 48U] ^ data[a + 64U] ^ data[a + 80U] ^ data[a + 96U] ^ data[a + 112U];
		if (parity)
			return;
	}

	// We have passed the Hamming check so extract the actual payload
	b = 0U;
	for (unsigned int a = 0U; a < 11U; a++, b++)
		DMREmbeddedData_m_data[b] = data[a];
	for (unsigned int a = 16U; a < 27U; a++, b++)
		DMREmbeddedData_m_data[b] = data[a];
	for (unsigned int a = 32U; a < 42U; a++, b++)
		DMREmbeddedData_m_data[b] = data[a];
	for (unsigned int a = 48U; a < 58U; a++, b++)
		DMREmbeddedData_m_data[b] = data[a];
	for (unsigned int a = 64U; a < 74U; a++, b++)
		DMREmbeddedData_m_data[b] = data[a];
	for (unsigned int a = 80U; a < 90U; a++, b++)
		DMREmbeddedData_m_data[b] = data[a];
	for (unsigned int a = 96U; a < 106U; a++, b++)
		DMREmbeddedData_m_data[b] = data[a];

	// Extract the 5 bit CRC
	unsigned int crc = 0U;
	if (data[42])  crc += 16U;
	if (data[58])  crc += 8U;
	if (data[74])  crc += 4U;
	if (data[90])  crc += 2U;
	if (data[106]) crc += 1U;

	// Now CRC check this
	if (!CRC_checkFiveBit(DMREmbeddedData_m_data, crc))
		return;

	DMREmbeddedData_m_valid = true;

	// Extract the FLCO
	unsigned char flco;
	dmrUtils_bitsToByteBE(DMREmbeddedData_m_data + 0U, &flco);
	DMREmbeddedData_m_FLCO = (int)(flco & 0x3FU);
}

bool DMREmbeddedData_getLC(DMRLC_T * lc)
{
	if (!DMREmbeddedData_m_valid)
	{
		return false;
	}

	if ((DMREmbeddedData_m_FLCO != FLCO_GROUP) && (DMREmbeddedData_m_FLCO != FLCO_USER_USER))
	{
		return false;
	}

	DMRLCfromBits(DMREmbeddedData_m_data, lc);
	return true;
}

bool DMREmbeddedData_isValid(void)
{
	return DMREmbeddedData_m_valid;
}

int DMREmbeddedData_getFLCO(void)
{
	return DMREmbeddedData_m_FLCO;
}

void DMREmbeddedData_reset(void)
{
	m_state = LCS_NONE;
	DMREmbeddedData_m_valid = false;
}

bool DMREmbeddedData_getRawData(unsigned char* data)
{
	if (!DMREmbeddedData_m_valid)
	{
		return false;
	}

	dmrUtils_bitsToByteBE(DMREmbeddedData_m_data + 0U,  &data[0U]);
	dmrUtils_bitsToByteBE(DMREmbeddedData_m_data + 8U,  &data[1U]);
	dmrUtils_bitsToByteBE(DMREmbeddedData_m_data + 16U, &data[2U]);
	dmrUtils_bitsToByteBE(DMREmbeddedData_m_data + 24U, &data[3U]);
	dmrUtils_bitsToByteBE(DMREmbeddedData_m_data + 32U, &data[4U]);
	dmrUtils_bitsToByteBE(DMREmbeddedData_m_data + 40U, &data[5U]);
	dmrUtils_bitsToByteBE(DMREmbeddedData_m_data + 48U, &data[6U]);
	dmrUtils_bitsToByteBE(DMREmbeddedData_m_data + 56U, &data[7U]);
	dmrUtils_bitsToByteBE(DMREmbeddedData_m_data + 64U, &data[8U]);

	return true;
}
