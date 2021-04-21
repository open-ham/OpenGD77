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

#include "drivers/fsl_port.h"
#include "interfaces/i2c.h"

#if defined(USING_EXTERNAL_DEBUGGER)
#include "SeggerRTT/RTT/SEGGER_RTT.h"
#endif

volatile int isI2cInUse = 0;

void I2C0aInit(void)
{
    // I2C0a to AT24C512 EEPROM & AT1846S
    const port_pin_config_t porte24_config = {/* Internal pull-up resistor is enabled */
                                              kPORT_PullUp,
                                              /* Fast slew rate is configured */
                                              kPORT_FastSlewRate,
                                              /* Passive filter is disabled */
                                              kPORT_PassiveFilterDisable,
                                              /* Open drain is enabled */
                                              kPORT_OpenDrainEnable,
                                              /* Low drive strength is configured */
                                              kPORT_LowDriveStrength,
                                              /* Pin is configured as I2C0_SCL */
                                              kPORT_MuxAlt5,
                                              /* Pin Control Register fields [15:0] are not locked */
                                              kPORT_UnlockRegister};
    PORT_SetPinConfig(Port_I2C0a_SCL, Pin_I2C0a_SCL, &porte24_config);

    const port_pin_config_t porte25_config = {/* Internal pull-up resistor is enabled */
                                              kPORT_PullUp,
                                              /* Fast slew rate is configured */
                                              kPORT_FastSlewRate,
                                              /* Passive filter is disabled */
                                              kPORT_PassiveFilterDisable,
                                              /* Open drain is enabled */
                                              kPORT_OpenDrainEnable,
                                              /* Low drive strength is configured */
                                              kPORT_LowDriveStrength,
                                              /* Pin is configured as I2C0_SDA */
                                              kPORT_MuxAlt5,
                                              /* Pin Control Register fields [15:0] are not locked */
                                              kPORT_UnlockRegister};
    PORT_SetPinConfig(Port_I2C0a_SDA, Pin_I2C0a_SDA, &porte25_config);

    NVIC_SetPriority(I2C0_IRQn, 3);
    isI2cInUse = 0;

	I2C0Setup();
}

void I2C0bInit(void)
{
	// I2C0b to ALPU-MP-1413
    const port_pin_config_t portb2_config = {/* Internal pull-up resistor is enabled */
                                             kPORT_PullUp,
                                             /* Fast slew rate is configured */
                                             kPORT_FastSlewRate,
                                             /* Passive filter is disabled */
                                             kPORT_PassiveFilterDisable,
                                             /* Open drain is enabled */
                                             kPORT_OpenDrainEnable,
                                             /* Low drive strength is configured */
                                             kPORT_LowDriveStrength,
                                             /* Pin is configured as I2C0_SCL */
                                             kPORT_MuxAlt2,
                                             /* Pin Control Register fields [15:0] are not locked */
                                             kPORT_UnlockRegister};
    PORT_SetPinConfig(Port_I2C0b_SCL, Pin_I2C0b_SCL, &portb2_config);

    const port_pin_config_t portb3_config = {/* Internal pull-up resistor is enabled */
                                             kPORT_PullUp,
                                             /* Fast slew rate is configured */
                                             kPORT_FastSlewRate,
                                             /* Passive filter is disabled */
                                             kPORT_PassiveFilterDisable,
                                             /* Open drain is enabled */
                                             kPORT_OpenDrainEnable,
                                             /* Low drive strength is configured */
                                             kPORT_LowDriveStrength,
                                             /* Pin is configured as I2C0_SDA */
                                             kPORT_MuxAlt2,
                                             /* Pin Control Register fields [15:0] are not locked */
                                             kPORT_UnlockRegister};
    PORT_SetPinConfig(Port_I2C0b_SDA, Pin_I2C0b_SDA, &portb3_config);

    NVIC_SetPriority(I2C0_IRQn, 3);
}

void I2C0Setup(void)
{
    i2c_master_config_t masterConfig;

	/*
	 * masterConfig.baudRate_Bps = 100000U;
	 * masterConfig.enableStopHold = false;
	 * masterConfig.glitchFilterWidth = 0U;
	 * masterConfig.enableMaster = true;
	 */
	I2C_MasterGetDefaultConfig(&masterConfig);
	masterConfig.baudRate_Bps = I2C_BAUDRATE;

	I2C_MasterInit(I2C0, &masterConfig, CLOCK_GetFreq(I2C0_CLK_SRC));
}
