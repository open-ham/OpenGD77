/*
 * Copyright (C)2020	Kai Ludwig, DG4KLU
 *               and	Roger Clark, VK3KYY / G4KYF
 *               and	Daniel Caujolle-Bert, F1RMB
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

#ifndef _OPENGD77_VOX_H_
#define _OPENGD77_VOX_H_

#include <stdbool.h>
#include <stdint.h>

void voxInit(void);
void voxSetParameters(uint8_t threshold, uint8_t tailHalfSecond);
bool voxIsEnabled(void);
bool voxIsTriggered(void);
void voxReset(void);
void voxTick(void);

#endif /* _OPENGD77_VOX_H_ */
