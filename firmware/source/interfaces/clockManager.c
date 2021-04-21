/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <interfaces/clockManager.h>
#include "fsl_rcm.h"
#include "fsl_pmc.h"

#include "clock_config.h"
#include "interfaces/pit.h"
#include "fsl_tickless_generic.h"
#include "interfaces/hr-c6000_spi.h"
#include "interfaces/i2c.h"

#if defined(USING_EXTERNAL_DEBUGGER)
#include "SeggerRTT/RTT/SEGGER_RTT.h"
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*
 * Set the clock configuration for HSRUN mode.
 */
static void APP_SetClockHsrun(void);

/*
 * Set the clock configuration for RUN mode from HSRUN mode.
 */
static void APP_SetClockRunFromHsrun(void);

/*
 * Set the clock configuration for RUN mode from VLPR mode.
 */
static void APP_SetClockRunFromVlpr(void);

/*
 * Set the clock configuration for VLPR mode.
 */
static void APP_SetClockVlpr(void);

/*
 * Power mode switch callback.
 */
status_t beforeChangeCallback(notifier_notification_block_t *notify, void *dataPtr);


/*******************************************************************************
 * Configurations
 ******************************************************************************/

/*Power mode configurations*/
static power_user_config_t vlprConfig = {
    kAPP_PowerModeVlpr,
    true
};
static power_user_config_t runConfig   = {
		kAPP_PowerModeRun,
        true
    };
static power_user_config_t hsrunConfig = {
		kAPP_PowerModeHsrun,
        true
    };

/* Initializes array of pointers to power mode configurations */
static notifier_user_config_t *powerConfigs[] = {
	&vlprConfig,
    &runConfig,
	&hsrunConfig,
};

static notifier_handle_t powerModeHandle;
static user_callback_data_t callbackData0;

/* Initializes callback configuration structure */
static notifier_callback_config_t callbackCfg0 = { beforeChangeCallback, kNOTIFIER_CallbackBeforeAfter, (void *)&callbackData0 };

/* Storage of callback configurations (see clockManagerInit()) */
static notifier_callback_config_t callbacks[1];



app_power_mode_t clockManagerCurrentRunMode;


static void updateRTOSAndPitTimings(void)
{
    SystemCoreClock = CLOCK_GetFreq(kCLOCK_CoreSysClk);
	vPortSetupTimerInterrupt();
	PIT_DisableInterrupts(PIT, kPIT_Chnl_0, kPIT_TimerInterruptEnable);
	PIT_SetTimerPeriod(PIT, kPIT_Chnl_0, USEC_TO_COUNT(100U, CLOCK_GetFreq(kCLOCK_BusClk)));
	PIT_EnableInterrupts(PIT, kPIT_Chnl_0, kPIT_TimerInterruptEnable);

    SPI0Setup();
    SPI1Setup();
    I2C0Setup();
}


/*******************************************************************************
 * Code
 ******************************************************************************/

void APP_SetClockVlpr(void)
{
#if defined(USING_EXTERNAL_DEBUGGER)
	//    SEGGER_RTT_printf(0,"APP_SetClockRunFromVlpr\n");
#endif

	CLOCK_SetSimSafeDivs();
	CLOCK_SetInternalRefClkConfig(kMCG_IrclkEnable, kMCG_IrcFast, 0U);

	/* MCG works in PEE mode now, will switch to BLPI mode. */

	CLOCK_ExternalModeToFbeModeQuick();                     /* Enter FBE. */
	CLOCK_SetFbiMode(kMCG_Dmx32Default, kMCG_DrsLow, NULL); /* Enter FBI. */
	CLOCK_SetLowPowerEnable(true);                          /* Enter BLPI. */

	CLOCK_SetSimConfig(&simConfig_BOARD_BootClockVLPR);

	updateRTOSAndPitTimings();
}


void APP_SetClockRunFromVlpr(void)
{
#if defined(USING_EXTERNAL_DEBUGGER)
	//    SEGGER_RTT_printf(0,"APP_SetClockRunFromVlpr\n");
#endif

	CLOCK_SetSimSafeDivs();

	/* Currently in BLPI mode, will switch to PEE mode. */
	/* Enter FBI. */
	CLOCK_SetLowPowerEnable(false);
	/* Enter FBE. */
	CLOCK_SetFbeMode(3U, kMCG_Dmx32Default, kMCG_DrsLow, NULL);
	/* Enter PBE. */
	CLOCK_SetPbeMode(kMCG_PllClkSelPll0, &mcgConfig_BOARD_BootClockRUN.pll0Config);
	/* Enter PEE. */
	CLOCK_SetPeeMode();

	CLOCK_SetSimConfig(&simConfig_BOARD_BootClockRUN);
	updateRTOSAndPitTimings();
}


void APP_SetClockHsrun(void)
{
#if defined(USING_EXTERNAL_DEBUGGER)
	//    SEGGER_RTT_printf(0,"APP_SetClockHsrun\n");
#endif

	CLOCK_SetPbeMode(kMCG_PllClkSelPll0, &mcgConfig_BOARD_BootClockHSRUN.pll0Config);
	CLOCK_SetPeeMode();
	CLOCK_SetSimConfig(&simConfig_BOARD_BootClockHSRUN);

	updateRTOSAndPitTimings();
}

void APP_SetClockRunFromHsrun(void)
{
#if defined(USING_EXTERNAL_DEBUGGER)
	//    SEGGER_RTT_printf(0,"APP_SetClockRunFromHsrun\n");
#endif

	CLOCK_SetPbeMode(kMCG_PllClkSelPll0, &mcgConfig_BOARD_BootClockRUN.pll0Config);
	CLOCK_SetPeeMode();
	CLOCK_SetSimConfig(&simConfig_BOARD_BootClockRUN);

	updateRTOSAndPitTimings();
}


status_t beforeChangeCallback(notifier_notification_block_t *notify, void *dataPtr)
{
	user_callback_data_t *userData     = (user_callback_data_t *)dataPtr;
	status_t ret                       = kStatus_Fail;
	app_power_mode_t targetMode        = ((power_user_config_t *)notify->targetConfig)->mode;
	smc_power_state_t originPowerState = userData->originPowerState;
	smc_power_state_t powerState;

	switch (notify->notifyType)
	{
		case kNOTIFIER_NotifyBefore:
#if defined(USING_EXTERNAL_DEBUGGER)
			//            SEGGER_RTT_printf(0,"kNOTIFIER_NotifyBefore %d\n",userData->beforeNotificationCounter);
#endif
			userData->beforeNotificationCounter++;
			ret = kStatus_Success;
			break;
		case kNOTIFIER_NotifyRecover:
#if defined(USING_EXTERNAL_DEBUGGER)
			//            SEGGER_RTT_printf(0,"kNOTIFIER_NotifyRecover\n");
#endif
			break;
		case kNOTIFIER_CallbackAfter:
#if defined(USING_EXTERNAL_DEBUGGER)
			//            SEGGER_RTT_printf(0,"kNOTIFIER_CallbackAfter\n",userData->afterNotificationCounter);
#endif
			userData->afterNotificationCounter++;
			powerState = SMC_GetPowerModeState(SMC);

			/*
			 * For some other platforms, if enter LLS mode from VLPR mode, when wakeup, the
			 * power mode is VLPR. But for some platforms, if enter LLS mode from VLPR mode,
			 * when wakeup, the power mode is RUN. In this case, the clock setting is still
			 * VLPR mode setting, so change to RUN mode setting here.
			 */
			if ((kSMC_PowerStateVlpr == originPowerState) && (kSMC_PowerStateRun == powerState))
			{
				APP_SetClockRunFromVlpr();
			}

			/*
			 * If enter stop modes when MCG in PEE mode, then after wakeup, the MCG is in PBE mode,
			 * need to enter PEE mode manually.
			 */
			if ((kAPP_PowerModeRun != targetMode) &&  (kAPP_PowerModeVlpr != targetMode))
			{
				if (kSMC_PowerStateRun == originPowerState)
				{
					/* Wait for PLL lock. */
					while (!(kMCG_Pll0LockFlag & CLOCK_GetStatusFlags()))
					{
					}
					CLOCK_SetPeeMode();
				}
			}
			ret = kStatus_Success;
			break;
		default:
			break;
	}
	return ret;
}


/*! @brief Show current power mode. */
// Not currently used
void APP_ShowPowerMode(smc_power_state_t currentPowerState)
{
#if defined(USING_EXTERNAL_DEBUGGER)
	switch (currentPowerState)
	{
		case kSMC_PowerStateRun:
			SEGGER_RTT_printf(0,"    Power mode: RUN\n");
			break;
		case kSMC_PowerStateVlpr:
			SEGGER_RTT_printf(0,"    Power mode: VLPR\n");
			break;
		case kSMC_PowerStateHsrun:
			SEGGER_RTT_printf(0,"    Power mode: HSRUN\n");
			break;
		default:
			SEGGER_RTT_printf(0,"    Power mode wrong\n");
			break;
	}
#endif
}

/*!
 * @brief check whether could switch to target power mode from current mode.
 * Return true if could switch, return false if could not switch.
 */
bool APP_CheckPowerMode(smc_power_state_t currentPowerState, app_power_mode_t targetPowerMode)
{
	bool modeValid = true;

	/*
	 * Check wether the mode change is allowed.
	 *
	 * 1. If current mode is HSRUN mode, the target mode must be RUN mode.
	 * 2. If current mode is RUN mode, the target mode must not be VLPW mode.
	 * 3. If current mode is VLPR mode, the target mode must not be HSRUN/WAIT/STOP mode.
	 * 4. If already in the target mode, don't need to change.
	 */
	switch (currentPowerState)
	{
		case kSMC_PowerStateHsrun:
			if (kAPP_PowerModeRun != targetPowerMode)
			{
#if defined(USING_EXTERNAL_DEBUGGER)
				SEGGER_RTT_printf(0,"Current mode is HSRUN, please choose RUN mode as the target mode.\n");
#endif
				modeValid = false;
			}
			break;

		case kSMC_PowerStateRun:
			break;

		case kSMC_PowerStateVlpr:
			if ((kAPP_PowerModeHsrun == targetPowerMode))
			{
#if defined(USING_EXTERNAL_DEBUGGER)
				SEGGER_RTT_printf(0,"Could not enter HSRUN/STOP/WAIT modes from VLPR mode.\n");
#endif
				modeValid = false;
			}
			break;
		default:
#if defined(USING_EXTERNAL_DEBUGGER)
			SEGGER_RTT_printf(0,"Wrong power state.\n");
#endif
			modeValid = false;
			break;
	}

	if (!modeValid)
	{
		return false;
	}

	/* Don't need to change power mode if current mode is already the target mode. */
	if (((kAPP_PowerModeRun == targetPowerMode) && (kSMC_PowerStateRun == currentPowerState)) ||
			((kAPP_PowerModeHsrun == targetPowerMode) && (kSMC_PowerStateHsrun == currentPowerState)) ||
			((kAPP_PowerModeVlpr == targetPowerMode) && (kSMC_PowerStateVlpr == currentPowerState)))
	{
#if defined(USING_EXTERNAL_DEBUGGER)
		SEGGER_RTT_printf(0,"Already in the target power mode.\n");
#endif
		return false;
	}

	return true;
}

/*!
 * @brief Power mode switch.
 * This function is used to register the notifier_handle_t struct's member userFunction.
 */
status_t APP_PowerModeSwitch(notifier_user_config_t *targetConfig, void *userData)
{
	smc_power_state_t currentPowerMode;         /* Local variable with current power mode */
	app_power_mode_t targetPowerMode;           /* Local variable with target power mode name*/
	power_user_config_t *targetPowerModeConfig; /* Local variable with target power mode configuration */

	targetPowerModeConfig = (power_user_config_t *)targetConfig;
	currentPowerMode      = SMC_GetPowerModeState(SMC);
	targetPowerMode       = targetPowerModeConfig->mode;

	switch (targetPowerMode)
	{
		case kAPP_PowerModeVlpr:
			APP_SetClockVlpr();
			SMC_SetPowerModeVlpr(SMC);
			while (kSMC_PowerStateVlpr != SMC_GetPowerModeState(SMC))
			{
			}
			clockManagerCurrentRunMode = kAPP_PowerModeVlpr;
			break;

		case kAPP_PowerModeRun:
			/* If enter RUN from HSRUN, fisrt change clock. */
			if (kSMC_PowerStateHsrun == currentPowerMode)
			{
				APP_SetClockRunFromHsrun();
			}

			/* Power mode change. */
			SMC_SetPowerModeRun(SMC);
			while (kSMC_PowerStateRun != SMC_GetPowerModeState(SMC))
			{
			}

			/* If enter RUN from VLPR, change clock after the power mode change. */
			if (kSMC_PowerStateVlpr == currentPowerMode)
			{
				APP_SetClockRunFromVlpr();
			}

			clockManagerCurrentRunMode = kAPP_PowerModeRun;
			break;

		case kAPP_PowerModeHsrun:
			SMC_SetPowerModeHsrun(SMC);
			while (kSMC_PowerStateHsrun != SMC_GetPowerModeState(SMC))
			{
			}

			APP_SetClockHsrun(); /* Change clock setting after power mode change. */
			clockManagerCurrentRunMode = kAPP_PowerModeHsrun;
			break;
		default:
#if defined(USING_EXTERNAL_DEBUGGER)
			SEGGER_RTT_printf(0,"Wrong value\n");
#endif

			break;
	}
#if defined(USING_EXTERNAL_DEBUGGER)
	//	SEGGER_RTT_printf(0,"%dHz\n", CLOCK_GetFreq(kCLOCK_CoreSysClk));
#endif
	return kStatus_Success;
}


void clockManagerSetRunMode(uint8_t targetConfigIndex)
{
	taskENTER_CRITICAL();
	callbackData0.originPowerState = SMC_GetPowerModeState(SMC);
	NOTIFIER_SwitchConfig(&powerModeHandle, targetConfigIndex - kAPP_PowerModeMin - 1, kNOTIFIER_PolicyAgreement);
	taskEXIT_CRITICAL();
}

void clockManagerInit(void)
{
	callbacks[0] = callbackCfg0;

	clockManagerCurrentRunMode = kAPP_PowerModeMin;
	memset(&callbackData0, 0, sizeof(user_callback_data_t));

	/* Create Notifier Handle */
	NOTIFIER_CreateHandle(&powerModeHandle, powerConfigs, ARRAY_SIZE(powerConfigs), callbacks, 1U, APP_PowerModeSwitch, NULL);

	SMC_SetPowerModeProtection(SMC, kSMC_AllowPowerModeAll);
}
