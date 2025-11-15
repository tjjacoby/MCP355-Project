/*
 * This file is part of the µOS++ distribution.
 *  [](https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include "diag/trace.h"
#include "stm32f0xx.h"

// ----------------------------------------------------------------------------

void init_GPIOA(void);
void init_ADC(void);
void init_DAC(void);

int main(int argc, char* argv[])

{

    init_GPIOA();
    init_DAC();      // <-- DAC enabled again
    init_ADC();

    while (1)
    {
        /* Wait for End-Of-Conversion */
        while (!(ADC1->ISR & ADC_ISR_EOC)) { }

        /* Read the 12-bit ADC result */
        uint16_t pot_value = (uint16_t)ADC1->DR;
        uint16_t voltage = (pot_value*3000)/4095;

        /* Print it via the debug trace */

        trace_printf("POT Value: %d\n", voltage);

        DAC->DHR12R1 = pot_value;
    }
}


void init_GPIOA(void)
{
    /* Enable GPIOA clock */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    /* Enable ADC clock */
    RCC->APB2ENR |= RCC_APB2ENR_ADCEN;

    /* Enable DAC clock */
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;

    /* PA1 → analog (MODER[3:2] = 11) */
    GPIOA->MODER |= (3U << 2);

    /* PA4 → analog (MODER[9:8] = 11) */
    GPIOA->MODER |= (3U << 8);
}

/* -------------------------------------------------------------------------
 * DAC – channel 1, buffer disabled, no trigger
 * ------------------------------------------------------------------------- */
void init_DAC(void)
{
    /* Make sure DAC is disabled before configuring */
    if (DAC->CR & DAC_CR_EN1) {
        DAC->CR &= ~DAC_CR_EN1;
    }

    /* Initialise output to 0 */
    DAC->DHR12R1 = 0;

    /* EN1 = 1   (enable channel 1)
     * BOFF1 = 1 (disable output buffer → direct 12-bit output)
     * TEN1 = 0  (no hardware trigger)                                   */
    DAC->CR |= DAC_CR_EN1;      // enable
    DAC->CR |= DAC_CR_BOFF1;    // buffer off
}

/* -------------------------------------------------------------------------
 * ADC – 12-bit, continuous, channel 1 (PA1)
 * ------------------------------------------------------------------------- */
void init_ADC(void)
{
    /* Disable ADC if it was already on */
    if (ADC1->CR & ADC_CR_ADEN) {
        ADC1->CR |= ADC_CR_ADDIS;
        while (ADC1->CR & ADC_CR_ADEN) { }
    }

    /* ---- Configuration --------------------------------------------------- */
    ADC1->CFGR1 &= ~ADC_CFGR1_RES;      // 12-bit resolution
    ADC1->CFGR1 &= ~ADC_CFGR1_ALIGN;    // right alignment
    ADC1->CFGR1 |=  ADC_CFGR1_CONT;     // continuous mode
    ADC1->CFGR1 &= ~ADC_CFGR1_OVRMOD;   // keep old data on overrun

    /* Channel selection – only CH1 */
    ADC1->CHSELR = 0;
    ADC1->CHSELR |= ADC_CHSELR_CHSEL1;

    /* Sampling time – 239.5 ADC cycles (SMP = 111) */
    ADC1->SMPR |= (7U << 0);

    /* ---- Enable ---------------------------------------------------------- */
    ADC1->CR |= ADC_CR_ADEN;            // enable ADC
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { }   // wait for ready

    /* Start the first conversion */
    ADC1->CR |= ADC_CR_ADSTART;
}

/* -------------------------------------------------------------------------
 * End of file
 * ------------------------------------------------------------------------- */
