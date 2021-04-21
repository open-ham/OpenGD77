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
#ifndef	_OPENGD77_DMRUTILS_H_
#define	_OPENGD77_DMRUTILS_H_

#include <stdbool.h>
#include <stdint.h>

void dmrUtils_byteToBitsBE(unsigned char byte, bool *bits);
void dmrUtils_byteToBitsLE(unsigned char byte, bool *bits);

void dmrUtils_bitsToByteBE(const bool *bits, uint8_t *out);
void dmrUtils_bitsToByteLE(const bool *bits, uint8_t *out);

unsigned int dmrUtils_compare(const unsigned char *bytes1, const unsigned char *bytes2, unsigned int length);

#endif
