/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
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
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
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
//
// STM32F0 empty sample (trace via DEBUG).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the DEBUG output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace-impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

/*
 *
16 ADC channels: ADC_IN0, …, ADC_IN15
ADC_IN0, ADC_IN1, …, ADC_IN7 ->PA0 – PA7
ADC_IN8, ADC_IN9 -> PB0, PB1
ADC_IN10, ADC_IN11, …, ADC_IN15 -> PC0 – PC5
Configure PA1 (ADC_IN1) as an analog mode pin
Note: Use ADC1->RegName to access ADC registers
 *
 *
 *
 * */

void init_GPIOA();
void init_DAC();
void init_ADC();

int main(int argc, char* argv[])
{
	// Initialize peripherals
	init_GPIOA();
	init_DAC();
	init_ADC();

	// At this stage the system clock should have already been configured
	// at high speed.

	// Infinite loop
	while (1)
	{

	}
}

void init_GPIOA()
{
	// Enable GPIOA clock
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

	// Enable ADC clock
	RCC->APB2ENR |= RCC_APB2ENR_ADCEN;

	// Enable DAC clock
	RCC->APB1ENR |= RCC_APB1ENR_DACEN;

	// Configure PA1 (ADC_IN1) as analog mode (MODER bits [3:2] = 11)
	GPIOA->MODER |= (3U << 2);

	// Configure PA4 (DAC_OUT1) as analog mode (MODER bits [9:8] = 11)
	GPIOA->MODER |= (3U << 8);
}

void init_DAC()
{
	/* DAC control register (DAC_CR)
	 * Bit[0]: EN1 - Enable DAC channel 1
	 * Bit[1]: BOFF1 - Buffer off
	 * Bit[2]: TEN1 - Trigger enable
	 */

	// Initialize DAC data holding register to 0
	DAC->DHR12R1 = 0x00;

	// Enable DAC channel 1
	// EN1 = 1 (bit 0), BOFF1 = 1 (bit 1), TEN1 = 0 (bit 2)
	DAC->CR |= (1U << 0);  // Enable channel 1
	DAC->CR |= (1U << 1);  // Disable output buffer (set BOFF1)
}

void init_ADC()
{
	/* ADC interrupt and status register (ADC_ISR)
	 * Bit [0]: ADRDY (ADC ready flag)
	 *     ADRDY = 0/1: ADC is not/is ready to start conversion
	 *     Hardware sets this bit after ADC is enabled (ADEN = 1) and when
	 *     ADC becomes ready to accept conversion requests
	 *     NOTE: Your software will need to wait for ADRDY = 1 before
	 *     trying to start the conversion process
	 * Bit [1]: EOSMP (end of sampling flag)
	 * Bit [2]: EOC (end of conversion flag, after sampling)
	 *     EOC = 0/1: channel conversion is not/is complete
	 *     Hardware sets this bit at the end of each conversion of a channel
	 *     when a new result is available in ADC_DR
	 *     Hardware clears this bit when ADC_DR is read
	 *     Software can clear this bit by writing 1 to it
	 */

	// Make sure ADC is not already enabled
	if (ADC1->CR & ADC_CR_ADEN) {
		ADC1->CR |= ADC_CR_ADDIS;  // Disable ADC if enabled
		while (ADC1->CR & ADC_CR_ADEN);  // Wait until disabled
	}

	// Configure ADC before enabling it

	// Set data resolution to 12 bits (RES[1:0] = 00 in ADC_CFGR1)
	ADC1->CFGR1 &= ~ADC_CFGR1_RES;  // Clear RES bits

	// Set right alignment (ALIGN = 0 in ADC_CFGR1)
	ADC1->CFGR1 &= ~ADC_CFGR1_ALIGN;  // Clear ALIGN bit

	// Set continuous conversion mode (CONT = 1 in ADC_CFGR1)
	ADC1->CFGR1 |= ADC_CFGR1_CONT;  // Set CONT bit

	// Preserve ADC_DR on overrun (OVRMOD = 0 in ADC_CFGR1)
	ADC1->CFGR1 &= ~ADC_CFGR1_OVRMOD;  // Clear OVRMOD bit

	// Select channel 1 (ADC_IN1 = PA1)
	ADC1->CHSELR = ADC_CHSELR_CHSEL1;  // Select channel 1

	// Set sampling time (example: 239.5 ADC clock cycles, SMP = 111)
	ADC1->SMPR |= (7U << 0);  // Set SMP[2:0] = 111

	// Enable the ADC
	ADC1->CR |= ADC_CR_ADEN;  // Set ADEN bit

	// Wait until ADC is ready (ADRDY = 1)
	while (!(ADC1->ISR & ADC_ISR_ADRDY));  // Wait for ADRDY

	// Start ADC conversion
	ADC1->CR |= ADC_CR_ADSTART;  // Set ADSTART bit
}


#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
