/* -*- mode: c; c-file-style: "k&r"; compile-command: "gcc -Wall -O2 -I../../firmware/include/hardware -static -s -o UC1701_font_tool UC1701_font_tool.c"; -*- */

/*
 * Copyright (C) 2019-2020 F1RMB Daniel Caujolle-Bert <f1rmb.daniel@gmail.com>
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

// Support %zu printf format under gcc for windows 
#if defined(__MINGW64__)
#define __USE_MINGW_ANSI_STDIO 1
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <wchar.h>
#include <locale.h>

#ifndef BUILD_FONT_TOOL
#define BUILD_FONT_TOOL
#endif

#ifdef JAPANESE_BUILD
#include "UC1701_charset_JA.h"
#else
#include "UC1701_charset.h"
#endif

#define VERSION_MAJOR     0
#define VERSION_MINOR     0
#define VERSION_REVISION  1

typedef struct
{
     int       width;
     int       height;
     int       bytesPerChar;
     int       charWidth;
     int       charHeight;
     uint8_t  *data;
     size_t    dataLen;
} xbitmap_t;

typedef struct {
     char mask;    /* char data will be bitwise AND with this */
     char lead;    /* start bytes of current char in utf-8 encoded character */
     uint32_t beg; /* beginning of codepoint range */
     uint32_t end; /* end of codepoint range */
     int bits_stored; /* the number of bits from the codepoint that fits in char */
} utf_t;
 
utf_t *utf[] = {
     /*             mask        lead        beg      end       bits */
     [0] = &(utf_t){0b00111111, 0b10000000, 0,       0,        6    },
     [1] = &(utf_t){0b01111111, 0b00000000, 0000,    0177,     7    },
     [2] = &(utf_t){0b00011111, 0b11000000, 0200,    03777,    5    },
     [3] = &(utf_t){0b00001111, 0b11100000, 04000,   0177777,  4    },
     [4] = &(utf_t){0b00000111, 0b11110000, 0200000, 04177777, 3    },
     &(utf_t){0},
};

uint8_t *hand_assigned(uint32_t cp)
{
     for (uint32_t i = 0; i < (sizeof(char_hand_assigned) / sizeof(char_hand_assigned[0])); i++)
     {
	  if (char_hand_assigned[i].offset == cp)
	       return char_hand_assigned[i].character;
     }

     return NULL;
}

int unusedChar(uint32_t c)
{
     for (uint32_t i = 0; i < (sizeof(freePositions) / sizeof(freePositions[0])); i++)
     {
	  if (c == freePositions[i])
	       return 1;
     }
     
     return 0;
}

int codepoint_len(const uint32_t cp)
{
     int len = 0;
     
     for(utf_t **u = utf; *u; ++u) {
	  if((cp >= (*u)->beg) && (cp <= (*u)->end)) {
	       break;
	  }
	  ++len;
     }

     if(len > 4) { /* Out of bounds */
	  fprintf(stderr, "%s() Out of bounds\n", __PRETTY_FUNCTION__);
	  exit(-11);
     }
     return len;
}

char *to_utf8(const uint32_t cp)
{
     static char ret[5];
     char *assignedChar;

     if (unusedChar(cp))
     {
	  ret[0] = 'F';
	  ret[1] = 'R';
	  ret[2] = 'E';
	  ret[3] = 'E';
	  ret[4] = 0;
     }
     else if ((assignedChar = (char *)hand_assigned(cp)))
     {
	  static char buf[6];
	  // Add a marker for hand assigned glyph
	  sprintf(buf, "*%s", assignedChar);
	  return buf;
	  //return assignedChar;
     }
     else
     {
	  const int bytes = codepoint_len(cp);
	  
	  int shift = utf[0]->bits_stored * (bytes - 1);
	  ret[0] = (cp >> shift & utf[bytes]->mask) | utf[bytes]->lead;
	  shift -= utf[0]->bits_stored;
	  for(int i = 1; i < bytes; ++i) {
	       ret[i] = (cp >> shift & utf[0]->mask) | utf[0]->lead;
	       shift -= utf[0]->bits_stored;
	  }
	  ret[bytes] = '\0';
     }
     
     return ret;
}

char *GDtoUTF8(uint8_t c)
{
     if ((c - 32) > (CHARS_PER_FONT - 1))
     {
	  fprintf(stderr, "%s() Out of font boundaries character '%c' (%d oct:%o), MaxValue is %d)\n", __PRETTY_FUNCTION__, c, c, c, (CHARS_PER_FONT - 1));
	  exit(-1);
     }

     for (uint32_t i = 0; i < (sizeof(char_hand_assigned) / sizeof(char_hand_assigned[0])); i++)
     {
	  if (char_hand_assigned[i].octal == c)
	  {
	       static char buf[5];
	       // Add a marker for hand assigned glyph
	       sprintf(buf, "%s", char_hand_assigned[i].character);
	       return buf;
	  }
     }

     // Not hand assigned, returns UTF-8 sequence
     static char ret[5];
     const int bytes = codepoint_len(c);
     
     int shift = utf[0]->bits_stored * (bytes - 1);
     ret[0] = (c >> shift & utf[bytes]->mask) | utf[bytes]->lead;
     shift -= utf[0]->bits_stored;
     for(int i = 1; i < bytes; ++i) {
	  ret[i] = (c >> shift & utf[0]->mask) | utf[0]->lead;
	  shift -= utf[0]->bits_stored;
     }
     ret[bytes] = '\0';
     
     return ret;
}

uint8_t UTF8toGD(wint_t c, size_t *cLen)
{
     uint32_t cp = c;
     const int bytes = codepoint_len(cp);
     static char ret[5];
     
     int shift = utf[0]->bits_stored * (bytes - 1);
     ret[0] = (cp >> shift & utf[bytes]->mask) | utf[bytes]->lead;
     shift -= utf[0]->bits_stored;
     for(int i = 1; i < bytes; ++i) {
	  ret[i] = (cp >> shift & utf[0]->mask) | utf[0]->lead;
	  shift -= utf[0]->bits_stored;
     }
     ret[bytes] = '\0';

     if (bytes == 1)
     {
	  if (unusedChar(c))
	  {
	       fprintf(stderr, "%s() character '%c' (%d oct:%o) is not part of the font yet.\n", __PRETTY_FUNCTION__, c, c, c);
	       exit(-1);
	  }
	  
	  *cLen = 1;
	  return (uint8_t)c;
     }

      for (uint32_t i = 0; i < (sizeof(char_hand_assigned) / sizeof(char_hand_assigned[0])); i++)
     {
	  //printf("compare '%s' %zu to '%s' %zu %d\n", char_hand_assigned[i].character, strlen((char *)char_hand_assigned[i].character), ret, strlen(ret), (uint8_t)ret[0]);
	  if (strncmp((char *)char_hand_assigned[i].character, ret, bytes) == 0)
	  {
	       static char buf[5];
	       // Add a marker for hand assigned glyph
	       sprintf(buf, "%c", char_hand_assigned[i].octal);
	       *cLen = bytes;
	       return char_hand_assigned[i].octal;
	  }
     }

     
     *cLen = 1;
     return c;
}

uint8_t willEncodeAsBinary(wint_t c)
{
     uint32_t cp = c;
     const int bytes = codepoint_len(cp);
     static char ret[5];
     
     int shift = utf[0]->bits_stored * (bytes - 1);
     ret[0] = (cp >> shift & utf[bytes]->mask) | utf[bytes]->lead;
     shift -= utf[0]->bits_stored;
     for(int i = 1; i < bytes; ++i) {
	  ret[i] = (cp >> shift & utf[0]->mask) | utf[0]->lead;
	  shift -= utf[0]->bits_stored;
     }
     ret[bytes] = '\0';

     if (bytes == 1)
     {
	  return 0;
     }

      for (uint32_t i = 0; i < (sizeof(char_hand_assigned) / sizeof(char_hand_assigned[0])); i++)
     {
	  if (strncmp((char *)char_hand_assigned[i].character, ret, bytes) == 0)
	  {
	       // 5 first entries are windows-1252
	       if (i >= 5)
	       {
		    return 1;
	       }
	  }
     }

      return 0;
}


/* 
 * Reverse bits in byte
 */
static uint8_t reverseBits(uint8_t b)
{
     b = (b & 0xf0) >> 4 | (b & 0x0f) << 4;
     b = (b & 0xcc) >> 2 | (b & 0x33) << 2;
     b = (b & 0xaa) >> 1 | (b & 0x55) << 1;
     return b;
}

/*
 * Save the content of the buffer to given filename
 */
static int SaveDataToFile(const char *filename, uint8_t *data, size_t dataLen)
{
     int     retval = 0;
     int     outFD;
     ssize_t wlen;
     
     // Write data to specified file
     if ((outFD = open(filename, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IWGRP)) == -1)
     {
	  perror("open()");
	  return -1;
     }
     
     //fprintf(stdout, "Writing %s\n", filename);
     
     if ((wlen = write(outFD, data, dataLen)) != dataLen)
     {
	  retval = -1;
	  perror("write()");
     }
     
     if ((close(outFD)) == -1)
     {
	  retval = -1;
	  perror("close()");
     }

     return retval;
}

/*
 * Parse XBitmap file and extract datas
 */
static int parseXBitmap(uint8_t *buffer, xbitmap_t *bitmap)
{
     char *p;
     char buf[32];
     char *pbuf;

     memset(buf, 0, sizeof(buf) / sizeof(buf[0]));
     
     if ((p = strstr((char *)buffer, "_width")) != NULL)
     {
	  pbuf = buf;
	  p += 6;
	  
	  while (*p == ' ')
	       p++;

	  while (*p != ' ')
	  {
	       *pbuf = *p;

	       p++;
	       pbuf++;
	  }
	  *pbuf = 0;

	  bitmap->width = atoi(buf);
     }
     else
	  return -1;
     
     if ((p = strstr((char *)buffer, "_height")))
     {
	  pbuf = buf;
	  p += 7;
	  
	  while (*p == ' ')
	       p++;

	  while (*p != ' ')
	  {
	       *pbuf = *p;

	       p++;
	       pbuf++;
	  }
	  *pbuf = 0;

	  bitmap->height = atoi(buf);
     }
     else
	  return -1;
     
     if ((p = strstr((char *)buffer, "_bits")))
     {
	  p += 5;

	  while ((*p != '0') && (*(p + 1) != 'x'))
	       p++;

	  bitmap->bytesPerChar = (((bitmap->height / CHARS_PER_FONT) * bitmap->width) / 8);
	  bitmap->charWidth = bitmap->height / CHARS_PER_FONT;
	  bitmap->charHeight = bitmap->width;
	  bitmap->dataLen = bitmap->bytesPerChar * CHARS_PER_FONT;
	  
	  uint8_t *data = NULL;

	  if ((data = calloc(bitmap->dataLen, sizeof(uint8_t))) == NULL)
	  {
	       perror("calloc()");
	       return -1;
	  }

	  char   *temp = strdup(p);
	  char   *ptemp = temp;
	  char   *otemp = temp;
	  size_t  counter = bitmap->dataLen;
	  
	  while ((counter > 0) && ((ptemp = strchr(temp, ',')) || ((ptemp = strchr(temp, '}')))))
	  {
	       char *pvalue = temp;
	       
	       *ptemp = 0;
	       temp += strlen(temp) + 1;
	       
	       *(data + (bitmap->dataLen - counter)) = (uint8_t)strtol(pvalue, NULL, 16);
	       
	       counter--;
	  }

	  free(otemp);
	  
	  bitmap->data = data;
     }
     else
	  return -1;
     
     return 0;
}

/*
 * Import XBitmap, then save it as UC1701 font array.
 */
static void importXBitmap(const char *filename)
{
     struct stat statBuf;
     
     // Unable to stat() the file (missing ?)
     if (stat(filename, &statBuf) == -1)
     {
	  fprintf(stderr, "Error. Unable to open the file '%s'\n", filename);
	  return;
     }

     // Check file size
     if (statBuf.st_size > 0)
     {
	  int        inFD;
	  uint8_t   *buffer = NULL;
	  xbitmap_t  bitmap = { .width = 0, .height = 0, .bytesPerChar = 0, .data = NULL, .dataLen = 0 };
	  char       outFilename[strlen(filename) + strlen("-to_import.h") + 1];

	  fprintf(stdout, "Importing XBitmap font '%s'...\n", filename);
	  
	  // Open the file for reading
	  if ((inFD = open(filename, O_RDONLY)) == -1)
	  {
	       perror("open()");
	       return;
	  }
	  
	  // Alloc memory chunk to store file content
	  if ((buffer = calloc(statBuf.st_size + 1, sizeof(uint8_t))) == NULL)
	  {
	       perror("calloc()");

	       if (close(inFD) == -1)
	       {
		    perror("close()");
	       }
	       
	       return;
	  }

	  // read file content failed or size mismatched
	  if (read(inFD, buffer, statBuf.st_size) != statBuf.st_size)
	  {
	       perror("read()");
	       
	       free(buffer);
	       
	       if (close(inFD) == -1)
	       {
		    perror("close()");
	       }
	       return;
	  }

	  // Remove unwanted characters
	  uint8_t  *p = buffer;

	  while (*p)
	  {
	       if (*p == '\t')
		    *p = ' ';

	       if ((*p == '\n') || (*p == '\r'))
		    *p = ' ';
	       
	       p++;
	  }
	  
	  if (parseXBitmap(buffer, &bitmap) != 0)
	  {
	       fprintf(stderr, "Error while parsing XBitmap source file.\n");
	       
	       if (buffer != NULL)
		    free(buffer);
	       
	       if (bitmap.data != NULL)
		    free(bitmap.data);
	       
	       if (close(inFD) == -1)
	       {
		    perror("close()");
	       }
	       return;
	  }
	  
#if 0
	  printf("Bitmap Width: %d\n", bitmap.width);
	  printf("Bitmap Height: %d\n", bitmap.height);
	  printf("Bitmap Char Width: %d\n", bitmap.charWidth);
	  printf("Bitmap Char Height: %d\n", bitmap.charHeight);
	  printf("Bitmap Bytes per Char %d\n", bitmap.bytesPerChar);
	  printf("Bitmap Data Length: %zu\n", bitmap.dataLen);
	  printf("Number of chars: %zu\n", (bitmap.dataLen / bitmap.bytesPerChar));
	  printf("First Char: %d\n", 32);
	  printf("Last Char: %zu\n", ((bitmap.dataLen / bitmap.bytesPerChar) + 32) - 1);
#endif
 
	  memcpy(outFilename, filename, strlen(filename) + 1);

	  char *token;
	  
	  if ((token = strrchr(outFilename, '.')))
	       *token = 0;

	  size_t   lineLen = 0;
	  switch (bitmap.charHeight)
	  {
	  case 8:
	  case 16:
	       lineLen = (/* '   ' */ 3 + ((/* 0x.. */ 4 + /* ', ' */ 2 ) * bitmap.bytesPerChar) + /* '// [000] 1234\n' */ 17 + 1);
	       break;
	  case 32:
	       /*                                                                                             */
	       lineLen = (/* '   ' */ 3 + ((/* 0x.. */ 4 + /* ', ' */ 2 ) * (bitmap.bytesPerChar / 4)) + /* '// [000] 1234\n' */ 17 + 1);
	       lineLen *= 4; // Lines are splitted in 4
	       break;
	       
	  default:
	       fprintf(stderr, "Unsupported matrix size\n");
	       abort();
	  }

	  size_t bufferLen = 2048 + (CHARS_PER_FONT * lineLen) + /* '};' */ 2 + /* NULL */1;
	  char outBuffer[bufferLen];
	  char *fontName = strdup(outFilename);
	  strcat(outFilename, "-to_import.h");

	  sprintf(outBuffer, "const uint8_t %s[] = {\n   0x00, 0x00, // Ignored.\n   32        , // first char ASCII code.\n   %3zu       , // last char ASCII code.\n   %-10d, // width of the character in pixels.\n   %-10d, // height of the character in pixels.\n   %-10d, // number of line of bytes per character.\n   %-10d, // bytes per character (normally width * height / 8).\n",
		  fontName, (((bitmap.dataLen / bitmap.bytesPerChar) + 32) - 1), bitmap.charWidth, bitmap.charHeight, (bitmap.charHeight == 32) ? 4 : 1, bitmap.bytesPerChar);

	  p = bitmap.data;
	  if ((bitmap.charHeight == 6) || (bitmap.charHeight == 8))
	  {
	       size_t counter = 0;
	       for (size_t i = 0; i < bitmap.dataLen; i++)
	       {
		    if (counter == 0)
		    {
			 strcat(outBuffer, "   ");
		    }
		    
		    if (counter > 0)
		    {
			 strcat(outBuffer, ", ");
		    }
		    
		    sprintf(outBuffer, "%s0x%02x", outBuffer, reverseBits((*p)));
		    p++;
		    
		    if (bitmap.charWidth == 6)
		    {
			 if (counter == 5)
			 {
			      sprintf(outBuffer, "%s, // [%3zu] '%s'\n", outBuffer, 32 + (i / bitmap.charWidth), to_utf8(32 + (i / bitmap.charWidth)));
			 }
			 
			 counter = (counter + 1) % 6;
		    }
		    else if (bitmap.charWidth == 8)
		    {
			 if (counter == 7)
			 {
			      sprintf(outBuffer, "%s, // [%3zu] '%s'\n", outBuffer, 32 + (i / bitmap.charWidth), to_utf8(32 + (i / bitmap.charWidth)));
			 }
			 
			 counter = (counter + 1) % 8;
		    }
	       }
	  }
	  else if (bitmap.charHeight == 16)
	  {
	       int counter = 0;
	       for(size_t i = 0; i < /*CHARS_PER_FONT*/ bitmap.dataLen / bitmap.bytesPerChar; i++)
	       {
		    uint8_t pix[16];
		    uint8_t *p1 = pix, *p2 = p1 + 8;
		    
		    for (uint8_t j = 0; j < 8; j++)
		    {
			 *p2 = reverseBits(*p);
			 *p1 = reverseBits(*(p + 1));
			 
			 p1++;
			 p2++;
			 
			 p += 2;
		    }
		    
		    // Write data to output file
		    for (uint8_t j = 0; j < bitmap.bytesPerChar; j++)
		    {
			 if (counter == 0)
			 {
			      strcat(outBuffer, "   ");
			 }
			 
			 if (counter > 0)
			 {
			      strcat(outBuffer, ", ");
			 }
			 
			 sprintf(outBuffer, "%s0x%02X", outBuffer, pix[j]);
			 
			 if (counter == 15)
			 {
			      sprintf(outBuffer, "%s, // [%3zu] '%s'\n", outBuffer, 32 + i, to_utf8(32 + i));
			 }
			 
			 counter = (counter + 1) % 16;
		    }
	       }
	  }
	  else if (bitmap.charHeight == 32)
	  {
	       int counter = 0;
	       for(size_t i = 0; i < bitmap.dataLen / bitmap.bytesPerChar; i++)
	       {
		    uint8_t pix[16 * 4];
		    uint8_t *p1 = pix, *p2 = p1 + 16, *p3 = p2 + 16, *p4 = p3 + 16;
		    
		    for (uint8_t j = 0; j < 16; j++)
		    {
			 *p1 = reverseBits(*(p + 3));
			 *p2 = reverseBits(*(p + 2));
			 *p3 = reverseBits(*(p + 1));
			 *p4 = reverseBits(*(p + 0));

			 p1++;
			 p2++;
			 p3++;
			 p4++;
			 
			 p += 4;
		    }
		    
		    // Write data to output file
		    for (uint8_t j = 0; j < bitmap.bytesPerChar; j++)
		    {
			 if (counter == 0)
			 {
			      strcat(outBuffer, "   ");
			 }
			 
			 if (counter > 0)
			 {
			      strcat(outBuffer, ", ");
			 }
			 
			 sprintf(outBuffer, "%s0x%02x", outBuffer, pix[j]);
			 
			 if (counter == 15)
			 {
			      if (j == (bitmap.bytesPerChar -1))
				   sprintf(outBuffer, "%s, // [%3zu] '%s'\n", outBuffer, 32 + i, to_utf8(32 + i));
			      else
				   strcat(outBuffer, ",\n");
			 }
			 
			 counter = (counter + 1) % 16;
		    }
	       }
	  }
	  
	  if ((token = strrchr(outBuffer, ',')))
	  {
	       *token = ' ';
	  }

	  strcat(outBuffer, "};\n");
	  
	  // Save buffer to file
	  fprintf(stdout, "Exporting font as UC1701 formatted array to '%s'\n", outFilename);
	  if (SaveDataToFile(outFilename, (uint8_t *)outBuffer, strlen(outBuffer)) == 0)
	  {
	       fprintf(stdout, "File '%s' successfuly saved.\n\n", outFilename);
	  }

	  if (buffer != NULL)
	       free(buffer);
	  
	  if (bitmap.data != NULL)
	       free(bitmap.data);
	  
	  if (close(inFD) == -1)
	  {
	       perror("close()");
	       return;
	  }
     }
}

/*
 * Exports UC1701 font to XBitmap
 */
static void exportUC1701Font(const uint8_t *fontData, const char *fontName)
{
     int16_t  charWidthPixels;
     int16_t  charHeightPixels;
     int16_t  bytesPerChar;
     int16_t  startCode;
     int16_t  endCode;
     int16_t  pageHeightPerCharacter;
     uint8_t *currentFont = (uint8_t *)fontData;
     
     fprintf(stdout, "Exporting font '%s' to XBitmap file '%s-exported.xbm'\n", fontName, fontName);

     startCode               = *(currentFont + 2);  // get first defined character
     endCode                 = *(currentFont + 3);
     charWidthPixels         = *(currentFont + 4);  // width in pixel of one char
     charHeightPixels  	     = *(currentFont + 5);  // page count per char
     pageHeightPerCharacter  = *(currentFont + 6);
     bytesPerChar            = *(currentFont + 7);  // bytes per char

#if 0
     printf("  startCode: %d\n", startCode);
     printf("  endCode: %d\n", endCode);
     printf("  Number of Chars: %d\n", (endCode - startCode) + 1);
     printf("  charWidthPixels: %d\n", charWidthPixels);
     printf("  charHeightPixels: %d\n", charHeightPixels);
     printf("  pageHeightPerCharacter: %d\n", pageHeightPerCharacter);
     printf("  bytesPerChar: %d\n", bytesPerChar);
#else
     (void)pageHeightPerCharacter;
#endif
     
     size_t   fontLength = (endCode - startCode) + 1;
     uint8_t *data = (currentFont + 8);
     size_t   lineLen = 0;
     
     char     filename[256];
     
     switch (charHeightPixels)
     {
     case 8:
     case 16:
	  lineLen = (/* '   ' */ 3 + ((/* 0x.. */ 4 + /* ', ' */ 2 ) * bytesPerChar) + /* '\n' */ 1 + 1);
	  break;
     case 32:
	  /*                                                                                             */
	  lineLen = (/* '   ' */ 3 + ((/* 0x.. */ 4 + /* ', ' */ 2 ) * (bytesPerChar / 4)) + /* '\n' */ 1 + 1);
	  lineLen *= 4; // Lines are splitted in 4
	  break;
	  
     default:
	  fprintf(stderr, "Unsupported matrix size\n");
	  abort();
     }

     
     size_t bufferLen = 256 + (fontLength * lineLen) + /* '};' */ 2 + /* NULL */1;
     char buffer[bufferLen];
     
     memset(buffer, 0, (sizeof(buffer) / sizeof(buffer[0])));
     memset(filename, 0, (sizeof(filename) / sizeof(filename[0])));
     
     sprintf(filename, "%s%s", fontName, "-exported.xbm");
     
     // 8x6 and 8x8
     if (charHeightPixels == 8)
     {
	  uint8_t *p = data;

	  memset(buffer, 0, (sizeof(buffer) / sizeof(buffer[0])));
	  
	  sprintf(buffer, "#define %s_width %d\n", fontName, charHeightPixels);
	  sprintf(buffer, "%s#define %s_height %zu\n", buffer, fontName, fontLength * charWidthPixels);
	  strcat(buffer, "static unsigned char ");
	  sprintf(buffer, "%s%s_bits[] = {\n", buffer, fontName);
	  
	  int counter = 0;
	  for(size_t i = 0; i < fontLength; i++)
	  {
	       for (uint8_t j = 0; j < charWidthPixels; j++)
	       {
		    if (counter == 0)
		    {
			 strcat(buffer, "   ");
		    }
		    
		    if (counter > 0)
		    {
			 strcat(buffer, ", ");
		    }
		    
		    sprintf(buffer, "%s0x%02x", buffer, reverseBits((*p)));
		    p++;

		    if (charWidthPixels == 6)
		    {
			 if (counter == 5)
			 {
			      strcat(buffer, ",\n");
			 }
			 
			 counter = (counter + 1) % 6;
		    }
		    else if (charWidthPixels == 8)
		    {
			 if (counter == 7)
			 {
			      strcat(buffer, ",\n");
			 }
			 
			 counter = (counter + 1) % 8;
		    }
	       }
	  }
     } // 8x16
     else if (charHeightPixels == 16)
     {
	  uint8_t *p = data;
	  
	  memset(buffer, 0, (sizeof(buffer) / sizeof(buffer[0])));
	  
	  sprintf(buffer, "#define %s_width %d\n", fontName, charHeightPixels);
	  sprintf(buffer, "%s#define %s_height %zu\n", buffer, fontName, fontLength * charWidthPixels);
	  strcat(buffer, "static unsigned char ");
	  sprintf(buffer, "%s%s_bits[] = {\n", buffer, fontName);
	  
	  int counter = 0;
	  for(size_t i = 0; i < fontLength; i++)
	  {
	       uint8_t pix[16];
	       uint8_t *p2 = pix, *p1 = p2 + 1;
	       
	       for (uint8_t j = 0; j < 8; j++)
	       {
		    *p1 = reverseBits(*p);
		    *p2 = reverseBits(*(p + 8));
		    
		    p1 += 2;
		    p2 += 2;
		    
		    p++;
	       }
	       
	       p += 8;
	       
	       // Write data to output file
	       for (uint8_t j = 0; j < bytesPerChar; j++)
	       {
		    if (counter == 0)
		    {
			 strcat(buffer, "   ");
		    }
		    
		    if (counter > 0)
		    {
			 strcat(buffer, ", ");
		    }
		    
		    sprintf(buffer, "%s0x%02x", buffer, pix[j]);
		    
		    if (counter == 15)
		    {
			 strcat(buffer, ",\n");
		    }
		    
		    counter = (counter + 1) % 16;
	       }
	  }
     } // 16x32
     else if (charHeightPixels == 32)
     {
	  uint8_t *p = data;
	  
	  memset(buffer, 0, (sizeof(buffer) / sizeof(buffer[0])));
	  
	  sprintf(buffer, "#define %s_width %d\n", fontName, charHeightPixels);
	  sprintf(buffer, "%s#define %s_height %zu\n", buffer, fontName, fontLength * charWidthPixels);
	  strcat(buffer, "static unsigned char ");
	  sprintf(buffer, "%s%s_bits[] = {\n", buffer, fontName);
	  
	  int counter = 0;
	  for(size_t i = 0; i < fontLength; i++)
	  {
	       uint8_t pix[16 * 4];
	       uint8_t *p4 = pix, *p3 = p4 + 1, *p2 = p3 + 1, *p1 = p2 + 1;
	       
	       for (uint8_t j = 0; j < 16; j++)
	       {
		    *p1 = reverseBits(*p);
		    *p2 = reverseBits(*(p + 16));
		    *p3 = reverseBits(*(p + 32));
		    *p4 = reverseBits(*(p + 48));
		    
		    p1 += 4;
		    p2 += 4;
		    p3 += 4;
		    p4 += 4;
		    
		    p++;
	       }
	       
	       p += 48;
	       
	       // Write data to output file
	       for (uint8_t j = 0; j < bytesPerChar; j++)
	       {
		    if (counter == 0)
		    {
			 strcat(buffer, "   ");
		    }
		    
		    if (counter > 0)
		    {
			 strcat(buffer, ", ");
		    }
		    
		    sprintf(buffer, "%s0x%02x", buffer, pix[j]);
		    
		    if (counter == 15)
		    {
			 strcat(buffer, ",\n");
		    }
		    
		    counter = (counter + 1) % 16;
	       }
	  }
     }
     
     buffer[strlen(buffer) - 2] = 0;
     strcat(buffer, "\n};\n");

     if (SaveDataToFile(filename, (uint8_t *)buffer, strlen(buffer)) == 0)
     {
	  fprintf(stdout, "File '%s' successfuly saved.\n\n", filename);
     }
}

static void language_to_utf8(const char *inputFile)
{
     struct stat statBuf;
     
     // Unable to stat() the file (missing ?)
     if (stat(inputFile, &statBuf) == -1)
     {
	  fprintf(stderr, "Error. Unable to open the file '%s'\n", inputFile);
	  return;
     }

     // Check file size
     if (statBuf.st_size > 0)
     {
	  int        inFD = -1;
	  int        outFD = -1;
	  ssize_t    wlen;
	  uint8_t   *buffer = NULL;
	  char       outFilename[strlen(inputFile) + strlen("-to_utf-8.h") + 1];

	  memcpy(outFilename, inputFile, strlen(inputFile) + 1);

	  char *token;
	  
	  if ((token = strrchr(outFilename, '.')))
	       *token = 0;
	  strcat(outFilename, "-to_utf-8.h");
	  
	  fprintf(stdout, "Converting language file '%s'...\n", inputFile);
	  
	  // Open the file for reading
	  if ((inFD = open(inputFile, O_RDONLY)) == -1)
	  {
	       perror("open()");
	       goto failure;
	  }
	  
	  // Alloc memory chunk to store file content
	  if ((buffer = calloc(statBuf.st_size + 1, sizeof(uint8_t))) == NULL)
	  {
	       perror("calloc()");
	       goto failure;
	  }

	  // read file content failed or size mismatched
	  if (read(inFD, buffer, statBuf.st_size) != statBuf.st_size)
	  {
	       perror("read()");
	       goto failure;
	  }

	  // Write data to specified file
	  fprintf(stdout, "Create %s\n", outFilename);
	  fflush(stdout);
	  if ((outFD = open(outFilename, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IWGRP)) == -1)
	  {
	       perror("open()");
	       goto failure;
	  }
	  
	  uint8_t  *p = buffer;
	  size_t    line = 0;

	  while (*p != '\0')
	  {
	       if (*p == '\n' || *p == '\r' || *p == '\t' || *p == ' ')
	       {
		    // DOS 2 UNIX
		    if (*p == '\r')
		    {
			 p++;
			 continue;
		    }
		    else if (*p == '\n')
			 line++;

		    // Write as is
		    if (line) // ignore first line, as it contains emacs encoding comment
		    {
			 if ((wlen = write(outFD, p, 1)) != 1)
			 {
			      perror("write()");
			      goto failure;
			 }
		    }
	       }
	       else
	       {
		    char *c = GDtoUTF8(*p);
		    size_t l = strlen(c);

		    if (line)
		    {
			 if ((wlen = write(outFD, c, l)) != l)
			 {
			      perror("write()");
			      goto failure;
			 }
		    }
	       }

	       p++;
	  }
	  
     failure:
	  if (outFD != -1)
	  {
	       if (close(outFD) == -1)
	       {
		    perror("close()");
	       }
	  }
	  
	  if (buffer != NULL)
	       free(buffer);

	  if (inFD != -1)
	  {
	       if (close(inFD) == -1)
	       {
		    perror("close()");
	       }
	  }
     }
}

static void language_to_gd(const char *inputFile)
{
     struct stat statBuf;
     
     // Unable to stat() the file (missing ?)
     if (stat(inputFile, &statBuf) == -1)
     {
	  fprintf(stderr, "Error. Unable to open the file '%s'\n", inputFile);
	  return;
     }

     // Check file size
     if (statBuf.st_size > 0)
     {
	  FILE      *in = NULL;
	  int        outFD = -1;
	  ssize_t    wlen;
	  char       outFilename[strlen(inputFile) + strlen("-to_opengd77.h") + 1];

	  memcpy(outFilename, inputFile, strlen(inputFile) + 1);

	  char *token;
	  
	  if ((token = strrchr(outFilename, '.')))
	       *token = 0;
	  strcat(outFilename, "-to_opengd77.h");
	  
	  fprintf(stdout, "Converting language file '%s'...\n", inputFile);
	  
	  // Open the file for reading
	  if ((in = fopen(inputFile, "r")) == NULL)
	  {
	       perror("fopen()");
	       goto failure;
	  }
	  
	  // Write data to specified file
	  fprintf(stdout, "Create %s\n", outFilename);
	  fflush(stdout);
	  if ((outFD = open(outFilename, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IWGRP)) == -1)
	  {
	       perror("open()");
	       goto failure;
	  }

	  char header[80];
	  size_t    line = 0;
	  wint_t p;
	  uint8_t binEncoding = 0;

	  memset(header, 0, sizeof(header) / sizeof(header[0]));

	  // Check what kind of header we will put
	  while ((p = fgetwc(in)) != WEOF)
	  {
	       if ((binEncoding = willEncodeAsBinary(p)) == 1)
		    break;
	  }
	  
	  sprintf(header, "/* -*- coding: %s; -*- */", ((binEncoding == 1) ? "binary" : "windows-1252-unix"));
	  
	  if ((wlen = write(outFD, header, strlen(header))) != strlen(header))
	  {
	       perror("write()");
	       goto failure;
	  }

	  // Rewind input file
	  rewind(in);
	  
	  while ((p = fgetwc(in)) != WEOF)
	  {
	       size_t cLen = 1;

	       if (p == '\n' || p == '\r' || p == '\t' || p == ' ')
	       {
		    // DOS 2 UNIX
		    if (p == '\r')
			 continue;
		    else if (p == '\n')
		    {
			 line++;
		    }

		    if ((wlen = write(outFD, &p, 1)) != 1)
		    {
			 perror("write()");
			 goto failure;
		    }
	       }
	       else
	       {
		    char c = UTF8toGD(p, &cLen);
		    
		    if ((wlen = write(outFD, &c, 1)) != 1)
		    {
			 perror("write()");
			 goto failure;
		    }
	       }
	  }
	  
     failure:
	  if (outFD != -1)
	  {
	       if (close(outFD) == -1)
	       {
		    perror("close()");
	       }
	  }
	  
	  if (in != NULL)
	  {
	       if (fclose(in) != 0)
	       {
		    perror("fclose()");
	       }
	  }
     }
}

static void displayHelp(void)
{
     fprintf(stdout, "UC1701 font tool v%d.%d.%d - 2019-2020 - F1RMB Daniel Caujolle-Bert.\n", VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
     fprintf(stdout, "\n");
     fprintf(stdout, "      --export, -e                    : Export all UC1701 fonts to XBitmap files.\n");
     fprintf(stdout, "      --import, -i <file.xbm>         : Convert given file to UC1701 format.\n");
     fprintf(stdout, "      --from-utf-8, -f <input_file.h> : Convert language file from UTF-8 to OpenGD encoding.\n");
     fprintf(stdout, "      --to-utf-8, -t <input_file.h>   : Convert language file from OpenGD77 to UTF-8 encoding.\n");
     fprintf(stdout, "\n");
}

int main(int argc, char **argv)
{
#ifdef JAPANESE_BUILD
     fprintf(stdout, "*** Japanese Edition ***\n");
#endif
     if (argc > 1)
     {
//	  setlocale(LC_CTYPE, NULL);//"ISO-8859-1");//"C");

	  for (int i = 1; i < argc; i++)
	  {
	       
	       if ((strcmp(argv[i], "-e") == 0) || (strcmp(argv[i], "--export") == 0))
	       {
		    exportUC1701Font(font_6x8, "font_6x8");
		    exportUC1701Font(font_6x8_bold, "font_6x8_bold");
		    exportUC1701Font(font_8x8, "font_8x8");
		    exportUC1701Font(font_8x16, "font_8x16");
		    exportUC1701Font(font_16x32, "font_16x32");
		    //exportUC1701Font(font_AtariST_8x16, "font_AtariST_8x16");
	       }
	       else if ((strcmp(argv[i], "-i") == 0) || (strcmp(argv[i], "--import") == 0))
	       {
		    if ((argc - (i + 1)) > 0)
		    {
			 importXBitmap(argv[i + 1]);
			 i++;
		    }
		    else
		    {
			 fprintf(stderr, "Missing argument\n");
			 displayHelp();

			 return EXIT_FAILURE;
		    }
	       }
	       else if ((strcmp(argv[i], "-f") == 0) || (strcmp(argv[i], "--from-utf-8") == 0))
	       {
		    if ((argc - (i + 1)) > 0)
		    {
			 char *oldLocale = strdup(setlocale(LC_ALL, NULL));
			 char *locale = setlocale(LC_ALL, "en_US.utf8");
			 
			 language_to_gd(argv[i + 1]);
			 
			 (void)locale; // shutup gcc
			 setlocale(LC_ALL, oldLocale);
			 free(oldLocale);
			 i++;
		    }
		    else
		    {
			 fprintf(stderr, "Missing argument\n");
			 displayHelp();

			 return EXIT_FAILURE;
		    }
	       }
	       else if ((strcmp(argv[i], "-t") == 0) || (strcmp(argv[i], "--to-utf-8") == 0))
	       {
		    if ((argc - (i + 1)) > 0)
		    {
			 language_to_utf8(argv[i + 1]);
			 i++;
		    }
		    else
		    {
			 fprintf(stderr, "Missing argument\n");
			 displayHelp();

			 return EXIT_FAILURE;
		    }
	       }
	       else
	       {
		    fprintf(stderr, "Unknown argument '%s'\n", argv[i]);
		    displayHelp();
		    return EXIT_FAILURE;
	       }

	  }
     }
     else
     {
	  displayHelp();

	  return EXIT_FAILURE;
     }

     return EXIT_SUCCESS;
}
