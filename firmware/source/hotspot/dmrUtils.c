/*
 *	Copyright (C) 2009,2014,2015,2016 Jonathan Naylor, G4KLX
 *
 *   Ported to OpenGD77 by Roger Clark VK3KYY / G4KYF
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 */

#include "hotspot/dmrUtils.h"


void dmrUtils_byteToBitsBE(unsigned char byte, bool *bits)
{
	bits[0U] = (byte & 0x80U) == 0x80U;
	bits[1U] = (byte & 0x40U) == 0x40U;
	bits[2U] = (byte & 0x20U) == 0x20U;
	bits[3U] = (byte & 0x10U) == 0x10U;
	bits[4U] = (byte & 0x08U) == 0x08U;
	bits[5U] = (byte & 0x04U) == 0x04U;
	bits[6U] = (byte & 0x02U) == 0x02U;
	bits[7U] = (byte & 0x01U) == 0x01U;
}

void dmrUtils_byteToBitsLE(unsigned char byte, bool *bits)
{
	bits[0U] = (byte & 0x01U) == 0x01U;
	bits[1U] = (byte & 0x02U) == 0x02U;
	bits[2U] = (byte & 0x04U) == 0x04U;
	bits[3U] = (byte & 0x08U) == 0x08U;
	bits[4U] = (byte & 0x10U) == 0x10U;
	bits[5U] = (byte & 0x20U) == 0x20U;
	bits[6U] = (byte & 0x40U) == 0x40U;
	bits[7U] = (byte & 0x80U) == 0x80U;
}


void dmrUtils_bitsToByteBE(const bool* bits, uint8_t *out)
{
	*out  = bits[0U] ? 0x80U : 0x00U;
	*out |= bits[1U] ? 0x40U : 0x00U;
	*out |= bits[2U] ? 0x20U : 0x00U;
	*out |= bits[3U] ? 0x10U : 0x00U;
	*out |= bits[4U] ? 0x08U : 0x00U;
	*out |= bits[5U] ? 0x04U : 0x00U;
	*out |= bits[6U] ? 0x02U : 0x00U;
	*out |= bits[7U] ? 0x01U : 0x00U;
}

void dmrUtils_bitsToByteLE(const bool* bits, uint8_t *out)
{
	*out  = bits[0U] ? 0x01U : 0x00U;
	*out |= bits[1U] ? 0x02U : 0x00U;
	*out |= bits[2U] ? 0x04U : 0x00U;
	*out |= bits[3U] ? 0x08U : 0x00U;
	*out |= bits[4U] ? 0x10U : 0x00U;
	*out |= bits[5U] ? 0x20U : 0x00U;
	*out |= bits[6U] ? 0x40U : 0x00U;
	*out |= bits[7U] ? 0x80U : 0x00U;
}

unsigned int dmrUtils_compare(const unsigned char *bytes1, const unsigned char *bytes2, unsigned int length)
{
	unsigned int diffs = 0U;

	for (unsigned int i = 0U; i < length; i++)
	{
		unsigned char v = bytes1[i] ^ bytes2[i];
		while (v != 0U)
		{
			v &= v - 1U;
			diffs++;
		}
	}

	return diffs;
}
