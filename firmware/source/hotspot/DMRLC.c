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

#include "hotspot/dmrDefines.h"
#include "hotspot/dmrUtils.h"
#include "hotspot/DMRLC.h"

void DMRLC3(int flco, unsigned int srcId, unsigned int dstId, DMRLC_T *lc)
{
	lc->PF = false;
	lc->R = false;
	lc->FLCO = flco;
	lc->FID = 0U;
	lc->options = 0U;
	lc->srcId = srcId;
	lc->dstId = dstId;
}

void DMRLCfromBytes(const unsigned char *bytes, DMRLC_T *lc)
{
	lc->PF = false;
	lc->R = false;
	lc->FLCO = FLCO_GROUP;
	lc->FID = 0U;
	lc->options = 0U;
	lc->srcId = 0;
	lc->dstId = 0;

	lc->PF = (bytes[0U] & 0x80U) == 0x80U;
	lc->R  = (bytes[0U] & 0x40U) == 0x40U;

	lc->FLCO = bytes[0U] & 0x3FU;

	lc->FID = bytes[1U];

	lc->options = bytes[2U];

	lc->dstId = (((uint32_t)bytes[3U]) << 16) + (((uint32_t)bytes[4U]) << 8) + ((uint32_t)bytes[5U]);
	lc->srcId = (((uint32_t)bytes[6U]) << 16) + (((uint32_t)bytes[7U]) << 8) + ((uint32_t)bytes[8U]);
}

void DMRLCfromBits(const bool *bits, DMRLC_T *lc)
{
	lc->PF = false;
	lc->R = false;
	lc->FLCO = FLCO_GROUP;
	lc->FID = 0U;
	lc->options = 0U;
	lc->srcId = 0U;
	lc->dstId = 0U;

	lc->PF = bits[0U];
	lc->R  = bits[1U];

	uint8_t temp1, temp2, temp3;
	dmrUtils_bitsToByteBE(bits + 0U, &temp1);
	lc->FLCO = temp1 & 0x3FU;

	dmrUtils_bitsToByteBE(bits + 8U, &temp2);
	lc->FID = temp2;

	dmrUtils_bitsToByteBE(bits + 16U, &temp3);
	lc->options = temp3;

	uint8_t d1, d2, d3;
	dmrUtils_bitsToByteBE(bits + 24U, &d1);
	dmrUtils_bitsToByteBE(bits + 32U, &d2);
	dmrUtils_bitsToByteBE(bits + 40U, &d3);

	uint8_t s1, s2, s3;
	dmrUtils_bitsToByteBE(bits + 48U, &s1);
	dmrUtils_bitsToByteBE(bits + 56U, &s2);
	dmrUtils_bitsToByteBE(bits + 64U, &s3);

	lc->srcId = (((uint32_t)s1) << 16) | (((uint32_t)s2) << 8) | ((uint32_t)s3);
	lc->dstId = (((uint32_t)d1) << 16) | (((uint32_t)d2) << 8) | ((uint32_t)d3);
}

void DMRLC0(DMRLC_T *lc)
{
	lc->PF = false;
	lc->R = false;
	lc->FLCO = FLCO_GROUP;
	lc->FID = 0U;
	lc->options = 0U;
	lc->srcId = 0U;
	lc->dstId = 0U;
}


void DMRLC_getDataFromBytes(unsigned char *bytes, const DMRLC_T *lc)
{
	bytes[0U] = (unsigned char)lc->FLCO;

	if (lc->PF)
	{
		bytes[0U] |= 0x80U;
	}
	if (lc->R)
	{
		bytes[0U] |= 0x40U;
	}

	bytes[1U] = lc->FID;

	bytes[2U] = lc->options;

	bytes[3U] = (lc->dstId >> 16) & 0xFF;
	bytes[4U] = (lc->dstId >> 8) & 0xFF;
	bytes[5U] = (lc->dstId & 0xFF);

	bytes[6U] = (lc->srcId >> 16) & 0xFF;
	bytes[7U] = (lc->srcId >> 8) & 0xFF;
	bytes[8U] = (lc->srcId  & 0xFF);
}

void DMRLC_getDataFromBits(bool *bits, const DMRLC_T *lc)
{
	unsigned char bytes[9U];
	DMRLC_getDataFromBytes(bytes,lc);

	dmrUtils_byteToBitsBE(bytes[0U], bits + 0U);
	dmrUtils_byteToBitsBE(bytes[1U], bits + 8U);
	dmrUtils_byteToBitsBE(bytes[2U], bits + 16U);
	dmrUtils_byteToBitsBE(bytes[3U], bits + 24U);
	dmrUtils_byteToBitsBE(bytes[4U], bits + 32U);
	dmrUtils_byteToBitsBE(bytes[5U], bits + 40U);
	dmrUtils_byteToBitsBE(bytes[6U], bits + 48U);
	dmrUtils_byteToBitsBE(bytes[7U], bits + 56U);
	dmrUtils_byteToBitsBE(bytes[8U], bits + 64U);
}
