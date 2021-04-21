/* -*- mode: c; c-file-style: "k&r"; compile-command: "gcc -Wall -O2 -static -s -o converter converter.c"; -*- */

/*
 * Copyright (C) 2019 F1RMB Daniel Caujolle-Bert <f1rmb.daniel@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

/*
*******************************************************************
***********              BIG FAT WARNING                ***********
*******************************************************************
*** It's a quick and dirt hack to convert Adafruit font files   ***
*** format to a usable XBitmap image. Hand editing is required. ***
***                                                             ***
***                                                             ***
*** About OpenGD77 fonts (8x16 and 16x32): the two fonts has    ***
*** been created with this tool, but hand edited, so don't cut  ***
*** and paste the glyphs generated from this tool.              ***
***                                                             ***
*******************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "gfxfont.h"


#define PROGMEM 

//#include "roboto_cond_bold_12.h"
//#include "roboto_medium_12.h"
//#include "roboto_mono_13.h"
//#include "roboto_mono_bold_14.h"

// Used as 8x16
//#include "roboto_mono_light_14.h"
// Used as 16x32
#include "roboto_mono_bold_26.h"

//GFXfont *gfxFont = (GFXfont *)&Roboto_Mono_Bold_14; //Roboto_Mono_Light_14;
GFXfont *gfxFont = (GFXfont *)&Roboto_Mono_Bold_26; //Roboto_Mono_Light_14;
//GFXfont *gfxFont = (GFXfont *)&Roboto_Mono_13; //Roboto_Mono_Light_14;

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

typedef struct
{
     uint8_t width;
     uint8_t height;
     uint8_t *bitmap;
     uint8_t bitmapLen;
     uint8_t baseline;
} myPix_t;


myPix_t myPix;


#ifndef pgm_read_byte
 #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif

#ifndef pgm_read_word
 #define pgm_read_word(addr) (*(const unsigned short *)(addr))
#endif

inline GFXglyph * pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c)
{
    return gfxFont->glyph + c;
}

static int SaveDataToFile(const char *filename, myPix_t *pix)
{
     int     retval = 0;
     int     outFD;
     ssize_t wlen;
     char    pixName[strlen(filename) + 5];
     char    buf[256];

     strcpy(pixName, filename);
     char *p;

     if ((p = strrchr(pixName, '.')))
	  *p = 0;

     while ((p = strchr(pixName, '-')))
	  *p = '_';

     // Write data to specified file
     if ((outFD = open(filename, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IWGRP)) == -1)
     {
	  perror("open()");
	  return -1;
     }
     
     sprintf(buf, "#define %s_width %d\n", pixName, pix->width);
     if ((wlen = write(outFD, buf, strlen(buf))) != strlen(buf))
     {
	  retval = -1;
	  perror("write()");
	  goto exitFailure;
     }

     sprintf(buf, "#define %s_height %d\n", pixName, pix->height);
     if ((wlen = write(outFD, buf, strlen(buf))) != strlen(buf))
     {
	  retval = -1;
	  perror("write()");
	  goto exitFailure;
     }

     sprintf(buf, "static char %s_bits[] = {\n", pixName);
     if ((wlen = write(outFD, buf, strlen(buf))) != strlen(buf))
     {
	  retval = -1;
	  perror("write()");
	  goto exitFailure;
     }

     for (uint16_t i = 0; i < pix->bitmapLen; i++)
     {
	  sprintf(buf, "0x%02x, ", *(pix->bitmap + i));
	  if ((wlen = write(outFD, buf, strlen(buf))) != strlen(buf))
	  {
	       retval = -1;
	       perror("write()");
	       goto exitFailure;
	  }

	  if ((((i + 1) % 16) == 0))
	  {
	       if (write(outFD, "\n", 1) != 1)
	       {
		    retval = -1;
		    perror("write()");
		    goto exitFailure;
	       }
	  }
     }
     
     sprintf(buf, "\n};\n");
     if ((wlen = write(outFD, buf, strlen(buf))) != strlen(buf))
     {
	  retval = -1;
	  perror("write()");
	  goto exitFailure;
     }

exitFailure:
     if ((close(outFD)) == -1)
     {
	  retval = -1;
	  perror("close()");
     }

     return retval;
}

inline uint8_t * pgm_read_bitmap_ptr(const GFXfont *gfxFont)
{
    return gfxFont->bitmap;
}

uint8_t modifyBit(uint8_t n, uint8_t p) 
{ 
    uint8_t mask = 1 << p; 
    return (n & ~mask) | ((1 << p) & mask);
}

void writePixel(myPix_t *pix, int16_t x, int16_t y, uint16_t color){
//     printf("\n%s(): X:%d, Y:%d, color: %d\n", __PRETTY_FUNCTION__, x, y, color);


//     printf("pix->bitmapLen: %d\n", pix->bitmapLen);
//     printf("pix->height: %d\n", pix->height);
     uint8_t realWlen = pix->bitmapLen / pix->height;
     uint8_t lineNum = (pix->height + (y - pix->baseline));
//     uint8_t lineRest = realWlen - ((pix->width / 8) * 8);
     
//     printf("realWlen: %d\n", realWlen);
//     printf("Width len rest: %d\n", lineRest);
//     printf("LineNum #: %d\n", lineNum);

//     printf("Byte pos: %d\n", lineNum * ((pix->width / 8) + (lineRest * lineNum))
//     uint8_t bitOffset = (lineNum * pix->width + (pix->width - ((pix->width / 8) * 8))) + x;
//     uint8_t bitOffset = ((lineNum - 1) * realWlen) + x;
//     printf("Bit offset: %d\n", bitOffset);
//     printf("Byte Offset: %d (rest: %d)\n", bitOffset / 8, bitOffset % 8);

//     uint8_t *pos = pix->bitmap + (bitOffset / 8);
     uint8_t offset = ((lineNum - 1) * realWlen) + (x / 8);
     
     //if (x > (realWlen * 8))
     
     //printf("Offset: %d\n", offset);
     
     uint8_t *pos = pix->bitmap + offset;

     if (x > 7)
     {
	  x -= (8 * (x / 8));
     }
     
     // Set bit to 1 at given position
     *pos = modifyBit(*pos, x);
}


void drawFastVLine(myPix_t *pix, int16_t x, int16_t y, int16_t h, uint16_t color) {
     printf("%s(): X: %d, Y: %d, H: %d\n", __PRETTY_FUNCTION__, x, y, h);
     //writeLine(x, y, x, y+h-1, color);
}
void writeFastVLine(myPix_t *pix, int16_t x, int16_t y, int16_t h, uint16_t color) {
     drawFastVLine(pix, x, y, h, color);
}
void fillRect(myPix_t *pix, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
     for (int16_t i=x; i<x+w; i++) {
	  writeFastVLine(pix, i, y, h, color);
     }
}
void writeFillRect(myPix_t *pix, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
     fillRect(pix, x,y,w,h,color);
}

void drawChar(myPix_t *pix, int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y) {

     printf("%s(): '%c'\n", __PRETTY_FUNCTION__, c);
     c -= (uint8_t)pgm_read_byte(&gfxFont->first);
     GFXglyph *glyph  = pgm_read_glyph_ptr(gfxFont, c);
     uint8_t  *bitmap = pgm_read_bitmap_ptr(gfxFont);
     
     uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
     uint8_t  w  = pgm_read_byte(&glyph->width),
                 h  = pgm_read_byte(&glyph->height);
     int8_t   xo = pgm_read_byte(&glyph->xOffset),
                 yo = pgm_read_byte(&glyph->yOffset);
     uint8_t  xx, yy, bits = 0, bit = 0;
     int16_t  xo16 = 0, yo16 = 0;
     
     if(size_x > 1 || size_y > 1) {
	  xo16 = xo;
	  yo16 = yo;
     }
     
     for(yy=0; yy<h; yy++) {
	  for(xx=0; xx<w; xx++) {
	       if(!(bit++ & 7)) {
                    bits = pgm_read_byte(&bitmap[bo++]);
	       }
	       if(bits & 0x80) {
                    if(size_x == 1 && size_y == 1) {
			 writePixel(pix, x+xo+xx, y+yo+yy, color);
                    } else {
			 writeFillRect(pix, x+(xo16+xx)*size_x, y+(yo16+yy)*size_y,
				       size_x, size_y, color);
                    }
	       }
	       bits <<= 1;
	  }
     }
}

void dump(myPix_t *pix)
{
     if (pix->bitmap)
     {
	  free(pix->bitmap);
	  pix->bitmap = NULL;
     }

     uint8_t w = pix->width / 8;

//     printf("W: %d\n", w);

     if ((pix->width / 8) != 0)
	  w++;
     
//     printf("W: %d\n", w);
//     uint8_t h = pix->height;
     
     pix->bitmapLen = w * pix->height;
//     printf("bitmapLen: %d\n", pix->bitmapLen);
     
     if ((pix->bitmap = calloc(pix->bitmapLen, sizeof(*pix->bitmap))) == NULL)
     {
	  perror("calloc()");
	  abort();
     }

//     printf("allocated: sizeof(*bitmap): %ld  %ld\n", sizeof(*pix->bitmap), sizeof(uint8_t));

//     drawChar(pix, 0, 0, 'R', 1, 0, 1, 1);
//     SaveDataToFile("part-00.xbm", pix);
//     return;

//     free(pix->bitmap);
     for (uint8_t i = 32; i < 126; i++)
     {
	  char buf[256];
	  
	  drawChar(pix, 0, 0, i, 1, 0, 1, 1);
	  sprintf(buf, "new-part-%02d.xbm", i - 32);
	  SaveDataToFile(buf, pix);

	  memset(pix->bitmap, 0, pix->bitmapLen);
     }

     free(pix->bitmap);
     pix->bitmap = NULL;
}
     
void findMaxSize(myPix_t *pix)
{
     pix->width = 0;
     pix->height = 0;
     printf("%s():\n", __PRETTY_FUNCTION__);

     //printf("First: %d\n", pgm_read_byte(&gfxFont->first));
     //printf("Last: %d\n", pgm_read_byte(&gfxFont->last));

     uint8_t first = pgm_read_byte(&gfxFont->first);
     for (uint8_t i = 0; i <= (pgm_read_byte(&gfxFont->last) - first); i++)
     {
	  GFXglyph *glyph  = pgm_read_glyph_ptr(gfxFont, i);
	  //uint8_t  *bitmap = pgm_read_bitmap_ptr(gfxFont);
	  
	  //uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
	  uint8_t  w  = pgm_read_byte(&glyph->width);
	  uint8_t h  = pgm_read_byte(&glyph->height);
	  //int8_t   xo = pgm_read_byte(&glyph->xOffset);
	  //int8_t yo = pgm_read_byte(&glyph->yOffset);
	  
	  //printf("'%c'[%d]: %d x %d\n", i + first, i + 1, w, h);
	  
	  pix->width = max(pix->width, w);
	  pix->height = max(pix->height, h);
     }

//printf("Max Size: %d x %d\n", pix->width, pix->height);

     uint8_t ya = (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
     printf("yAdvance: %d\n", ya);

     pix->baseline = ya - pix->height;
     printf("pix->baseline: %d\n", pix->baseline);
}


int main(int argc, char **argv)
{
     myPix.width = 0;
     myPix.height = 0;
     myPix.bitmap = NULL;
     myPix.baseline = 0;

     findMaxSize(&myPix);
     printf("Max Size: %d x %d\n", myPix.width, myPix.height);

     dump(&myPix);

     return EXIT_SUCCESS;
}
