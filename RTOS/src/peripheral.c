/*
 * peripheral.c
 *
 *  Created on: Nov 4, 2021
 *      Author: Sarker Nadir Afridi Azmi
 */

#include "peripheral.h"

void initSysTick(uint32_t loadValue)
{
    NVIC_ST_CTRL_R |= NVIC_ST_CTRL_CLK_SRC | NVIC_ST_CTRL_INTEN | NVIC_ST_CTRL_ENABLE;
    NVIC_ST_RELOAD_R = loadValue & 0x00FFFFFF;
}

void initLedPb()
{
    enablePort(PORTA);
    enablePort(PORTB);
    enablePort(PORTC);
    enablePort(PORTD);
    enablePort(PORTE);
    enablePort(PORTF);

    // PB2 is connected to the anode of the LED's
    selectPinPushPullOutput(PORTB, 2);
    setPinValue(PORTB, 2, 1);

    selectPinPushPullOutput(PORTA, 2);
    selectPinPushPullOutput(PORTA, 3);
    selectPinPushPullOutput(PORTA, 4);
    selectPinPushPullOutput(PORTE, 0);
    selectPinPushPullOutput(PORTF, 2);

    setPinCommitControl(PORTD, 7);

    // Configure all the push buttons pins to be inputs
    selectPinDigitalInput(PORTC, 4);
    selectPinDigitalInput(PORTC, 5);
    selectPinDigitalInput(PORTC, 6);
    selectPinDigitalInput(PORTC, 7);
    selectPinDigitalInput(PORTD, 6);
    selectPinDigitalInput(PORTD, 7);

    // Configure pull-ups for all the push buttons
    enablePinPullup(PORTC, 4);
    enablePinPullup(PORTC, 5);
    enablePinPullup(PORTC, 6);
    enablePinPullup(PORTC, 7);
    enablePinPullup(PORTD, 6);
    enablePinPullup(PORTD, 7);
}
