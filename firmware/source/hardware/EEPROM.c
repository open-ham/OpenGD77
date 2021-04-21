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
#include "hardware/EEPROM.h"
#if defined(USING_EXTERNAL_DEBUGGER)
#include "SeggerRTT/RTT/SEGGER_RTT.h"
#endif

const uint8_t EEPROM_ADDRESS 	= 0x50;
const uint8_t EEPROM_PAGE_SIZE 	= 128;

static bool _EEPROM_Write(int address, uint8_t *buf, int size);

bool EEPROM_Write(int address, uint8_t *buf, int size)
{
	bool retVal;

	if (address / 128 == (address + size) / 128)
	{
		// All of the data is in the same page in the EEPROM so can just be written sequentially in one write
		retVal = _EEPROM_Write(address, buf, size);
	}
	else
	{
		// Either there is more data than the page size or the data needs to be split across multiple page boundaries
		int writeSize = 128 - (address % 128);
		retVal = true;// First time though need to prime the while loop

		while ((writeSize > 0) && (retVal == true))
		{
			retVal = _EEPROM_Write(address, buf, writeSize);
			address += writeSize;
			buf += writeSize;
			size -= writeSize;
			if (size > 128)
			{
				writeSize = 128;
			}
			else
			{
				writeSize = size;
			}
		}
	}
	return retVal;
}

/* This was the original EEPROM_Write function, but its now been wrapped by the new EEPROM_Write
 * While calls this function as necessary to handle write across 128 byte page boundaries
 * and also for writes larger than 128 bytes.
 */
static bool _EEPROM_Write(int address, uint8_t *buf, int size)
{
	const int COMMAND_SIZE = 2;
	int transferSize;
	uint8_t tmpBuf[COMMAND_SIZE];
	i2c_master_transfer_t masterXfer;
	status_t status;

	if (isI2cInUse)
	{
#if defined(USING_EXTERNAL_DEBUGGER) && defined(DEBUG_I2C)
		SEGGER_RTT_printf(0, "Clash in EEPROM_Write (2) with %d\n",isI2cInUse);
#endif
		return false;
	}
	taskENTER_CRITICAL();
	isI2cInUse = 1;


	while(size > 0)
	{
		transferSize = (size > EEPROM_PAGE_SIZE) ? EEPROM_PAGE_SIZE : size;
		tmpBuf[0] = address >> 8;
		tmpBuf[1] = address & 0xff;

		memset(&masterXfer, 0, sizeof(masterXfer));
		masterXfer.slaveAddress = EEPROM_ADDRESS;
		masterXfer.direction = kI2C_Write;
		masterXfer.subaddress = 0;
		masterXfer.subaddressSize = 0;
		masterXfer.data = tmpBuf;
		masterXfer.dataSize = COMMAND_SIZE;
		masterXfer.flags = kI2C_TransferNoStopFlag;//kI2C_TransferDefaultFlag;

		// EEPROM Will not respond if it is busy completing the previous write.
		// So repeat the write command until it responds or timeout after 50
		// attempts 1mS apart

		int timeoutCount = 50;
		status = kStatus_Success;
		do
		{
			if(status != kStatus_Success)
			{
				vTaskDelay(portTICK_PERIOD_MS * 1);
			}

			status = I2C_MasterTransferBlocking(I2C0, &masterXfer);

		} while((status != kStatus_Success) && (timeoutCount-- > 0));

		if (status != kStatus_Success)
		{
			isI2cInUse = 0;
			taskEXIT_CRITICAL();
			return false;
		}

		memset(&masterXfer, 0, sizeof(masterXfer));
		masterXfer.slaveAddress = EEPROM_ADDRESS;
		masterXfer.direction = kI2C_Write;
		masterXfer.subaddress = 0;
		masterXfer.subaddressSize = 0;
		masterXfer.data = buf;
		masterXfer.dataSize = transferSize;
		masterXfer.flags = kI2C_TransferNoStartFlag;//kI2C_TransferDefaultFlag;

		status = I2C_MasterTransferBlocking(I2C0, &masterXfer);
		if (status != kStatus_Success)
		{
			isI2cInUse = 0;
			taskEXIT_CRITICAL();
			return false;
		}
		address += transferSize;
		size -= transferSize;
	}

	isI2cInUse = 0;
	taskEXIT_CRITICAL();

	return true;
}

bool EEPROM_Read(int address, uint8_t *buf, int size)
{
	const int COMMAND_SIZE = 2;
	uint8_t tmpBuf[COMMAND_SIZE];
	i2c_master_transfer_t masterXfer;
	status_t status;


	if (isI2cInUse)
	{
#if defined(USING_EXTERNAL_DEBUGGER) && defined(DEBUG_I2C)
		SEGGER_RTT_printf(0, "Clash in EEPROM_Read (2) with %d\n",isI2cInUse);
#endif
		return false;
	}

	taskENTER_CRITICAL();
	isI2cInUse = 2;

	tmpBuf[0] = address >> 8;
	tmpBuf[1] = address & 0xff;

	memset(&masterXfer, 0, sizeof(masterXfer));
	masterXfer.slaveAddress = EEPROM_ADDRESS;
	masterXfer.direction = kI2C_Write;
	masterXfer.subaddress = 0;
	masterXfer.subaddressSize = 0;
	masterXfer.data = tmpBuf;
	masterXfer.dataSize = COMMAND_SIZE;
	masterXfer.flags = kI2C_TransferNoStopFlag;

	int timeoutCount = 50;
	status = kStatus_Success;
	do
	{
		if(status != kStatus_Success)
		{
			vTaskDelay(portTICK_PERIOD_MS * 1);
		}

		status = I2C_MasterTransferBlocking(I2C0, &masterXfer);

	}while((status != kStatus_Success) && (timeoutCount-- > 0));

	if (status != kStatus_Success)
	{
		isI2cInUse = 0;
		taskEXIT_CRITICAL();
		return false;
	}

	memset(&masterXfer, 0, sizeof(masterXfer));
	masterXfer.slaveAddress = EEPROM_ADDRESS;
	masterXfer.direction = kI2C_Read;
	masterXfer.subaddress = 0;
	masterXfer.subaddressSize = 0;
	masterXfer.data = buf;
	masterXfer.dataSize = size;
	masterXfer.flags = kI2C_TransferRepeatedStartFlag;

	status = I2C_MasterTransferBlocking(I2C0, &masterXfer);
	if (status != kStatus_Success)
	{
		isI2cInUse = 0;
		taskEXIT_CRITICAL();
		return false;
	}

	isI2cInUse = false;
	taskEXIT_CRITICAL();

	return true;
}
