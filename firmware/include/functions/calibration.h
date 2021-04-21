/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
 * 				and  Kai Ludwig, DG4KLU
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
#ifndef _OPENGD77_CALIBRATION_H_
#define _OPENGD77_CALIBRATION_H_

#include "hardware/SPI_Flash.h"

typedef enum
{
	CalibrationBandUHF,
	CalibrationBandVHF,
	CalibrationBandMAX
} CalibrationBand_t;

typedef enum
{
	CalibrationSection_DACDATA_SHIFT,
	CalibrationSection_TWOPOINT_MOD,
	CalibrationSection_Q_MOD2_OFFSET,
	CalibrationSection_PHASE_REDUCE,
	CalibrationSection_PGA_GAIN,
	CalibrationSection_VOICE_GAIN_TX,
	CalibrationSection_GAIN_TX,
	CalibrationSection_PADRV_IBIT,
	CalibrationSection_XMITTER_DEV_WIDEBAND,
	CalibrationSection_XMITTER_DEV_NARROWBAND,
	CalibrationSection_DEV_TONE,
	CalibrationSection_DAC_VGAIN_ANALOG,
	CalibrationSection_VOLUME_ANALOG,
	CalibrationSection_NOISE1_TH_WIDEBAND,
	CalibrationSection_NOISE1_TH_NARROWBAND,
	CalibrationSection_NOISE2_TH_WIDEBAND,
	CalibrationSection_NOISE2_TH_NARROWBAND,
	CalibrationSection_RSSI3_TH_WIDEBAND,
	CalibrationSection_RSSI3_TH_NARROWBAND,
	CalibrationSection_SQUELCH_TH
} CalibrationSection_t;

typedef struct
{
	uint16_t offset;
	uint16_t mod;

	union
	{
		uint16_t value;
		uint8_t  bytes[2];
	};
} CalibrationDataResult_t;


typedef struct calibrationPowerValues
{
	uint32_t lowPower;
	uint32_t highPower;
} calibrationPowerValues_t;

typedef struct calibrationRSSIMeter
{
	uint8_t minVal;
	uint8_t rangeVal;
} calibrationRSSIMeter_t;

typedef struct deviationToneStruct
{
	uint8_t dtmf;
	uint8_t tone;
	uint8_t ctcss_wide;
	uint8_t ctcss_narrow;
	uint8_t dcs_wide;
	uint8_t dcs_narrow;
} deviationToneStruct_t;

extern const int MAX_PA_DAC_VALUE;


bool calibrationInit(void);
bool calibrationGetSectionData(CalibrationBand_t band, CalibrationSection_t section, CalibrationDataResult_t *o);
void calibrationGetPowerForFrequency(int freq, calibrationPowerValues_t *powerSettings);
bool calibrationGetRSSIMeterParams(calibrationRSSIMeter_t *rssiMeterValues);
bool calibrationCheckAndCopyToCommonLocation(bool forceReload);

#endif
