//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------
// School: University of Victoria, Canada.
// Course: ECE 355 "Microprocessor-Based Systems".
// This is template code for Part 2 of Introductory Lab.
//
// See "system/include/cmsis/stm32f051x8.h" for register/bit definitions.
// See "system/src/cmsis/vectors_stm32f051x8.c" for handler declarations.
// ----------------------------------------------------------------------------

#include <stdio.h>
#include "diag/Trace.h"
#include "cmsis/cmsis_device.h"

// ----------------------------------------------------------------------------
//
// STM32F0 empty sample (trace via $(trace)).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the $(trace) output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"


/* Definitions of registers and their bits are
   given in system/include/cmsis/stm32f051x8.h */


/* Clock prescaler for TIM2 timer: no prescaling */
#define myTIM2_PRESCALER ((uint16_t)0x0000)
/* Maximum possible setting for overflow */
#define myTIM2_PERIOD ((uint32_t)0xFFFFFFFF)

void myGPIOB_Init(void);
void myTIM2_Init(void);
void myEXTI_Init(void);


// Declare/initialize your global variables here...
// NOTE: You'll need at least one global variable
// (say, timerTriggered = 0 or 1) to indicate
// whether TIM2 has started counting or not.
int timerTriggered = 0;
//int edge_counter =0;
int counter = 0;


/*** Call this function to boost the STM32F0xx clock to 48 MHz ***/

void SystemClock48MHz( void )
{
//
// Disable the PLL
//
    RCC->CR &= ~(RCC_CR_PLLON);
//
// Wait for the PLL to unlock
//
    while (( RCC->CR & RCC_CR_PLLRDY ) != 0 );
//
// Configure the PLL for 48-MHz system clock
//
    RCC->CFGR = 0x00280000;
//
// Enable the PLL
//
    RCC->CR |= RCC_CR_PLLON;
//
// Wait for the PLL to lock
//
    while (( RCC->CR & RCC_CR_PLLRDY ) != RCC_CR_PLLRDY );
//
// Switch the processor to the PLL clock source
//
    RCC->CFGR = ( RCC->CFGR & (~RCC_CFGR_SW_Msk)) | RCC_CFGR_SW_PLL;
//
// Update the system with the new clock frequency
//
    SystemCoreClockUpdate();

}

/*****************************************************************/


int
main(int argc, char* argv[])
 {

	SystemClock48MHz();

	trace_printf("This is Part 2 of Introductory Lab...\n");
	trace_printf("System clock: %u Hz\n", SystemCoreClock);

        RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN;  /* Enable SYSCFG clock */

	myGPIOB_Init();		/* Initialize I/O port PB */
	myTIM2_Init();		/* Initialize timer TIM2 */
	myEXTI_Init();		/* Initialize EXTI */
	while (1)
	{
		// Nothing is going on here...
	}

	return 0;

}


void myGPIOB_Init()
{
	/* Enable clock for GPIOB peripheral */
	// Relevant register: RCC->AHBENR
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN; //_Msk


	/* Configure PB2 as input */
	// Relevant register: GPIOB->MODER
	GPIOB-> MODER &= ~(GPIO_MODER_MODER2);

	/* Ensure no pull-up/pull-down for PB2 */
	// Relevant register: GPIOB->PUPDR
	GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR2);
}


void myTIM2_Init()
{
	/* Enable clock for TIM2 peripheral */
	// Relevant register: RCC->APB1ENR

	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;


	/* Configure TIM2: buffer auto-reload, count up, stop on overflow,
	 * enable update events, interrupt on overflow only */
	// Relevant register: TIM2->CR1
	TIM2->CR1 = ((uint16_t) 0x008C);

	/* Set clock prescaler value */
	TIM2->PSC = myTIM2_PRESCALER;
	/* Set auto-reloaded delay */
	TIM2->ARR = myTIM2_PERIOD;

	/* Update timer registers */
	// Relevant register: TIM2->EGR
	TIM2->EGR = ((uint16_t) 0x001);

	/* Assign TIM2 interrupt priority = 0 in NVIC */
	// Relevant register: NVIC->IP[3], or use NVIC_SetPriority
	NVIC_SetPriority(TIM2_IRQn, 0);

	/* Enable TIM2 interrupts in NVIC */
	// Relevant register: NVIC->ISER[0], or use NVIC_EnableIRQ
	NVIC_EnableIRQ(TIM2_IRQn);

	/* Enable update interrupt generation */
	// Relevant register: TIM2->DIER
	TIM2->DIER |= TIM_DIER_UIE;

}


void myEXTI_Init()
{

	/* Map EXTI2 line to PB2 */
	// Relevant register: SYSCFG->EXTICR[0]
	SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI2_PB; //MIGHT BE WRONG

	/* EXTI2 line interrupts: set rising-edge trigger */
	// Relevant register: EXTI->RTSR
	EXTI->RTSR |= EXTI_RTSR_TR2;

	/* Unmask interrupts from EXTI2 line */
	// Relevant register: EXTI->IMR
	EXTI->IMR |= EXTI_IMR_MR2;

	/* Assign EXTI2 interrupt priority = 0 in NVIC */
	// Relevant register: NVIC->IP[2], or use NVIC_SetPriority
	NVIC_SetPriority(EXTI2_3_IRQn, 0);

	/* Enable EXTI2 interrupts in NVIC */
	// Relevant register: NVIC->ISER[0], or use NVIC_EnableIRQ
	NVIC_EnableIRQ(EXTI2_3_IRQn);
}


/* This handler is declared in system/src/cmsis/vectors_stm32f051x8.c */
/* Global variable to count overflows */
uint32_t overflow_count = 0;

void TIM2_IRQHandler()
{
    /* Check if update interrupt flag is set */
    if ((TIM2->SR & TIM_SR_UIF) != 0)
    {
        trace_printf("\n*** Timer Overflow! ***\n");
        overflow_count++; /* Increment overflow counter */
        //Clear update interrupt flag reset timer
        TIM2->SR &= ~(TIM_SR_UIF);
        TIM2->CR1 |= TIM_CR1_CEN;

    }
}

void EXTI2_3_IRQHandler()
{
    /* Check if EXTI2 interrupt pending flag is set */
    if ((EXTI->PR & EXTI_PR_PR2) != 0)
    {
        if (timerTriggered == 0)
        {
            /* First edge: start timer */
            TIM2->CNT = 0x00; /* Clear counter */
            overflow_count = 0; /* Reset overflow counter */
            TIM2->CR1 |= TIM_CR1_CEN; /* Start timer */
            timerTriggered = 1;
        }
        else
        {
            /* Second edge: stop timer and calculate period */
            TIM2->CR1 &= ~(TIM_CR1_CEN); /* Stop timer */
            uint32_t count = TIM2->CNT; /* Read counter */

            uint64_t total_ticks = ((uint64_t)overflow_count * 0xFFFFFFFF) + count; /* Account for overflows */

            // Calculate period in ms
            float period_float = ((1.0f / 48000000.0f) * total_ticks) * 1000.0f;
            // Calculate frequency in cHz (centiHertz)
            float freq_float = 0;
            if(period_float > 0)
            {
            	freq_float = (1.0f / (period_float));
            }

            /* Convert to unsigned int for printing */
            uint32_t period = (uint32_t)period_float; /* Period in microseconds */
            uint32_t freq = (uint32_t)(freq_float*10000000); /* Frequency in kHz */

            /* Print results (correct unit: microseconds) */
            trace_printf("PERIOD: %u ms\nFREQ: %u kHz\n", period, freq);

            timerTriggered = 0; /* Reset for next measurement */
        }

        /* Clear EXTI2 interrupt pending flag */
        EXTI->PR |= EXTI_PR_PR2;
    }
}


#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
/*
 * MAX PERIOD:
 * Time for 1 over flow: 32 bit int so:  2^32 / 48 MHZ => 89.45 secs
 * Now how many overflows can we have => 32 bit int store the counter so (89.45 secs * 2^32) This is max period
 * BUT! that number cant fit into our 32bit period, so max value for period is 2^32 ms
 *
 * MIN PERIOD: One tick 1/48MHZ given no latency
 * MAX VAULE FOR FREQ IS 2^32 so min period =  1/2^32 ms
 * We foudn this to be around 2.49 us
 *
 * */
