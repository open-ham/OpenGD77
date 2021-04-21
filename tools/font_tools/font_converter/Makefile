
src = UC1701_font_tool.c
obj = $(src:.c=.o)

CC = gcc
CFLAGS = -Wall -Wno-restrict -O2 -DBUILD_FONT_TOOL -I../../../firmware/include/hardware
CFLAGS_JA = -Wall -Wno-restrict -O2 -DBUILD_FONT_TOOL -DJAPANESE_BUILD -I../../../firmware/include/hardware
CFLAGS_DEBUG = -Wall -Wno-restrict -O0 -g -DBUILD_FONT_TOOL -I../../../firmware/include/hardware
LDFLAGS = -static -s
LDFLAGS_DEBUG =

ifeq ($(OS),Windows_NT)
    bin = UC1701_font_tool.exe
    RM = del
else
    bin = UC1701_font_tool
    RM = rm -f
endif

UC1701_font_tool: $(obj)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

UC1701_font_tool_JA: CFLAGS = $(CFLAGS_JA)
UC1701_font_tool_JA: $(obj)
	$(CC) $(CFLAGS_JA) -o $@ $^ $(LDFLAGS)

JA: UC1701_font_tool_JA

all: UC1701_font_tool

debug: CFLAGS = $(CFLAGS_DEBUG)
debug: LDFLAGS = $(LDFLAGS_DEBUG)
debug: clean UC1701_font_tool

.PHONY: clean

clean:
	$(RM) $(obj) $(bin) *~
