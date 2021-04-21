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

#include "functions/codeplug.h"
#include "main.h"
#include "functions/settings.h"
#include "functions/ticks.h"
#include "user_interface/menuSystem.h"
#include "user_interface/uiUtilities.h"
#include "user_interface/uiLocalisation.h"
#include "functions/voicePrompts.h"
#include "interfaces/clockManager.h"
#include "functions/rxPowerSaving.h"
#include "interfaces/wdog.h"

#if defined(USING_EXTERNAL_DEBUGGER)
#include "SeggerRTT/RTT/SEGGER_RTT.h"
#endif

#ifdef NDEBUG
#error A firmware compiled in Release mode will not work, yet
#error Change target build to Debug then Clean the build and recompile
#endif

//#define READ_CPUID

void mainTask(void *data);
#if defined(READ_CPUID)
void debugReadCPUID(void);
#endif

TaskHandle_t mainTaskHandle;

static uint32_t lowbatteryTimer = 0;
static const int LOW_BATTERY_INTERVAL = ((1000 * 60) * 5); // 5 minute;
static const int LOW_BATTERY_WARNING_VOLTAGE_DIFFERENTIAL = 6;	// Offset between the minimum voltage and when the battery warning audio starts. 6 = 0.6V
static bool updateMessageOnScreen = false;

void mainTaskInit(void)
{
	xTaskCreate(mainTask,                    /* pointer to the task */
			"fw main task",                      /* task name for kernel awareness debugging */
			5000L / sizeof(portSTACK_TYPE),      /* task stack size */
			NULL,                      			 /* optional task startup argument */
			6U,                                  /* initial priority */
			mainTaskHandle					 /* optional task handle to create */
	);

	vTaskStartScheduler();
}

static void die(bool usbMonitoring)
{
#if !defined(PLATFORM_RD5R)
	GPIO_PinWrite(GPIO_Keep_Power_On, Pin_Keep_Power_On, 0);// This is normally already done before this function is called.
	// But do it again, just in case, as its important that the radio will turn off when the power control is turned to off
#endif

	trxPowerUpDownRxAndC6000(false, true);

	if (usbMonitoring)
	{
		while(1U)
		{
			tick_com_request();
		}
	}
	else
	{
		watchdogDeinit();
		clockManagerSetRunMode(kAPP_PowerModeVlpr);
		while(true);
	}
}

void powerOffFinalStage(void)
{
	uint32_t m;

	// If TXing, get back to RX (this function can be called on low battery event).
	if (trxTransmissionEnabled)
	{
		trxTransmissionEnabled = false;
		trxActivateRx();
		trxIsTransmitting = false;
		LEDs_PinWrite(GPIO_LEDred, Pin_LEDred, 0);
	}

	// Restore DMR filter settings if the radio is turned off whilst in monitor mode
	if (monitorModeData.isEnabled && (monitorModeData.savedRadioMode == RADIO_MODE_DIGITAL))
	{
		nonVolatileSettings.dmrCcTsFilter = monitorModeData.savedDMRCcTsFilter;
		nonVolatileSettings.dmrDestinationFilter = monitorModeData.savedDMRDestinationFilter;
	}

	// If user was in a private call when they turned the radio off we need to restore the last Tg prior to stating the Private call.
	// to the nonVolatile Setting overrideTG, otherwise when the radio is turned on again it be in PC mode to that station.
	if ((trxTalkGroupOrPcId >> 24) == PC_CALL_FLAG)
	{
		settingsSet(nonVolatileSettings.overrideTG, uiDataGlobal.tgBeforePcMode);
	}

	menuHotspotRestoreSettings();

	m = fw_millis();
	settingsSaveSettings(true);

	// Give it a bit of time before pulling the plug as DM-1801 EEPROM looks slower
	// than GD-77 to write, then quickly power cycling triggers settings reset.
	while (1U)
	{
		if ((fw_millis() - m) > 50)
		{
			break;
		}
	}

	nonVolatileSettings.displayBacklightPercentageOff = 0;
	displayEnableBacklight(false);

#if !defined(PLATFORM_RD5R)
	// This turns the power off to the CPU.
	GPIO_PinWrite(GPIO_Keep_Power_On, Pin_Keep_Power_On, 0);
#endif

	die(false);
}

static void showErrorMessage(const char *message)
{
	ucClearBuf();
	ucPrintCentered(((DISPLAY_SIZE_Y - FONT_SIZE_3_HEIGHT) >> 1), message, FONT_SIZE_3);
	ucRender();
}

static void showLowBattery(void)
{
	showErrorMessage(currentLanguage->low_battery);
}

static void keyBeepHandler(uiEvent_t *ev, bool PTTToggledDown)
{
	bool isScanning = uiVFOModeIsScanning() || uiChannelModeIsScanning();

	// Do not send any beep while scanning, otherwise enabling the AMP will be handled as a valid signal detection.
	if (((ev->keys.event & (KEY_MOD_LONG | KEY_MOD_PRESS)) == (KEY_MOD_LONG | KEY_MOD_PRESS)) ||
			((ev->keys.event & KEY_MOD_UP) == KEY_MOD_UP))
	{

		if ((PTTToggledDown == false) && (isScanning == false))
		{
			if (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_BEEP)
			{
				soundSetMelody(nextKeyBeepMelody);
			}
			else
			{
				soundSetMelody(MELODY_KEY_BEEP);
			}
			nextKeyBeepMelody = (int *)MELODY_KEY_BEEP;// set back to the default beep
		}
		else
		{ 	// Reset the beep sound if we are scanning, otherwise the AudioAssist
			// beep will be played instead of the normal one.

			if (isScanning)
			{
				if (melody_play != NULL)
				{
					soundStopMelody();
				}
			}
			else
			{
				soundSetMelody(MELODY_KEY_BEEP);
			}
		}
	}
	else
	{
		if ((ev->keys.event & (KEY_MOD_LONG | KEY_MOD_DOWN)) == (KEY_MOD_LONG | KEY_MOD_DOWN))
		{
			if ((PTTToggledDown == false) && (isScanning == false))
			{
				soundSetMelody(MELODY_KEY_LONG_BEEP);
			}
		}
	}
}

#if !defined(PLATFORM_GD77S)
static bool validateUpdateCallback(void)
{
	if (uiDataGlobal.MessageBox.keyPressed == KEY_GREEN)
	{
		updateMessageOnScreen = false;
		settingsSetOptionBit(BIT_SETTINGS_UPDATED, false);
		return true;
	}

	return false;
}

static void settingsUpdateAudioAlert(void)
{
	if (nonVolatileSettings.audioPromptMode >= AUDIO_PROMPT_MODE_VOICE_LEVEL_1)
	{
		voicePromptsInit();
		voicePromptsAppendPrompt(PROMPT_SETTINGS_UPDATE);
		voicePromptsPlay();
	}
	else
	{
		soundSetMelody(MELODY_ACK_BEEP);
	}
}
#endif

void mainTask(void *data)
{
	keyboardCode_t keys;
	int key_event;
	int keyFunction;
	uint32_t buttons;
	int button_event;
	uint32_t rotary;
	int rotary_event;
	int function_event;
	uiEvent_t ev = { .buttons = 0, .keys = NO_KEYCODE, .rotary = 0, .function = 0, .events = NO_EVENT, .hasEvent = false, .time = 0 };
	bool keyOrButtonChanged = false;
	bool wasRestoringDefaultsettings = false;
	int *quickkeyPushedMenuMelody = NULL;

	clockManagerInit();
	// Init SPI
	SPIInit();

	// Init I2S
	init_I2S();
	setup_I2S();
	// Init ADC
	adcInit();

	// Init DAC
	dac_init();

	// Init I2C
	I2C0aInit();
	gpioInitCommon();
	buttonsInit();
	LEDsInit();
	keyboardInit();
	rotarySwitchInit();

	buttonsCheckButtonsEvent(&buttons, &button_event, false);// Read button state and event

	if (buttons & BUTTON_SK2)
	{
		wasRestoringDefaultsettings = true;
		settingsRestoreDefaultSettings();
		settingsLoadSettings();
	}
	else
	{
		wasRestoringDefaultsettings = settingsLoadSettings();
	}
	gpioInitDisplay();
	displayInit(settingsIsOptionBitSet(BIT_INVERSE_VIDEO));


	// We shouldn't go further if calibration related initialization has failed
	if ((SPI_Flash_init() == false) || (calibrationInit() == false) || (calibrationCheckAndCopyToCommonLocation(false) == false))
	{
		showErrorMessage("CAL DATA ERROR");
		USB_DeviceApplicationInit();
		die(true);
	}

	// Init AT1846S
	AT1846Init();

	// Init HR-C6000
	HRC6000_init();
	AT1846Postinit();

	// Init HR-C6000 interrupts
	init_HR_C6000_interrupts();

	// VOX init
	voxInit();

	// Small startup delay after initialization to stabilize system
	//  vTaskDelay(portTICK_PERIOD_MS * 500);

	init_pit();

	// Wait up to 100mS. For the voltage to stabilise, especially on the RD5R and DM1801.
	int batteryLowRetries = 100;
	while((batteryLowRetries-- > 0) && (adcGetBatteryVoltage() < CUTOFF_VOLTAGE_UPPER_HYST))
	{
		adcTriggerConversion(1);
		vTaskDelay(portTICK_PERIOD_MS * 1);
	}

	if (adcGetBatteryVoltage() < CUTOFF_VOLTAGE_UPPER_HYST)
	{
		showLowBattery();
#if !defined(PLATFORM_RD5R)
		GPIO_PinWrite(GPIO_Keep_Power_On, Pin_Keep_Power_On, 0);
#endif
		die(false);
	}

	init_hrc6000_task();

	menuRadioInfosInit(); // Initialize circular buffer
	watchdogInit(menuRadioInfosPushBackVoltage);

	soundInitBeepTask();

#if defined(USING_EXTERNAL_DEBUGGER)
	SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_TRIM);
	SEGGER_RTT_printf(0,"Segger RTT initialised\n");
	SEGGER_RTT_printf(0,"Core Clock = %dHz \n", CLOCK_GetFreq(kCLOCK_CoreSysClk));
#endif

	// Clear boot melody and image
#if defined(PLATFORM_GD77S)
	if ((buttons & (BUTTON_SK2 | BUTTON_ORANGE)) == ((BUTTON_SK2 | BUTTON_ORANGE)))
#else
	if ((buttons & BUTTON_SK2) && ((keyboardRead() & (SCAN_UP | SCAN_DOWN)) == (SCAN_UP | SCAN_DOWN)))
#endif
	{
		settingsEraseCustomContent();
	}

	lastheardInitList();
	codeplugInitCaches();
	dmrIDCacheInit();
	voicePromptsCacheInit();

	if (wasRestoringDefaultsettings)
	{
		enableVoicePromptsIfLoaded();
	}

	// Need to take care if the user has already been fully notified about the settings update
	wasRestoringDefaultsettings = settingsIsOptionBitSet(BIT_SETTINGS_UPDATED);

	// Should be initialized before the splash screen, as we don't want melodies when VOX is enabled
	voxSetParameters(nonVolatileSettings.voxThreshold, nonVolatileSettings.voxTailUnits);

	// Get DTMF contacts duration settings
	codeplugGetSignallingDTMFDurations(&uiDataGlobal.DTMFContactList.durations);


#if defined(PLATFORM_GD77S)
	// Those act as a toggles

	// Band limits
	if ((buttons & (BUTTON_SK1 | BUTTON_PTT)) == (BUTTON_SK1 | BUTTON_PTT))
	{
		if (nonVolatileSettings.txFreqLimited != BAND_LIMITS_ON_LEGACY_DEFAULT)
		{
			settingsSet(nonVolatileSettings.txFreqLimited, BAND_LIMITS_ON_LEGACY_DEFAULT);
		}
		else
		{
			settingsSet(nonVolatileSettings.txFreqLimited, BAND_LIMITS_NONE);
		}

		voicePromptsInit();
		voicePromptsAppendLanguageString(&currentLanguage->band_limits);
		voicePromptsAppendLanguageString(nonVolatileSettings.txFreqLimited == BAND_LIMITS_ON_LEGACY_DEFAULT ? &currentLanguage->on : &currentLanguage->off);
		voicePromptsPlay();
	}
	// Hotspot mode
	else if ((buttons & BUTTON_SK1) == BUTTON_SK1)
	{
		settingsSet(nonVolatileSettings.hotspotType, (uint8_t) ((nonVolatileSettings.hotspotType == HOTSPOT_TYPE_MMDVM) ? HOTSPOT_TYPE_BLUEDV : HOTSPOT_TYPE_MMDVM));

		voicePromptsInit();
		voicePromptsAppendLanguageString(&currentLanguage->hotspot_mode);
		voicePromptsAppendString((nonVolatileSettings.hotspotType == HOTSPOT_TYPE_MMDVM) ? "MMDVM" : "BlueDV");
		voicePromptsPlay();
	}
#endif

	menuSystemInit();

	// Now that init is complete, change back to Run mode before initialisating the USB.
	// As at the moment we don't have a way to change clock rates and maintain the USB connection
	clockManagerSetRunMode(kAPP_PowerModeRun);
	USB_DeviceApplicationInit();

	// Reset buttons/key states in case some where pressed while booting.
	button_event = EVENT_BUTTON_NONE;
	buttons = BUTTON_NONE;
	key_event = EVENT_KEY_NONE;
	keys.event = 0;
	keys.key = 0;

	lowbatteryTimer = fw_millis() + 5000;// Check battery 5 seconds after the firmware starts

	while (1U)
	{
		if (timer_maintask == 0)
		{
			timer_maintask = 10;
			taskENTER_CRITICAL();
			alive_maintask = true;
			taskEXIT_CRITICAL();

			tick_com_request();

			handleTimerCallbacks();

			keyboardCheckKeyEvent(&keys, &key_event); // Read keyboard state and event

			buttonsCheckButtonsEvent(&buttons, &button_event, (keys.key != 0)); // Read button state and event

			rotarySwitchCheckRotaryEvent(&rotary, &rotary_event); // Rotary switch state and event (GD-77S only)


#if !defined(PLATFORM_GD77S)
			if (wasRestoringDefaultsettings && (menuSystemGetRootMenuNumber() != UI_SPLASH_SCREEN))
			{
				wasRestoringDefaultsettings = false;
				updateMessageOnScreen = true;

				snprintf(uiDataGlobal.MessageBox.message, MESSAGEBOX_MESSAGE_LEN_MAX, "%s", "Settings\nupdate");
				uiDataGlobal.MessageBox.type = MESSAGEBOX_TYPE_INFO;
				uiDataGlobal.MessageBox.decoration = MESSAGEBOX_DECORATION_FRAME;
				uiDataGlobal.MessageBox.buttons = MESSAGEBOX_BUTTONS_OK;
				uiDataGlobal.MessageBox.validatorCallback = validateUpdateCallback;
				menuSystemPushNewMenu(UI_MESSAGE_BOX);

				addTimerCallback(settingsUpdateAudioAlert, 500, false);// Need to delay playing this for a while, because otherwise it may get played before the volume is turned up enough to hear it.
			}
#endif

			// VOX Checking
			if (voxIsEnabled())
			{
				// if a key/button event happen, reset the VOX.
				if ((key_event == EVENT_KEY_CHANGE) || (button_event == EVENT_BUTTON_CHANGE) || (keys.key != 0) || (buttons != BUTTON_NONE))
				{
					voxReset();
				}
				else
				{
					if (!trxTransmissionEnabled && voxIsTriggered() && ((buttons & BUTTON_PTT) == 0))
					{
						button_event = EVENT_BUTTON_CHANGE;
						buttons |= BUTTON_PTT;
					}
					else if (trxTransmissionEnabled && ((voxIsTriggered() == false) || (keys.event & KEY_MOD_PRESS)))
					{
						button_event = EVENT_BUTTON_CHANGE;
						buttons &= ~BUTTON_PTT;
					}
					else if (trxTransmissionEnabled && voxIsTriggered())
					{
						// Any key/button event reset the vox
						if ((button_event != EVENT_BUTTON_NONE) || (keys.event != EVENT_KEY_NONE))
						{
							voxReset();
							button_event = EVENT_BUTTON_CHANGE;
							buttons &= ~BUTTON_PTT;
						}
						else
						{
							buttons |= BUTTON_PTT;
						}
					}
				}
			}


			// If the settings update message is still on screen, don't permit to start xmitting.
			if (updateMessageOnScreen && (buttons & BUTTON_PTT))
			{
				button_event = EVENT_BUTTON_CHANGE;
				buttons &= ~BUTTON_PTT;
			}

			// EVENT_*_CHANGED can be cleared later, so check this now as hasEvent has to be set anyway.
			keyOrButtonChanged = ((key_event != NO_EVENT) || (button_event != NO_EVENT) || (rotary_event != NO_EVENT));

			if (headerRowIsDirty == true)
			{
				int currentMenu = menuSystemGetCurrentMenuNumber();

				if ((currentMenu == UI_CHANNEL_MODE) || (currentMenu == UI_VFO_MODE))
				{
#if defined(PLATFORM_RD5R)
					ucFillRect(0, 0, 128, 8, true);
#else
					ucClearRows(0, 2, false);
#endif
					uiUtilityRenderHeader(uiVFOModeDualWatchIsScanning());
					ucRenderRows(0, 2);
				}

				headerRowIsDirty = false;
			}

			if (keypadLocked || PTTLocked)
			{
				if (keypadLocked && ((buttons & BUTTON_PTT) == 0))
				{
					if (key_event == EVENT_KEY_CHANGE)
					{
						bool continueToFilterKeys = true;

						// A key is pressed, but a message box is currently displayed (probably a private call notification)
						if (menuSystemGetCurrentMenuNumber() == UI_MESSAGE_BOX)
						{
							// Clear any key but RED and GREEN
							if ((keys.key == KEY_RED) || (keys.key == KEY_GREEN))
							{
								continueToFilterKeys = false;
							}
						}

						if (continueToFilterKeys)
						{
							if ((PTTToggledDown == false) && (menuSystemGetCurrentMenuNumber() != UI_LOCK_SCREEN))
							{
								menuSystemPushNewMenu(UI_LOCK_SCREEN);
							}

							key_event = EVENT_KEY_NONE;
						}

						if ((nonVolatileSettings.bitfieldOptions & BIT_PTT_LATCH) && PTTToggledDown)
						{
							PTTToggledDown = false;
						}
					}

					// Lockout ORANGE AND BLUE (BLACK stay active regardless lock status, useful to trigger backlight)
#if defined(PLATFORM_RD5R)
					if ((button_event == EVENT_BUTTON_CHANGE) && (buttons & BUTTON_SK2))
#else
					if ((button_event == EVENT_BUTTON_CHANGE) && ((buttons & BUTTON_ORANGE) || (buttons & BUTTON_SK2)))
#endif
					{
						if ((PTTToggledDown == false) && (menuSystemGetCurrentMenuNumber() != UI_LOCK_SCREEN))
						{
							menuSystemPushNewMenu(UI_LOCK_SCREEN);
						}

						button_event = EVENT_BUTTON_NONE;

						if ((nonVolatileSettings.bitfieldOptions & BIT_PTT_LATCH) && PTTToggledDown)
						{
							PTTToggledDown = false;
						}
					}
				}
				else if (PTTLocked)
				{
					if ((buttons & BUTTON_PTT) && (button_event == EVENT_BUTTON_CHANGE))
					{
						// PTT button is pressed, but a message box is currently displayed, and PC allowance is set to PTT,
						// hence it's probably a private call accept, so let the PTT button being handled later in the code
						if (((menuSystemGetCurrentMenuNumber() == UI_MESSAGE_BOX) && (nonVolatileSettings.privateCalls == ALLOW_PRIVATE_CALLS_PTT)) == false)
						{
							soundSetMelody(MELODY_ERROR_BEEP);

							if (menuSystemGetCurrentMenuNumber() != UI_LOCK_SCREEN)
							{
								menuSystemPushNewMenu(UI_LOCK_SCREEN);
							}

							button_event = EVENT_BUTTON_NONE;
							// Clear PTT button
							buttons &= ~BUTTON_PTT;
						}
					}
					else if ((buttons & BUTTON_SK2) && KEYCHECK_DOWN(keys, KEY_STAR))
					{
						if (menuSystemGetCurrentMenuNumber() != UI_LOCK_SCREEN)
						{
							menuSystemPushNewMenu(UI_LOCK_SCREEN);
						}
					}
				}
			}

			int trxMode = trxGetMode();

#if ! defined(PLATFORM_GD77S)
			if ((key_event == EVENT_KEY_CHANGE) && ((buttons & BUTTON_PTT) == 0) && (keys.key != 0))
			{
				if (KEYCHECK_LONGDOWN(keys, KEY_RED) && (uiVFOModeIsScanning() == false) && (uiChannelModeIsScanning() == false))
				{
					uiDataGlobal.currentSelectedContactIndex = 0;
					menuSystemPopAllAndDisplayRootMenu();
				}
			}

			//
			// PTT toggle feature
			//
			// PTT is locked down, but any button, except SK1 or SK2(1750Hz in FM) or DTMF Key in Analog,
			// or Up/Down with LH on screen in Digital, is pressed, virtually release PTT
			if (((nonVolatileSettings.bitfieldOptions & BIT_PTT_LATCH) && PTTToggledDown) &&
					(((button_event & EVENT_BUTTON_CHANGE) && (
#if ! defined(PLATFORM_RD5R)
							(buttons & BUTTON_ORANGE) ||
#endif
							((trxMode != RADIO_MODE_ANALOG) && (buttons & BUTTON_SK2)))) ||
							((keys.key != 0) && (keys.event & KEY_MOD_UP) &&
									(((trxMode == RADIO_MODE_ANALOG) && keyboardKeyIsDTMFKey(keys.key)) == false) &&
									(((trxMode == RADIO_MODE_DIGITAL) && menuTxScreenDisplaysLastHeard() && ((keys.key == KEY_UP) || (keys.key == KEY_DOWN))) == false))))
			{
				PTTToggledDown = false;
				button_event = EVENT_BUTTON_CHANGE;
				buttons = BUTTON_NONE;
				key_event = NO_EVENT;
				keys.key = 0;
			}

			// PTT toggle action
			if (nonVolatileSettings.bitfieldOptions & BIT_PTT_LATCH)
			{
				if (button_event == EVENT_BUTTON_CHANGE)
				{
					if (buttons & BUTTON_PTT)
					{
						if (PTTToggledDown == false)
						{
							// PTT toggle works only if a TOT value is defined.
							if (currentChannelData->tot != 0)
							{
								PTTToggledDown = true;
							}
						}
						else
						{
							PTTToggledDown = false;
						}
					}
				}

				if (PTTToggledDown && ((buttons & BUTTON_PTT) == 0))
				{
					buttons |= BUTTON_PTT;
				}
			}
			else
			{
				if (PTTToggledDown)
				{
					PTTToggledDown = false;
				}
			}
#endif

			if (button_event == EVENT_BUTTON_CHANGE)
			{
				displayLightTrigger(true);
				if ((buttons & BUTTON_PTT) != 0)
				{
					int currentMenu = menuSystemGetCurrentMenuNumber();

					/*
					 * This code would prevent transmission on simplex if the radio is receiving a DMR signal.
					 * if ((slot_state == DMR_STATE_IDLE || trxDMRMode == DMR_MODE_PASSIVE)  &&
					 *
					 */
					if ((trxMode != RADIO_MODE_NONE) &&
							(settingsUsbMode != USB_MODE_HOTSPOT) &&
							(currentMenu != UI_POWER_OFF) &&
							(currentMenu != UI_SPLASH_SCREEN) &&
							(currentMenu != UI_TX_SCREEN))
					{
						bool wasScanning = false;

						if (uiDataGlobal.Scan.active || uiDataGlobal.Scan.toneActive)
						{
							if (currentMenu == UI_VFO_MODE)
							{
								uiVFOModeStopScanning();
							}
							else
							{
								uiChannelModeStopScanning();
							}
							wasScanning = true;
						}
						else
						{
							if (currentMenu == UI_LOCK_SCREEN)
							{
								menuLockScreenPop();
							}
						}

						currentMenu = menuSystemGetCurrentMenuNumber();

						if (wasScanning)
						{
							// Mode was blinking, hence it needs to be redrawn as it could be in its hidden phase.
							uiUtilityRedrawHeaderOnly(false);
						}
						else
						{
							if (((currentMenu == UI_MESSAGE_BOX) && (menuSystemGetPreviousMenuNumber() == UI_PRIVATE_CALL))
									&& (nonVolatileSettings.privateCalls == ALLOW_PRIVATE_CALLS_PTT))
							{
								acceptPrivateCall(uiDataGlobal.receivedPcId, uiDataGlobal.receivedPcTS);
								menuPrivateCallDismiss();
							}
							else if (((currentMenu == MENU_CONTACT_LIST_SUBMENU) || (currentMenu == MENU_CONTACT_QUICKLIST)) && dtmfSequenceIsKeying())
							{
								dtmfSequenceReset();
							}

							// Need to call menuSystemGetCurrentMenuNumber() again, as something has probably
							// changed since last above calls
							if (menuSystemGetCurrentMenuNumber() != UI_MESSAGE_BOX)
							{
#if defined(PLATFORM_GD77S)
								if (uiChannelModeTransmitDTMFContactForGD77S())
								{
									button_event = EVENT_BUTTON_NONE;
									buttons &= ~BUTTON_PTT;
								}
								else
								{
#endif

									rxPowerSavingSetState(ECOPHASE_POWERSAVE_INACTIVE);

									menuSystemPushNewMenu(UI_TX_SCREEN);
#if defined(PLATFORM_GD77S)
								}
#endif
							}
							else
							{
								button_event = EVENT_BUTTON_NONE;
								buttons &= ~BUTTON_PTT;
							}
						}
					}
				}

#if (! defined(PLATFORM_GD77S)) && (! defined(PLATFORM_RD5R))
				if ((buttons & (BUTTON_SK1 | BUTTON_ORANGE | BUTTON_ORANGE_EXTRA_LONG_DOWN)) == (BUTTON_SK1 | BUTTON_ORANGE | BUTTON_ORANGE_EXTRA_LONG_DOWN))
				{
					settingsSaveSettings(true);
					soundSetMelody(MELODY_ACK_BEEP);
				}
#endif

				// Toggle backlight
				if ((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_MANUAL) && (buttons & BUTTON_SK1))
				{
					displayEnableBacklight(! displayIsBacklightLit());
				}
			}

			if (!trxTransmissionEnabled && (updateLastHeard == true))
			{
				lastHeardListUpdate((uint8_t *)DMR_frame_buffer, false);
				updateLastHeard = false;
			}

			if ((nonVolatileSettings.hotspotType == HOTSPOT_TYPE_OFF) ||
					((nonVolatileSettings.hotspotType != HOTSPOT_TYPE_OFF) && (settingsUsbMode != USB_MODE_HOTSPOT))) // Do not filter anything in HS mode.
			{
				if ((uiDataGlobal.PrivateCall.state == PRIVATE_CALL_DECLINED) &&
						(slot_state == DMR_STATE_IDLE))
				{
					menuPrivateCallClear();
				}

				if ((trxTransmissionEnabled == false) && (trxIsTransmitting == false) &&
						(uiDataGlobal.displayQSOState == QSO_DISPLAY_CALLER_DATA) && (nonVolatileSettings.privateCalls > ALLOW_PRIVATE_CALLS_OFF))
				{
					if (HRC6000GetReceivedTgOrPcId() == (trxDMRID | (PC_CALL_FLAG << 24)))
					{
						int receivedSrcId = HRC6000GetReceivedSrcId();

						if ((uiDataGlobal.PrivateCall.state == PRIVATE_CALL_NOT_IN_CALL) &&
								(trxTalkGroupOrPcId != (receivedSrcId | (PC_CALL_FLAG << 24))) &&
								(receivedSrcId != uiDataGlobal.PrivateCall.lastID))
						{
							if ((receivedSrcId & 0xFFFFFF) >= 1000000)
							{
								menuSystemPushNewMenu(UI_PRIVATE_CALL);
							}
						}
					}
				}
			}

#if defined(PLATFORM_GD77S) && defined(READ_CPUID)
			if ((buttons & (BUTTON_SK1 | BUTTON_ORANGE | BUTTON_PTT)) == (BUTTON_SK1 | BUTTON_ORANGE | BUTTON_PTT))
			{
				debugReadCPUID();
			}
#endif

			ev.function = 0;
			function_event = NO_EVENT;
			keyFunction = NO_EVENT;
			int currentMenu = menuSystemGetCurrentMenuNumber();
			if (KEYCHECK_SHORTUP_NUMBER(keys) && (buttons & BUTTON_SK2) && ((currentMenu == UI_VFO_MODE) || (currentMenu == UI_CHANNEL_MODE)))
			{
				keyFunction = codeplugGetQuickkeyFunctionID(keys.key);
				int menuFunction = QUICKKEY_MENUID(keyFunction);

#if 0 // For demo screen
				if (keys.key == '0')
				{
						static uint8_t demo = 90;
						keyFunction = (UI_HOTSPOT_MODE << 8) | demo; // Hotspot demo mode (able to take screengrabs)

						if (++demo > 99)
						{
							demo = 90;
						}
				}
#endif

#if defined(PLATFORM_RD5R)
				if (keys.key == '5')
				{
					menuFunction = 0;
					keyFunction = FUNC_TOGGLE_TORCH;
					keyboardReset();
				}
				else
				{
#endif
					if ((keyFunction != 0) && (currentMenu == UI_CHANNEL_MODE || currentMenu == UI_VFO_MODE || currentMenu == menuFunction))
					{
						if (QUICKKEY_TYPE(keyFunction) == QUICKKEY_MENU)
						{
							if ((menuFunction > 0) && (menuFunction < NUM_MENU_ENTRIES))
							{
								if (currentMenu != menuFunction)
								{
									menuSystemPushNewMenu(menuFunction);

									// Store the beep build by the new menu status. It will be restored after
									// the call of menuSystemCallCurrentMenuTick(), below
									quickkeyPushedMenuMelody = nextKeyBeepMelody;
								}
							}
							ev.function = keyFunction;
							buttons = BUTTON_NONE;
							rotary = 0;
							key_event = EVENT_KEY_NONE;
							button_event = EVENT_BUTTON_NONE;
							rotary_event = EVENT_ROTARY_NONE;
							keys.key = 0;
							keys.event = 0;
							function_event = FUNCTION_EVENT;
						}
						else if ((QUICKKEY_TYPE(keyFunction) == QUICKKEY_CONTACT) && (currentMenu != menuFunction))
						{
							int contactIndex = QUICKKEY_CONTACTVALUE(keyFunction);

							if ((contactIndex >= CODEPLUG_CONTACTS_MIN) && (contactIndex <= CODEPLUG_CONTACTS_MAX))
							{
								if (codeplugContactGetDataForIndex(contactIndex, &currentContactData))
								{
									// Use quickkey contact as overrides (contact and its TS, if any)
									menuPrivateCallClear();
									setOverrideTGorPC(currentContactData.tgNumber, (currentContactData.callType == CONTACT_CALLTYPE_PC));

									trxTalkGroupOrPcId = currentContactData.tgNumber;
									if (currentContactData.callType == CONTACT_CALLTYPE_PC)
									{
										trxTalkGroupOrPcId |= (PC_CALL_FLAG << 24);
									}

									// Contact has a TS override set
									if ((currentContactData.reserve1 & 0x01) == 0x00)
									{
										int ts = ((currentContactData.reserve1 & 0x02) >> 1);
										trxSetDMRTimeSlot(ts);
										tsSetManualOverride(((menuSystemGetRootMenuNumber() == UI_CHANNEL_MODE) ? CHANNEL_CHANNEL : (CHANNEL_VFO_A + nonVolatileSettings.currentVFONumber)), (ts + 1));
									}
									ev.function = FUNC_REDRAW;
									function_event = FUNCTION_EVENT;
								}
							}
						}
					}
					keyboardReset();
#if defined(PLATFORM_RD5R)
				}
#endif
			}
			ev.buttons = buttons;
			ev.keys = keys;
			ev.rotary = rotary;
			ev.events = function_event | (button_event << 1) | (rotary_event << 3) | key_event;
			ev.hasEvent = keyOrButtonChanged || function_event;
			ev.time = fw_millis();

			/*
			 * We probably can't terminate voice prompt playback in main, because some screens need to a follow-on playback if the prompt was playing when a button was pressed
			 *
			if ((nonVolatileSettings.audioPromptMode == AUDIO_PROMPT_MODE_SILENT || voicePromptIsActive)   && (ev.keys.event & KEY_MOD_DOWN))
			{
				voicePromptsTerminate();
			}
			*/
			//if (((ev.keys.key >='0' && ev.keys.key <='9') && (((ev.keys.event & (KEY_MOD_DOWN | KEY_MOD_LONG)) == (KEY_MOD_DOWN | KEY_MOD_LONG))) && (ev.buttons & BUTTON_SK2)))

			// Clear the Quickkey slot on SK2 + longdown 0..9 KEY
			if (KEYCHECK_LONGDOWN_NUMBER(ev.keys) && BUTTONCHECK_DOWN(&ev, BUTTON_SK2))
			{
				// Only allow quick keys to be cleared on the 2 main screens
				if (currentMenu == UI_CHANNEL_MODE || currentMenu == UI_VFO_MODE)
				{
					saveQuickkeyMenuLongValue(ev.keys.key, 0, 0);
					soundSetMelody(MELODY_QUICKKEYS_CLEAR_ACK_BEEP);
				}
				else
				{
					soundSetMelody(MELODY_NACK_BEEP);
				}

				// Reset keyboard and event, as this keyboard event HAVE to
				// be ignore by the current menu.
				keyboardReset();
				ev.buttons = BUTTON_NONE;
				ev.keys.event = 0;
				ev.keys.key = 0;
				ev.rotary = 0;
				ev.events = NO_EVENT;
				ev.hasEvent = false;
			}

			menuSystemCallCurrentMenuTick(&ev);

			// Restore the beep built when a menu was pushed by the quickkey above.
			if (quickkeyPushedMenuMelody)
			{
				nextKeyBeepMelody = quickkeyPushedMenuMelody;
				quickkeyPushedMenuMelody = NULL;
				ev.keys.event = KEY_MOD_UP; // Trick to force keyBeepHandler() to set that beep
			}

			// Beep sounds aren't allowed in these modes.
			if (((nonVolatileSettings.audioPromptMode == AUDIO_PROMPT_MODE_SILENT) || voicePromptsIsPlaying()) /*|| (nonVolatileSettings.audioPromptMode == AUDIO_PROMPT_MODE_VOICE)*/)
			{
				if (melody_play != NULL)
				{
					soundStopMelody();
				}
			}
			else
			{
				if ((menuSystemGetCurrentMenuNumber() != UI_SPLASH_SCREEN) &&
						((((key_event == EVENT_KEY_CHANGE) || (button_event == EVENT_BUTTON_CHANGE))
								&& ((buttons & BUTTON_PTT) == 0) && (ev.keys.key != 0))
								|| (function_event == FUNCTION_EVENT)))
				{
					keyBeepHandler(&ev, PTTToggledDown);
				}
			}

#if defined(PLATFORM_RD5R)
			if (keyFunction == FUNC_TOGGLE_TORCH)
			{
				torchToggle();
			}
#endif


#if defined(PLATFORM_GD77S)
			if (trxTransmissionEnabled == false &&
#else
			if ((menuSystemGetCurrentMenuNumber() != UI_TX_SCREEN) &&
#endif
					(batteryVoltage < (CUTOFF_VOLTAGE_LOWER_HYST + LOW_BATTERY_WARNING_VOLTAGE_DIFFERENTIAL)) &&
					(fw_millis() > lowbatteryTimer))
			{

				if (melody_play == NULL)
				{
					if (nonVolatileSettings.audioPromptMode < AUDIO_PROMPT_MODE_VOICE_LEVEL_1)
					{
						soundSetMelody(MELODY_LOW_BATTERY);
					}
					else
					{
						voicePromptsInit();
						voicePromptsAppendLanguageString(&currentLanguage->low_battery);
						voicePromptsPlay();
					}
					lowbatteryTimer = fw_millis() + LOW_BATTERY_INTERVAL;
				}
			}

			// Low battery or poweroff (non RD-5R)
#if defined(PLATFORM_RD5R)
			if ((batteryVoltage < CUTOFF_VOLTAGE_LOWER_HYST)
					&& (menuSystemGetCurrentMenuNumber() != UI_POWER_OFF))
#else
			if (((GPIO_PinRead(GPIO_Power_Switch, Pin_Power_Switch) != 0)
					|| (batteryVoltage < CUTOFF_VOLTAGE_LOWER_HYST))
					&& (menuSystemGetCurrentMenuNumber() != UI_POWER_OFF))
#endif
			{
				GPIO_PinWrite(GPIO_audio_amp_enable, Pin_audio_amp_enable, 0);
				soundSetMelody(NULL);

				if (batteryVoltage < CUTOFF_VOLTAGE_LOWER_HYST)
				{
					showLowBattery();
					powerOffFinalStage();
				}
#if ! defined(PLATFORM_RD5R)
				else
				{
					menuSystemPushNewMenu(UI_POWER_OFF);
				}
#endif // ! PLATFORM_RD5R
			}

			if (((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_AUTO)
					|| (nonVolatileSettings.backlightMode == BACKLIGHT_MODE_BUTTONS)
					|| (nonVolatileSettings.backlightMode == BACKLIGHT_MODE_SQUELCH)) && (menuDataGlobal.lightTimer > 0))
			{
				// Countdown only in (AUTO), (BUTTONS) or (SQUELCH + no audio)
				if ((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_AUTO) || (nonVolatileSettings.backlightMode == BACKLIGHT_MODE_BUTTONS) ||
						((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_SQUELCH) && ((getAudioAmpStatus() & AUDIO_AMP_MODE_RF) == 0)))
				{
					menuDataGlobal.lightTimer--;
				}

				if (menuDataGlobal.lightTimer == 0)
				{
					displayEnableBacklight(false);
				}
			}

			if (voicePromptsIsPlaying())
			{
				voicePromptsTick();
			}
			soundTickMelody();
			voxTick();

#if defined(PLATFORM_RD5R) // Needed for platforms which can't control the poweroff
			settingsSaveIfNeeded(false);
#endif
		}

		if (!trxTransmissionEnabled && !trxIsTransmitting)
		{

			if (txPAEnabled)
			{
#if defined(USING_EXTERNAL_DEBUGGER) && defined(DEBUG_DMR)
				SEGGER_RTT_printf(0, "ERROR: RADIO STILL TRANSMITTING !!!!\n");
#endif
				trxActivateRx();
			}
			else
			{
				bool hasSignal = false;
				if (rxPowerSavingIsRxOn())
				{
					switch(trxGetMode())
					{
						case RADIO_MODE_ANALOG:
							if (melody_play == NULL)
							{
								hasSignal = trxCheckAnalogSquelch();
							}
							break;
						case RADIO_MODE_DIGITAL:
							if (slot_state == DMR_STATE_IDLE)
							{
								hasSignal = trxCheckDigitalSquelch();
							}
							else
							{
								if (PITCounter > readDMRRSSI)
								{
									readDMRRSSI = PITCounter + 100000;// hold of for a very long time
									trxReadRSSIAndNoise();
								}
								hasSignal = true;
							}
							break;
						default: // RADIO_MODE_NONE
							break;
					}
				}

				rxPowerSavingTick(&ev,hasSignal);

			}
		}
		vTaskDelay(0);
	}
}

#if defined(READ_CPUID)
void debugReadCPUID(void)
{
	char tmp[6];
	char buf[512]={0};
	uint8_t *p = (uint8_t *)0x40048054;
	USB_DEBUG_PRINT("\nCPU ID\n");
	vTaskDelay(portTICK_PERIOD_MS * 1);
	for(int i = 0; i < 16; i++)
	{
		sprintf(tmp, "%02x ", *p);
		strcat(buf, tmp);
		p++;
	}
	USB_DEBUG_PRINT(buf);

	vTaskDelay(portTICK_PERIOD_MS * 1);
	USB_DEBUG_PRINT("\nProtection bytes\n");
	vTaskDelay(portTICK_PERIOD_MS * 1);

	buf[0] = 0;
#if defined(PLATFORM_DM1801)
	p = (uint8_t *)0x3800;
#else
	p = (uint8_t *)0x7f800;
#endif
	for(int i = 0; i < 36; i++)
	{
		sprintf(tmp, "%02x ", *p);
		strcat(buf, tmp);
		p++;
	}
	USB_DEBUG_PRINT(buf);
}
#endif
