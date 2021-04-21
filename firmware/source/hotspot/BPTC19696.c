/*
 *	 Copyright (C) 2012 by Ian Wraith
 *   Copyright (C) 2015 by Jonathan Naylor G4KLX
 *
 *   Ported to OpenGD77 by Roger Clark VK3KYY / G4KYF
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
#include "hotspot/BPTC19696.h"
#include "hotspot/dmrUtils.h"
#include "hotspot/Hamming.h"

__attribute__((section(".data.$RAM2"))) static bool BPTC19696_rawData[196];
__attribute__((section(".data.$RAM2"))) static bool BPTC19696_deInterData[196];

static void BPTC19696_decodeExtractBinary(const unsigned char* in);
static void BPTC19696_decodeErrorCheck(void);
static void BPTC19696_decodeDeInterleave(void);
static void BPTC19696_decodeExtractData(unsigned char* data);

static void BPTC19696_encodeExtractData(const unsigned char* in);
static void BPTC19696_encodeInterleave(void);
static void BPTC19696_encodeErrorCheck(void);
static void BPTC19696_encodeExtractBinary(unsigned char* data);

#if false // debug only
void debugBitPatterns(bool *data)
{
	char buf[256];

	memset(buf, 0, 256);
	for (int i = 0; i < 192; i++)
	{
		buf[i] = data[i] ? '1' : '0';
	}
#if defined(USING_EXTERNAL_DEBUGGER)
    SEGGER_RTT_printf(0, "pattern %s\r\n",buf);
#endif
}
#endif

void BPTC19696_init(void)
{
	memset(BPTC19696_rawData, 0, sizeof(BPTC19696_rawData));
	memset(BPTC19696_deInterData, 0, sizeof(BPTC19696_deInterData));
}

// The main decode function
void BPTC19696_decode(const unsigned char* in, unsigned char* out)
{
	//  Get the raw binary
	BPTC19696_decodeExtractBinary(in);

	// Deinterleave
	BPTC19696_decodeDeInterleave();

	// Error check
	BPTC19696_decodeErrorCheck();

	// Extract Data
	BPTC19696_decodeExtractData(out);
}

// The main encode function
void BPTC19696_encode(const unsigned char* in, unsigned char* out)
{
	// Extract Data
	BPTC19696_encodeExtractData(in);

	// Error check
	BPTC19696_encodeErrorCheck();

	// Deinterleave
	BPTC19696_encodeInterleave();

	//  Get the raw binary
	BPTC19696_encodeExtractBinary(out);
}

static void BPTC19696_decodeExtractBinary(const unsigned char* in)
{
	// First block
	dmrUtils_byteToBitsBE(in[0U],  BPTC19696_rawData + 0U);
	dmrUtils_byteToBitsBE(in[1U],  BPTC19696_rawData + 8U);
	dmrUtils_byteToBitsBE(in[2U],  BPTC19696_rawData + 16U);
	dmrUtils_byteToBitsBE(in[3U],  BPTC19696_rawData + 24U);
	dmrUtils_byteToBitsBE(in[4U],  BPTC19696_rawData + 32U);
	dmrUtils_byteToBitsBE(in[5U],  BPTC19696_rawData + 40U);
	dmrUtils_byteToBitsBE(in[6U],  BPTC19696_rawData + 48U);
	dmrUtils_byteToBitsBE(in[7U],  BPTC19696_rawData + 56U);
	dmrUtils_byteToBitsBE(in[8U],  BPTC19696_rawData + 64U);
	dmrUtils_byteToBitsBE(in[9U],  BPTC19696_rawData + 72U);
	dmrUtils_byteToBitsBE(in[10U], BPTC19696_rawData + 80U);
	dmrUtils_byteToBitsBE(in[11U], BPTC19696_rawData + 88U);
	dmrUtils_byteToBitsBE(in[12U], BPTC19696_rawData + 96U);

	// Handle the two bits
	bool bits[8U];
	dmrUtils_byteToBitsBE(in[20U], bits);
	BPTC19696_rawData[98U] = bits[6U];
	BPTC19696_rawData[99U] = bits[7U];

	// Second block
	dmrUtils_byteToBitsBE(in[21U], BPTC19696_rawData + 100U);
	dmrUtils_byteToBitsBE(in[22U], BPTC19696_rawData + 108U);
	dmrUtils_byteToBitsBE(in[23U], BPTC19696_rawData + 116U);
	dmrUtils_byteToBitsBE(in[24U], BPTC19696_rawData + 124U);
	dmrUtils_byteToBitsBE(in[25U], BPTC19696_rawData + 132U);
	dmrUtils_byteToBitsBE(in[26U], BPTC19696_rawData + 140U);
	dmrUtils_byteToBitsBE(in[27U], BPTC19696_rawData + 148U);
	dmrUtils_byteToBitsBE(in[28U], BPTC19696_rawData + 156U);
	dmrUtils_byteToBitsBE(in[29U], BPTC19696_rawData + 164U);
	dmrUtils_byteToBitsBE(in[30U], BPTC19696_rawData + 172U);
	dmrUtils_byteToBitsBE(in[31U], BPTC19696_rawData + 180U);
	dmrUtils_byteToBitsBE(in[32U], BPTC19696_rawData + 188U);
}

// Deinterleave the raw data
static void BPTC19696_decodeDeInterleave(void)
{
	for (unsigned int i = 0U; i < 196U; i++)
	{
		BPTC19696_deInterData[i] = false;
	}

	// The first bit is R(3) which is not used so can be ignored
	for (unsigned int a = 0U; a < 196U; a++)
	{
		// Calculate the interleave sequence
		unsigned int interleaveSequence = (a * 181U) % 196U;
		// Shuffle the data
		BPTC19696_deInterData[a] = BPTC19696_rawData[interleaveSequence];
	}
}

// Check each row with a Hamming (15,11,3) code and each column with a Hamming (13,9,3) code
static void BPTC19696_decodeErrorCheck(void)
{
	bool fixing;
	unsigned int count = 0U;
	do {
		fixing = false;

		// Run through each of the 15 columns
		bool col[13U];
		for (unsigned int c = 0U; c < 15U; c++)
		{
			unsigned int pos = c + 1U;
			for (unsigned int a = 0U; a < 13U; a++)
			{
				col[a] = BPTC19696_deInterData[pos];
				pos = pos + 15U;
			}

			if (Hamming_decode1393(col))
			{
				unsigned int pos = c + 1U;
				for (unsigned int a = 0U; a < 13U; a++)
				{
					BPTC19696_deInterData[pos] = col[a];
					pos = pos + 15U;
				}

				fixing = true;
			}
		}

		// Run through each of the 9 rows containing data
		for (unsigned int r = 0U; r < 9U; r++)
		{
			unsigned int pos = (r * 15U) + 1U;
			if (Hamming_decode15113_2(BPTC19696_deInterData + pos))
				fixing = true;
		}

		count++;
	} while (fixing && count < 5U);
}

// Extract the 96 bits of payload
static void BPTC19696_decodeExtractData(unsigned char* data)
{
	bool bData[96U];
	unsigned int pos = 0U;

	for (unsigned int a = 4U; a <= 11U; a++, pos++)
	{
		bData[pos] = BPTC19696_deInterData[a];
	}
	for (unsigned int a = 16U; a <= 26U; a++, pos++)
	{
		bData[pos] = BPTC19696_deInterData[a];
	}

	for (unsigned int a = 31U; a <= 41U; a++, pos++)
	{
		bData[pos] = BPTC19696_deInterData[a];
	}

	for (unsigned int a = 46U; a <= 56U; a++, pos++)
	{
		bData[pos] = BPTC19696_deInterData[a];
	}

	for (unsigned int a = 61U; a <= 71U; a++, pos++)
	{
		bData[pos] = BPTC19696_deInterData[a];
	}

	for (unsigned int a = 76U; a <= 86U; a++, pos++)
	{
		bData[pos] = BPTC19696_deInterData[a];
	}

	for (unsigned int a = 91U; a <= 101U; a++, pos++)
	{
		bData[pos] = BPTC19696_deInterData[a];
	}

	for (unsigned int a = 106U; a <= 116U; a++, pos++)
	{
		bData[pos] = BPTC19696_deInterData[a];
	}

	for (unsigned int a = 121U; a <= 131U; a++, pos++)
	{
		bData[pos] = BPTC19696_deInterData[a];
	}

	dmrUtils_bitsToByteBE(bData + 0U,  &data[0U]);
	dmrUtils_bitsToByteBE(bData + 8U,  &data[1U]);
	dmrUtils_bitsToByteBE(bData + 16U, &data[2U]);
	dmrUtils_bitsToByteBE(bData + 24U, &data[3U]);
	dmrUtils_bitsToByteBE(bData + 32U, &data[4U]);
	dmrUtils_bitsToByteBE(bData + 40U, &data[5U]);
	dmrUtils_bitsToByteBE(bData + 48U, &data[6U]);
	dmrUtils_bitsToByteBE(bData + 56U, &data[7U]);
	dmrUtils_bitsToByteBE(bData + 64U, &data[8U]);
	dmrUtils_bitsToByteBE(bData + 72U, &data[9U]);
	dmrUtils_bitsToByteBE(bData + 80U, &data[10U]);
	dmrUtils_bitsToByteBE(bData + 88U, &data[11U]);
}

// Extract the 96 bits of payload
static void BPTC19696_encodeExtractData(const unsigned char* in)
{
	bool bData[96U];
	dmrUtils_byteToBitsBE(in[0U],  bData + 0U);
	dmrUtils_byteToBitsBE(in[1U],  bData + 8U);
	dmrUtils_byteToBitsBE(in[2U],  bData + 16U);
	dmrUtils_byteToBitsBE(in[3U],  bData + 24U);
	dmrUtils_byteToBitsBE(in[4U],  bData + 32U);
	dmrUtils_byteToBitsBE(in[5U],  bData + 40U);
	dmrUtils_byteToBitsBE(in[6U],  bData + 48U);
	dmrUtils_byteToBitsBE(in[7U],  bData + 56U);
	dmrUtils_byteToBitsBE(in[8U],  bData + 64U);
	dmrUtils_byteToBitsBE(in[9U],  bData + 72U);
	dmrUtils_byteToBitsBE(in[10U], bData + 80U);
	dmrUtils_byteToBitsBE(in[11U], bData + 88U);

	for (unsigned int i = 0U; i < 196U; i++)
		BPTC19696_deInterData[i] = false;

	unsigned int pos = 0U;
	for (unsigned int a = 4U; a <= 11U; a++, pos++)
		BPTC19696_deInterData[a] = bData[pos];

	for (unsigned int a = 16U; a <= 26U; a++, pos++)
		BPTC19696_deInterData[a] = bData[pos];

	for (unsigned int a = 31U; a <= 41U; a++, pos++)
		BPTC19696_deInterData[a] = bData[pos];

	for (unsigned int a = 46U; a <= 56U; a++, pos++)
		BPTC19696_deInterData[a] = bData[pos];

	for (unsigned int a = 61U; a <= 71U; a++, pos++)
		BPTC19696_deInterData[a] = bData[pos];

	for (unsigned int a = 76U; a <= 86U; a++, pos++)
		BPTC19696_deInterData[a] = bData[pos];

	for (unsigned int a = 91U; a <= 101U; a++, pos++)
		BPTC19696_deInterData[a] = bData[pos];

	for (unsigned int a = 106U; a <= 116U; a++, pos++)
		BPTC19696_deInterData[a] = bData[pos];

	for (unsigned int a = 121U; a <= 131U; a++, pos++)
		BPTC19696_deInterData[a] = bData[pos];
}

// Check each row with a Hamming (15,11,3) code and each column with a Hamming (13,9,3) code
static void BPTC19696_encodeErrorCheck(void)
{

	// Run through each of the 9 rows containing data
	for (unsigned int r = 0U; r < 9U; r++)
	{
		unsigned int pos = (r * 15U) + 1U;
		Hamming_encode15113_2(BPTC19696_deInterData + pos);
	}

	// Run through each of the 15 columns
	bool col[13U];
	for (unsigned int c = 0U; c < 15U; c++)
	{
		unsigned int pos = c + 1U;
		for (unsigned int a = 0U; a < 13U; a++)
		{
			col[a] = BPTC19696_deInterData[pos];
			pos = pos + 15U;
		}

		Hamming_encode1393(col);

		pos = c + 1U;
		for (unsigned int a = 0U; a < 13U; a++)
		{
			BPTC19696_deInterData[pos] = col[a];
			pos = pos + 15U;
		}
	}
}

// Interleave the raw data
static void BPTC19696_encodeInterleave(void)
{
	for (unsigned int i = 0U; i < 196U; i++)
		BPTC19696_rawData[i] = false;

	// The first bit is R(3) which is not used so can be ignored
	for (unsigned int a = 0U; a < 196U; a++)
	{
		// Calculate the interleave sequence
		unsigned int interleaveSequence = (a * 181U) % 196U;
		// Unshuffle the data
		BPTC19696_rawData[interleaveSequence] = BPTC19696_deInterData[a];
	}
}

static void BPTC19696_encodeExtractBinary(unsigned char* data)
{
	// First block
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 0U,  &data[0U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 8U,  &data[1U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 16U, &data[2U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 24U, &data[3U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 32U, &data[4U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 40U, &data[5U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 48U, &data[6U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 56U, &data[7U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 64U, &data[8U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 72U, &data[9U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 80U, &data[10U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 88U, &data[11U]);

	// Handle the two bits
	unsigned char byteData;
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 96U, &byteData);
	data[12U] = (data[12U] & 0x3FU) | ((byteData >> 0) & 0xC0U);
	data[20U] = (data[20U] & 0xFCU) | ((byteData >> 4) & 0x03U);

	// Second block
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 100U,  &data[21U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 108U,  &data[22U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 116U,  &data[23U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 124U,  &data[24U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 132U,  &data[25U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 140U,  &data[26U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 148U,  &data[27U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 156U,  &data[28U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 164U,  &data[29U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 172U,  &data[30U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 180U,  &data[31U]);
	dmrUtils_bitsToByteBE(BPTC19696_rawData + 188U,  &data[32U]);
}
