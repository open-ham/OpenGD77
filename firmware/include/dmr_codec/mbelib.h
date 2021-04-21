/*
 * Copyright (C) 2010 mbelib Author
 * GPG Key ID: 0xEA5EFE2C (9E7A 5527 9CDC EBF7 BF1B  D772 4F98 E863 EA5E FE2C)
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

#ifndef _OPENGD77_MBELIB_H_
#define _OPENGD77_MBELIB_H_

#include <stdbool.h>
#include <stdint.h>

void mbe_checkGolayBlock(long int *block);
int mbe_golay2312(char *in, char *out);
int mbe_eccAmbe3600x2450C0(char ambe_fr[4][24]);
int mbe_eccAmbe3600x2450Data(char ambe_fr[4][24], char *ambe_d);
void mbe_demodulateAmbe3600x2450Data(char ambe_fr[4][24]);
void prepare_framedata(uint8_t *indata, char *ambe_d, int *errs, int *errs2);

#endif /* _OPENGD77_MBELIB_H_ */
