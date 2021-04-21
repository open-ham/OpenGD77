/*
 * Copyright (C)2019 Kai Ludwig, DG4KLU
 *
 * Additional code by Roger Clark VK3KYY / G4KYF
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

#include "hotspot/dmrDefines.h"
#include "hardware/HR-C6000.h"
#include "functions/settings.h"
#if defined(USING_EXTERNAL_DEBUGGER)
#include "SeggerRTT/RTT/SEGGER_RTT.h"
#endif
#include "functions/trx.h"
#include "hotspot/uiHotspot.h"
#include "user_interface/uiUtilities.h"
#include "functions/voicePrompts.h"
#include "interfaces/gpio.h"
#include "interfaces/interrupts.h"


static const int QSO_TIMER_TIMEOUT              = 2400;


static const int SYS_INT_SEND_REQUEST_REJECTED  = 0x80;
static const int SYS_INT_SEND_START 			= 0x40;
static const int SYS_INT_SEND_END 				= 0x20;
static const int SYS_INT_POST_ACCESS			= 0x10;
static const int SYS_INT_RECEIVED_DATA			= 0x08;
static const int SYS_INT_RECEIVED_INFORMATION	= 0x04;
static const int SYS_INT_ABNORMAL_EXIT			= 0x02;
static const int SYS_INT_PHYSICAL_LAYER			= 0x01;

static const int WAKEUP_RETRY_PERIOD			= 600;// The official firmware seems to use a 600mS retry period.

TaskHandle_t fwhrc6000TaskHandle;

const uint8_t TG_CALL_FLAG = 0x00;
const uint8_t PC_CALL_FLAG = 0x03;
const uint8_t SILENCE_AUDIO[27] = {	0xB9U, 0xE8U, 0x81U, 0x52U, 0x61U, 0x73U, 0x00U, 0x2AU, 0x6BU, 0xB9U, 0xE8U, 0x81U, 0x52U,
									0x61U, 0x73U, 0x00U, 0x2AU, 0x6BU, 0xB9U, 0xE8U, 0x81U, 0x52U, 0x61U, 0x73U, 0x00U, 0x2AU, 0x6BU };



// GD-77 FW V3.1.1 data from 0x76010 / length 0x06
static const uint8_t MS_sync_pattern[] = { 0xd5, 0xd7, 0xf7, 0x7f, 0xd7, 0x57 };          //Mobile Station Sync Pattern for voice calls Repeater or Simplex
//static const uint8_t TDMA1_sync_pattern[] = { 0xf7, 0xfd, 0xd5, 0xdd, 0xfd, 0x55 };       //TDMA1 Sync Pattern for voice calls TDMA Simplex
//static const uint8_t TDMA2_sync_pattern[] = { 0xd7, 0x55, 0x7f, 0x5f, 0xf7, 0xf5 };      //TDMA2 Sync Pattern for voice calls TDMA Simplex

// GD-77 FW V3.1.1 data from 0x75F70 / length 0x20
static const uint8_t spi_init_values_2[] = { 0x69, 0x69, 0x96, 0x96, 0x96, 0x99, 0x99, 0x99, 0xa5, 0xa5, 0xaa, 0xaa, 0xcc, 0xcc, 0x00, 0xf0, 0x01, 0xff, 0x01, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x10, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// GD-77 FW V3.1.1 data from 0x75F90 / length 0x10
static const uint8_t spi_init_values_3[] = { 0x00, 0x00, 0x14, 0x1e, 0x1a, 0xff, 0x3d, 0x50, 0x07, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// GD-77 FW V3.1.1 data from 0x75FA0 / length 0x07
static const uint8_t spi_init_values_4[] = { 0x00, 0x03, 0x01, 0x02, 0x05, 0x1e, 0xf0 };
// GD-77 FW V3.1.1 data from 0x75FA8 / length 0x05
static const uint8_t spi_init_values_5[] = { 0x00, 0x00, 0xeb, 0x78, 0x67 };
// GD-77 FW V3.1.1 data from 0x75FB0 / length 0x60
static const uint8_t spi_init_values_6[] = { 0x32, 0xef, 0x00, 0x31, 0xef, 0x00, 0x12, 0xef, 0x00, 0x13, 0xef, 0x00, 0x14, 0xef, 0x00, 0x15, 0xef, 0x00, 0x16, 0xef, 0x00, 0x17, 0xef, 0x00, 0x18, 0xef, 0x00, 0x19, 0xef, 0x00, 0x1a, 0xef, 0x00, 0x1b, 0xef, 0x00, 0x1c, 0xef, 0x00, 0x1d, 0xef, 0x00, 0x1e, 0xef, 0x00, 0x1f, 0xef, 0x00, 0x20, 0xef, 0x00, 0x21, 0xef, 0x00, 0x22, 0xef, 0x00, 0x23, 0xef, 0x00, 0x24, 0xef, 0x00, 0x25, 0xef, 0x00, 0x26, 0xef, 0x00, 0x27, 0xef, 0x00, 0x28, 0xef, 0x00, 0x29, 0xef, 0x00, 0x2a, 0xef, 0x00, 0x2b, 0xef, 0x00, 0x2c, 0xef, 0x00, 0x2d, 0xef, 0x00, 0x2e, 0xef, 0x00, 0x2f, 0xef, 0x00 };

static const uint8_t spiInitReg0x04_PLL[4][2] = { {0x0B,0x40},//Set PLL M Register
												{0x0C,0x32},//Set PLL Dividers
												{0xB9,0x05},// ??
												{0x0A,0x01}};//Set Clock Source Enable CLKOUT Pin

static const uint8_t spiInitReg0x04MultiInit[57][2] = {
	{0x00, 0x00},   //Clear all Reset Bits which forces a reset of all internal systems
	{0x10, 0x6E},   //Set DMR,Tier2,Timeslot Mode, Layer 2, Repeater, Aligned, Slot1
	{0x11, 0x80},   //Set LocalChanMode to Default Value
	{0x13, 0x00},   //Zero Cend_Band Timing advance
	{0x1F, 0x10},   //Set LocalEMB  DMR Colour code in upper 4 bits - defaulted to 1, and is updated elsewhere in the code
	{0x20, 0x00},   //Set LocalAccessPolicy to Impolite
	{0x21, 0xA0},   //Set LocalAccessPolicy1 to Polite to Color Code  (unsure why there are two registers for this)
	{0x22, 0x26},   //Start Vocoder Decode, I2S mode
	{0x22, 0x86},   //Start Vocoder Encode, I2S mode
	{0x25, 0x0E},   //Undocumented Register
	{0x26, 0x7D},   //Undocumented Register
	{0x27, 0x40},   //Undocumented Register
	{0x28, 0x7D},   //Undocumented Register
	{0x29, 0x40},   //Undocumented Register
	{0x2A, 0x0B},   //Set spi_clk_cnt to default value
	{0x2B, 0x0B},   //According to Datasheet this is a Read only register For FM Squelch
	{0x2C, 0x17},   //According to Datasheet this is a Read only register For FM Squelch
	{0x2D, 0x05},   //Set FM Compression and Decompression points (?)
	{0x2E, 0x04},   //Set tx_pre_on (DMR Transmission advance) to 400us
	{0x2F, 0x0B},   //Set I2S Clock Frequency
	{0x32, 0x02},   //Set LRCK_CNT_H CODEC Operating Frequency to default value
	{0x33, 0xFF},   //Set LRCK_CNT_L CODEC Operating Frequency to default value
	{0x34, 0xF0},   //Set FM Filters on and bandwidth to 12.5Khz
	{0x35, 0x28},   //Set FM Modulation Coefficient
	{0x3E, 0x28},   //Set FM Modulation Offset
	{0x3F, 0x10},   //Set FM Modulation Limiter
	{0x36, 0x00},   //Enable all clocks
	{0x37, 0x00},   //Set mcu_control_shift to default. (codec under HRC-6000 control)
	{0x4B, 0x1B},   //Set Data packet types to defaults
	{0x4C, 0x00},   //Set Data packet types to defaults
	{0x56, 0x00}, 	//Undocumented Register
	{0x5F, 0xF0},  //G4EML Enable Sync detection for MS, BS , TDMA1 or TDMA2 originated signals (Was originally 0xC0)
	{0x81, 0xFF}, 	//Enable all Interrupts
	{0xD1, 0xC4},   //According to Datasheet this register is for FM DTMF (?)

	// --- start subroutine spi_init_daten_senden_sub()
	{0x01, 0x70}, 	//set 2 point Mod, swap receive I and Q, receive mode IF (?)    (Presumably changed elsewhere)
	{0x03, 0x00},   //zero Receive I Offset
	{0x05, 0x00},   //Zero Receive Q Offset
	{0x12, 0x15}, 	//Set rf_pre_on Receive to transmit switching advance
	{0xA1, 0x80}, 	//According to Datasheet this register is for FM Modulation Setting (?)
	{0xC0, 0x0A},   //Set RF Signal Advance to 1ms (10x100us)
	{0x06, 0x21},   //Use SPI vocoder under MCU control
	{0x07, 0x0B},   //Set IF Frequency H to default 450KHz
	{0x08, 0xB8},   //Set IF Frequency M to default 450KHz
	{0x09, 0x00},   //Set IF Frequency L to default 450KHz
	{0x0D, 0x10},   //Set Voice Superframe timeout value
	{0x0E, 0x8E},   //Register Documented as Reserved
	{0x0F, 0xB8},   //FSK Error Count
	{0xC2, 0x00},   //Disable Mic Gain AGC
	{0xE0, 0x8B},   //CODEC under MCU Control, LineOut2 Enabled, Mic_p Enabled, I2S Slave Mode
	{0xE1, 0x0F},   //Undocumented Register (Probably associated with CODEC)
	{0xE2, 0x06},   //CODEC  Anti Pop Enabled, DAC Output Enabled
	{0xE3, 0x52},   //CODEC Default Settings
	{0xE4, 0x4A},   //CODEC   LineOut Gain 2dB, Mic Stage 1 Gain 0dB, Mic Stage 2 Gain 30dB
	{0xE5, 0x1A},   //CODEC Default Setting
	// --- end subroutine spi_init_daten_senden_sub()

	{0x40, 0xC3},  	//Enable DMR Tx, DMR Rx, Passive Timing, Normal mode
	{0x41, 0x40},   //Receive during next timeslot
	{0x37, 0x9E},
};

static const uint8_t spiInitReg0x04_PostInitBlock1[8][2] = {
	{0x04, 0xE8},  //Set Mod2 output offset
	{0x46, 0x37},  //Set Mod1 Amplitude
	{0x48, 0x03},  //Set 2 Point Mod Bias
	{0x47, 0xE8},  //Set 2 Point Mod Bias
	{0x41, 0x20},  //set sync fail bit (reset?)
	{0x40, 0x03},  //Disable DMR Tx and Rx
	{0x41, 0x00},  //Reset all bits.
	{0x00, 0x3F},  //Reset DMR Protocol and Physical layer modules.
};

static const uint8_t spiInitReg0x04_PostInitBlock2[42][2] =  {
	{0x10, 0x6E},  //Set DMR, Tier2, Timeslot mode, Layer2, Repeater, Aligned, Slot 1
	{0x1F, 0x10},  // Set Local EMB. DMR Colour code in upper 4 bits - defaulted to 1, and is updated elsewhere in the code
	{0x26, 0x7D},  //Undocumented Register
	{0x27, 0x40},  //Undocumented Register
	{0x28, 0x7D},  //Undocumented Register
	{0x29, 0x40},  //Undocumented Register
	{0x2A, 0x0B},  //Set SPI Clock to default value
	{0x2B, 0x0B},  //According to Datasheet this is a Read only register For FM Squelch
	{0x2C, 0x17},  //According to Datasheet this is a Read only register For FM Squelch
	{0x2D, 0x05},  //Set FM Compression and Decompression points (?)
	{0x56, 0x00},  //Undocumented Register
	{0x5F, 0xF0},  //G4EML Enable Sync detection for MS, BS , TDMA1 or TDMA2 originated signals (Was Originally 0xC0)
	{0x81, 0xFF},  //Enable all Interrupts
	{0x01, 0x70},  //Set 2 Point Mod, Swap Rx I and Q, Rx Mode IF
	{0x03, 0x00},  //Zero Receive I Offset
	{0x05, 0x00},  //Zero Receive Q Offset
	{0x12, 0x15},  //Set RF Switching Receive to Transmit Advance
	{0xA1, 0x80},  //According to Datasheet this register is for FM Modulation Setting (?)
	{0xC0, 0x0A},  //Set RF Signal Advance to 1ms (10x100us)
	{0x06, 0x21},  //Use SPI vocoder under MCU control
	{0x07, 0x0B},  //Set IF Frequency H to default 450KHz
	{0x08, 0xB8},  //Set IF Frequency M to default 450KHz
	{0x09, 0x00},  //Set IF Frequency l to default 450KHz
	{0x0D, 0x10},  //Set Voice Superframe timeout value
	{0x0E, 0x8E},  //Register Documented as Reserved
	{0x0F, 0xB8},  //FSK Error Count
	{0xC2, 0x00},  //Disable Mic Gain AGC
	{0xE0, 0x8B},  //CODEC under MCU Control, LineOut2 Enabled, Mic_p Enabled, I2S Slave Mode
	{0xE1, 0x0F},  //Undocumented Register (Probably associated with CODEC)
	{0xE2, 0x06},  //CODEC  Anti Pop Enabled, DAC Output Enabled
	{0xE3, 0x52},  //CODEC Default Settings
	{0xE5, 0x1A},  //CODEC Default Setting
	{0x26, 0x7D},  //Undocumented Register
	{0x27, 0x40},  //Undocumented Register
	{0x28, 0x7D},  //Undocumented Register
	{0x29, 0x40},  //Undocumented Register
	{0x41, 0x20},  //Set Sync Fail Bit  (Reset?)
	{0x40, 0xC3},  //Enable DMR Tx and Rx, Passive Timing
	{0x41, 0x40},  //Set Receive During Next Slot Bit
	{0x01, 0x70},  //Set 2 Point Mod, Swap Rx I and Q, Rx Mode IF
	{0x10, 0x6E},  //Set DMR, Tier2, Timeslot mode, Layer2, Repeater, Aligned, Slot 1
	{0x00, 0x3F},  //Reset DMR Protocol and Physical layer modules.
};

static volatile uint8_t reg_0x51;
static volatile uint8_t reg_0x5F;
static volatile uint8_t reg_0x82;
static volatile uint8_t reg_0x84;
static volatile uint8_t reg_0x86;
static volatile uint8_t reg_0x90;
static volatile uint8_t reg_0x98;

volatile bool hasEncodedAudio = false;
volatile bool rxCRCisValid = false;
volatile bool hotspotDMRTxFrameBufferEmpty = true;
volatile bool hotspotDMRRxFrameBufferAvailable = false;

volatile uint8_t DMR_frame_buffer[DMR_FRAME_BUFFER_SIZE];
uint8_t deferredUpdateBuffer[DMR_FRAME_BUFFER_SIZE*6];
volatile bool deferredBufferAvailable = false;
volatile uint8_t *deferredUpdateBufferOutPtr = deferredUpdateBuffer;
volatile uint8_t *deferredUpdateBufferInPtr = deferredUpdateBuffer;

const int NUM_AMBE_BLOCK_PER_DMR_FRAME = 3;
const int NUM_AMBE_BUFFERS = 2;
const int AMBE_AUDIO_LENGTH = 27;
const int LENGTH_AMBE_BLOCK = 9;
const uint8_t *deferredUpdateBufferEnd = (uint8_t *)deferredUpdateBuffer + (AMBE_AUDIO_LENGTH * NUM_AMBE_BUFFERS) -1;
int ambeBufferCount = 0;

static volatile int int_timeout;

static volatile uint32_t receivedTgOrPcId;
static volatile uint32_t receivedSrcId;
volatile int slot_state;
static volatile int tick_cnt;
static volatile int skip_count;
static volatile bool transitionToTX = false;
volatile uint32_t readDMRRSSI = 0;
static int qsodata_timer = 0;


static volatile int tx_sequence = 0;

static volatile int timeCode = -1;
static volatile int receivedTimeCode;
static volatile int rxColorCode;
static volatile int repeaterWakeupResponseTimeout = 0;
static volatile int isWaking = WAKING_MODE_NONE;
static volatile int rxwait;// used for Repeater wakeup sequence
static volatile int rxcnt;// used for Repeater wakeup sequence
static volatile int tsLockCount = 0;

volatile int lastTimeCode = 0;
static volatile uint8_t previousLCBuf[12];
volatile bool updateLastHeard = false;
volatile int dmrMonitorCapturedTS = -1;
static volatile int dmrMonitorCapturedTimeout;
static volatile int TAPhase = 0;
char talkAliasText[33];

static bool callAcceptFilter(void);
static void setupPcOrTGHeader(void);
static inline void HRC6000SysInterruptHandler(void);
static inline void HRC6000TimeslotInterruptHandler(void);
static inline void HRC6000RxInterruptHandler(void);
static inline void HRC6000TxInterruptHandler(void);
static void HRC6000TransitionToTx(void);
static void triggerQSOdataDisplay(void);

enum RXSyncClass { SYNC_CLASS_HEADER = 0, SYNC_CLASS_VOICE = 1, SYNC_CLASS_DATA = 2, SYNC_CLASS_RC = 3};
enum RXSyncType { MS_SYNC =0 , BS_SYNC =1 , TDMA1_SYNC = 2 , TDMA2_SYNC =3};

static const int START_TICK_TIMEOUT = 20;
static const int END_TICK_TIMEOUT 	= 13;

static volatile int lastRxColorCode = 0;
static bool ccHold = true;
static int ccHoldTimer = 0;
static const int CCHOLDVALUE = 1000;			//1 second
static int wakeTriesCount;

static void writeSPIRegister0x04Multi(const uint8_t values[][2], uint8_t length)
{
	for(int i = 0; i < length; i++)
	{
		SPI0WritePageRegByte(0x04, values[i][0], values[i][1]);
	}
}

void HRC6000_init(void)
{
	gpioInitC6000Interface();

    GPIO_PinWrite(GPIO_INT_C6000_RESET, Pin_INT_C6000_RESET, 1);
    GPIO_PinWrite(GPIO_INT_C6000_PWD, Pin_INT_C6000_PWD, 1);

    // Wake up C6000
	vTaskDelay(portTICK_PERIOD_MS * 10);
    GPIO_PinWrite(GPIO_INT_C6000_PWD, Pin_INT_C6000_PWD, 0);
	vTaskDelay(portTICK_PERIOD_MS * 10);

	// --- start spi_init_daten_senden()
	writeSPIRegister0x04Multi(spiInitReg0x04_PLL,4);

	SPI0WritePageRegByteArray(0x01, 0x04, MS_sync_pattern, 0x06);
	SPI0WritePageRegByteArray(0x01, 0x10, spi_init_values_2, 0x20);
	SPI0WritePageRegByteArray(0x01, 0x30, spi_init_values_3, 0x10);
	SPI0WritePageRegByteArray(0x01, 0x40, spi_init_values_4, 0x07);
	SPI0WritePageRegByteArray(0x01, 0x51, spi_init_values_5, 0x05);
	SPI0WritePageRegByteArray(0x01, 0x60, spi_init_values_6, 0x60);

	writeSPIRegister0x04Multi(spiInitReg0x04MultiInit,57);


	// ------ start spi_more_init
	// --- start sub_1B5A4
	SPI0SeClearPageRegByteWithMask(0x04, 0x06, 0xFD, 0x02); // SET OpenMusic bit (play Boot sound and Call Prompts)
	// --- end sub_1B5A4

	// --- start sub_1B5DC
	const int SIZE_OF_FILL_BUFFER = 96;// Original size was 128
	uint8_t spi_values[SIZE_OF_FILL_BUFFER];
	memset(spi_values, 0xAA, SIZE_OF_FILL_BUFFER);
	SPI0WritePageRegByteArray(0x03, 0x00, spi_values, SIZE_OF_FILL_BUFFER);
	// --- end sub_1B5DC

	// --- start sub_1B5A4
	SPI0SeClearPageRegByteWithMask(0x04, 0x06, 0xFD, 0x00); // CLEAR OpenMusic bit (play Boot sound and Call Prompts)
	// --- end sub_1B5A4


	writeSPIRegister0x04Multi(spiInitReg0x04_PostInitBlock1,8);
	SPI0WritePageRegByteArray(0x01, 0x04, MS_sync_pattern, 0x06);
	writeSPIRegister0x04Multi(spiInitReg0x04_PostInitBlock2,42);
	SPI0WritePageRegByte(0x04, 0xE4, 0xC0 + nonVolatileSettings.micGainDMR);  //CODEC   LineOut Gain 6dB, Mic Stage 1 Gain 0dB, Mic Stage 2 Gain default is 11 =  33dB

	clearActiveDMRID();
}

void setMicGainDMR(uint8_t gain)
{
	SPI0WritePageRegByte(0x04, 0xE4, 0x40 + gain);  //CODEC   LineOut Gain 2dB, Mic Stage 1 Gain 0dB, Mic Stage 2 Gain default is 11 =  33dB
}

static inline bool checkTimeSlotFilter(void)
{
	if (trxTransmissionEnabled)
	{
		return (timeCode == trxGetDMRTimeSlot());
	}
	else
	{
		if (nonVolatileSettings.dmrCcTsFilter & DMR_TS_FILTER_PATTERN)
		{
			int currentTS = trxGetDMRTimeSlot();

			dmrMonitorCapturedTS = currentTS;
			dmrMonitorCapturedTimeout = nonVolatileSettings.dmrCaptureTimeout * 1000;

			return (timeCode == currentTS);
		}
		else
		{
			if (timeCode != -1)
			{
				if ((tsLockCount > 4) && ((dmrMonitorCapturedTS == -1) || (dmrMonitorCapturedTS == timeCode)))
				{
					dmrMonitorCapturedTS = timeCode;
					dmrMonitorCapturedTimeout = nonVolatileSettings.dmrCaptureTimeout * 1000;
					return true;
				}
			}
		}
	}

	return false;
}

bool checkTalkGroupFilter(void)
{
	if (((receivedTgOrPcId >> 24) == PC_CALL_FLAG) && (settingsUsbMode != USB_MODE_HOTSPOT))
	{
		return ((nonVolatileSettings.privateCalls != 0) || ((trxTalkGroupOrPcId >> 24) == PC_CALL_FLAG));
	}

	switch(nonVolatileSettings.dmrDestinationFilter)
	{
		case DMR_DESTINATION_FILTER_TG:
			return ((trxTalkGroupOrPcId & 0x00FFFFFF) == receivedTgOrPcId);
			break;

		case DMR_DESTINATION_FILTER_DC:
			return codeplugContactsContainsPC(receivedSrcId);
			break;

		case DMR_DESTINATION_FILTER_RXG:
			{
				for(int i = 0; i < currentRxGroupData.NOT_IN_CODEPLUG_numTGsInGroup; i++)
				{
					if (currentRxGroupData.NOT_IN_CODEPLUG_contactsTG[i] == receivedTgOrPcId)
					{
						return true;
					}
				}

				return false;
			}
			break;

		default:
			break;
	}

	return true;
}

bool checkColourCodeFilter(void)
{
	return (rxColorCode == trxGetDMRColourCode());
}

void transmitTalkerAlias(void)
{
	if (TAPhase % 2 == 0)
	{
		setupPcOrTGHeader();
	}
	else
	{
		uint8_t TA_LCBuf[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
		int taPosition = 2;
		int taOffset, taLength;

		switch(TAPhase / 2)
		{
			case 0:
				taPosition 	= 3;
				TA_LCBuf[2]= (0x01 << 6) | (strlen(talkAliasText) << 1);
				taOffset	= 0;
				taLength	= 6;
				break;
			case 1:
				taOffset	= 6;
				taLength	= 7;
				break;
			case 2:
				taOffset	= 13;
				taLength	= 7;
				break;
			case 3:
				taOffset	= 20;
				taLength	= 7;
				break;
			default:
				taOffset	= 0;
				taLength	= 0;
				break;
		}

		TA_LCBuf[0]= (TAPhase/2) + 0x04;
		memcpy(&TA_LCBuf[taPosition], &talkAliasText[taOffset], taLength);

		SPI0WritePageRegByteArray(0x02, 0x00, (uint8_t*)TA_LCBuf, taPosition + taLength);// put LC into hardware
	}
	TAPhase++;
	if (TAPhase > 8)
	{
		TAPhase = 0;
	}
}

void PORTC_IRQHandler(void)
{

	if (interruptsWasPinTriggered(Port_INT_C6000_SYS, Pin_INT_C6000_SYS))
	{
		HRC6000SysInterruptHandler();
		interruptsClearPinFlags(Port_INT_C6000_SYS, Pin_INT_C6000_SYS);
	}

	if (interruptsWasPinTriggered(Port_INT_C6000_RF_RX, Pin_INT_C6000_TS))
	{
		HRC6000TimeslotInterruptHandler();
		interruptsClearPinFlags(Port_INT_C6000_RF_RX, Pin_INT_C6000_TS);
	}

	if (interruptsWasPinTriggered(Port_INT_C6000_RF_RX, Pin_INT_C6000_RF_RX))
	{
		HRC6000RxInterruptHandler();
		interruptsClearPinFlags(Port_INT_C6000_RF_RX, Pin_INT_C6000_RF_RX);
	}

	if (interruptsWasPinTriggered(Port_INT_C6000_RF_TX, Pin_INT_C6000_RF_TX))
	{
		HRC6000TxInterruptHandler();
		interruptsClearPinFlags(Port_INT_C6000_RF_TX, Pin_INT_C6000_RF_TX);
	}

	int_timeout = 0;

    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
    exception return operation might vector to incorrect interrupt */
    __DSB();
}


inline static void HRC6000SysSendRejectedInt(void)
{
	/*
		Bit7: In DMR mode: indicates that the transmission request rejects the interrupt without a sub-status register.
		In DMR mode, it indicates that this transmission request is rejected because the channel is busy;
	 */
}

inline static void HRC6000SysSendStartInt(void)
{
/*
		Bit6: In DMR mode: indicates the start of transmission; in MSK mode: indicates that the ping-pong buffer is half-full interrupted.
		In DMR mode, the sub-status register 0x84 is transmitted at the beginning, and the corresponding interrupt can be masked by 0x85.
	 */

	SPI0ReadPageRegByte(0x04, 0x84, &reg_0x84);  //Read sub status register

	/*
		The sub-status registers indicate
		seven interrupts that initiate the transmission, including:
		Bit7: Voice transmission starts

		Bit6: OACSU requests to send interrupts, including first-time send and resend requests.

		Bit5: End-to-end voice enhanced encryption interrupt, including EMB72bits update interrupt
		and voice 216bits key update interrupt, which are distinguished by Bit5~Bit4 of Register
		0x88, where 01 indicates EMB72bits update interrupt and 10 indicates voice 216bits key update interrupt.

		Bit4: 	The Vocoder configuration returns an interrupt (this interrupt is sent by the HR_C6000
				to the MCU when the MCU manually configures the AMBE3000). This interrupt is only
				valid when using the external AMBE3000 vocoder.

		Bit3: Data transmission starts

		Bit2: Data partial retransmission

		Bit1: Data retransmission

		Bit0: The vocoder is initialized to an interrupt. This interrupt is only valid when using an external AMBE3000 or AMBE1000 vocoder.

		In MSK mode, there is no sub-interrupt status.
	*/
}

inline static void HRC6000SysSendEndInt(void)
{
	/*
		Bit5: In DMR mode: indicates the end of transmission; in MSK mode: indicates the end of	transmission.
	*/
//	SPI0ReadPageRegByte(0x04, 0x86, &reg_0x86);  //Read Interrupt Flag Register2

	/*
		In DMR mode, there is a sub-status register 0x86 at the end of the transmission, and the
		corresponding interrupt can be masked by 0x87. The sub-status register indicates six interrupts that
		generate the end of the transmission, including:

		Bit7: 	Indicates that the service transmission is completely terminated, including voice and data.
				The MCU distinguishes whether the voice or data is sent this time. Confirming that the
				data service is received is the response packet that receives the correct feedback.

		Bit6: 	Indicates that a Fragment length confirmation packet is sent in the sliding window data
				service without immediate feedback.

		Bit5: VoiceOACSU wait timeout

		Bit4: 	The Layer 2 mode handles the interrupt. The MCU sends the configuration information
				to the last processing timing of the chip to control the interrupt. If after the interrupt, the
				MCU has not written all the information to be sent in the next frame to the chip, the next
				time slot cannot be Configured to send time slots. This interrupt is only valid when the chip
				is operating in Layer 2 mode.

		Bit3: 	indicates that a Fragment that needs to be fed back confirms the completion of the data
				packet transmission. The interrupt is mainly applied to the acknowledgment message after
				all the data packets have been sent or the data packet that needs to be fed back in the sliding
				window data service is sent to the MCU to start waiting for the timing of the Response
				packet. Device.

		Bit2 : ShortLC Receive Interrupt

		Bit1: BS activation timeout interrupt

		In MSK mode, there is no substate interrupt.
	*/
}

inline static void HRC6000SysPostAccessInt(void)
{
	/*
	Bit4:
		In DMR mode: indicates the access interruption;
		In MSK mode: indicates that the response response is interrupted.

		In DMR mode, the access interrupt has no sub-status register.
		After receiving the interrupt, it indicates that the access voice communication mode is post-access. the way.

		In MSK mode, this interrupt has no substatus registers.
	*/

	// Late entry into ongoing RX
	if (slot_state == DMR_STATE_IDLE)
	{
		codecInit();
		LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 1);

		SPI0WritePageRegByte(0x04, 0x41, 0x50);     //Receive only in next timeslot
		slot_state = DMR_STATE_RX_1;
		lastHeardClearLastID();// Tell the LastHeard system that this is a new start

		skip_count = 2;// RC. seems to be something to do with late entry but I'm but sure what, or whether its still needed

		if (settingsUsbMode == USB_MODE_HOTSPOT)
		{
			DMR_frame_buffer[27 + 0x0c] = HOTSPOT_RX_START_LATE;
			hotspotDMRRxFrameBufferAvailable = true;
		}
	}
}


inline static void HRC6000SysReceivedDataInt(void)
{
	/*
		Bit3: 	In DMR mode: indicates that the control frame parsing completion interrupt;
				In MSK mode: indicates the receive interrupt.

		In DMR mode, this interrupt has no sub-status register, but the error and receive type of its received
		data is given by the 0x51 register. The DLLRecvDataType, DLLRecvCRC are used to indicate the
		received data type and the error status, and the MCU accordingly performs the corresponding status.

		Display, you can also block the corresponding interrupt.
		In MSK mode, this interrupt has no substate interrupts.

		The FMB frame's EMB information parsing completion prompt is also the completion of the
		interrupt, which is distinguished by judging the 0x51 register SyncClass=0.
	*/
	int rxDataType;
	int rxSyncClass;
	int rpi;
	int rxSyncType;

	SPI0ReadPageRegByte(0x04, 0x51, &reg_0x51);

	//read_SPI_page_reg_byte_SPI0(0x04, 0x57, &reg_0x57);// Kai said that the official firmware uses this register instead of 0x52 for the timecode

	rxDataType 	= (reg_0x51 >> 4) & 0x0f;//Data Type or Voice Frame sequence number
	rxSyncClass = (reg_0x51 >> 0) & 0x03;//Received Sync Class  0=Sync Header 1=Voice 2=data 3=RC
	rxCRCisValid = (((reg_0x51 >> 2) & 0x01) == 0);// CRC is OK if its 0
	rpi = (reg_0x51 >> 3) & 0x01;

	SPI0ReadPageRegByte(0x04, 0x5f, &reg_0x5F);
	rxSyncType=reg_0x5F & 0x03;                       //received Sync Type

	if (rxSyncType==BS_SYNC)                                           // if we are receiving from a base station (Repeater)
	{
		trxDMRModeRx = DMR_MODE_RMO;                               // switch to RMO mode to allow reception
	}
	else
	{
		trxDMRModeRx = DMR_MODE_DMO;								   // not base station so must be DMO

	}


	if (((slot_state == DMR_STATE_RX_1) || (slot_state == DMR_STATE_RX_2)) && ((rpi != 0) || (rxCRCisValid != true) || !checkColourCodeFilter()))
	{
		// Something is not correct
		return;
	}
	tick_cnt = 0;

	// Wait for the repeater to wakeup and count the number of frames where the Timecode (TS number) is toggling correctly
	if (slot_state == DMR_STATE_REPEATER_WAKE_3)
	{
		if (lastTimeCode != timeCode)
		{
			rxcnt++;// timecode has toggled correctly
		}
		else
		{
			rxcnt = 0;// timecode has not toggled correctly so reset the counter used in the TS state machine
		}
	}

	// Note only detect terminator frames in Active mode, because in passive we can see our own terminators echoed back which can be a problem

	if ((rxSyncClass == SYNC_CLASS_DATA) && (rxDataType == 2))        //Terminator with LC
	{
		if ((trxDMRModeRx == DMR_MODE_DMO) && callAcceptFilter())
		{
			slot_state = DMR_STATE_RX_END;
			trxIsTransmitting = false;

			if (settingsUsbMode == USB_MODE_HOTSPOT)
			{
				DMR_frame_buffer[27 + 0x0c] = HOTSPOT_RX_STOP;
				hotspotDMRRxFrameBufferAvailable = true;
			}
			return;
		}
		else
		{
			if (checkTimeSlotFilter() && (lastTimeCode != timeCode))
			{
				disableAudioAmp(AUDIO_AMP_MODE_RF);
			}
		}
	}


	if ((slot_state != 0) && (skip_count > 0) && (rxSyncClass != SYNC_CLASS_DATA) && ((rxDataType & 0x07) == 0x01))
	{
		skip_count--;
	}

	// Check for correct received packet
	if ((rxCRCisValid == true) && (rpi == 0) && (slot_state < DMR_STATE_TX_START_1))
	{
		// Start RX
		if (slot_state == DMR_STATE_IDLE)
		{
			if (checkColourCodeFilter())// Voice LC Header
			{
				codecInit();
				LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 1);

				SPI0WritePageRegByte(0x04, 0x41, 0x50);     //Receive only in next timeslot
				slot_state = DMR_STATE_RX_1;

				skip_count = 0;
				lastHeardClearLastID();// Tell the LastHeard system that this is a new start

				if (settingsUsbMode == USB_MODE_HOTSPOT)
				{
					DMR_frame_buffer[27 + 0x0c] = HOTSPOT_RX_START;
					hotspotDMRRxFrameBufferAvailable = true;
				}
			}
			else
			{
				//SEGGER_RTT_printf(0, "Conditions failed\n");
			}
		}
		else
		{
			int sequenceNumber = (rxDataType & 0x07);
			// Detect/decode voice packet and transfer it into the output soundbuffer

			if (((skip_count == 0) || ((receivedSrcId != trxDMRID) && (receivedSrcId != 0x00))) &&
			(rxSyncClass != SYNC_CLASS_DATA) && (sequenceNumber >= 0x01) && (sequenceNumber <= 0x06) &&
			(((trxDMRModeRx == DMR_MODE_RMO) && (checkTimeSlotFilter() && lastTimeCode != timeCode)) || ((trxDMRModeRx == DMR_MODE_DMO) &&
			 (slot_state == DMR_STATE_RX_1))) && checkTalkGroupFilter() && checkColourCodeFilter())
			{
				ccHold = true;				//don't allow CC to change if we are receiving audio

				if((settingsUsbMode != USB_MODE_HOTSPOT) && ((getAudioAmpStatus() & AUDIO_AMP_MODE_RF) == 0) &&
						(((trxDMRModeRx == DMR_MODE_RMO) && (tsLockCount > 4)) || (trxDMRModeRx == DMR_MODE_DMO)))
				{
					enableAudioAmp(AUDIO_AMP_MODE_RF);
				}

				if (sequenceNumber == 1)
				{
					triggerQSOdataDisplay();
				}

				SPI1ReadPageRegByteArray(0x03, 0x00, DMR_frame_buffer + 0x0C, 27);

				if (settingsUsbMode == USB_MODE_HOTSPOT)
				{
					DMR_frame_buffer[27 + 0x0c] = HOTSPOT_RX_AUDIO_FRAME;
					DMR_frame_buffer[27 + 0x0c + 1] = (rxDataType & 0x07);// audio sequence number
					hotspotDMRRxFrameBufferAvailable = true;
				}
				else
				{
					hasEncodedAudio = true;// tell foreground that there is audio to encode
				}
			}
		}
	}

	if (timeCode != -1)
	{
		lastTimeCode = timeCode;
	}
}

inline static void HRC6000SysReceivedInformationInt(void)
{
}

inline static void HRC6000SysAbnormalExitInt(void)
{
	SPI0ReadPageRegByte(0x04, 0x98, &reg_0x98);
	/*
		Bit1: In DMR mode: indicates that the voice is abnormally exited;
		In DMR mode, the cause of the abnormality in DMR mode is the unexpected abnormal voice
		interrupt generated inside the state machine. The corresponding voice exception type is obtained
		through Bit2~Bit0 of register address 0x98.
	*/
}

inline static void HRC6000SysPhysicalLayerInt(void)
{

	/*
		Bit0: physical layer separate work reception interrupt
		The physical layer works independently to receive interrupts without a sub-status register. The
		interrupt is generated in the physical layer single working mode. After receiving the data, the
		interrupt is generated, and the MCU is notified to read the corresponding register to obtain the
		received data. This interrupt is typically tested in bit error rate or other performance in physical
		layer mode.
	*/
}

inline static void HRC6000SysInterruptHandler(void)
{
	uint8_t reg0x52;
	SPI0ReadPageRegByte(0x04, 0x82, &reg_0x82);  //Read Interrupt Flag Register1
	SPI0ReadPageRegByte(0x04, 0x52, &reg0x52);  //Read Received CC and CACH
	rxColorCode = (reg0x52 >> 4) & 0x0f;


	if (!trxTransmissionEnabled) // ignore the LC data when we are transmitting
	{
		//if ((!ccHold) && (nonVolatileSettings.dmrDestinationFilter < DMR_CCTS_FILTER_CC))
		if ((!ccHold) && (nonVolatileSettings.dmrCcTsFilter == DMR_CCTS_FILTER_NONE || nonVolatileSettings.dmrCcTsFilter == DMR_CCTS_FILTER_TS))
		{
			if(rxColorCode == lastRxColorCode)
			{
				trxSetDMRColourCode(rxColorCode);
			}
			lastRxColorCode = rxColorCode;
		}

		uint8_t LCBuf[12];
		SPI0ReadPageRegBytAarray(0x02, 0x00, LCBuf, 12);// read the LC from the C6000
		SPI0ReadPageRegByte(0x04, 0x51, &reg_0x51);
		bool rxCRCStatus = (((reg_0x51 >> 2) & 0x01) == 0);// CRC is OK if its 0

		if (rxCRCStatus && ((LCBuf[0] == TG_CALL_FLAG) || (LCBuf[0] == PC_CALL_FLAG) || ((LCBuf[0] >= FLCO_TALKER_ALIAS_HEADER) && (LCBuf[0] <= FLCO_GPS_INFO))) &&
			(memcmp((uint8_t *)previousLCBuf, LCBuf, 12) != 0))
		{

			if (((checkTimeSlotFilter() || (trxDMRModeRx == DMR_MODE_DMO))) && checkColourCodeFilter()) // only do this for the selected timeslot, or when in Active mode
			{
				if ((LCBuf[0] == TG_CALL_FLAG) || (LCBuf[0] == PC_CALL_FLAG))
				{
					receivedTgOrPcId 	= (LCBuf[0] << 24) + (LCBuf[3] << 16) + (LCBuf[4] << 8) + (LCBuf[5] << 0);// used by the call accept filter
					receivedSrcId 		= (LCBuf[6] << 16) + (LCBuf[7] << 8) + (LCBuf[8] << 0);// used by the call accept filter

					if ((receivedTgOrPcId != 0) && (receivedSrcId != 0) && checkTalkGroupFilter()) // only store the data if its actually valid
					{
						if (updateLastHeard == false)
						{
							memcpy((uint8_t *)DMR_frame_buffer, LCBuf, 12);
							updateLastHeard = true;	//lastHeardListUpdate((uint8_t *)DMR_frame_buffer);
						}

					}
				}
				else
				{
					if ((updateLastHeard == false) && (receivedTgOrPcId != 0) && checkTalkGroupFilter())
					{
						memcpy((uint8_t *)DMR_frame_buffer, LCBuf, 12);
						updateLastHeard = true;//lastHeardListUpdate((uint8_t *)DMR_frame_buffer);
					}
				}
			}
		}
		memcpy((uint8_t *)previousLCBuf, LCBuf, 12);
	}
	else
	{
		ccHold = true;					//prevent CC change when transmitting.
	}

	if (reg_0x82 & SYS_INT_SEND_REQUEST_REJECTED)
	{
		HRC6000SysSendRejectedInt();
	}

	if (reg_0x82 & SYS_INT_SEND_START)
	{
		HRC6000SysSendStartInt();
	}
	else
	{
		reg_0x84 = 0x00;
	}

	if (reg_0x82 & SYS_INT_SEND_END)// Kai's comment was InterSendStop interrupt
	{
		HRC6000SysSendEndInt();
	}
	else
	{
		reg_0x86 = 0x00;
	}

	if (reg_0x82 & SYS_INT_POST_ACCESS)
	{
		HRC6000SysPostAccessInt();
	}

	if (reg_0x82 & SYS_INT_RECEIVED_DATA)
	{
		HRC6000SysReceivedDataInt();
	}

	if (reg_0x82 & SYS_INT_RECEIVED_INFORMATION)
	{
		HRC6000SysReceivedInformationInt();
	}
	else
	{
		reg_0x90 = 0x00;
	}

	if (reg_0x82 & SYS_INT_ABNORMAL_EXIT)
	{
		HRC6000SysAbnormalExitInt();
	}
	else
	{
		reg_0x98 = 0x00;
	}

	if (reg_0x82 & SYS_INT_PHYSICAL_LAYER)
	{
		HRC6000SysPhysicalLayerInt();
	}

	SPI0WritePageRegByte(0x04, 0x83, reg_0x82);  //Clear remaining Interrupt Flags
	timer_hrc6000task = 0;
}

static void HRC6000TransitionToTx(void)
{
	disableAudioAmp(AUDIO_AMP_MODE_RF);
	LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
	codecInit();

	SPI0WritePageRegByte(0x04, 0x21, 0xA2); // Set Polite to Color Code and Reset vocoder encodingbuffer
	SPI0WritePageRegByte(0x04, 0x22, 0x86); // Start Vocoder Encode, I2S mode
	slot_state = DMR_STATE_TX_START_1;
}

inline static void HRC6000TimeslotInterruptHandler(void)
{
	uint8_t reg0x52;
	SPI0ReadPageRegByte(0x04, 0x52, &reg0x52);  	//Read CACH Register to get the timecode (TS number)
	receivedTimeCode = ((reg0x52 & 0x04) >> 2);				// extract the timecode from the CACH register

	if ((slot_state == DMR_STATE_REPEATER_WAKE_3) || (timeCode == -1))			//if we are waking up the repeater, or we don't currently have a valid value for the timecode
	{
		tsLockCount = 0;
		timeCode = receivedTimeCode;							//use the received TC directly from the CACH
	}
	else
	{
		timeCode = !timeCode;									//toggle the timecode.
		if (timeCode == receivedTimeCode)						//if this agrees with the received version
		{
			if (tsLockCount < 5)
			{
				tsLockCount++;
			}

			if (rxcnt > 0)							         //decrement the disagree count
			{
				rxcnt--;
			}
		}
		else												//if there is a disagree it might be just a glitch so ignore it a couple of times.
		{
			rxcnt++;										//count the number of disagrees.
			tsLockCount--;

			if (rxcnt > 3)										//if we have had four disagrees then re-sync.
			{
				timeCode = receivedTimeCode;
				rxcnt = 0;
				tsLockCount = 0;
			}
		}
	}

	// RX/TX state machine
#if defined(USING_EXTERNAL_DEBUGGER) && defined(DEBUG_DMR)
	SEGGER_RTT_printf(0, "state:%d\n",slot_state);
#endif
	switch (slot_state)
	{
		case DMR_STATE_RX_1: // Start RX (first step)
			if (trxDMRModeRx == DMR_MODE_RMO)
			{

				if( !isWaking && trxTransmissionEnabled && !checkTimeSlotFilter() && (rxcnt == 0) && (tsLockCount > 4))
				{
					HRC6000TransitionToTx();
				}
				else
				{
					LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 1);
					readDMRRSSI = PITCounter + 150;// wait 15 ticks (of approximately 1mS) before reading the RSSI
				}
			}
			else
			{
				// When in Active (simplex) mode. We need to only receive on one of the 2 timeslots, otherwise we get random data for the other slot
				// and this can sometimes be interpreted as valid data, which then screws things up.
				SPI0WritePageRegByte(0x04, 0x41, 0x00);     //No Transmit or receive in next timeslot
				readDMRRSSI = PITCounter + 150;// wait 15 ticks (of approximately 1mS) before reading the RSSI
				slot_state = DMR_STATE_RX_2;
			}
			break;

		case DMR_STATE_RX_2: // Start RX (second step)
			SPI0WritePageRegByte(0x04, 0x41, 0x50);     //Receive only in next timeslot
			slot_state = DMR_STATE_RX_1;
			break;

		case DMR_STATE_RX_END: // Stop RX
			clearActiveDMRID();
			init_digital_DMR_RX();
			disableAudioAmp(AUDIO_AMP_MODE_RF);
			LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
			uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
			slot_state = DMR_STATE_IDLE;
			trxIsTransmitting = false;
			break;

		case DMR_STATE_TX_START_1: // Start TX (second step)
			LEDs_PinWrite(GPIO_LEDred, Pin_LEDred, 1);// for repeater wakeup
			setupPcOrTGHeader();
			SPI0WritePageRegByte(0x04, 0x41, 0x80);    //Transmit during next Timeslot
			SPI0WritePageRegByte(0x04, 0x50, 0x10);    //Set Data Type to 0001 (Voice LC Header), Data, LCSS=00
			trxIsTransmitting = true;
			slot_state = DMR_STATE_TX_START_2;
			break;

		case DMR_STATE_TX_START_2: // Start TX (third step)
			SPI0WritePageRegByte(0x04, 0x41, 0x00); 	//Do nothing on the next TS
			slot_state = DMR_STATE_TX_START_3;
			break;

		case DMR_STATE_TX_START_3: // Start TX (fourth step)
			SPI0WritePageRegByte(0x04, 0x41, 0x80);     //Transmit during Next Timeslot
			SPI0WritePageRegByte(0x04, 0x50, 0x10);     //Set Data Type to 0001 (Voice LC Header), Data, LCSS=00
			slot_state = DMR_STATE_TX_START_4;
			break;

		case DMR_STATE_TX_START_4: // Start TX (fifth step)
			SPI0WritePageRegByte(0x04, 0x41, 0x00); 	//Do nothing on the next TS
			slot_state = DMR_STATE_TX_START_5;

			if (settingsUsbMode != USB_MODE_HOTSPOT)
			{
				ambeBufferCount = 0;
				deferredUpdateBufferOutPtr = deferredUpdateBuffer;
				deferredUpdateBufferInPtr = deferredUpdateBuffer;
				soundReceiveData();
			}
			break;

		case DMR_STATE_TX_START_5: // Start TX (sixth step)
			SPI0WritePageRegByte(0x04, 0x41, 0x80);   //Transmit during next Timeslot
			SPI0WritePageRegByte(0x04, 0x50, 0x10);   //Set Data Type to 0001 (Voice LC Header), Data, LCSS=00
			tx_sequence = 0;
			TAPhase = 0;
			slot_state = DMR_STATE_TX_1;
			break;

		case DMR_STATE_TX_1: // Ongoing TX (inactive timeslot)

			SPI0WritePageRegByte(0x04, 0x41, 0x00);
			if ((trxTransmissionEnabled == false) && (tx_sequence == 0))
			{
				slot_state = DMR_STATE_TX_END_1; // only exit here to ensure staying in the correct timeslot
			}
			else
			{
				slot_state = DMR_STATE_TX_2;
			}
			break;

		case DMR_STATE_TX_2: // Ongoing TX (active timeslot)
			if (trxTransmissionEnabled)
			{
				if (settingsUsbMode != USB_MODE_HOTSPOT)
				{
					if (nonVolatileSettings.bitfieldOptions & BIT_TRANSMIT_TALKER_ALIAS)
					{
						if(tx_sequence == 0)
						{
							transmitTalkerAlias();
						}
					}
					if (ambeBufferCount >= NUM_AMBE_BLOCK_PER_DMR_FRAME)
					{
						SPI1WritePageRegByteArray(0x03, 0x00, (uint8_t*)deferredUpdateBufferOutPtr, 27);// send the audio bytes to the hardware
						deferredUpdateBufferOutPtr += 27;

						if (deferredUpdateBufferOutPtr > deferredUpdateBufferEnd)
						{
							deferredUpdateBufferOutPtr = deferredUpdateBuffer;
						}
						ambeBufferCount -= 3;
					}
					else
					{
						SPI1WritePageRegByteArray(0x03, 0x00, SILENCE_AUDIO, 27);// send the audio bytes to the hardware
					}
				}
				else
				{
					if(tx_sequence == 0)
					{
						SPI0WritePageRegByteArray(0x02, 0x00, (uint8_t*)deferredUpdateBuffer, 0x0c);// put LC into hardware
					}

					if (!hotspotDMRTxFrameBufferEmpty)
					{
						SPI1WritePageRegByteArray(0x03, 0x00, (uint8_t*)(deferredUpdateBuffer+0x0C), 27);// send the audio bytes to the hardware
					}
					else
					{
						SPI1WritePageRegByteArray(0x03, 0x00, SILENCE_AUDIO, 27);// send the audio bytes to the hardware
					}

					hotspotDMRTxFrameBufferEmpty = true;// we have finished with the current frame data from the hotspot
				}
			}
			else
			{
				SPI1WritePageRegByteArray(0x03, 0x00, (uint8_t*)SILENCE_AUDIO, 27);// send the audio bytes to the hardware
			}

			//write_SPI_page_reg_bytearray_SPI1(0x03, 0x00, (uint8_t*)(DMR_frame_buffer+0x0C), 27);// send the audio bytes to the hardware
			SPI0WritePageRegByte(0x04, 0x41, 0x80); // Transmit during next Timeslot
			SPI0WritePageRegByte(0x04, 0x50, 0x08 + (tx_sequence << 4)); // Data Type= sequence number 0 - 5 (Voice Frame A) , Voice, LCSS = 0

			tx_sequence++;
			if (tx_sequence > 5)
			{
				tx_sequence = 0;
			}
			slot_state = DMR_STATE_TX_1;
			break;

		case DMR_STATE_TX_END_1: // Stop TX (first step)
			if (nonVolatileSettings.bitfieldOptions & BIT_TRANSMIT_TALKER_ALIAS)
			{
				setupPcOrTGHeader();
			}
			SPI1WritePageRegByteArray(0x03, 0x00, SILENCE_AUDIO, 27);// send silence audio bytes
			SPI0WritePageRegByte(0x04, 0x41, 0x80);  //Transmit during Next Timeslot
			SPI0WritePageRegByte(0x04, 0x50, 0x20);  // Data Type =0010 (Terminator with LC), Data, LCSS=0
			slot_state = DMR_STATE_TX_END_2;
			break;

		case DMR_STATE_TX_END_2: // Stop TX (second step)
			// Need to hold on this TS after Tx ends otherwise if DMR Mon TS filtering is disabled the radio may switch timeslot
			dmrMonitorCapturedTS = trxGetDMRTimeSlot();
			dmrMonitorCapturedTimeout = nonVolatileSettings.dmrCaptureTimeout * 1000;
#ifdef THREE_STATE_SHUTDOWN
			slot_state = DMR_STATE_TX_END_3_RMO;
#else

			if (trxDMRModeTx == DMR_MODE_RMO)
			{
				SPI0WritePageRegByte(0x04, 0x40, 0xC3);  //Enable DMR Tx and Rx, Passive Timing
				SPI0WritePageRegByte(0x04, 0x41, 0x50);   //  Receive during Next Timeslot And Layer2 Access success Bit
				slot_state = DMR_STATE_TX_END_3_RMO;
			}
			else
			{
				init_digital_DMR_RX();
				slot_state = DMR_STATE_TX_END_3_DMO;
			}
#endif
			break;

		case DMR_STATE_TX_END_3_DMO:
			txstopdelay = 30;
			slot_state = DMR_STATE_IDLE;
			trxIsTransmitting = false;
			break;

		case DMR_STATE_TX_END_3_RMO:
			skip_count=2;// Hold off displaying or opening the DMR squelch frames under some conditions
			if (trxDMRModeTx == DMR_MODE_RMO)
			{
				LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 1);
				SPI0WritePageRegByte(0x04, 0x41, 0x50);   //  Receive during Next Timeslot And Layer2 Access success Bit
				slot_state = DMR_STATE_RX_1;
			}
			else
			{
				LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
				slot_state = DMR_STATE_IDLE;
			}
			trxIsTransmitting = false;
			break;

		case DMR_STATE_REPEATER_WAKE_1:
			{
				LEDs_PinWrite(GPIO_LEDred, Pin_LEDred, 1);// Turn on the Red LED while when we transmit the wakeup frame
				uint8_t spi_tx1[] = { 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
				spi_tx1[7] = (trxDMRID >> 16) & 0xFF;
				spi_tx1[8] = (trxDMRID >> 8) & 0xFF;
				spi_tx1[9] = (trxDMRID >> 0) & 0xFF;
				SPI0WritePageRegByteArray(0x02, 0x00, spi_tx1, 0x0c);
				SPI0WritePageRegByte(0x04, 0x50, 0x30);
				SPI0WritePageRegByte(0x04, 0x41, 0x80);
				trxIsTransmitting = true;
			}
			repeaterWakeupResponseTimeout = WAKEUP_RETRY_PERIOD;
			slot_state = DMR_STATE_REPEATER_WAKE_2;
			break;

		case DMR_STATE_REPEATER_WAKE_2:
			LEDs_PinWrite(GPIO_LEDred, Pin_LEDred, 0);// Turn off the Red LED while we are waiting for the repeater to wakeup
			init_digital_DMR_RX();
			rxcnt = 0;
			slot_state = DMR_STATE_REPEATER_WAKE_3;
			break;

		case DMR_STATE_REPEATER_WAKE_3:
			if (rxcnt > 3)
			{
				// wait for the signal from the repeater to have toggled timecode at least three times, i.e the signal should be stable and we should be able to go into Tx
				slot_state = DMR_STATE_RX_1;
				trxIsTransmitting = false;
				isWaking = WAKING_MODE_NONE;
			}
			break;

		default:
			break;
	}

	// Timeout interrupted RX
	if (slot_state < DMR_STATE_TX_START_1)
	{
		tick_cnt++;
		if ((slot_state == DMR_STATE_IDLE) && (tick_cnt > START_TICK_TIMEOUT))
		{
			init_digital_DMR_RX();
			clearActiveDMRID();
			disableAudioAmp(AUDIO_AMP_MODE_RF);
			LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
			tick_cnt = 0;
			uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
			slot_state = DMR_STATE_IDLE;
			trxIsTransmitting = false;
		}
		else
		{
			if (((slot_state == DMR_STATE_RX_1) || (slot_state == DMR_STATE_RX_1)) && (tick_cnt > END_TICK_TIMEOUT))
			{
				init_digital_DMR_RX();
				clearActiveDMRID();
				disableAudioAmp(AUDIO_AMP_MODE_RF);
				LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
				tick_cnt = 0;
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
				slot_state = DMR_STATE_IDLE;
				trxIsTransmitting = false;
			}
		}
	}
	timer_hrc6000task = 0;// I don't think this actually does anything. Its probably redundant legacy code
}

void HRC6000RxInterruptHandler(void)
{
	trxActivateRx();
}

void HRC6000TxInterruptHandler(void)
{
	trxActivateTx();
}


void init_HR_C6000_interrupts(void)
{
	init_digital_state();

	interruptsInitC6000Interface();
}

void init_digital_state(void)
{
	int_timeout = 0;
	slot_state = DMR_STATE_IDLE;
	trxIsTransmitting = false;
	tick_cnt = 0;
	skip_count = 0;
	qsodata_timer = 0;
}

void init_digital_DMR_RX(void)
{
	SPI0WritePageRegByte(0x04, 0x40, 0xC3);  //Enable DMR Tx, DMR Rx, Passive Timing, Normal mode
	SPI0WritePageRegByte(0x04, 0x41, 0x20);  //Set Sync Fail Bit (Reset?))
	SPI0WritePageRegByte(0x04, 0x41, 0x00);  //Reset
	SPI0WritePageRegByte(0x04, 0x41, 0x20);  //Set Sync Fail Bit (Reset?)
	SPI0WritePageRegByte(0x04, 0x41, 0x50);  //Receive during next Timeslot
}

void reset_timeslot_detection(void)
{
	disableAudioAmp(AUDIO_AMP_MODE_RF);
	LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
	timeCode = -1;// Clear current timecode synchronisation
	tsLockCount = 0;
	lastTimeCode = 0;
	dmrMonitorCapturedTS = -1;
	dmrMonitorCapturedTimeout = 0;
}

void init_digital(void)
{
	reset_timeslot_detection();
	init_digital_DMR_RX();
	init_digital_state();
	NVIC_EnableIRQ(PORTC_IRQn);

	codecInit();
}

void terminate_digital(void)
{
	disableAudioAmp(AUDIO_AMP_MODE_RF);
	LEDs_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
	init_digital_state();
	NVIC_DisableIRQ(PORTC_IRQn);
}



void triggerQSOdataDisplay(void)
{
	// If this is the start of a newly received signal, we always need to trigger the display to show this, even if its the same station calling again.
	// Of if the display is holding on the PC accept text and the incoming call is not a PC
	if ((qsodata_timer == 0) || ((uiDataGlobal.PrivateCall.state == PRIVATE_CALL_ACCEPT) && (DMR_frame_buffer[0] == TG_CALL_FLAG)))
	{
		uiDataGlobal.displayQSOState = QSO_DISPLAY_CALLER_DATA;
	}

	qsodata_timer = QSO_TIMER_TIMEOUT;
}

void init_hrc6000_task(void)
{
	xTaskCreate(fw_hrc6000_task,                        /* pointer to the task */
				"fw hrc6000 task",                      /* task name for kernel awareness debugging */
				5000L / sizeof(portSTACK_TYPE),      /* task stack size */
				NULL,                      			 /* optional task startup argument */
				4U,                                  /* initial priority */
				fwhrc6000TaskHandle					 /* optional task handle to create */
				);
}

void fw_hrc6000_task(void *data)
{
	while (1U)
	{
		if (timer_hrc6000task == 0)
		{
			timer_hrc6000task = 10;
			taskENTER_CRITICAL();
			alive_hrc6000task = true;
			taskEXIT_CRITICAL();

			if (trxGetMode() == RADIO_MODE_DIGITAL)
			{
				tick_HR_C6000();
			}
		}

		vTaskDelay(0);
	}
}

void setupPcOrTGHeader(void)
{
	uint8_t spi_tx[12];

	spi_tx[0] = (trxTalkGroupOrPcId >> 24) & 0xFF;
	spi_tx[1] = 0x00;
	spi_tx[2] = 0x00;
	spi_tx[3] = (trxTalkGroupOrPcId >> 16) & 0xFF;
	spi_tx[4] = (trxTalkGroupOrPcId >> 8) & 0xFF;
	spi_tx[5] = (trxTalkGroupOrPcId >> 0) & 0xFF;
	spi_tx[6] = (trxDMRID >> 16) & 0xFF;
	spi_tx[7] = (trxDMRID >> 8) & 0xFF;
	spi_tx[8] = (trxDMRID >> 0) & 0xFF;
	spi_tx[9] = 0x00;
	spi_tx[10] = 0x00;
	spi_tx[11] = 0x00;
	SPI0WritePageRegByteArray(0x02, 0x00, spi_tx, 0x0c);
}

bool callAcceptFilter(void)
{
	 if (settingsUsbMode == USB_MODE_HOTSPOT)
	 {
		//In Hotspot mode, we need to accept all incoming traffic, otherwise private calls won't work
		if ((DMR_frame_buffer[0] == TG_CALL_FLAG) || (DMR_frame_buffer[0] == PC_CALL_FLAG))
		{
			return true;
		}
		else
		{
			return false;// Not a PC or TG call
		}
	 }
	 else
	 {
		 return ((DMR_frame_buffer[0] == TG_CALL_FLAG) || (receivedTgOrPcId == (trxDMRID | (PC_CALL_FLAG << 24))));
	 }
}



void tick_HR_C6000(void)
{

	if((nonVolatileSettings.dmrCcTsFilter & DMR_CC_FILTER_PATTERN) == 0)
	{
		if(slot_state == DMR_STATE_IDLE)
		{
			if (ccHoldTimer < CCHOLDVALUE)
			{
				ccHoldTimer++;
			}
			else
			{
				ccHold = false;
			}
		}
		else
		{
			ccHoldTimer = 0;
		}
	}


	if ((trxTransmissionEnabled == true) && (isWaking == WAKING_MODE_NONE))
	{
		if (slot_state == DMR_STATE_IDLE)
		{
			// Because the ISR's also write to the SPI we need to disable interrupts on Port C when doing any SPI transfers.
			// Otherwise there could be clashes in the SPI subsystem.
			// This is possibly not the ideal solution, and a better solution may be found at a later date
			// But at least it should prevent things going too badly wrong
			NVIC_DisableIRQ(PORTC_IRQn);
			SPI0WritePageRegByte(0x04, 0x40, 0xE3); // TX and RX enable, Active Timing.
			SPI0WritePageRegByte(0x04, 0x21, 0xA2); // Set Polite to Color Code and Reset vocoder encodingbuffer
			SPI0WritePageRegByte(0x04, 0x22, 0x86); // Start Vocoder Encode, I2S mode
			NVIC_EnableIRQ(PORTC_IRQn);

			if (trxDMRModeTx == DMR_MODE_DMO)
			{
				if (settingsUsbMode != USB_MODE_HOTSPOT)
				{
					codecInit();
				}
				else
				{
					// Note. We don't increment the buffer indexes, because this is also the first frame of audio and we need it later
					NVIC_DisableIRQ(PORTC_IRQn);
					SPI0WritePageRegByteArray(0x02, 0x00, (uint8_t *)&audioAndHotspotDataBuffer.hotspotBuffer[wavbuffer_read_idx], 0x0c);// put LC into hardware
					NVIC_EnableIRQ(PORTC_IRQn);
					memcpy((uint8_t *)deferredUpdateBuffer, (uint8_t *)&audioAndHotspotDataBuffer.hotspotBuffer[wavbuffer_read_idx], 27 + 0x0C);
					hotspotDMRTxFrameBufferEmpty = false;
				}
				slot_state = DMR_STATE_TX_START_1;
			}
			else
			{
				if (settingsUsbMode != USB_MODE_HOTSPOT)
				{
					codecInit();
				}
				isWaking = WAKING_MODE_WAITING;
				wakeTriesCount = 0;
				slot_state = DMR_STATE_REPEATER_WAKE_1;
			}
		}
		else
		{
			// Note. In Tier 2 Passive (Repeater operation). The radio will already be receiving DMR frames from the repeater
			// And the transition from Rx to Tx is handled in the Timeslot ISR state machine
		}
	}


#define TIMEOUT 200
	// Timeout interrupt
	// This code appears to check whether there has been a TS ISR in the last 200 RTOS ticks
	// If not, it reinitialises the DMR subsystem
	// Its unclear whether this is still needed, as it would indicate that perhaps some other condition is not being handled correctly
	if (slot_state != DMR_STATE_IDLE)
	{
		if (int_timeout < TIMEOUT)
		{
			int_timeout++;
			if (int_timeout == TIMEOUT)
			{
				init_digital();// sets 	int_timeout=0;
				clearActiveDMRID();
				uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
				slot_state = DMR_STATE_IDLE;
				trxIsTransmitting = false;
			}
		}
	}
	else
	{
		int_timeout = 0;
	}

	if (trxTransmissionEnabled)
	{
		if (isWaking == WAKING_MODE_WAITING)
		{
			if (repeaterWakeupResponseTimeout > 0)
			{
				repeaterWakeupResponseTimeout--;
			}
			else
			{
				wakeTriesCount++;
				if (wakeTriesCount > codeplugGetRepeaterWakeAttempts())
				{
					isWaking = WAKING_MODE_FAILED;// signal that the Wake process has failed.
				}
				else
				{
					NVIC_DisableIRQ(PORTC_IRQn);
					SPI0WritePageRegByte(0x04, 0x40, 0xE3); // TX and RX enable, Active Timing.
					SPI0WritePageRegByte(0x04, 0x21, 0xA2); // Set Polite to Color Code and Reset vocoder encodingbuffer
					SPI0WritePageRegByte(0x04, 0x22, 0x86); // Start Vocoder Encode, I2S mode
					NVIC_EnableIRQ(PORTC_IRQn);
					repeaterWakeupResponseTimeout = WAKEUP_RETRY_PERIOD;
					slot_state = DMR_STATE_REPEATER_WAKE_1;
				}
			}
		}
		else
		{
			// normal operation. Not waking the repeater
			if (settingsUsbMode == USB_MODE_HOTSPOT)
			{
				if ((hotspotDMRTxFrameBufferEmpty == true) && (wavbuffer_count > 0))
				{
					memcpy((uint8_t *)deferredUpdateBuffer, (uint8_t *)&audioAndHotspotDataBuffer.hotspotBuffer[wavbuffer_read_idx], 27 + 0x0C);
					wavbuffer_read_idx++;
					if (wavbuffer_read_idx > (HOTSPOT_BUFFER_COUNT - 1))
					{
						wavbuffer_read_idx = 0;
					}

					if (wavbuffer_count > 0)
					{
						wavbuffer_count--;
					}
					hotspotDMRTxFrameBufferEmpty = false;
				}
			}
			else
			{
				// Once there are 2 buffers available they can be encoded into one AMBE block
				// The will happen  prior to the data being needed in the TS ISR, so that by the time tick_codec_encode encodes complete,
				// the data is ready to be used in the TS ISR
				if (wavbuffer_count >= 2)
				{
					codecEncodeBlock((uint8_t *)deferredUpdateBufferInPtr);

					deferredUpdateBufferInPtr += LENGTH_AMBE_BLOCK;

					if (deferredUpdateBufferInPtr > deferredUpdateBufferEnd)
					{
						deferredUpdateBufferInPtr = deferredUpdateBuffer;
					}
					ambeBufferCount++;
				}
			}
		}
	}
	else
	{
		// receiving RF DMR
		if (settingsUsbMode == USB_MODE_HOTSPOT)
		{
			if (hotspotDMRRxFrameBufferAvailable == true)
			{
				hotspotRxFrameHandler((uint8_t *)DMR_frame_buffer);
				hotspotDMRRxFrameBufferAvailable = false;
			}
		}
		else
		{
			if (monitorModeData.isEnabled && (monitorModeData.DMRTimeout > 0))
			{
				if (!rxCRCisValid)
				{
					monitorModeData.DMRTimeout--;
					if (monitorModeData.DMRTimeout == 0)
					{
						// switch to analogue
						trxSetModeAndBandwidth(RADIO_MODE_ANALOG, true);
						currentChannelData->sql =  CODEPLUG_MIN_VARIABLE_SQUELCH;;
						trxSetRxCSS(CODEPLUG_CSS_NONE);
						headerRowIsDirty = true;
					}
				}
				else
				{
					//found DMR signal
					//SEGGER_RTT_printf(0, "%d\n",monitorModeData.DMRTimeout);
					monitorModeData.DMRTimeout = 0;
				}
			}

			// voice prompts take priority over incoming DMR audio
			if (!voicePromptsIsPlaying() && hasEncodedAudio)
			{
				hasEncodedAudio = false;
				codecDecode((uint8_t *)DMR_frame_buffer + 0x0C, 3);
				soundTickRXBuffer();
			}
		}

		if (qsodata_timer > 0)
		{
			// Only timeout the QSO data display if not displaying the Private Call Accept Yes/No text
			// if menuUtilityReceivedPcId is non zero the Private Call Accept text is being displayed
			if (uiDataGlobal.PrivateCall.state != PRIVATE_CALL_ACCEPT)
			{
				qsodata_timer--;

				if (qsodata_timer == 0)
				{
					uiDataGlobal.displayQSOState = QSO_DISPLAY_DEFAULT_SCREEN;
				}
			}
		}
	}

	if ((dmrMonitorCapturedTS != -1) && (dmrMonitorCapturedTimeout > 0))
	{
		dmrMonitorCapturedTimeout--;
		if (dmrMonitorCapturedTimeout == 0)
		{
			dmrMonitorCapturedTS = -1;// Reset the TS capture
		}
	}

	rxCRCisValid = false;// Reset this
}

// RC. I had to use accessor functions for the isWaking flag
// because the compiler seems to have problems with volatile vars as externs used by other parts of the firmware (the Tx Screen)
void clearIsWakingState(void)
{
	isWaking = WAKING_MODE_NONE;
}

int getIsWakingState(void)
{
	return isWaking;
}

void clearActiveDMRID(void)
{
	memset((uint8_t *)previousLCBuf, 0x00, 12);
	memset((uint8_t *)DMR_frame_buffer, 0x00, 12);
	receivedTgOrPcId 	= 0x00;
	receivedSrcId 		= 0x00;
}

int HRC6000GetReceivedTgOrPcId(void)
{
	return receivedTgOrPcId;
}

int HRC6000GetReceivedSrcId(void)
{
	return receivedSrcId;
}

void HRC6000ClearTimecodeSynchronisation(void)
{
	timeCode = -1;
}
