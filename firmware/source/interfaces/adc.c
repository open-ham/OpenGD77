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

#include "interfaces/adc.h"
#include "functions/settings.h"

static volatile uint32_t adc_channel;
static volatile uint32_t adcBatteryVoltage;
static volatile uint32_t adcVOX;
static volatile uint32_t adcTemperature;
volatile uint32_t dispTemp=0;
volatile int averageLength = 0;
const int TEMPERATURE_DECIMAL_RESOLUTION = 1000000;


const int CUTOFF_VOLTAGE_UPPER_HYST = 64;
const int CUTOFF_VOLTAGE_LOWER_HYST = 62;
const int BATTERY_MAX_VOLTAGE = 82;
void approxRollingAverage (unsigned int newSample);

void adcTriggerConversion(int channelOverride)
{
    adc16_channel_config_t adc16ChannelConfigStruct;

    if (channelOverride != NO_ADC_CHANNEL_OVERRIDE)
    {
    	adc_channel = channelOverride;
    }
    adc16ChannelConfigStruct.channelNumber = adc_channel;
    adc16ChannelConfigStruct.enableInterruptOnConversionCompleted = true;
    adc16ChannelConfigStruct.enableDifferentialConversion = false;
    ADC16_SetChannelConfig(ADC0, 0, &adc16ChannelConfigStruct);
}

void adcInit(void)
{
	adc16_config_t adc16ConfigStruct;

	taskENTER_CRITICAL();
	adc_channel = 1;// Next channel to sample. Channel 1 is the battery
	adcBatteryVoltage = 0;
	adcVOX = 0;

	taskEXIT_CRITICAL();

    ADC16_GetDefaultConfig(&adc16ConfigStruct);
    ADC16_Init(ADC0, &adc16ConfigStruct);
    ADC16_EnableHardwareTrigger(ADC0, false);
    ADC16_DoAutoCalibration(ADC0);

    EnableIRQ(ADC0_IRQn);

    adcTriggerConversion(NO_ADC_CHANNEL_OVERRIDE);
}
void approxRollingAverage (unsigned int newSample)
{
#if defined(PLATFORM_DM1801)
	const int AVERAGING_LENGTH = 1000;
#elif defined(PLATFORM_RD5R)
	const int AVERAGING_LENGTH = 250;
#else
	const int AVERAGING_LENGTH = 250;
#endif
	newSample *= TEMPERATURE_DECIMAL_RESOLUTION;
	if (averageLength == 0)
	{
		// set initial reading to the sample
		adcTemperature = newSample;
	}

	// Gradually increase the averaging length, to get to a stable reading quicker
	if (averageLength < AVERAGING_LENGTH)
	{
		averageLength += 1;
	}

	adcTemperature -= adcTemperature / averageLength;
	adcTemperature += newSample / averageLength;
}

void ADC0_IRQHandler(void)
{
	uint32_t result = ADC16_GetChannelConversionValue(ADC0, 0);

    switch (adc_channel)
    {
    case 1:
    	adcBatteryVoltage = result;
    	adc_channel = 3;// get channel 3 next
    	break;
    case 3:
    	adcVOX = result;
    	adc_channel = 26;// get channel 26 next
    	break;
    case 26:
    	approxRollingAverage(result);
    	adc_channel = 1;// get channel 1 next
    	break;
    }

    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
    exception return operation might vector to incorrect interrupt */
    __DSB();
}

// result of conversion is rounded voltage*10 as integer
int adcGetBatteryVoltage(void)
{
	return (int)(adcBatteryVoltage / 41.6f + 0.5f);
}

int getVOX(void)
{
	return adcVOX;
}

int getTemperature(void)
{
#if defined(PLATFORM_DM1801)
	const int OFFSET = 9420;// Value needs to be validated as average for this radio
#elif defined(PLATFORM_RD5R)
	const int OFFSET = 9130;// Value needs to be validated as average for this radio
#else
	const int OFFSET = 9250;// Value needs to be validated as average for this radio
#endif

	return  (OFFSET + (nonVolatileSettings.temperatureCalibration * 10) - (adcTemperature  / (TEMPERATURE_DECIMAL_RESOLUTION / 10))) / 2;
}
