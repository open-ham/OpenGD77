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

#include "interfaces/i2s.h"

#define I2S_DMA_TX 0
#define I2S_DMA_RX 1

edma_handle_t g_EDMA_TX_Handle;
edma_handle_t g_EDMA_RX_Handle;

sai_edma_handle_t g_SAI_TX_Handle;
sai_edma_handle_t g_SAI_RX_Handle;
volatile bool g_TX_SAI_in_use = false;

void I2STransferTransmit(uint8_t *buff,size_t bufferLen)
{
	sai_transfer_t xfer =
	{
			.data = buff,
			.dataSize = bufferLen
	};

	SAI_TransferSendEDMA(I2S0, &g_SAI_TX_Handle, &xfer);
	g_TX_SAI_in_use = true;
}

void I2STransferReceive(uint8_t *buff,size_t bufferLen)
{
	sai_transfer_t xfer =
	{
			.data = buff,
			.dataSize = bufferLen
	};

	SAI_TransferReceiveEDMA(I2S0, &g_SAI_RX_Handle, &xfer);
}

void I2SReset(void)
{
    g_TX_SAI_in_use = false;
    SAI_TxSoftwareReset(I2S0, kSAI_ResetAll);
	SAI_TxEnable(I2S0, true);
    SAI_RxSoftwareReset(I2S0, kSAI_ResetAll);
	SAI_RxEnable(I2S0, true);
}

void I2STerminateTransfers(void)
{
    SAI_TransferTerminateSendEDMA(I2S0, &g_SAI_TX_Handle);
    SAI_TransferTerminateSendEDMA(I2S0, &g_SAI_RX_Handle);
}

void init_I2S(void)
{
    /* PORTA16 is configured as I2S_FS*/
    PORT_SetPinMux(Port_I2S_FS_C6000, Pin_I2S_FS_C6000, kPORT_MuxAlt6);

    /* PORTA14 is configured as I2S_CK */
    PORT_SetPinMux(Port_I2S_CK_C6000, Pin_I2S_CK_C6000, kPORT_MuxAlt6);

    /* PORTA12 is configured as I2S_RX */
    PORT_SetPinMux(Port_I2S_RX_C6000, Pin_I2S_RX_C6000, kPORT_MuxAlt6);

    /* PORTA15 is configured as I2S_TX */
    PORT_SetPinMux(Port_I2S_TX_C6000, Pin_I2S_TX_C6000, kPORT_MuxAlt6);

    NVIC_SetPriority(I2S0_Tx_IRQn, 3);
    NVIC_SetPriority(I2S0_Rx_IRQn, 3);
}

void SAI_TX_Callback(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData)
{
	g_TX_SAI_in_use = false;
	soundSendData();
}

void SAI_RX_Callback(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData)
{
	soundReceiveData();
}

void setup_I2S(void)
{
	sai_config_t s_TxConfig;
	sai_config_t s_RxConfig;

	edma_config_t edma_config;

    SAI_TxGetDefaultConfig(&s_TxConfig);
    SAI_RxGetDefaultConfig(&s_RxConfig);

    SAI_TxInit(I2S0, &s_TxConfig);
    SAI_RxInit(I2S0, &s_RxConfig);

	I2S0->TCR1 = 0x00000006;
	I2S0->TCR2 = 0x42000000;
	I2S0->TCR3 = 0x00010000;
	I2S0->TCR4 = 0x00011F18;
	I2S0->TCR5 = 0x1F1F0F00;
	I2S0->TMR  = 0x00000000;
	I2S0->TCSR = 0x00000C01;

	I2S0->RCR1 = 0x00000006;
	I2S0->RCR2 = 0x02000000;
	I2S0->RCR3 = 0x00010000;
	I2S0->RCR4 = 0x00011F18;
	I2S0->RCR5 = 0x1F1F0F00;
	I2S0->RMR  = 0x00000000;
	I2S0->RCSR = 0x00000C01;

	DMAMUX_Init(DMAMUX0);
    DMAMUX_SetSource(DMAMUX0, I2S_DMA_TX, 0b001101); // 0b001101..I2S0_Tx_Signal
    DMAMUX_SetSource(DMAMUX0, I2S_DMA_RX, 0b001100); // 0b001100..I2S0_Rx_Signal
    DMAMUX_EnableChannel(DMAMUX0, I2S_DMA_TX);
    DMAMUX_EnableChannel(DMAMUX0, I2S_DMA_RX);

	EDMA_GetDefaultConfig(&edma_config);
	EDMA_Init(DMA0, &edma_config);
    EDMA_CreateHandle(&g_EDMA_TX_Handle, DMA0, I2S_DMA_TX);
    EDMA_CreateHandle(&g_EDMA_RX_Handle, DMA0, I2S_DMA_RX);

    SAI_TransferTxCreateHandleEDMA(I2S0, &g_SAI_TX_Handle, SAI_TX_Callback, NULL, &g_EDMA_TX_Handle);
    SAI_TransferRxCreateHandleEDMA(I2S0, &g_SAI_RX_Handle, SAI_RX_Callback, NULL, &g_EDMA_RX_Handle);

    sai_transfer_format_t SAI_TX_format;
    SAI_TX_format.sampleRate_Hz = kSAI_SampleRate8KHz;
    SAI_TX_format.bitWidth = kSAI_WordWidth16bits;
    SAI_TX_format.stereo = kSAI_Stereo;
    SAI_TX_format.masterClockHz = 512 * SAI_TX_format.sampleRate_Hz;
    SAI_TX_format.watermark = NUM_I2S_BUFFERS;
    SAI_TX_format.channel = 0;
    SAI_TX_format.protocol = kSAI_BusI2S;
    SAI_TransferTxSetFormatEDMA(I2S0, &g_SAI_TX_Handle, &SAI_TX_format, SAI_TX_format.masterClockHz, SAI_TX_format.masterClockHz);

    sai_transfer_format_t SAI_RX_format;
    SAI_RX_format.sampleRate_Hz = kSAI_SampleRate8KHz;
    SAI_RX_format.bitWidth = kSAI_WordWidth16bits;
    SAI_RX_format.stereo = kSAI_Stereo;
    SAI_RX_format.masterClockHz = 512 * SAI_RX_format.sampleRate_Hz;
    SAI_RX_format.watermark = NUM_I2S_BUFFERS;
    SAI_RX_format.channel = 1;
    SAI_RX_format.protocol = kSAI_BusI2S;
    SAI_TransferTxSetFormatEDMA(I2S0, &g_SAI_RX_Handle, &SAI_RX_format, SAI_RX_format.masterClockHz, SAI_RX_format.masterClockHz);
}
