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

#ifndef _OPENGD77_DMRLC_H_
#define _OPENGD77_DMRLC_H_

#include "hotspot/dmrDefines.h"

typedef struct DMRLC
{
	bool        PF;
	bool        R;
	int         FLCO;
	uint8_t 	FID;
	uint8_t 	options;
	uint32_t  	srcId;
	uint32_t  	dstId;
	uint8_t		rawData[12];
} DMRLC_T;

void DMRLC3(int flco, unsigned int srcId, unsigned int dstId, DMRLC_T *lc);
void DMRLCfromBytes(const unsigned char *bytes, DMRLC_T *lc);
void DMRLCfromBits(const bool *bits, DMRLC_T *lc);

void DMRLC_getDataFromBytes(unsigned char *bytes, const DMRLC_T *lc);
void DMRLC_getDataFromBits(bool *bits, const DMRLC_T *lc);

#endif
