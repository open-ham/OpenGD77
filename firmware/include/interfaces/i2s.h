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

#ifndef _OPENGD77_I2S_H_
#define _OPENGD77_I2S_H_

#include "fsl_port.h"
#include "fsl_dmamux.h"
#include "fsl_edma.h"
#include "fsl_sai.h"
#include "fsl_sai_edma.h"

#include "functions/sound.h"

#define NUM_I2S_BUFFERS 4


#if defined(PLATFORM_GD77) || defined(PLATFORM_GD77S)

// I2S to C6000 (I2S)
// OUT/ON  A16 - I2S FS to C6000
// OUT/OFF A14 - I2S CK to C6000
// OUT/ON  A12 - I2S RX to C6000
// IN      A15 - I2S TX to C6000
#define Port_I2S_FS_C6000    PORTA
#define GPIO_I2S_FS_C6000    GPIOA
#define Pin_I2S_FS_C6000     16
#define Port_I2S_CK_C6000    PORTA
#define GPIO_I2S_CK_C6000    GPIOA
#define Pin_I2S_CK_C6000     14
#define Port_I2S_RX_C6000    PORTA
#define GPIO_I2S_RX_C6000    GPIOA
#define Pin_I2S_RX_C6000     12
#define Port_I2S_TX_C6000    PORTA
#define GPIO_I2S_TX_C6000    GPIOA
#define Pin_I2S_TX_C6000     15

#elif defined(PLATFORM_DM1801)

// I2S to C6000 (I2S)
// OUT/ON  A16 - I2S FS to C6000
// OUT/OFF A14 - I2S CK to C6000
// OUT/ON  A12 - I2S RX to C6000
// IN      A15 - I2S TX to C6000
#define Port_I2S_FS_C6000    PORTA
#define GPIO_I2S_FS_C6000    GPIOA
#define Pin_I2S_FS_C6000     16
#define Port_I2S_CK_C6000    PORTA
#define GPIO_I2S_CK_C6000    GPIOA
#define Pin_I2S_CK_C6000     14
#define Port_I2S_RX_C6000    PORTA
#define GPIO_I2S_RX_C6000    GPIOA
#define Pin_I2S_RX_C6000     12
#define Port_I2S_TX_C6000    PORTA
#define GPIO_I2S_TX_C6000    GPIOA
#define Pin_I2S_TX_C6000     15

#elif defined(PLATFORM_RD5R)

// I2S to C6000 (I2S)
// OUT/ON  A16 - I2S FS to C6000
// OUT/OFF A14 - I2S CK to C6000
// OUT/ON  A12 - I2S RX to C6000
// IN      A15 - I2S TX to C6000
#define Port_I2S_FS_C6000    PORTA
#define GPIO_I2S_FS_C6000    GPIOA
#define Pin_I2S_FS_C6000     16
#define Port_I2S_CK_C6000    PORTA
#define GPIO_I2S_CK_C6000    GPIOA
#define Pin_I2S_CK_C6000     14
#define Port_I2S_RX_C6000    PORTA
#define GPIO_I2S_RX_C6000    GPIOA
#define Pin_I2S_RX_C6000     12
#define Port_I2S_TX_C6000    PORTA
#define GPIO_I2S_TX_C6000    GPIOA
#define Pin_I2S_TX_C6000     15

#endif


extern volatile bool g_TX_SAI_in_use;
extern sai_edma_handle_t g_SAI_TX_Handle;
extern sai_edma_handle_t g_SAI_RX_Handle;

void init_I2S(void);
void setup_I2S(void);
void I2SReset(void);
void I2STerminateTransfers(void);
void I2STransferReceive(uint8_t *buff,size_t bufferLen);
void I2STransferTransmit(uint8_t *buff,size_t bufferLen);

#endif /* _OPENGD77_I2S_H_ */
