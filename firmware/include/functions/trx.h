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

#ifndef _OPENGD77_TRX_H_
#define _OPENGD77_TRX_H_

#include "functions/sound.h"
#include "interfaces/i2c.h"
#include "functions/calibration.h"
#include "functions/codeplug.h"

typedef enum
{
	CSS_NONE = 0,
	CSS_CTCSS,
	CSS_DCS,
	CSS_DCS_INVERTED
} CSSTypes_t;

typedef struct
{
	int calTableMinFreq;
	int minFreq;
	int maxFreq;
} frequencyHardwareBand_t;

typedef struct
{
	int minFreq;
	int maxFreq;
} frequencyBand_t;

typedef struct trxFrequency
{
	int rxFreq;
	int txFreq;
} trxFrequency_t;

enum RADIO_MODE { RADIO_MODE_NONE, RADIO_MODE_ANALOG, RADIO_MODE_DIGITAL };
enum DMR_ADMIT_CRITERIA { ADMIT_CRITERIA_ALWAYS, ADMIT_CRITERIA_CHANNEL_FREE, ADMIT_CRITERIA_COLOR_CODE };
enum DMR_MODE { DMR_MODE_AUTO, DMR_MODE_DMO, DMR_MODE_RMO, DMR_MODE_SFR };
enum RADIO_FREQUENCY_BAND_NAMES { RADIO_BAND_VHF = 0, RADIO_BAND_220MHz, RADIO_BAND_UHF, RADIO_BANDS_TOTAL_NUM };
enum TRX_FREQ_BAND { TRX_RX_FREQ_BAND = 0, TRX_TX_FREQ_BAND };

extern const frequencyHardwareBand_t 	RADIO_HARDWARE_FREQUENCY_BANDS[RADIO_BANDS_TOTAL_NUM];
extern const frequencyBand_t			DEFAULT_USER_FREQUENCY_BANDS[RADIO_BANDS_TOTAL_NUM];
extern frequencyBand_t					USER_FREQUENCY_BANDS[RADIO_BANDS_TOTAL_NUM];

extern const uint8_t TRX_NUM_CTCSS;
extern const uint16_t TRX_CTCSSTones[];

extern const uint16_t TRX_DCS_TONE;
extern const uint8_t TRX_NUM_DCS;
extern const uint16_t TRX_DCSCodes[];

extern int trxDMRModeRx;
extern int trxDMRModeTx;

extern volatile bool trxTransmissionEnabled;
extern volatile bool trxIsTransmitting;
extern uint32_t trxTalkGroupOrPcId;
extern uint32_t trxDMRID;
extern int txstopdelay;
extern volatile uint8_t trxRxSignal;
extern volatile uint8_t trxRxNoise;
extern volatile uint8_t trxTxVox;
extern volatile uint8_t trxTxMic;
extern calibrationPowerValues_t trxPowerSettings;
extern int trxCurrentBand[2];
extern volatile bool txPAEnabled;

void I2C_AT1846_set_register_with_mask(uint8_t reg, uint16_t mask, uint16_t value, uint8_t shift);

bool trxCarrierDetected(void);
bool trxCheckDigitalSquelch(void);
bool trxCheckAnalogSquelch(void);
int	trxGetMode(void);
int	trxGetBandwidthIs25kHz(void);
int	trxGetFrequency(void);
void trxSetModeAndBandwidth(int mode, bool bandwidthIs25kHz);
void trxSetFrequency(int fRx,int fTx, int dmrMode);
void trxSetRX(void);
void trxSetTX(void);
void trxAT1846RxOff(void);
void trxAT1846RxOn(void);
void trxActivateRx(void);
void trxActivateTx(void);
void trxSetPowerFromLevel(int powerLevel);
void trxUpdate_PA_DAC_Drive(void);
uint16_t trxGetPA_DAC_Drive(void);
int trxGetPowerLevel(void);
void trxUpdateC6000Calibration(void);
void trxUpdateAT1846SCalibration(void);
void trxSetDMRColourCode(int colourCode);
int trxGetDMRColourCode(void);
int trxGetDMRTimeSlot(void);
void trxSetDMRTimeSlot(int timeslot);
void trxSetTxCSS(uint16_t tone);
void trxSetRxCSS(uint16_t tone);
bool trxCheckCSSFlag(uint16_t tone);
bool trxCheckFrequencyInAmateurBand(int tmp_frequency);
int trxGetBandFromFrequency(int frequency);
int trxGetNextOrPrevBandFromFrequency(int frequency, bool nextBand);
void trxReadVoxAndMicStrength(void);
void trxReadRSSIAndNoise(void);
uint8_t trxGetCalibrationVoiceGainTx(void);
void trxSelectVoiceChannel(uint8_t channel);
void trxSetTone1(int toneFreq);
void trxSetTone2(int toneFreq);
void trxSetDTMF(int code);
void trxUpdateTsForCurrentChannelWithSpecifiedContact(struct_codeplugContact_t *contactData);
uint32_t trxDCSEncode(uint16_t dcsCode);
void trxSetMicGainFM(uint8_t gain);

void trxEnableTransmission(void);
void trxDisableTransmission(void);
void trxPowerUpDownRxAndC6000(bool powerUp, bool includeC6000);
uint8_t trxGetAnalogFilterLevel();
void trxSetAnalogFilterLevel(uint8_t newFilterLevel);
#endif /* _OPENGD77_TRX_H_ */
