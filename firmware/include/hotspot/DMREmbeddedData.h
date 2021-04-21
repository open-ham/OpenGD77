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

#ifndef _OPENGD77_DMREMBEDDED_DATA_H_
#define _OPENGD77_DMREMBEDDED_DATA_H_

#include "hotspot/DMRLC.h"

void DMREmbeddedData_setLC(const DMRLC_T *lc);
unsigned char DMREmbeddedData_getData(unsigned char *data, unsigned char n);
void DMREmbeddedData_initEmbeddedDataBuffers(void);
bool DMREmbeddedData_addData(const unsigned char *data, unsigned char lcss);
unsigned char DMREmbeddedData_getData(unsigned char *data, unsigned char n);
bool DMREmbeddedData_getLC(DMRLC_T *lc);
int DMREmbeddedData_getFLCO(void);
bool DMREmbeddedData_getRawData(unsigned char *data);

#endif
