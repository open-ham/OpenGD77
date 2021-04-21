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

#ifndef _OPENGD77_HR_C6000_H_
#define _OPENGD77_HR_C6000_H_

#include <FreeRTOS.h>
#include <task.h>

#include "io/keyboard.h"

#include "interfaces/hr-c6000_spi.h"
#include "usb/usb_com.h"

#include "dmr_codec/codec.h"


#define DMR_FRAME_BUFFER_SIZE 64

extern const uint8_t TG_CALL_FLAG;
extern const uint8_t PC_CALL_FLAG;
extern volatile int slot_state;
extern volatile uint8_t DMR_frame_buffer[DMR_FRAME_BUFFER_SIZE];
extern volatile bool updateLastHeard;
extern volatile int dmrMonitorCapturedTS;
extern char talkAliasText[33];
extern volatile uint32_t readDMRRSSI;

enum DMR_SLOT_STATE { DMR_STATE_IDLE, DMR_STATE_RX_1, DMR_STATE_RX_2, DMR_STATE_RX_END,
					  DMR_STATE_TX_START_1, DMR_STATE_TX_START_2, DMR_STATE_TX_START_3, DMR_STATE_TX_START_4, DMR_STATE_TX_START_5,
					  DMR_STATE_TX_1, DMR_STATE_TX_2, DMR_STATE_TX_END_1, DMR_STATE_TX_END_2, DMR_STATE_TX_END_3_RMO, DMR_STATE_TX_END_3_DMO,
					  DMR_STATE_REPEATER_WAKE_1, DMR_STATE_REPEATER_WAKE_2, DMR_STATE_REPEATER_WAKE_3,
					  DMR_STATE_REPEATER_WAKE_FAIL_1, DMR_STATE_REPEATER_WAKE_FAIL_2 };

enum WakingMode { WAKING_MODE_NONE, WAKING_MODE_WAITING, WAKING_MODE_FAILED };

void HRC6000_init(void);
void PORTC_IRQHandler(void);
void init_HR_C6000_interrupts(void);
void init_digital_state(void);
void init_digital_DMR_RX(void);
void reset_timeslot_detection(void);
void init_digital(void);
void terminate_digital(void);
void init_hrc6000_task(void);
void fw_hrc6000_task(void *data);
void tick_HR_C6000(void);

void clearIsWakingState(void);
int getIsWakingState(void);
void clearActiveDMRID(void);
void setMicGainDMR(uint8_t gain);
bool checkTalkGroupFilter(void);

int HRC6000GetReceivedTgOrPcId(void);
int HRC6000GetReceivedSrcId(void);
void HRC6000ClearTimecodeSynchronisation(void);
void HRC6000SetCCFilterMode(bool enable);

#endif /* _OPENGD77_HR_C6000_H_ */
