/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
 *
 *   Ported to OpenGD77 by Roger Clark VK3KYY / G4KYF
 *   Note.
 *   This file is from MMDVM_HS not MMDVMHost.
 *   It seems to be a modified version of CDMRSlot in MMDVMHost where external references to the encoding table etc
 *   have been combined inside this file.
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

#ifndef _OPENGD77_DMRSLOTTYPE_H_
#define _OPENGD77_DMRSLOTTYPE_H_

#include <stdbool.h>
#include <stdint.h>

void DMRSlotType_decode(const uint8_t *frame, uint32_t  *colorCode, uint32_t *dataType);
void DMRSlotType_encode(uint32_t colorCode, uint32_t dataType, uint8_t *frame);

#endif
