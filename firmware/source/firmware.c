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
 
/**
 * @file    firmware.c
 * @brief   Application entry point.
 */
#include <main.h>
#include <stdio.h>
#include "clock_config.h"
#include "MK22F51212.h"


/*
 * @brief   Application entry point.
 */

int main(void) {
	SCB->VTOR = 0x4000;

    BOARD_BootClockHSRUN();// Start in High Speed Run mode (118Mhz)
//    BOARD_BootClockRUN();// Use lower speeds. See clock_config.c    mcgConfig_BOARD_BootClockRUN

    mainTaskInit();

    return 0;
}
