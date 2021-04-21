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

#include "interfaces/wdog.h"
#include "interfaces/pit.h"

static void watchdogTick(void);
static void fw_watchdog_task(void *data);
TaskHandle_t fwwatchdogTaskHandle;
volatile bool alive_maintask;
volatile bool alive_beeptask;
volatile bool alive_hrc6000task;
float averageBatteryVoltage;
float previousAverageBatteryVoltage;
int batteryVoltage = 0;
bool headerRowIsDirty = false;

static WDOG_Type *wdog_base = WDOG;
static int watchdog_refresh_tick = 0;
static bool reboot = false;
static int batteryVoltageTick = 0;
static int batteryVoltageCallbackTick = 0;
static const int BATTERY_VOLTAGE_TICK_RELOAD = 100;
static const int BATTERY_VOLTAGE_CALLBACK_TICK_RELOAD = 20;
static const int AVERAGE_BATTERY_VOLTAGE_SAMPLE_WINDOW = 60.0f;// 120 secs = Sample window * BATTERY_VOLTAGE_TICK_RELOAD in milliseconds
static const int BATTERY_VOLTAGE_STABILISATION_TIME = 2000;// time in PIT ticks for the battery voltage from the ADC to stabilise
batteryHistoryCallback_t batteryCallbackFunction = NULL;

static void fw_watchdog_task(void *data)
{
	while (1U)
	{
		if (timer_watchdogtask == 0)
		{
			timer_watchdogtask = 10;
			watchdogTick();
		}
		vTaskDelay(0);
	}
}

static void watchdogTick(void)
{
	watchdog_refresh_tick++;
	if (watchdog_refresh_tick == 200)
	{
		if (alive_maintask && alive_beeptask && alive_hrc6000task && !reboot)
		{
			WDOG_Refresh(wdog_base);
		}
		alive_maintask = false;
		alive_beeptask = false;
		alive_hrc6000task = false;
		watchdog_refresh_tick = 0;
	}

	batteryVoltageTick++;
	if (batteryVoltageTick >= BATTERY_VOLTAGE_TICK_RELOAD)
	{
		batteryVoltage = adcGetBatteryVoltage();

		if (PITCounter < BATTERY_VOLTAGE_STABILISATION_TIME)
		{
			averageBatteryVoltage = batteryVoltage;
		}
		else
		{
			averageBatteryVoltage = (averageBatteryVoltage * (AVERAGE_BATTERY_VOLTAGE_SAMPLE_WINDOW - 1) + batteryVoltage) / AVERAGE_BATTERY_VOLTAGE_SAMPLE_WINDOW;
		}

		if (previousAverageBatteryVoltage != averageBatteryVoltage)
		{
			previousAverageBatteryVoltage = averageBatteryVoltage;
			headerRowIsDirty = true;
		}

		batteryVoltageTick = 0;
		batteryVoltageCallbackTick++;
		if (batteryCallbackFunction && (batteryVoltageCallbackTick >= BATTERY_VOLTAGE_CALLBACK_TICK_RELOAD))
		{
			batteryCallbackFunction(averageBatteryVoltage);
			batteryVoltageCallbackTick = 0;
		}
	}
	adcTriggerConversion(NO_ADC_CHANNEL_OVERRIDE);// need the ADC value next time though, so request conversion now, so that its ready by the time we need it
}

void watchdogReboot(void)
{
	reboot = true;
}

void watchdogInit(batteryHistoryCallback_t cb)
{
	wdog_config_t config;

	WDOG_GetDefaultConfig(&config);
	config.timeoutValue = 0x3ffU;
	WDOG_Init(wdog_base, &config);
	for (uint32_t i = 0; i < 256; i++)
	{
		wdog_base->RSTCNT;
	}

	batteryCallbackFunction = cb;

	watchdog_refresh_tick = 0;

	alive_maintask = false;
	alive_beeptask = false;
	alive_hrc6000task = false;

	batteryVoltage = adcGetBatteryVoltage();
	averageBatteryVoltage = batteryVoltage;
	previousAverageBatteryVoltage = 0;// need to set this to zero to force an immediate display update.
	batteryVoltageTick = 0;
	batteryVoltageCallbackTick = 0;

	if (batteryCallbackFunction)
	{
		batteryCallbackFunction(batteryVoltage);
	}

	xTaskCreate(fw_watchdog_task,                        /* pointer to the task */
			"WDT",                      /* task name for kernel awareness debugging */
			1000L / sizeof(portSTACK_TYPE),      /* task stack size */
			NULL,                      			 /* optional task startup argument */
			5U,                                  /* initial priority */
			fwwatchdogTaskHandle					 /* optional task handle to create */
	);
}

void watchdogDeinit(void)
{
	WDOG_Deinit(wdog_base);
}
