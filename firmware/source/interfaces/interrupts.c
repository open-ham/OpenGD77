/*
 * Copyright (C)2020 Roger Clark VK3KYY
 *
 */
#include <stdbool.h>
#include <stdint.h>
#include "interfaces/gpio.h"

void interruptsInitC6000Interface(void)
{
    PORT_SetPinInterruptConfig(Port_INT_C6000_SYS, Pin_INT_C6000_SYS, kPORT_InterruptFallingEdge);
    PORT_SetPinInterruptConfig(Port_INT_C6000_TS, Pin_INT_C6000_TS, kPORT_InterruptFallingEdge);
    PORT_SetPinInterruptConfig(Port_INT_C6000_RF_RX, Pin_INT_C6000_RF_RX, kPORT_InterruptFallingEdge);
    PORT_SetPinInterruptConfig(Port_INT_C6000_RF_TX, Pin_INT_C6000_RF_TX, kPORT_InterruptFallingEdge);

    NVIC_SetPriority(PORTC_IRQn, 3);
}

bool interruptsWasPinTriggered(PORT_Type *port,uint32_t pin)
{
	return (1U << pin) & PORT_GetPinsInterruptFlags(port);
}

void interruptsClearPinFlags(PORT_Type *port, uint32_t pin)
{
    PORT_ClearPinsInterruptFlags(port, (1U << pin));
}
