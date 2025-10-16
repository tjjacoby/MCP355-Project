#include <stdio.h>
#include "diag/Trace.h"
#include "cmsis/cmsis_device.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

/* Clock prescaler for TIM2 timer: no prescaling */
#define myTIM2_PRESCALER ((uint16_t)0x0000)
/* Delay count for TIM2 timer: 1/4 sec at 48 MHz */
#define myTIM2_PERIOD ((uint32_t)12000000)

// <<< DEFINES FOR DEBOUNCE TIMER (TIM3)
/* Prescaler for a 1kHz timer clock (48MHz / (47999 + 1)) */
#define myTIM3_PRESCALER ((uint16_t)47999)
/* 50ms debounce delay (50 * 1ms) */
#define myTIM3_PERIOD ((uint16_t)50)


void myGPIOA_Init(void);
void myGPIOC_Init(void);
void myTIM2_Init(void);
void myTIM3_Init(void); // <<< ADDED: Prototype for debounce timer init
void myEXTI_Init(void);

/* Global variable indicating which LED is blinking */
volatile uint16_t blinkingLED = ((uint16_t)0x0100);

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

int main(int argc, char* argv[])
{
    SystemClock48MHz();
    //trace_printf("System clock: %u Hz\n", SystemCoreClock);

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN; /* Enable SYSCFG clock */

    myGPIOA_Init();  /* Initialize I/O port PA */
    myGPIOC_Init();  /* Initialize I/O port PC */
    myTIM2_Init();   /* Initialize timer TIM2 for blinking */
    myTIM3_Init();   // <<< ADDED: Initialize TIM3 for debouncing
    myEXTI_Init();   /* Initialize EXTI for PA0 */

    while (1)
    {
        /* All work is done in interrupts */
    }

    return 0;
}

void myGPIOA_Init()
{
    /* Enable clock for GPIOA peripheral */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    /* Configure PA0 as input */
    GPIOA->MODER &= ~(GPIO_MODER_MODER0);

    /* Ensure no pull-up/pull-down for PA0 (or use pull-down if needed) */
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR0);
    /* Optional: Add pull-down to avoid floating input */
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR0_1; // Pull-down
}

void myGPIOC_Init()
{
    /* Enable clock for GPIOC peripheral */
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    /* Configure PC8 and PC9 as outputs */
    GPIOC->MODER |= (GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0);
    /* Ensure push-pull mode selected for PC8 and PC9 */
    GPIOC->OTYPER &= ~(GPIO_OTYPER_OT_8 | GPIO_OTYPER_OT_9);
    /* Ensure high-speed mode for PC8 and PC9 */
    GPIOC->OSPEEDR |= (GPIO_OSPEEDER_OSPEEDR8 | GPIO_OSPEEDER_OSPEEDR9);
    /* Ensure no pull-up/pull-down for PC8 and PC9 */
    GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPDR8 | GPIO_PUPDR_PUPDR9);
}

void myTIM2_Init()
{
    /* Enable clock for TIM2 peripheral */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* Configure TIM2: buffer auto-reload, count up, stop on overflow,
     * enable update events, interrupt on overflow only */
    TIM2->CR1 = ((uint16_t)0x008C);

    /* Set clock prescaler value */
    TIM2->PSC = myTIM2_PRESCALER;
    /* Set auto-reloaded delay */
    TIM2->ARR = myTIM2_PERIOD;

    /* Update timer registers */
    TIM2->EGR = ((uint16_t)0x0001);

    /* Assign TIM2 interrupt priority = 0 in NVIC */
    NVIC_SetPriority(TIM2_IRQn, 0);

    /* Enable TIM2 interrupts in NVIC */
    NVIC_EnableIRQ(TIM2_IRQn);

    /* Enable update interrupt generation */
    TIM2->DIER |= TIM_DIER_UIE;
    /* Start counting timer pulses */
    TIM2->CR1 |= TIM_CR1_CEN;
}

// <<< ADDED: New function to initialize the debounce timer TIM3
void myTIM3_Init()
{
    /* Enable clock for TIM3 peripheral */
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    /* Configure TIM3 for one-shot mode for debouncing */
    TIM3->CR1 = TIM_CR1_OPM; // One-Pulse Mode

    /* Set clock prescaler value */
    TIM3->PSC = myTIM3_PRESCALER;
    /* Set auto-reloaded delay */
    TIM3->ARR = myTIM3_PERIOD;

    /* Enable update interrupt generation */
    TIM3->DIER |= TIM_DIER_UIE;

    /* Assign TIM3 interrupt priority = 2 (lower than EXTI) in NVIC */
    NVIC_SetPriority(TIM3_IRQn, 2);

    /* Enable TIM3 interrupts in NVIC */
    NVIC_EnableIRQ(TIM3_IRQn);
}


void myEXTI_Init()
{
    /* Map PA0 to EXTI0 */
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0; /* Clear EXTI0 configuration */
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PA; /* Set PA0 as EXTI0 source */

    /* Enable EXTI0 interrupt */
    EXTI->IMR |= EXTI_IMR_MR0; /* Unmask EXTI0 */
    EXTI->RTSR |= EXTI_RTSR_TR0; /* Trigger on rising edge */

    /* Configure NVIC for EXTI0 */
    NVIC_SetPriority(EXTI0_1_IRQn, 1); /* Set priority (lower than TIM2) */
    NVIC_EnableIRQ(EXTI0_1_IRQn); /* Enable EXTI0_1 interrupt */
}

/* EXTI0 interrupt handler - Triggered on initial button press */
// <<< MODIFIED: This ISR is now much shorter
void EXTI0_1_IRQHandler(void)
{
    /* Check if EXTI0 interrupt flag is set */
    if (EXTI->PR & EXTI_PR_PR0)
    {
        // <<< 1. Disable this interrupt to prevent bounces
        EXTI->IMR &= ~EXTI_IMR_MR0;

        // <<< 2. Clear the interrupt flag
        EXTI->PR |= EXTI_PR_PR0;

        // <<< 3. Start the one-shot debounce timer
        TIM3->CR1 |= TIM_CR1_CEN;
    }
}

// <<< ADDED: New ISR for the debounce timer
/* TIM3 interrupt handler - Triggered after debounce delay */
void TIM3_IRQHandler(void)
{
    // Check if update interrupt flag is set
    if ((TIM3->SR & TIM_SR_UIF) != 0)
    {
        // Clear the interrupt flag
        TIM3->SR &= ~(TIM_SR_UIF);

        // <<< 4. Verify the button is still pressed after the delay
        if ((GPIOA->IDR & GPIO_IDR_0) != 0)
        {
            // <<< 5. It was a real press! Execute the logic.
            GPIOC->BRR = blinkingLED; /* Turn off currently blinking LED */
            blinkingLED ^= ((uint16_t)0x0300); /* Switch blinking LED */
            GPIOC->BSRR = blinkingLED; /* Turn on switched LED */
            trace_printf("\nSwitching the blinking LED...\n");
        }

        // <<< 6. Re-enable the EXTI interrupt for the next press
        EXTI->IMR |= EXTI_IMR_MR0;
    }
}


/* TIM2 interrupt handler (unchanged) */
void TIM2_IRQHandler()
{
    uint16_t LEDstate;

    /* Check if update interrupt flag is set */
    if ((TIM2->SR & TIM_SR_UIF) != 0)
    {
        /* Read current PC output and isolate PC8 and PC9 bits */
        LEDstate = GPIOC->ODR & ((uint16_t)0x0300);
        if (LEDstate == 0) /* If LED is off, turn it on... */
        {
            GPIOC->BSRR = blinkingLED;
        }
        else /* ...else (LED is on), turn it off */
        {
            GPIOC->BRR = blinkingLED;
        }

        TIM2->SR &= ~(TIM_SR_UIF); /* Clear update interrupt flag */
        TIM2->CR1 |= TIM_CR1_CEN; /* Restart stopped timer */
    }
}

#pragma GCC diagnostic pop
