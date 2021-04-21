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

#include <string.h>
#include "functions/ticks.h"

#define PIT_COUNTS_PER_MS  10U

extern volatile uint32_t PITCounter;
typedef struct
{
	timerCallback_t  funPtr;
	uint32_t         PIT_TriggerTime;
} timerCallbackbackStruct_t;

#define MAX_NUM_TIMER_CALLBACKS 8
timerCallbackbackStruct_t callbacksArray[MAX_NUM_TIMER_CALLBACKS];// As a global this will get cleared by the compiler

uint32_t fw_millis(void)
{
	return (PITCounter / PIT_COUNTS_PER_MS);
}

void handleTimerCallbacks(void)
{
	int i = 0;

	while((callbacksArray[i].funPtr != NULL) && (i < MAX_NUM_TIMER_CALLBACKS))
	{
		if (PITCounter > callbacksArray[i].PIT_TriggerTime)
		{
			callbacksArray[i].funPtr();// call the function
			memmove(&callbacksArray[i], &callbacksArray[i + 1], ((MAX_NUM_TIMER_CALLBACKS - 1) - i) * sizeof(timerCallbackbackStruct_t));
			callbacksArray[MAX_NUM_TIMER_CALLBACKS - 1].funPtr = NULL;
		}
		else
		{
			i++;
		}
	}
}

bool addTimerCallback(timerCallback_t funPtr, uint32_t delayIn_mS, bool updateExistingCallbackTime)
{
	uint32_t callBackTime = PITCounter + (delayIn_mS * PIT_COUNTS_PER_MS);

	for(int i = 0; i < MAX_NUM_TIMER_CALLBACKS; i++)
	{
		if (callbacksArray[i].funPtr == NULL)
		{
			callbacksArray[i].funPtr = funPtr;
			callbacksArray[i].PIT_TriggerTime = callBackTime;
			return true;
		}

		if ((callbacksArray[i].funPtr == funPtr) && updateExistingCallbackTime)
		{
			callbacksArray[i].PIT_TriggerTime = callBackTime;
			return true;
		}

		// callbacksArray[i] must be non-null pointer
		if (callbacksArray[i].PIT_TriggerTime > callBackTime)
		{
			if (i != (MAX_NUM_TIMER_CALLBACKS - 1))
			{
				// shuffle all other callbacks down in the list if there is space
				memmove(&callbacksArray[i+1], &callbacksArray[i], ((MAX_NUM_TIMER_CALLBACKS - 1) - i) * sizeof(timerCallbackbackStruct_t));
			}
			callbacksArray[i].funPtr = funPtr;
			callbacksArray[i].PIT_TriggerTime = callBackTime;
			return true;
		}
	}
	return false;
}
