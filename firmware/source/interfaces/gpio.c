/*
 * Copyright (C)2020 Roger Clark VK3KYY
 *
 */

#include "interfaces/gpio.h"

gpio_pin_config_t pin_config_input =
{
	kGPIO_DigitalInput,
	0
};

gpio_pin_config_t pin_config_output =
{
	kGPIO_DigitalOutput,
	0
};

void gpioInitButtons(void)
{

	PORT_SetPinMux(Port_PTT, Pin_PTT, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_SK1, Pin_SK1, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_SK2, Pin_SK2, kPORT_MuxAsGpio);
#if ! defined(PLATFORM_RD5R)
	PORT_SetPinMux(Port_Orange, Pin_Orange, kPORT_MuxAsGpio);
#endif

	GPIO_PinInit(GPIO_PTT, Pin_PTT, &pin_config_input);
	GPIO_PinInit(GPIO_SK1, Pin_SK1, &pin_config_input);
	GPIO_PinInit(GPIO_SK2, Pin_SK2, &pin_config_input);
	#if ! defined(PLATFORM_RD5R)
	GPIO_PinInit(GPIO_Orange, Pin_Orange, &pin_config_input);
#endif
}



void gpioInitCommon(void)
{
    CLOCK_EnableClock(kCLOCK_PortA);
    CLOCK_EnableClock(kCLOCK_PortB);
    CLOCK_EnableClock(kCLOCK_PortC);
    CLOCK_EnableClock(kCLOCK_PortD);
    CLOCK_EnableClock(kCLOCK_PortE);

#if !defined(PLATFORM_RD5R)
    // Power On/Off logic
    PORT_SetPinMux(Port_Keep_Power_On, Pin_Keep_Power_On, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_Power_Switch, Pin_Power_Switch, kPORT_MuxAsGpio);

    // Power On/Off logic
    GPIO_PinInit(GPIO_Keep_Power_On, Pin_Keep_Power_On, &pin_config_output);
    GPIO_PinInit(GPIO_Power_Switch, Pin_Power_Switch, &pin_config_input);

    // Power On/Off logic
	GPIO_PinWrite(GPIO_Keep_Power_On, Pin_Keep_Power_On, 1);
#endif


    // Speaker mute and RX/TX mux init
    PORT_SetPinMux(Port_audio_amp_enable, Pin_audio_amp_enable, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_RX_audio_mux, Pin_RX_audio_mux, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_TX_audio_mux, Pin_TX_audio_mux, kPORT_MuxAsGpio);
    GPIO_PinInit(GPIO_audio_amp_enable, Pin_audio_amp_enable, &pin_config_output);
    GPIO_PinInit(GPIO_RX_audio_mux, Pin_RX_audio_mux, &pin_config_output);
    GPIO_PinInit(GPIO_TX_audio_mux, Pin_TX_audio_mux, &pin_config_output);
    GPIO_PinWrite(GPIO_audio_amp_enable, Pin_audio_amp_enable, 0);
    GPIO_PinWrite(GPIO_RX_audio_mux, Pin_RX_audio_mux, 0);
    GPIO_PinWrite(GPIO_TX_audio_mux, Pin_TX_audio_mux, 0);

    // UHF/VHF RX/TX amp init
    PORT_SetPinMux(Port_VHF_RX_amp_power, Pin_VHF_RX_amp_power, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_UHF_RX_amp_power, Pin_UHF_RX_amp_power, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_UHF_TX_amp_power, Pin_UHF_TX_amp_power, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_VHF_TX_amp_power, Pin_VHF_TX_amp_power, kPORT_MuxAsGpio);

    GPIO_PinInit(GPIO_VHF_RX_amp_power, Pin_VHF_RX_amp_power, &pin_config_output);
    GPIO_PinInit(GPIO_UHF_RX_amp_power, Pin_UHF_RX_amp_power, &pin_config_output);

    GPIO_PinInit(GPIO_UHF_TX_amp_power, Pin_UHF_TX_amp_power, &pin_config_output);
    GPIO_PinInit(GPIO_VHF_TX_amp_power, Pin_VHF_TX_amp_power, &pin_config_output);

    GPIO_PinWrite(GPIO_VHF_RX_amp_power, Pin_VHF_RX_amp_power, 0);
    GPIO_PinWrite(GPIO_UHF_RX_amp_power, Pin_UHF_RX_amp_power, 0);

    GPIO_PinWrite(GPIO_UHF_TX_amp_power, Pin_UHF_TX_amp_power, 0);
    GPIO_PinWrite(GPIO_VHF_TX_amp_power, Pin_VHF_TX_amp_power, 0);
}

void gpioInitDisplay()
{
#if ! defined(PLATFORM_GD77S)
	PORT_SetPinMux(Port_Display_CS, Pin_Display_CS, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_Display_RST, Pin_Display_RST, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_Display_RS, Pin_Display_RS, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_Display_SCK, Pin_Display_SCK, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_Display_SDA, Pin_Display_SDA, kPORT_MuxAsGpio);

#ifdef DISPLAY_LED_PWM

#if defined(PLATFORM_RD5R)
    PORT_SetPinMux(Port_Display_Light, Pin_Display_Light, kPORT_MuxAlt7);/* Configured as PWM FTM0_CH2 */
#else
    PORT_SetPinMux(Port_Display_Light, Pin_Display_Light, kPORT_MuxAlt4);/* Configured as PWM FTM0_CH3 */
#endif

	ftm_config_t ftmInfo;
	ftm_chnl_pwm_signal_param_t ftmParam;

	ftmParam.chnlNumber = BOARD_FTM_CHANNEL;
	ftmParam.level = kFTM_HighTrue;
	ftmParam.dutyCyclePercent = 0U;// initially 0%
	ftmParam.firstEdgeDelayPercent = 0U;
	FTM_GetDefaultConfig(&ftmInfo);
	FTM_Init(BOARD_FTM_BASEADDR, &ftmInfo);/* Initialize FTM module */
	/* Configure ftm params with frequency 25kHz
	 * Because a lower clock speed is used during Rx, the speed during Rx will be approximately 10kHz
	 */
	FTM_SetupPwm(BOARD_FTM_BASEADDR, &ftmParam, 1U, kFTM_CenterAlignedPwm, 25000U, CLOCK_GetFreq(kCLOCK_BusClk));
	FTM_StartTimer(BOARD_FTM_BASEADDR, kFTM_SystemClock);

#else // ! DISPLAY_LED_PWM

	PORT_SetPinMux(Port_Display_Light, Pin_Display_Light, kPORT_MuxAsGpio);
	GPIO_PinInit(GPIO_Display_Light, Pin_Display_Light, &pin_config_output);
	GPIO_PinWrite(GPIO_Display_Light, Pin_Display_Light, 0);

#endif // DISPLAY_LED_PWM

	GPIO_PinInit(GPIO_Display_CS, Pin_Display_CS, &pin_config_output);
	GPIO_PinInit(GPIO_Display_RST, Pin_Display_RST, &pin_config_output);
	GPIO_PinInit(GPIO_Display_RS, Pin_Display_RS, &pin_config_output);
	GPIO_PinInit(GPIO_Display_SCK, Pin_Display_SCK, &pin_config_output);
	GPIO_PinInit(GPIO_Display_SDA, Pin_Display_SDA, &pin_config_output);
#endif
}

void gpioSetDisplayBacklightIntensityPercentage(uint8_t intensityPercentage)
{
#if ! defined(PLATFORM_GD77S)
#ifdef DISPLAY_LED_PWM
	static uint8_t prevPercentage = 255;

	if (prevPercentage != intensityPercentage)
	{
		prevPercentage = intensityPercentage;
		FTM_UpdateChnlEdgeLevelSelect(BOARD_FTM_BASEADDR, BOARD_FTM_CHANNEL, kFTM_NoPwmSignal); // Disable channel output before updating the dutycycle
		FTM_UpdatePwmDutycycle(BOARD_FTM_BASEADDR, BOARD_FTM_CHANNEL, kFTM_CenterAlignedPwm, intensityPercentage); // Update PWM duty cycle
		FTM_SetSoftwareTrigger(BOARD_FTM_BASEADDR, true); // Software trigger to update registers
		FTM_UpdateChnlEdgeLevelSelect(BOARD_FTM_BASEADDR, BOARD_FTM_CHANNEL, kFTM_HighTrue);    // Start channel output with updated dutycycle
	}
#endif
#endif // ! PLATFORM_GD77S
}

void gpioInitFlash(void)
{
    PORT_SetPinMux(Port_SPI_FLASH_CS_U, Pin_SPI_FLASH_CS_U, kPORT_MuxAsGpio);//CS
    GPIO_PinInit(GPIO_SPI_FLASH_CS_U, Pin_SPI_FLASH_CS_U, &pin_config_output);

    PORT_SetPinMux(Port_SPI_FLASH_CLK_U, Pin_SPI_FLASH_CLK_U, kPORT_MuxAsGpio);//CLK
    GPIO_PinInit(GPIO_SPI_FLASH_CLK_U, Pin_SPI_FLASH_CLK_U, &pin_config_output);

    PORT_SetPinMux(Port_SPI_FLASH_DI_U, Pin_SPI_FLASH_DI_U, kPORT_MuxAsGpio);//Data out
    GPIO_PinInit(GPIO_SPI_FLASH_DI_U, Pin_SPI_FLASH_DI_U, &pin_config_input);

    PORT_SetPinMux(Port_SPI_FLASH_DO_U, Pin_SPI_FLASH_DO_U, kPORT_MuxAsGpio);//Data in
    GPIO_PinInit(GPIO_SPI_FLASH_DO_U, Pin_SPI_FLASH_DO_U, &pin_config_output);
}

void gpioInitKeyboard(void)
{
	#if ! defined(PLATFORM_GD77S)
	port_pin_config_t config =
	{
			kPORT_PullUp,
			kPORT_FastSlewRate,
			kPORT_PassiveFilterDisable,
			kPORT_OpenDrainDisable,
			kPORT_LowDriveStrength,
			kPORT_MuxAsGpio,
			kPORT_UnlockRegister
	};

    // column lines
	PORT_SetPinMux(Port_Key_Col0, Pin_Key_Col0, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_Key_Col1, Pin_Key_Col1, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_Key_Col2, Pin_Key_Col2, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_Key_Col3, Pin_Key_Col3, kPORT_MuxAsGpio);

    // row lines
    PORT_SetPinConfig(Port_Key_Row0, Pin_Key_Row0, &config);
    PORT_SetPinConfig(Port_Key_Row1, Pin_Key_Row1, &config);
    PORT_SetPinConfig(Port_Key_Row2, Pin_Key_Row2, &config);
    PORT_SetPinConfig(Port_Key_Row3, Pin_Key_Row3, &config);
    PORT_SetPinConfig(Port_Key_Row4, Pin_Key_Row4, &config);

    // column lines
    GPIO_PinInit(GPIO_Key_Col0, Pin_Key_Col0, &pin_config_input);
    GPIO_PinInit(GPIO_Key_Col1, Pin_Key_Col1, &pin_config_input);
    GPIO_PinInit(GPIO_Key_Col2, Pin_Key_Col2, &pin_config_input);
    GPIO_PinInit(GPIO_Key_Col3, Pin_Key_Col3, &pin_config_input);

    // row lines
    GPIO_PinInit(GPIO_Key_Row0, Pin_Key_Row0, &pin_config_input);
    GPIO_PinInit(GPIO_Key_Row1, Pin_Key_Row1, &pin_config_input);
    GPIO_PinInit(GPIO_Key_Row2, Pin_Key_Row2, &pin_config_input);
    GPIO_PinInit(GPIO_Key_Row3, Pin_Key_Row3, &pin_config_input);
    GPIO_PinInit(GPIO_Key_Row4, Pin_Key_Row4, &pin_config_input);
#endif // ! PLATFORM_GD77S
}

void gpioInitLEDs(void)
{
    PORT_SetPinMux(Port_LEDgreen, Pin_LEDgreen, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_LEDred, Pin_LEDred, kPORT_MuxAsGpio);

    GPIO_PinInit(GPIO_LEDgreen, Pin_LEDgreen, &pin_config_output);
    GPIO_PinInit(GPIO_LEDred, Pin_LEDred, &pin_config_output);

#if defined(PLATFORM_RD5R)
	// Built-in torch
	PORT_SetPinMux(Port_Torch, Pin_Torch, kPORT_MuxAsGpio);
	GPIO_PinInit(GPIO_Torch, Pin_Torch, &pin_config_output);
#endif
}

void gpioInitRotarySwitch(void)
{
#if defined(PLATFORM_GD77S)
	PORT_SetPinMux(Port_RotarySW_Line0, Pin_RotarySW_Line0, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_RotarySW_Line1, Pin_RotarySW_Line1, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_RotarySW_Line2, Pin_RotarySW_Line2, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_RotarySW_Line3, Pin_RotarySW_Line3, kPORT_MuxAsGpio);

	GPIO_PinInit(GPIO_RotarySW_Line0, Pin_RotarySW_Line0, &pin_config_input);
	GPIO_PinInit(GPIO_RotarySW_Line1, Pin_RotarySW_Line1, &pin_config_input);
	GPIO_PinInit(GPIO_RotarySW_Line2, Pin_RotarySW_Line2, &pin_config_input);
	GPIO_PinInit(GPIO_RotarySW_Line3, Pin_RotarySW_Line3, &pin_config_input);
#endif
}

void gpioInitC6000Interface(void)
{
    // C6000 interrupts
    PORT_SetPinMux(Port_INT_C6000_RF_RX, Pin_INT_C6000_RF_RX, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_INT_C6000_RF_TX, Pin_INT_C6000_RF_TX, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_INT_C6000_SYS, Pin_INT_C6000_SYS, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_INT_C6000_TS, Pin_INT_C6000_TS, kPORT_MuxAsGpio);
    GPIO_PinInit(GPIO_INT_C6000_RF_RX, Pin_INT_C6000_RF_RX, &pin_config_input);
    GPIO_PinInit(GPIO_INT_C6000_RF_TX, Pin_INT_C6000_RF_TX, &pin_config_input);
    GPIO_PinInit(GPIO_INT_C6000_SYS, Pin_INT_C6000_SYS, &pin_config_input);
    GPIO_PinInit(GPIO_INT_C6000_TS, Pin_INT_C6000_TS, &pin_config_input);

    // Connections with C6000
    PORT_SetPinMux(Port_INT_C6000_RESET, Pin_INT_C6000_RESET, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_INT_C6000_PWD, Pin_INT_C6000_PWD, kPORT_MuxAsGpio);
    GPIO_PinInit(GPIO_INT_C6000_RESET, Pin_INT_C6000_RESET, &pin_config_output);
    GPIO_PinInit(GPIO_INT_C6000_PWD, Pin_INT_C6000_PWD, &pin_config_output);
}
