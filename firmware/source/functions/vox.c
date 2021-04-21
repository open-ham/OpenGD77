/*
 * Copyright (C)2020	Kai Ludwig, DG4KLU
 *               and	Roger Clark, VK3KYY / G4KYF
 *               and	Daniel Caujolle-Bert, F1RMB
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

#include "functions/vox.h"
#include "interfaces/adc.h"
#include "functions/sound.h"
#include "functions/settings.h"
#include "interfaces/pit.h"
#include "utils.h"


#define PIT_COUNTS_PER_MS  10U


static const uint32_t VOX_TAIL_TIME_UNIT = (PIT_COUNTS_PER_MS * 500); // 500ms tail unit
static const uint16_t VOX_SETTLE_TIME = 4000; // Countdown before doing first real sampling;

typedef struct
{
	bool     triggered;
	uint8_t  preTrigger;
	uint8_t  threshold; // threshold is a super low value, 8 bits are enough
	uint16_t sampled;
	uint16_t averaged;
	uint16_t noiseFloor;
	uint32_t nextTimeSampling;
	uint8_t  tailUnits;
	uint32_t tailTime;
	uint16_t settleCount;
} voxData_t;

static voxData_t vox;

void voxInit(void)
{
	voxReset();
	vox.threshold = 0;
	vox.noiseFloor = 0;
	vox.tailUnits = 1;
	vox.settleCount = VOX_SETTLE_TIME;
}

void voxSetParameters(uint8_t threshold, uint8_t tailHalfSecond)
{
	vox.triggered = false;
	vox.threshold = threshold;

	voxReset();
	vox.tailUnits = tailHalfSecond;
	vox.settleCount = VOX_SETTLE_TIME;
}

bool voxIsEnabled(void)
{
	return ((currentChannelData->flag4 & 0x40) && (settingsUsbMode != USB_MODE_HOTSPOT));
}

bool voxIsTriggered(void)
{
	return vox.triggered;
}

void voxReset(void)
{
	vox.triggered = false;
	vox.sampled = 0;
	vox.averaged = 0;
	vox.nextTimeSampling = PITCounter + PIT_COUNTS_PER_MS; // now + 1ms
	vox.tailTime = 0;
	vox.settleCount = VOX_SETTLE_TIME >> 1;
	vox.preTrigger = 0;
}

void voxTick(void)
{
	static uint16_t sampledNoise = 0;

	if (voxIsEnabled())
	{
		if (PITCounter >= vox.nextTimeSampling)
		{
			if ((getAudioAmpStatus() & (AUDIO_AMP_MODE_RF | AUDIO_AMP_MODE_BEEP)))
			{
				voxReset();
			}
			else
			{
				uint16_t sample = getVOX();

				if (vox.settleCount > 0)
				{
					vox.settleCount--;
					return;
				}

				vox.sampled += sample;
				vox.averaged = (vox.sampled + (1 << (2 - 1))) >> 2;
				vox.sampled -= vox.averaged;

				if (vox.averaged >= (vox.noiseFloor + vox.threshold))
				{
					vox.preTrigger = SAFE_MIN((vox.preTrigger + 1), 100);

					// We need 100ms of level above the noise to trigger the VOX
					if (vox.preTrigger >= 100)
					{
						vox.triggered = true;
						vox.tailTime = PITCounter + (vox.tailUnits * VOX_TAIL_TIME_UNIT);
					}
				}
				else
				{
					// Noise is sampled all the time, when the transceiver is silent, and not XMitting
					if (vox.triggered == false)
					{
						// Noise floor averaging
						sampledNoise += sample;
						vox.noiseFloor = (sampledNoise + (1 << (2 - 1))) >> 2;
						sampledNoise -= vox.noiseFloor;
					}
				}
			}

			vox.nextTimeSampling = PITCounter + PIT_COUNTS_PER_MS; // now + 1ms
		}

		if ((vox.tailTime != 0) && (PITCounter >= vox.tailTime))
		{
			vox.triggered = false;
			vox.tailTime = 0;
			vox.preTrigger = 0;
		}
	}

	if (vox.triggered && (settingsUsbMode == USB_MODE_HOTSPOT))
	{
		voxReset();
	}

}
