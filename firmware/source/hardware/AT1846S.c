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

#include "hardware/AT1846S.h"
#include "functions/settings.h"
#include "functions/trx.h"
#if defined(USING_EXTERNAL_DEBUGGER)
#include "SeggerRTT/RTT/SEGGER_RTT.h"
#endif

typedef struct
{
	bool cached[2];
	uint8_t lowByte[2];
	uint8_t highByte[2];
} RegCache_t;

static RegCache_t registerCache[127];// all values will be initialised to false,0,0 because its a global
static int currentRegisterBank = 0;

static const uint8_t AT1846InitSettings[][AT1846_BYTES_PER_COMMAND] = {
		{0x30, 0x00, 0x04}, // Poweron 1846s
		{0x04, 0x0F, 0xD0}, // Clock mode 25.6MHz/26MHz
			// from data table at 0x22f18
		{0x1F, 0x10, 0x00}, // gpio6 = sq out, all others are hi-z
		{0x09, 0x03, 0xAC}, // LDO regulator control (same in GD-77 as UV-82)
		{0x24, 0x00, 0x01}, // PLL control - uses default value (PLL lock detection etc)
		{0x31, 0x00, 0x31}, // UNDOCUMENTED - use recommended value
		{0x33, 0x45, 0xF5}, // agc number (recommended value)
		{0x34, 0x2B, 0x89}, // Rx digital gain (recommend value)
		{0x3F, 0x32, 0x63}, // This register is not in the list and defaults to 0x23C6
		{0x41, 0x47, 0x0F}, // Digital voice gain, (bits 6:0) however default value is supposed to be 0x4006 hence some bits are being set outside the documented range
		{0x42, 0x10, 0x36}, // RDA1846 lists this as Vox Shut threshold
		{0x43, 0x00, 0xBB}, // FM deviation
		{0x44, 0x06, 0xFF}, // Rx and tx gain controls
		{0x47, 0x7F, 0x2F}, // UNDOCUMENTED - UV82 and GD77 use the same values
		{0x4E, 0x00, 0x82}, // UNDOCUMENTED - use recommended value
		{0x4F, 0x2C, 0x62}, // UNDOCUMENTED - use recommended value
		{0x53, 0x00, 0x94}, // UNDOCUMENTED - use recommended value
		{0x54, 0x2A, 0x3C}, // UNDOCUMENTED - use recommended value
		{0x55, 0x00, 0x81}, // UNDOCUMENTED - use recommended value
		{0x56, 0x0B, 0x02}, // SQ detection time (SQ setting)
		{0x57, 0x1C, 0x00}, // bypass rssi_lpfilter
		{0x5A, 0x49, 0x35}, // SQ detection time (SQ setting)
		{0x58, 0xBC, 0xCD}, // Filters custom setting
		{0x62, 0x32, 0x63}, // modu_det_th (SQ setting)
		{0x4E, 0x20, 0x82}, // UNDOCUMENTED - use recommended value
		{0x63, 0x16, 0xAD}, // Pre_emphasis bypass threshold (recommended value)
			// end data table at 0x22f18
			// Calibration start
		{0x30, 0x40, 0xA4}, // Setup to calibrate
};

static const uint8_t AT1846PostinitSettings[][AT1846_BYTES_PER_COMMAND] = {
		{0x58, 0xBC, 0xED}, // Filters custom setting
		{0x0A, 0x7B, 0xA0}, // AGC Table
		{0x41, 0x47, 0x31}, // Digital voice gain, (bits 6:0) however default value is supposed to be 0x4006 hence some bits are being set outside the documented range
		{0x44, 0x05, 0xFF}, // Rx and tx gain controls
		{0x59, 0x09, 0xD2}, // Tx FM devitation
		{0x44, 0x05, 0xCF}, // Rx and tx gain controls
		{0x44, 0x05, 0xCC}, // Rx and tx gain controls
		{0x48, 0x1A, 0x32}, // noise1_th (SQ setting)
		{0x60, 0x1A, 0x32}, // noise2_th (SQ setting)
		{0x3F, 0x29, 0xD1}, // Rssi3_th (SQ setting)
		{0x0A, 0x7B, 0xA0}, // AGC Table
		{0x49, 0x0C, 0x96}, // setting SQ open and shut threshold
		{0x33, 0x45, 0xF5}, // agc number (recommended value)
		{0x41, 0x47, 0x0F}, // Digital voice gain, (bits 6:0) however default value is supposed to be 0x4006 hence some bits are being set outside the documented range
		{0x42, 0x10, 0x36}, // RDA1846 lists this as Vox Shut threshold
		{0x43, 0x00, 0xBB}, // FM deviation
		};

const uint8_t AT1846FM12P5kHzSettings[][AT1846_BYTES_PER_COMMAND] = {
		{0x3A, 0x44, 0xCB}, // 12.5 kHz settings
		{0x15, 0x11, 0x00}, // IF tuning bits (12:9)
		{0x32, 0x44, 0x95}, // agc target power
		{0x3A, 0x40, 0xC3}, // modu_det_sel (SQ setting)
		{0x3F, 0x28, 0xD0}, // Rssi3_th (SQ setting)
		{0x3C, 0x0F, 0x1E}, // Pk_det_th (SQ setting)
		{0x48, 0x1D, 0xB6}, // noise1_th (SQ setting)
		{0x62, 0x14, 0x25}, // modu_det_th (SQ setting)
		{0x65, 0x24, 0x94}, // setting th_sif for SQ rssi detect
		{0x66, 0xEB, 0x2E}, // rssi_comp  and afc range
		{0x7F, 0x00, 0x01}, // Goto page 1 registers
		{0x06, 0x00, 0x14}, // AGC Table (recommended value for 12.5kHz bandwidth operation)
		{0x07, 0x02, 0x0C}, // AGC Table (recommended value for 12.5kHz bandwidth operation)
		{0x08, 0x02, 0x14}, // AGC Table (recommended value for 12.5kHz bandwidth operation)
		{0x09, 0x03, 0x0C}, // AGC Table (recommended value for 12.5kHz bandwidth operation)
		{0x0A, 0x03, 0x14}, // AGC Table (recommended value for 12.5kHz bandwidth operation)
		{0x0B, 0x03, 0x24}, // AGC Table (recommended value for 12.5kHz bandwidth operation)
		{0x0C, 0x03, 0x44}, // AGC Table (recommended value for 12.5kHz bandwidth operation)
		{0x0D, 0x13, 0x44}, // AGC Table (recommended value for 12.5kHz bandwidth operation)
		{0x0E, 0x1B, 0x44}, // AGC Table (recommended value for 12.5kHz bandwidth operation)
		{0x0F, 0x3F, 0x44}, // AGC Table (recommended value for 12.5kHz bandwidth operation)
		{0x12, 0xE0, 0xEB}, // AGC Table (recommended value for 12.5kHz bandwidth operation)
		{0x7F, 0x00, 0x00}, // Go back to page 0 registers
		};

const uint8_t AT1846FM25kHzSettings[][AT1846_BYTES_PER_COMMAND] = {
		{0x3A, 0x40, 0xCB}, // 25 kHz settings
		{0x15, 0x1F, 0x00}, // IF tuning bits (12:9)
		{0x32, 0x75, 0x64}, // agc target power
		{0x3A, 0x44, 0xC3}, // modu_det_sel (SQ setting)
		{0x3F, 0x29, 0xD2}, // Rssi3_th (SQ setting)
		{0x3C, 0x0E, 0x1C}, // Pk_det_th (SQ setting)
		{0x48, 0x1E, 0x38}, // noise1_th (SQ setting)
		{0x62, 0x37, 0x67}, // modu_det_th (SQ setting)
		{0x65, 0x24, 0x8A}, // setting th_sif for SQ rssi detect
		{0x66, 0xFF, 0x2E}, // rssi_comp  and afc range
		{0x7F, 0x00, 0x01}, // Goto page 1 registers
		{0x06, 0x00, 0x24}, // AGC Table (recommended value for 25kHz bandwidth operation)
		{0x07, 0x02, 0x14}, // AGC Table (recommended value for 25kHz bandwidth operation)
		{0x08, 0x02, 0x24}, // AGC Table (recommended value for 25kHz bandwidth operation)
		{0x09, 0x03, 0x14}, // AGC Table (recommended value for 25kHz bandwidth operation)
		{0x0A, 0x03, 0x24}, // AGC Table (recommended value for 25kHz bandwidth operation)
		{0x0B, 0x03, 0x44}, // AGC Table (recommended value for 25kHz bandwidth operation)
		{0x0C, 0x03, 0x84}, // AGC Table (recommended value for 25kHz bandwidth operation)
		{0x0D, 0x13, 0x84}, // AGC Table (recommended value for 25kHz bandwidth operation)
		{0x0E, 0x1B, 0x84}, // AGC Table (recommended value for 25kHz bandwidth operation)
		{0x0F, 0x3F, 0x84}, // AGC Table (recommended value for 25kHz bandwidth operation)
		{0x12, 0xE0, 0xEB}, // AGC Table (recommended value for 25kHz bandwidth operation)
		{0x7F, 0x00, 0x00}, // Go back to page 0 registers
		};

const uint8_t AT1846FMSettings[][AT1846_BYTES_PER_COMMAND] = {
		{0x33, 0x44, 0xA5}, // agc number (recommended value)
		{0x41, 0x44, 0x31}, // Digital voice gain, (bits 6:0) however default value is supposed to be 0x4006 hence some bits are being set outside the documented range
		{0x42, 0x10, 0xF0}, // RDA1846 lists this as Vox Shut threshold
		{0x43, 0x00, 0xA9}, // FM deviation
		{0x58, 0xBC, 0x05}, // Enable some filters for FM e.g. de-emphasis / pre-emphasis / High and Low Pass Filters
		{0x44, 0x06, 0xFF}, // set internal volume to 100% . Details from Colin G4EML
		{0x3A, 0x40, 0xCB}, // modu_det_sel (SQ setting)
		{0x40, 0x00, 0x30}  // UNDOCUMENTED. THIS IS THE MAGIC REGISTER WHICH ALLOWS LOW FREQ AUDIO BY SETTING THE LS BIT. So it should be cleared to receive FM
		};

const uint8_t AT1846DMRSettings[][AT1846_BYTES_PER_COMMAND] = {
		{0x15, 0x11, 0x00}, // IF tuning bits (12:9)
		{0x40, 0x00, 0x31}, // UNDOCUMENTED. THIS IS THE MAGIC REGISTER WHICH ALLOWS LOW FREQ AUDIO BY SETTING THE LS BIT
		{0x32, 0x44, 0x95}, // agc target power
		{0x33, 0x45, 0xF5}, // agc number (recommended value)
		{0x3F, 0x28, 0xD0}, // Rssi3_th (SQ setting)
		{0x3C, 0x0F, 0x1E}, // Pk_det_th (SQ setting)
		{0x3A, 0x00, 0xC2}, // modu_det_sel (SQ setting). Tx No mic input, as the DMR signal directly modulates the master reference oscillator
		{0x41, 0x47, 0x31}, // Digital voice gain, (bits 6:0) however default value is supposed to be 0x4006 hence some bits are being set outside the documented range
		{0x42, 0x10, 0x36}, // RDA1846 lists this as Vox Shut threshold
		{0x43, 0x00, 0xBB}, // FM deviation
		{0x44, 0x06, 0xCC}, // set internal volume to 80%
		{0x48, 0x1D, 0xB6}, // noise1_th (SQ setting)
		{0x58, 0xBC, 0xFD}, // Disable all filters in DMR mode
		{0x62, 0x14, 0x25}, // modu_det_th (SQ setting)
		{0x65, 0x24, 0x94}, // setting th_sif for SQ rssi detect
		{0x66, 0xEB, 0x2E}, // rssi_comp  and afc range
		};



void I2C_AT1846S_send_Settings(const uint8_t settings[][AT1846_BYTES_PER_COMMAND], int numSettings)
{
	//taskENTER_CRITICAL();
	for(int i = 0; i < numSettings; i++)
	{
		AT1846SWriteReg2byte(settings[i][0], settings[i][1], settings[i][2]);
	}
	//taskEXIT_CRITICAL();
}

void AT1846Init(void)
{
	// --- start of AT1846_init()
	//taskENTER_CRITICAL();
	AT1846SWriteReg2byte(0x30, 0x00, 0x01); // Soft reset
	vTaskDelay(portTICK_PERIOD_MS * 50);

	I2C_AT1846S_send_Settings(AT1846InitSettings, sizeof(AT1846InitSettings) / AT1846_BYTES_PER_COMMAND);
	vTaskDelay(portTICK_PERIOD_MS * 50);

	AT1846SWriteReg2byte(0x30, 0x40, 0xA6); // chip_cal_en Enable calibration
	vTaskDelay(portTICK_PERIOD_MS * 100);

	AT1846SWriteReg2byte(0x30, 0x40, 0x06); // chip_cal_en Disable calibration
	vTaskDelay(portTICK_PERIOD_MS * 10);
	// Calibration end
	// --- end of AT1846_init()

	I2C_AT1846S_send_Settings(AT1846FM12P5kHzSettings, sizeof(AT1846FM12P5kHzSettings) / AT1846_BYTES_PER_COMMAND);// initially set the bandwidth for 12.5 kHz

	AT1846SetClearReg2byteWithMask(0x4e, 0xff, 0x3f, 0x00, 0x80); // Select cdcss mode for tx
	//taskEXIT_CRITICAL();
	vTaskDelay(portTICK_PERIOD_MS * 200);
}

void AT1846Postinit(void)
{
	I2C_AT1846S_send_Settings(AT1846PostinitSettings, sizeof(AT1846PostinitSettings) / AT1846_BYTES_PER_COMMAND);
}

void AT1846SetBandwidth(void)
{
	if (trxGetBandwidthIs25kHz())
	{
		// 25 kHz settings
		I2C_AT1846S_send_Settings(AT1846FM25kHzSettings, sizeof(AT1846FM25kHzSettings) / AT1846_BYTES_PER_COMMAND);
		AT1846SetClearReg2byteWithMask(0x30, 0xCF, 0x9F, 0x30, 0x00); // Set the 25Khz Bits and turn off the Rx and Tx
	}
	else
	{
		// 12.5 kHz settings
		I2C_AT1846S_send_Settings(AT1846FM12P5kHzSettings, sizeof(AT1846FM12P5kHzSettings) / AT1846_BYTES_PER_COMMAND);
		AT1846SetClearReg2byteWithMask(0x30, 0xCF, 0x9F, 0x00, 0x00); // Clear the 25Khz Bits and turn off the Rx and Tx
	}

	AT1846SetClearReg2byteWithMask(0x30, 0xFF, 0x9F, 0x00, 0x20); // Turn the Rx On
}

void AT1846SetMode(void)
{
	AT1846SetBandwidth();

	if (trxGetMode() == RADIO_MODE_ANALOG)
	{
		I2C_AT1846S_send_Settings(AT1846FMSettings, sizeof(AT1846FMSettings) / AT1846_BYTES_PER_COMMAND);
	}
	else
	{
		I2C_AT1846S_send_Settings(AT1846DMRSettings, sizeof(AT1846DMRSettings) / AT1846_BYTES_PER_COMMAND);
	}
}

void AT1846ReadVoxAndMicStrength(void)
{
	//taskENTER_CRITICAL();
	AT1846SReadReg2byte(0x1a, (uint8_t *)&trxTxVox, (uint8_t *)&trxTxMic);
	//taskEXIT_CRITICAL();
}

void AT1846ReadRSSIAndNoise(void)
{
	//taskENTER_CRITICAL();
	AT1846SReadReg2byte(0x1b, (uint8_t *)&trxRxSignal, (uint8_t *)&trxRxNoise);
	//taskEXIT_CRITICAL();
}


int AT1846SetClearReg2byteWithMask(uint8_t reg, uint8_t mask1, uint8_t mask2, uint8_t val1, uint8_t val2)
{
    status_t status;
	uint8_t tmp_val1, tmp_val2;

	if ((registerCache[reg].cached[currentRegisterBank]))
	{
		tmp_val1 = registerCache[reg].highByte[currentRegisterBank];
		tmp_val2 = registerCache[reg].lowByte[currentRegisterBank];
	}
	else
	{
		status = AT1846SReadReg2byte(reg, &tmp_val1, &tmp_val2);
	    if (status != kStatus_Success)
	    {
	    	return status;
	    }
	}

	tmp_val1 = val1 | (tmp_val1 & mask1);
	tmp_val2 = val2 | (tmp_val2 & mask2);
	status = AT1846SWriteReg2byte(reg, tmp_val1, tmp_val2);

	return status;
}

status_t AT1846SReadReg2byte(uint8_t reg, uint8_t *val1, uint8_t *val2)
{
    i2c_master_transfer_t masterXfer;
    status_t status;
    uint8_t buff[4];// Transfers are always 3 bytes but pad to 4 byte boundary

    if (isI2cInUse)
    {
#if defined(USING_EXTERNAL_DEBUGGER) && defined(DEBUG_I2C)
    	SEGGER_RTT_printf(0, "Clash in read_I2C_reg_2byte (4) with %d\n",isI2cInUse);
#endif
    	return kStatus_Success;
    }
    isI2cInUse = 4;

	buff[0] = reg;

    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = AT1846S_I2C_MASTER_SLAVE_ADDR_7BIT;
    masterXfer.direction = kI2C_Write;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data = buff;
    masterXfer.dataSize = 1;
    masterXfer.flags = kI2C_TransferDefaultFlag;

    status = I2C_MasterTransferBlocking(I2C0, &masterXfer);
    if (status != kStatus_Success)
    {
    	isI2cInUse = 0;
    	return status;
    }

    masterXfer.slaveAddress = AT1846S_I2C_MASTER_SLAVE_ADDR_7BIT;
    masterXfer.direction = kI2C_Read;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data = buff;
    masterXfer.dataSize = 2;
    masterXfer.flags = kI2C_TransferDefaultFlag;

    status = I2C_MasterTransferBlocking(I2C0, &masterXfer);
    if (status != kStatus_Success)
    {
    	isI2cInUse = 0;
    	return status;
    }

    *val1 = buff[0];
    *val2 = buff[1];

    isI2cInUse = 0;
	return status;
}

status_t AT1846SWriteReg2byte(uint8_t reg, uint8_t val1, uint8_t val2)
{
    i2c_master_transfer_t masterXfer;
    status_t status;
    uint8_t buff[4];// Transfers are always 3 bytes but pad to 4 byte boundary

    if (reg == 0x7f)
    {
    	currentRegisterBank = val2;
    }
    else
    {
    	if ((registerCache[reg].cached[currentRegisterBank]) && (registerCache[reg].highByte[currentRegisterBank] == val1) &&  (registerCache[reg].lowByte[currentRegisterBank] == val2))
    	{
    		//SEGGER_RTT_printf(0, "%d 0x%02x 0x%02x 0x%02x\n",currentRegisterBank,reg,val1,val2);
    		return kStatus_Success;
    	}
    	else
    	{
    		//SEGGER_RTT_printf(0, "W            %d 0x%02x 0x%02x 0x%02x\n",currentRegisterBank,reg,val1,val2);
    	}
    }

    if (isI2cInUse)
    {
#if defined(USING_EXTERNAL_DEBUGGER) && defined(DEBUG_I2C)
    	SEGGER_RTT_printf(0, "Clash in write_I2C_reg_2byte (3) with %d\n",isI2cInUse);
#endif
    	return kStatus_Success;
    }
    isI2cInUse = 3;

	buff[0] = reg;
	buff[1] = val1;
	buff[2] = val2;

    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = AT1846S_I2C_MASTER_SLAVE_ADDR_7BIT;
    masterXfer.direction = kI2C_Write;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data = buff;
    masterXfer.dataSize = 3;
    masterXfer.flags = kI2C_TransferDefaultFlag;

    status = I2C_MasterTransferBlocking(I2C0, &masterXfer);

    isI2cInUse = 0;

    if (reg != 0x7F)
    {
	    registerCache[reg].cached[currentRegisterBank] = true;
	    registerCache[reg].highByte[currentRegisterBank] = val1;
	    registerCache[reg].lowByte[currentRegisterBank] = val2;
    }

	return status;
}
