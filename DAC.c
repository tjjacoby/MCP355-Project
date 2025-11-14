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
  // At this stage the system clock should have already been configured
  // at high speed.

  // Infinite loop
  while (1)
    {
       // Add your code here.
    }
  	  //
}

void init_GPIOA()
{
	//Config port PA1 (ACD_IN1) as an analog mode pin
}
void init_DAC()
{
	/*	DAC control register (DAC_CR)
	 * 	BIT[0]: EN1
	 *  BIT[1]: BOFF1
	 *  BIT[2]: TEN1
	*/

	DAC -> DHR12R1 = 0x00;
	
}
void init_ADC()
{
	/* ADC interrupt and status register (ADC_ISR)
	*	Bit [0]: ADRDY (ADC ready flag)
	*		ADRDY = 0/1: ADC is not/is ready to start conversion
			• Hardware sets this bit after ADC is enabled (ADEN = 1) and when ADC becomes ready to accept conversion requests
			• NOTE: Your software will need to wait for ADRDY = 1 before trying to start the conversion process
	*	Bit [1]: EOSMP (end of sampling flag)
	*	Bit [2]: EOC (end of conversion flag, after sampling)
	*		 EOC = 0/1: channel conversion is not/is complete
			• Hardware sets this bit at the end of each conversion of a channel when a new result is available in ADC_DR
			• Hardware clears this bit when ADC_DR is read
			• Software can clear this bit by writing 1 to it
	*
	*/
}


#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
