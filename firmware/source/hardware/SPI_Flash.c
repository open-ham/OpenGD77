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

#include "hardware/SPI_Flash.h"
#include "interfaces/gpio.h"

// private functions
static bool spi_flash_busy(void);
static void spi_flash_transfer_buf(uint8_t *inBuf,uint8_t *outBuf,int size);
static uint8_t spi_flash_transfer(uint8_t c);
static void spi_flash_setWriteEnable(bool cmd);
static inline void spi_flash_enable(void);
static inline void spi_flash_disable(void);
__attribute__((section(".data.$RAM2"))) uint8_t SPI_Flash_sectorbuffer[4096];


//COMMANDS. Not all implemented or used
#define W_EN 			0x06	//write enable
#define W_DE			0x04	//write disable
#define R_SR1			0x05	//read status reg 1
#define R_SR2			0x35	//read status reg 2
#define W_SR			0x01	//write status reg
#define PAGE_PGM		0x02	//page program
#define QPAGE_PGM		0x32	//quad input page program
#define BLK_E_64K		0xD8	//block erase 64KB
#define BLK_E_32K		0x52	//block erase 32KB
#define SECTOR_E		0x20	//sector erase 4KB
#define CHIP_ERASE		0xc7	//chip erase
#define CHIP_ERASE2		0x60	//=CHIP_ERASE
#define E_SUSPEND		0x75	//erase suspend
#define E_RESUME		0x7a	//erase resume
#define PDWN			0xb9	//power down
#define HIGH_PERF_M		0xa3	//high performance mode
#define CONT_R_RST		0xff	//continuous read mode reset
#define RELEASE			0xab	//release power down or HPM/Dev ID (deprecated)
#define R_MANUF_ID		0x90	//read Manufacturer and Dev ID (deprecated)
#define R_UNIQUE_ID		0x4b	//read unique ID (suggested)
#define R_JEDEC_ID		0x9f	//read JEDEC ID = Manuf+ID (suggested)
#define READ			0x03
#define FAST_READ		0x0b
#define SR1_BUSY_MASK	0x01
#define SR1_WEN_MASK	0x02
#define WINBOND_MANUF	0xef
  
bool SPI_Flash_init(void)
{
	int partNumber;

	gpioInitFlash();

    GPIO_PinWrite(GPIO_SPI_FLASH_CS_U, Pin_SPI_FLASH_CS_U, 1);// Disable
    GPIO_PinWrite(GPIO_SPI_FLASH_CLK_U, Pin_SPI_FLASH_CLK_U, 0);// Default clock pin to low

    partNumber = SPI_Flash_readPartID();

    if (partNumber == 0x4014 || partNumber == 0x4017 || partNumber == 0x4015)
    {
    	return true;
    }
    else
    {
    	return false;
    }
}

// Returns false for failed
// Note. There is no error checking that the device is not initially busy.
bool SPI_Flash_read(uint32_t addr, uint8_t *dataBuf, int size)
{
  uint8_t commandBuf[4]= { READ, addr >> 16, addr >> 8, addr };// command
  /*
   * This is very ineffecient and the Flash never seems to be busy
  if(spi_flash_busy())
  {
    return false;
  }
  */
  spi_flash_enable();
  spi_flash_transfer_buf(commandBuf, commandBuf, 4);
  for(int i = 0; i < size; i++)
  {
	  *dataBuf++ = spi_flash_transfer(0x00);
  }
  spi_flash_disable();
  return true;
}

bool SPI_Flash_write(uint32_t addr, uint8_t *dataBuf, int size)
{
	bool retVal = true;
	int flashWritePos = addr;
	int flashSector;
	int flashEndSector;
	int bytesToWriteInCurrentSector = size;

	flashSector	= flashWritePos / 4096;
	flashEndSector = (flashWritePos + size) / 4096;

	if (flashSector != flashEndSector)
	{
		bytesToWriteInCurrentSector = (flashEndSector * 4096) - flashWritePos;
	}

	SPI_Flash_read(flashSector * 4096, SPI_Flash_sectorbuffer, 4096);
	uint8_t *writePos = SPI_Flash_sectorbuffer + flashWritePos - (flashSector * 4096);
	memcpy(writePos, dataBuf, bytesToWriteInCurrentSector);

	retVal = SPI_Flash_eraseSector(flashSector * 4096);
	if (!retVal)
	{
		return false;
	}

	for (int i = 0; i < 16; i++)
	{
		retVal = SPI_Flash_writePage(flashSector * 4096 + i * 256, SPI_Flash_sectorbuffer + i * 256);
		if (!retVal)
		{
			return false;
		}
	}

	if (flashSector != flashEndSector)
	{
		uint8_t *bufPusOffset = (uint8_t *) dataBuf + bytesToWriteInCurrentSector;
		bytesToWriteInCurrentSector = size - bytesToWriteInCurrentSector;

		SPI_Flash_read(flashEndSector * 4096, SPI_Flash_sectorbuffer, 4096);
		memcpy(SPI_Flash_sectorbuffer, (uint8_t *) bufPusOffset, bytesToWriteInCurrentSector);

		retVal = SPI_Flash_eraseSector(flashEndSector * 4096);

		if (!retVal)
		{
			return false;
		}
		for (int i = 0; i < 16; i++)
		{
			retVal = SPI_Flash_writePage(flashEndSector * 4096 + i * 256, SPI_Flash_sectorbuffer + i * 256);

			if (!retVal)
			{
				return false;
			}
		}

	}
	return true;
}

int SPI_Flash_readStatusRegister(void)
{
	int r1,r2;

	spi_flash_enable();
	spi_flash_transfer(R_SR1);
	r1 = spi_flash_transfer(0xff);
	spi_flash_disable();
	spi_flash_enable();
	spi_flash_transfer(R_SR2);
	r2 = spi_flash_transfer(0xff);
	spi_flash_disable();

	return (((uint16_t)r2) << 8) | r1;
}

int SPI_Flash_readManufacturer(void)
{
	uint8_t commandBuf[4] = { R_JEDEC_ID, 0x00, 0x00, 0x00};

	spi_flash_enable();
	spi_flash_transfer_buf(commandBuf, commandBuf, 4);
	spi_flash_disable();

	return commandBuf[1];
}

int SPI_Flash_readPartID(void)
{
	uint8_t commandBuf[4] = { R_JEDEC_ID, 0x00, 0x00, 0x00};

	spi_flash_enable();
	spi_flash_transfer_buf(commandBuf, commandBuf, 4);
	spi_flash_disable();

	return (commandBuf[2] << 8) | commandBuf[3];
}

bool SPI_Flash_writePage(uint32_t addr_start,uint8_t *dataBuf)
{
	bool isBusy;
	int waitCounter = 5;// Worst case is something like 3mS
	uint8_t commandBuf[4]= { PAGE_PGM, addr_start >> 16, addr_start >> 8, 0x00} ;

	spi_flash_setWriteEnable(true);

	spi_flash_enable();

	spi_flash_transfer_buf(commandBuf, commandBuf, 4);// send the command and the address

	for(int i = 0; i < 0x100; i++)
	{
		spi_flash_transfer(*dataBuf++);
	}

	spi_flash_disable();

	do
	{
	    vTaskDelay(portTICK_PERIOD_MS * 1);
		isBusy = spi_flash_busy();
	} while ((waitCounter-- > 0) && isBusy);

	return !isBusy;
}

// Returns true if erased and false if failed.
bool SPI_Flash_eraseSector(uint32_t addr_start)
{
	int waitCounter = 500;// erase can take up to 500 mS
	bool isBusy;
	uint8_t commandBuf[4] = { SECTOR_E, addr_start >> 16, addr_start >> 8, 0x00};

	spi_flash_enable();
	spi_flash_setWriteEnable(true);
	spi_flash_disable();

	spi_flash_enable();
	spi_flash_transfer_buf(commandBuf, commandBuf, 4);
	spi_flash_disable();

	do
	{
	    vTaskDelay(portTICK_PERIOD_MS * 1);
		isBusy = spi_flash_busy();
	} while ((waitCounter-- > 0) && isBusy);

	return !isBusy;// If still busy after
}

static inline void spi_flash_enable(void)
{
    GPIO_PinInit(GPIO_SPI_FLASH_DO_U, Pin_SPI_FLASH_DO_U, &pin_config_output);
	GPIO_PinInit(GPIO_SPI_FLASH_CS_U, Pin_SPI_FLASH_CS_U, &pin_config_output);
	GPIO_SPI_FLASH_CS_U->PCOR = 1U << Pin_SPI_FLASH_CS_U;
}

static void spi_flash_disable(void)
{

	GPIO_SPI_FLASH_CS_U->PSOR = 1U << Pin_SPI_FLASH_CS_U;
	GPIO_PinInit(GPIO_SPI_FLASH_CS_U, Pin_SPI_FLASH_CS_U, &pin_config_input);
	GPIO_PinInit(GPIO_SPI_FLASH_DO_U, Pin_SPI_FLASH_DO_U, &pin_config_input);
}

static uint8_t spi_flash_transfer(uint8_t c)
{
	for (uint8_t bit = 0; bit < 8; bit++)
	{
		GPIO_SPI_FLASH_CLK_U->PCOR = 1U << Pin_SPI_FLASH_CLK_U;
//		__asm volatile( "nop" );
		if ((c & 0x80) == 0U)
		{
			GPIO_SPI_FLASH_DO_U->PCOR = 1U << Pin_SPI_FLASH_DO_U;// Hopefully the compiler will optimise this to a value rather than using a shift
		}
		else
		{
			GPIO_SPI_FLASH_DO_U->PSOR = 1U << Pin_SPI_FLASH_DO_U;// Hopefully the compiler will optimise this to a value rather than using a shift
		}
//		__asm volatile( "nop" );
		c <<= 1;
		c |= (GPIO_SPI_FLASH_DI_U->PDIR >> Pin_SPI_FLASH_DI_U) & 0x01U;
		// toggle the clock
		GPIO_SPI_FLASH_CLK_U->PSOR = 1U << Pin_SPI_FLASH_CLK_U;
//		__asm volatile( "nop" );

	}
	return c;
}

static void spi_flash_transfer_buf(uint8_t *inBuf, uint8_t *outBuf, int size)
{
	while(size-- > 0)
	{
		*outBuf++ = spi_flash_transfer(*inBuf++);
	}
}

static bool spi_flash_busy(void)
{
	uint8_t r1;

	spi_flash_enable();
	spi_flash_transfer(R_SR1);
	r1 = spi_flash_transfer(0xff);
	spi_flash_disable();

	if(r1 & SR1_BUSY_MASK)
	{
		return true;
	}
	return false;
}

static void spi_flash_setWriteEnable(bool cmd)
{
	spi_flash_enable();
	spi_flash_transfer(cmd ? W_EN : W_DE);
	spi_flash_disable();
}
