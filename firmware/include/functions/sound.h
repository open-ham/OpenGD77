/*
 * Copyright (C)2019 Kai Ludwig, DG4KLU
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

#ifndef _OPENGD77_SOUND_H_
#define _OPENGD77_SOUND_H_

#include <FreeRTOS.h>
#include <task.h>
#include "interfaces/i2s.h"


extern int melody_generic[512];
extern const int MELODY_POWER_ON[];
extern const int MELODY_PRIVATE_CALL[];
extern const int MELODY_KEY_BEEP[];
extern const int MELODY_KEY_LONG_BEEP[];
extern const int MELODY_ACK_BEEP[];
extern const int MELODY_NACK_BEEP[];
extern const int MELODY_ERROR_BEEP[];
extern const int MELODY_TX_TIMEOUT_BEEP[];
extern const int MELODY_DMR_TX_START_BEEP[];
extern const int MELODY_DMR_TX_STOP_BEEP[];
extern const int MELODY_KEY_BEEP_FIRST_ITEM[];
extern const int MELODY_LOW_BATTERY[];
extern const int MELODY_QUICKKEYS_CLEAR_ACK_BEEP[];
extern const int MELODY_RX_TGTSCC_WARNING_BEEP[];

extern volatile int *melody_play;
extern volatile int melody_idx;
extern volatile int micAudioSamplesTotal;
extern int soundBeepVolumeDivider;

#define WAV_BUFFER_SIZE 0xa0
#define WAV_BUFFER_COUNT 18
#define HOTSPOT_BUFFER_SIZE 50
#define HOTSPOT_BUFFER_COUNT 48

extern union sharedDataBuffer
{
	volatile uint8_t wavbuffer[WAV_BUFFER_COUNT][WAV_BUFFER_SIZE];
	volatile uint8_t hotspotBuffer[HOTSPOT_BUFFER_COUNT][HOTSPOT_BUFFER_SIZE];
	volatile uint8_t rawBuffer[HOTSPOT_BUFFER_COUNT * HOTSPOT_BUFFER_SIZE];
} audioAndHotspotDataBuffer;

extern volatile int wavbuffer_read_idx;
extern volatile int wavbuffer_write_idx;
extern volatile int wavbuffer_count;
extern uint8_t *currentWaveBuffer;

extern uint8_t spi_sound1[WAV_BUFFER_SIZE*2];
extern uint8_t spi_sound2[WAV_BUFFER_SIZE*2];
extern uint8_t spi_sound3[WAV_BUFFER_SIZE*2];
extern uint8_t spi_sound4[WAV_BUFFER_SIZE*2];

extern volatile bool g_TX_SAI_in_use;

extern uint8_t *spi_soundBuf;
extern sai_transfer_t xfer;

void soundInit(void);
void soundTerminateSound(void);
void soundSetMelody(const int *melody);
void soundCreateSong(const uint8_t *melody);
void soundInitBeepTask(void);
void soundSendData(void);
void soundReceiveData(void);
void soundStoreBuffer(void);
void soundRetrieveBuffer(void);
void soundTickRXBuffer(void);
void soundSetupBuffer(void);
void soundStopMelody(void);
void soundTickMelody(void);


//bit masks to track amp usage
#define AUDIO_AMP_MODE_NONE 	0
#define AUDIO_AMP_MODE_BEEP 	(1 << 0)
#define AUDIO_AMP_MODE_RF 		(1 << 1)
#define AUDIO_AMP_MODE_PROMPT 	(1 << 2)


uint8_t getAudioAmpStatus(void);
void enableAudioAmp(uint8_t mode);
void disableAudioAmp(uint8_t mode);

#endif /* _OPENGD77_SOUND_H_ */
