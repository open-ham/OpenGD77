/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
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

#include "hotspot/BPTC19696.h"
#include "hotspot/dmrDefines.h"
#include "hotspot/dmrUtils.h"
#include "hotspot/DMRShortLC.h"
#include "hotspot/Hamming.h"
#include "hotspot/RS129.h"



static void DMRShortLC_decodeExtractBinary(const unsigned char *in);
static bool DMRShortLC_decodeErrorCheck(void);
static void DMRShortLC_decodeDeInterleave(void);
static void DMRShortLC_decodeExtractData(unsigned char *data);
static void DMRShortLC_encodeExtractData(const unsigned char *in);
static void DMRShortLC_encodeInterleave(void);
static void DMRShortLC_encodeErrorCheck(void);
static void DMRShortLC_encodeExtractBinary(unsigned char *data);

static bool DMRShortLC_rawData[196];
static bool DMRShortLC_deInterData[196];

// The main decode function
bool DMRShortLC_decode(const unsigned char *in, unsigned char *out)
{
	//  Get the raw binary
	DMRShortLC_decodeExtractBinary(in);

	// Deinterleave
	DMRShortLC_decodeDeInterleave();

	// Error check
	bool ret = DMRShortLC_decodeErrorCheck();
	if (!ret)
		return false;

	// Extract Data
	DMRShortLC_decodeExtractData(out);

	return true;
}

// The main encode function
void DMRShortLC_encode(const unsigned char *in, unsigned char *out)
{
	// Extract Data
	DMRShortLC_encodeExtractData(in);

	// Error check
	DMRShortLC_encodeErrorCheck();

	// Deinterleave
	DMRShortLC_encodeInterleave();

	//  Get the raw binary
	DMRShortLC_encodeExtractBinary(out);
}

static void DMRShortLC_decodeExtractBinary(const unsigned char *in)
{
	dmrUtils_byteToBitsBE(in[0U], DMRShortLC_rawData + 0U);
	dmrUtils_byteToBitsBE(in[1U], DMRShortLC_rawData + 8U);
	dmrUtils_byteToBitsBE(in[2U], DMRShortLC_rawData + 16U);
	dmrUtils_byteToBitsBE(in[3U], DMRShortLC_rawData + 24U);
	dmrUtils_byteToBitsBE(in[4U], DMRShortLC_rawData + 32U);
	dmrUtils_byteToBitsBE(in[5U], DMRShortLC_rawData + 40U);
	dmrUtils_byteToBitsBE(in[6U], DMRShortLC_rawData + 48U);
	dmrUtils_byteToBitsBE(in[7U], DMRShortLC_rawData + 56U);
	dmrUtils_byteToBitsBE(in[8U], DMRShortLC_rawData + 64U);
}

// Deinterleave the raw data
static void DMRShortLC_decodeDeInterleave(void)
{
	for (unsigned int i = 0U; i < 68U; i++)
		DMRShortLC_deInterData[i] = false;

	for (unsigned int a = 0U; a < 67U; a++)
	{
		// Calculate the interleave sequence
		unsigned int interleaveSequence = (a * 4U) % 67U;
		// Shuffle the data
		DMRShortLC_deInterData[a] = DMRShortLC_rawData[interleaveSequence];
	}

	DMRShortLC_deInterData[67U] = DMRShortLC_rawData[67U];
}

// Check each row with a Hamming (17,12,3) code and each column with a parity bit
static bool DMRShortLC_decodeErrorCheck(void)
{
	// Run through each of the 3 rows containing data
	Hamming_decode17123(DMRShortLC_deInterData + 0U);
	Hamming_decode17123(DMRShortLC_deInterData + 17U);
	Hamming_decode17123(DMRShortLC_deInterData + 34U);

	// Run through each of the 17 columns
	for (unsigned int c = 0U; c < 17U; c++)
	{
		bool bit = DMRShortLC_deInterData[c + 0U] ^ DMRShortLC_deInterData[c + 17U] ^ DMRShortLC_deInterData[c + 34U];
		if (bit != DMRShortLC_deInterData[c + 51U])
			return false;
	}

	return true;
}

// Extract the 36 bits of payload
static void DMRShortLC_decodeExtractData(unsigned char *data)
{
	bool bData[40U];

	for (unsigned int i = 0U; i < 40U; i++)
		bData[i] = false;

	unsigned int pos = 4U;
	for (unsigned int a = 0U; a < 12U; a++, pos++)
		bData[pos] = DMRShortLC_deInterData[a];

	for (unsigned int a = 17U; a < 29U; a++, pos++)
		bData[pos] = DMRShortLC_deInterData[a];

	for (unsigned int a = 34U; a < 46U; a++, pos++)
		bData[pos] = DMRShortLC_deInterData[a];

	dmrUtils_bitsToByteBE(bData + 0U,  &data[0U]);
	dmrUtils_bitsToByteBE(bData + 8U,  &data[1U]);
	dmrUtils_bitsToByteBE(bData + 16U, &data[2U]);
	dmrUtils_bitsToByteBE(bData + 24U, &data[3U]);
	dmrUtils_bitsToByteBE(bData + 32U, &data[4U]);
}

// Extract the 36 bits of payload
static void DMRShortLC_encodeExtractData(const unsigned char *in)
{
	bool bData[40U];
	dmrUtils_byteToBitsBE(in[0U], bData + 0U);
	dmrUtils_byteToBitsBE(in[1U], bData + 8U);
	dmrUtils_byteToBitsBE(in[2U], bData + 16U);
	dmrUtils_byteToBitsBE(in[3U], bData + 24U);
	dmrUtils_byteToBitsBE(in[4U], bData + 32U);

	for (unsigned int i = 0U; i < 68U; i++)
		DMRShortLC_deInterData[i] = false;

	unsigned int pos = 4U;
	for (unsigned int a = 0U; a < 12U; a++, pos++)
		DMRShortLC_deInterData[a] = bData[pos];

	for (unsigned int a = 17U; a < 29U; a++, pos++)
		DMRShortLC_deInterData[a] = bData[pos];

	for (unsigned int a = 34U; a < 46U; a++, pos++)
		DMRShortLC_deInterData[a] = bData[pos];
}

// Check each row with a Hamming (17,12,3) code and each column with a parity bit
static void DMRShortLC_encodeErrorCheck(void)
{
	// Run through each of the 3 rows containing data
	Hamming_encode17123(DMRShortLC_deInterData + 0U);
	Hamming_encode17123(DMRShortLC_deInterData + 17U);
	Hamming_encode17123(DMRShortLC_deInterData + 34U);

	// Run through each of the 17 columns
	for (unsigned int c = 0U; c < 17U; c++)
		DMRShortLC_deInterData[c + 51U] = DMRShortLC_deInterData[c + 0U] ^ DMRShortLC_deInterData[c + 17U] ^ DMRShortLC_deInterData[c + 34U];
}

// Interleave the raw data
static void DMRShortLC_encodeInterleave(void)
{
	for (unsigned int i = 0U; i < 72U; i++)
		DMRShortLC_rawData[i] = false;

	for (unsigned int a = 0U; a < 67U; a++)
	{
		// Calculate the interleave sequence
		unsigned int interleaveSequence = (a * 4U) % 67U;
		// Unshuffle the data
		DMRShortLC_rawData[interleaveSequence] = DMRShortLC_deInterData[a];
	}

	DMRShortLC_rawData[67U] = DMRShortLC_deInterData[67U];
}

static void DMRShortLC_encodeExtractBinary(unsigned char* data)
{
	dmrUtils_bitsToByteBE(DMRShortLC_rawData + 0U,  &data[0U]);
	dmrUtils_bitsToByteBE(DMRShortLC_rawData + 8U,  &data[1U]);
	dmrUtils_bitsToByteBE(DMRShortLC_rawData + 16U, &data[2U]);
	dmrUtils_bitsToByteBE(DMRShortLC_rawData + 24U, &data[3U]);
	dmrUtils_bitsToByteBE(DMRShortLC_rawData + 32U, &data[4U]);
	dmrUtils_bitsToByteBE(DMRShortLC_rawData + 40U, &data[5U]);
	dmrUtils_bitsToByteBE(DMRShortLC_rawData + 48U, &data[6U]);
	dmrUtils_bitsToByteBE(DMRShortLC_rawData + 56U, &data[7U]);
	dmrUtils_bitsToByteBE(DMRShortLC_rawData + 64U, &data[8U]);
}
