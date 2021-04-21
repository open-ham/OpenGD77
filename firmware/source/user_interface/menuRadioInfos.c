/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
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
#include "user_interface/menuSystem.h"
#include "user_interface/uiLocalisation.h"
#include "user_interface/uiUtilities.h"
#include "utils.h"

static SemaphoreHandle_t battSemaphore = NULL;

#define VOLTAGE_BUFFER_LEN 128
static const float BATTERY_CRITICAL_VOLTAGE = 66.7f;
static const int TEMPERATURE_CRITICAL = 500; // 50°C
static float prevAverageBatteryVoltage = 0.0f;
static int prevTemperature = 0;

static menuStatus_t menuRadioInfosExitCode = MENU_STATUS_SUCCESS;

typedef struct
{
	int32_t  buffer[VOLTAGE_BUFFER_LEN];
	int32_t *head;
	int32_t *tail;
	int32_t *end;
	bool     modified;
} voltageCircularBuffer_t;

__attribute__((section(".data.$RAM2"))) voltageCircularBuffer_t batteryVoltageHistory;

enum { RADIO_INFOS_BATTERY_LEVEL = 0, RADIO_INFOS_TEMPERATURE_LEVEL, RADIO_INFOS_BATTERY_GRAPH, NUM_RADIO_INFOS_MENU_ITEMS };
enum { GRAPH_FILL = 0, GRAPH_LINE };

static int displayMode = RADIO_INFOS_BATTERY_LEVEL;
static int graphStyle = GRAPH_FILL;
static int battery_stack_iter = 0;
static const int BATTERY_ITER_PUSHBACK = 20;

static void updateScreen(bool forceRedraw);
static void handleEvent(uiEvent_t *ev);
static void updateVoicePrompts(bool spellIt);

static void circularBufferInit(voltageCircularBuffer_t *cb)
{
	cb->end = &cb->buffer[VOLTAGE_BUFFER_LEN - 1];
	cb->head = cb->buffer;
	cb->tail = cb->buffer;
	cb->modified = false;
}

static void circularBufferPushBack(voltageCircularBuffer_t *cb, const int32_t item)
{
	cb->modified = true;

	*cb->head = item;
	cb->head++;

    if(cb->head == cb->end)
    {
    	cb->head = cb->buffer;
    }

    if (cb->tail == cb->head)
    {
    	cb->tail++;

    	if(cb->tail == cb->end)
    	{
    		cb->tail = cb->buffer;
    	}
    }
}

static size_t circularBufferGetData(voltageCircularBuffer_t *cb, int32_t *data, size_t dataLen)
{
     size_t  count = 0;
     int32_t *p = cb->tail;

     while ((p != cb->head) && (count < dataLen))
     {
    	 *(data + count) = *p;

    	 p++;
    	 count++;

    	 if (p == cb->end)
    	 {
    		 p = cb->buffer;
    	 }
     }

     return count;
}

menuStatus_t menuRadioInfos(uiEvent_t *ev, bool isFirstRun)
{
	static uint32_t m = 0;

	if (isFirstRun)
	{
		menuDataGlobal.endIndex = NUM_RADIO_INFOS_MENU_ITEMS;
		ucClearBuf();
		menuDisplayTitle(currentLanguage->radio_info);
		ucRenderRows(0, 2);
		updateScreen(true);
		updateVoicePrompts(true);
	}
	else
	{
		if ((ev->time - m) > 500)
		{
			m = ev->time;
			updateScreen(false);// update the screen each 500ms to show any changes to the battery voltage or low battery
		}

		menuRadioInfosExitCode = MENU_STATUS_SUCCESS;

		if (ev->hasEvent)
		{
			handleEvent(ev);
		}
	}
	return menuRadioInfosExitCode;
}

static void updateScreen(bool forceRedraw)
{
	static bool blink = false;
	bool renderArrowOnly = true;

	switch (displayMode)
	{
		case RADIO_INFOS_BATTERY_LEVEL:
		{
			if ((prevAverageBatteryVoltage != averageBatteryVoltage) || (averageBatteryVoltage < BATTERY_CRITICAL_VOLTAGE) || forceRedraw)
			{
				char buffer[17];
				int volts, mvolts;
				const int x = 88;
				const int battLevelHeight = (DISPLAY_SIZE_Y - 28);

				prevAverageBatteryVoltage = averageBatteryVoltage;
				renderArrowOnly = false;

				if (forceRedraw)
				{
					// Clear whole drawing region
					ucFillRect(0, 14, DISPLAY_SIZE_X, DISPLAY_SIZE_Y - 14, true);
					// Draw...
					// Inner body frame
					ucDrawRoundRect(x + 1, 20, 26, DISPLAY_SIZE_Y - 22, 3, true);
					// Outer body frame
					ucDrawRoundRect(x, 19, 28, DISPLAY_SIZE_Y - 20, 3, true);
					// Positive pole frame
					ucFillRoundRect(x + 9, 15, 10, 6, 2, true);
				}
				else
				{
					// Clear voltage area
					ucFillRect(((x - (4 * 8)) >> 1) , 19 + 1, (4 * 8), (DISPLAY_SIZE_Y - 20) - 4, true);
					// Clear level area
					ucFillRoundRect(x + 4, 23, 20, battLevelHeight, 2, false);
				}

				getBatteryVoltage(&volts, &mvolts);
				snprintf(buffer, 17, "%1d.%1dV", volts, mvolts);
				ucPrintAt(((x - (4 * 8)) >> 1), 19 + 1, buffer, FONT_SIZE_3);
				snprintf(buffer, 17, "%d%%", getBatteryPercentage());
				ucPrintAt(((x - (strlen(buffer) * 8)) >> 1), (DISPLAY_SIZE_Y - 20)
#if defined(PLATFORM_RD5R)
						+ 7
#endif
						, buffer, FONT_SIZE_3);

				uint32_t h = (uint32_t)(((averageBatteryVoltage - CUTOFF_VOLTAGE_UPPER_HYST) * battLevelHeight) / (BATTERY_MAX_VOLTAGE - CUTOFF_VOLTAGE_UPPER_HYST));
				if (h > battLevelHeight)
				{
					h = battLevelHeight;
				}

				// Draw Level
				ucFillRoundRect(x + 4, 23 + battLevelHeight - h , 20, h, 2, (averageBatteryVoltage < BATTERY_CRITICAL_VOLTAGE) ? blink : true);

				if (voicePromptsIsPlaying() == false)
				{
					updateVoicePrompts(false);
				}
			}

			// Low blinking arrow
			ucFillTriangle(63, (DISPLAY_SIZE_Y - 1), 59, (DISPLAY_SIZE_Y - 5), 67, (DISPLAY_SIZE_Y - 5), blink);
		}
		break;

		case RADIO_INFOS_BATTERY_GRAPH:
		{
#define  CHART_WIDTH 104
			static int32_t hist[CHART_WIDTH];
			static size_t histLen = 0;
			bool newHistAvailable = false;

			// Grab history values.
			// There is a 10 ticks timeout, if it kicks in, history length will be 0, then
			// redraw will be done on the next run
			if (xSemaphoreTake(battSemaphore, (TickType_t)10) == pdTRUE)
			{
				if ((newHistAvailable = batteryVoltageHistory.modified) == true)
				{
					histLen = circularBufferGetData(&batteryVoltageHistory, hist, (sizeof(hist) / sizeof(hist[0])));
					batteryVoltageHistory.modified = false;
				}
				xSemaphoreGive(battSemaphore);
			}

			if (newHistAvailable || forceRedraw)
			{
				static const uint8_t chartX = 2 + (2 * 6) + 3 + 2;
				static const uint8_t chartY = 14 + 1 + 2;
				const int chartHeight = (DISPLAY_SIZE_Y - 26);

				// Min is 6.4V, Max is 8.2V
				// Pick: MIN @ 7V, MAX @ 8V
				uint32_t minVH = (uint32_t)(((70 - CUTOFF_VOLTAGE_UPPER_HYST) * chartHeight) / (BATTERY_MAX_VOLTAGE - CUTOFF_VOLTAGE_UPPER_HYST));
				uint32_t maxVH = (uint32_t)(((80 - CUTOFF_VOLTAGE_UPPER_HYST) * chartHeight) / (BATTERY_MAX_VOLTAGE - CUTOFF_VOLTAGE_UPPER_HYST));

				renderArrowOnly = false;

				// Redraw chart's axes, ticks and so on
				if (forceRedraw)
				{
					// Clear whole drawing region
					ucFillRect(0, 14, DISPLAY_SIZE_X, DISPLAY_SIZE_Y - 14, true);

					// 2 axis chart
					ucDrawFastVLine(chartX - 3, chartY - 2, chartHeight + 2 + 3, true);
					ucDrawFastVLine(chartX - 2, chartY - 2, chartHeight + 2 + 2, true);
					ucDrawFastHLine(chartX - 3, chartY + chartHeight + 2, CHART_WIDTH + 3 + 3, true);
					ucDrawFastHLine(chartX - 2, chartY + chartHeight + 1, CHART_WIDTH + 3 + 2, true);

					// Min/Max Voltage ticks and values
					ucDrawFastHLine(chartX - 6, (chartY + chartHeight) - minVH, 3, true);
					ucPrintAt(chartX - 3 - 12 - 3, ((chartY + chartHeight) - minVH) - 3, "7V", FONT_SIZE_1);
					ucDrawFastHLine(chartX - 6, (chartY + chartHeight) - maxVH, 3, true);
					ucPrintAt(chartX - 3 - 12 - 3, ((chartY + chartHeight) - maxVH) - 3, "8V", FONT_SIZE_1);

					// Time ticks
					for (uint8_t i = 0; i < CHART_WIDTH + 2; i += 22 /* ~ 15 minutes */)
					{
						ucSetPixel(chartX + i, (chartY + chartHeight) + 3, true);
					}
				}
				else
				{
					ucFillRect(chartX, chartY, CHART_WIDTH, chartHeight, true);
				}

				// Draw chart values, according to style
				for (size_t i = 0; i < histLen; i++)
				{
					uint32_t y = (uint32_t)(((hist[i] - CUTOFF_VOLTAGE_UPPER_HYST) * chartHeight) / (BATTERY_MAX_VOLTAGE - CUTOFF_VOLTAGE_UPPER_HYST));

					if (graphStyle == GRAPH_FILL)
					{
						ucDrawFastVLine(chartX + i, ((chartY + chartHeight) - y), y, true);
					}
					else
					{
						ucSetPixel(chartX + i, ((chartY + chartHeight) - y), true);
					}
				}

				// Min/Max dot lines
				for (uint8_t i = 0; i < CHART_WIDTH + 2; i++)
				{
					ucSetPixel(chartX + i, ((chartY + chartHeight) - minVH), (i % 2) ? false : true);
					ucSetPixel(chartX + i, ((chartY + chartHeight) - maxVH), (i % 2) ? false : true);
				}
			}

			// Upwards blinking arrow
			ucFillTriangle(63, (DISPLAY_SIZE_Y - 5), 59, (DISPLAY_SIZE_Y - 1), 67, (DISPLAY_SIZE_Y - 1), blink);

			if (voicePromptsIsPlaying() == false)
			{
				updateVoicePrompts(false);
			}
		}
		break;

		case RADIO_INFOS_TEMPERATURE_LEVEL:
		{
			int temperature = getTemperature();

			if ((prevTemperature != temperature) || (temperature > TEMPERATURE_CRITICAL) || forceRedraw)
			{
				char buffer[17];
				const int x = 102;
#if defined(PLATFORM_RD5R)
				const int temperatureHeight = (DISPLAY_SIZE_Y - 34);
				const int tankVCenter = (DISPLAY_SIZE_Y - 10);
				const int tankRadius = 9;
#else
				const int temperatureHeight = (DISPLAY_SIZE_Y - 40);
				const int tankVCenter = (DISPLAY_SIZE_Y - 14);
				const int tankRadius = 11;
#endif

				prevTemperature = temperature;
				renderArrowOnly = false;

				if (forceRedraw)
				{
					// Clear whole drawing region
					ucFillRect(0, 14, DISPLAY_SIZE_X, DISPLAY_SIZE_Y - 14, true);

					// Body frame
					ucDrawCircleHelper(x, 20 + 2, 7, (1 | 2), true);
					ucDrawCircleHelper(x, 20 + 2, 6, (1 | 2), true);
					ucSetPixel(x, (20 + 2) - 7, true);
					ucDrawFastVLine(x - 7, 20 + 2, temperatureHeight - 1, true);
					ucDrawFastVLine(x - 6, 20 + 2, temperatureHeight - 2, true);
					ucDrawFastVLine(x + 6, 20 + 2, temperatureHeight - 2, true);
					ucDrawFastVLine(x + 7, 20 + 2, temperatureHeight - 1, true);

					ucDrawCircle(x, tankVCenter, tankRadius, true);
					ucDrawCircle(x, tankVCenter, tankRadius - 1, true);
					ucFillCircle(x, tankVCenter, tankRadius - 3, ((temperature > TEMPERATURE_CRITICAL) ? !blink : true));
					ucFillRect(x - 5, 20, 11, temperatureHeight, true);

					// H lines, min/max markers
					ucDrawFastHLine(x - (7 + 5), 20, 5, true); // MAX: 70°C
					ucDrawFastHLine(x - (7 + 5), (temperatureHeight + 20) - 1, 5, true); // MIN: 10°C
				}
				else
				{
					// Clear temperature text area
					ucFillRect((((x - (7 + 5)) - (7 * 8)) >> 1), 20, (7 * 8), (DISPLAY_SIZE_Y - 20) - 4, true);

					// Clear thermo area
					ucFillCircle(x, tankVCenter, tankRadius - 3, ((temperature > TEMPERATURE_CRITICAL) ? !blink : true));
					ucFillRect(x - 4, 20, 9, temperatureHeight, true);
				}

				snprintf(buffer, 17, "%3d.%1d%s", (temperature / 10), abs(temperature % 10), currentLanguage->celcius);
				ucPrintAt((((x - (7 + 5)) - (7 * 8)) >> 1), (((DISPLAY_SIZE_Y - (14 + FONT_SIZE_3_HEIGHT)) >> 1) + 14), buffer, FONT_SIZE_3);

				uint32_t t = (uint32_t)((((CLAMP(temperature, 100, 700)) - 100) * temperatureHeight) / (700 - 100)); // clamp to 10..70 °C, then scale

				// Draw Level
				if (t)
				{
					ucFillRect(x - 4, 20 + temperatureHeight - t , 9, t, (temperature > TEMPERATURE_CRITICAL) ? blink : false);
				}

				if (voicePromptsIsPlaying() == false)
				{
					updateVoicePrompts(false);
				}
			}

			// Up/Down blinking arrow
			ucFillTriangle(63, (DISPLAY_SIZE_Y - 1), 59, (DISPLAY_SIZE_Y - 3), 67, (DISPLAY_SIZE_Y - 3), blink);
			ucFillTriangle(63, (DISPLAY_SIZE_Y - 5), 59, (DISPLAY_SIZE_Y - 3), 67, (DISPLAY_SIZE_Y - 3), blink);
		}
		break;
	}

	blink = !blink;

	ucRenderRows((renderArrowOnly ? (DISPLAY_NUMBER_OF_ROWS - 1) : 1), DISPLAY_NUMBER_OF_ROWS);
}

static void handleEvent(uiEvent_t *ev)
{
	if (ev->events & FUNCTION_EVENT)
	{
		if (QUICKKEY_TYPE(ev->function) == QUICKKEY_MENU)
		{
			displayMode = QUICKKEY_ENTRYID(ev->function);
			updateScreen(true);
			updateVoicePrompts(true);
			return;
		}
	}

	if (ev->events & BUTTON_EVENT)
	{
		if (repeatVoicePromptOnSK1(ev))
		{
			return;
		}
	}

	if (KEYCHECK_SHORTUP(ev->keys, KEY_RED) || KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
	{
		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_PRESS(ev->keys, KEY_DOWN))
	{
		if (displayMode < RADIO_INFOS_BATTERY_GRAPH)
		{
			displayMode++;
			updateScreen(true);
			updateVoicePrompts(true);
		}
	}
	else if (KEYCHECK_PRESS(ev->keys, KEY_UP))
	{
		if (displayMode > RADIO_INFOS_BATTERY_LEVEL)
		{
			displayMode--;
			updateScreen(true);
			updateVoicePrompts(true);
		}
	}
	else if (KEYCHECK_PRESS(ev->keys, KEY_LEFT))
	{
		if (displayMode == RADIO_INFOS_BATTERY_GRAPH)
		{
			if (graphStyle == GRAPH_LINE)
			{
				graphStyle = GRAPH_FILL;
				updateScreen(true);
			}
		}
	}
	else if (KEYCHECK_PRESS(ev->keys, KEY_RIGHT))
	{
		if (displayMode == RADIO_INFOS_BATTERY_GRAPH)
		{
			if (graphStyle == GRAPH_FILL)
			{
				graphStyle = GRAPH_LINE;
				updateScreen(true);
			}
		}
	}

	if (KEYCHECK_SHORTUP_NUMBER(ev->keys) && (BUTTONCHECK_DOWN(ev, BUTTON_SK2)))
	{
		saveQuickkeyMenuIndex(ev->keys.key, menuSystemGetCurrentMenuNumber(), displayMode, 0);
		return;
	}
}

void menuRadioInfosInit(void)
{
	battSemaphore = xSemaphoreCreateMutex();

	if (battSemaphore == NULL)
	{
		while(true); // Something better maybe ?
	}

	circularBufferInit(&batteryVoltageHistory);
}

// called every 2000 ticks
void menuRadioInfosPushBackVoltage(int32_t voltage)
{
	// Store value each 40k ticks
	if ((battery_stack_iter == 0) || (battery_stack_iter > BATTERY_ITER_PUSHBACK))
	{
		if (xSemaphoreTake(battSemaphore, (TickType_t)10) == pdTRUE)
		{
			circularBufferPushBack(&batteryVoltageHistory, voltage);
			xSemaphoreGive(battSemaphore);
		}

		battery_stack_iter = 0;
	}

	battery_stack_iter++;
}

static void updateVoicePrompts(bool spellIt)
{
	if (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1)
	{
		char buffer[17];

		voicePromptsInit();
		switch (displayMode)
		{
			case RADIO_INFOS_BATTERY_LEVEL:
			case RADIO_INFOS_BATTERY_GRAPH:
			{
				int volts, mvolts;

				voicePromptsAppendLanguageString(&currentLanguage->battery);
				getBatteryVoltage(&volts,  &mvolts);
				snprintf(buffer, 17, " %1d.%1d", volts, mvolts);
				voicePromptsAppendString(buffer);
				voicePromptsAppendPrompt(PROMPT_VOLTS);
			}
			break;
			case RADIO_INFOS_TEMPERATURE_LEVEL:
			{
				int temperature = getTemperature();

				voicePromptsAppendLanguageString(&currentLanguage->temperature);
				snprintf(buffer, 17, "%d.%1d", (temperature / 10), (temperature % 10));
				voicePromptsAppendString(buffer);
				voicePromptsAppendLanguageString(&currentLanguage->celcius);
			}
			break;
		}

		if (spellIt)
		{
			promptsPlayNotAfterTx();
		}
	}

}
