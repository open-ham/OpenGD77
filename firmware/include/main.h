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

#ifndef _OPENGD77_MAIN_H_
#define _OPENGD77_MAIN_H_

#include <stdint.h>
#include <stdio.h>

#include <FreeRTOS.h>
#include <task.h>

#include "utils.h"

#include "virtual_com.h"
#include "usb_com.h"

#include "io/buttons.h"
#include "io/LEDs.h"
#include "io/keyboard.h"
#include "io/rotary_switch.h"
#include "io/display.h"
#include "functions/vox.h"

#include "hardware/UC1701.h"

#include "interfaces/i2c.h"
#include "interfaces/hr-c6000_spi.h"
#include "interfaces/i2s.h"
#include "hardware/AT1846S.h"
#include "hardware/HR-C6000.h"
#include "interfaces/wdog.h"
#include "interfaces/adc.h"
#include "interfaces/dac.h"
#include "interfaces/pit.h"

#include "functions/sound.h"
#include "functions/trx.h"
#include "hardware/SPI_Flash.h"
#include "hardware/EEPROM.h"


void mainTaskInit(void);
void powerOffFinalStage(void);

#endif /* _OPENGD77_MAIN_H_ */
