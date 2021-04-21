/*
 * Copyright (C)2021 Roger Clark. VK3KYY / G4KYF
 */

#ifndef _ECOLEVELS_H_
#define _ECOLEVELS_H_

#include <stdint.h>
#include <stdbool.h>
#include "user_interface/menuSystem.h"

typedef enum
{ 	ECOPHASE_POWERSAVE_INACTIVE = 0,
	ECHOPHASE_POWERSAVE_ENTRY,
	ECOPHASE_POWERSAVE_ACTIVE___RX_IS_ON,
	ECOPHASE_POWERSAVE_ACTIVE___RX_IS_OFF
} ecoPhase_t;

void rxPowerSavingTick(uiEvent_t *ev, bool hasSignal);
void rxPowerSavingSetLevel(int newLevel);
void rxPowerSavingSetState(ecoPhase_t newState);
bool rxPowerSavingIsRxOn(void);
int rxPowerSavingGetLevel(void);
#endif
