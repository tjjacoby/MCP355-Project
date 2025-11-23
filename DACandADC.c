#include <stdio.h>
#include <stdlib.h>
#include "diag/trace.h"
#include "stm32f0xx.h"


void init_GPIOA(void);
void init_ADC(void);
void init_DAC(void);

int main(int argc, char* argv[])

{

    init_GPIOA();
    init_DAC();
    init_ADC();

    while (1)
    {
        // Wait for ADC conversion to be done: EOC flag: End-Of-Conversion
        while (!(ADC1->ISR & ADC_ISR_EOC));

        // Read ADC values
        uint16_t pot_value = (uint16_t)ADC1->DR;
        //ADC data reg ranges from 0-4095, convert to mV
        uint16_t voltage = (pot_value*3000)/4095;

        trace_printf("POT Value: %d\n", voltage);
        //Send ADC value to DAC
        DAC->DHR12R1 = pot_value;
    }
}


void init_GPIOA(void)
{
    // Enable GPIOA clock
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    // Enable ADC clock
    RCC->APB2ENR |= RCC_APB2ENR_ADCEN;

    // Enable DAC clock
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;

    // PA1 → analog (MODER[3:2] = 11)
    GPIOA->MODER |= (3U << 2);

    // PA4 → analog (MODER[9:8] = 11)
    GPIOA->MODER |= (3U << 8);
}

void init_DAC(void)
{
    // Make sure DAC is disabled before configuring
    if (DAC->CR & DAC_CR_EN1) {
        DAC->CR &= ~DAC_CR_EN1;
    }

    // init output to 0
    DAC->DHR12R1 = 0;

    /* EN1 = 1   (enable channel 1)
     * BOFF1 = 1 (disable output buffer → direct 12-bit output)
     * TEN1 = 0  (no hardware trigger)
     */
    DAC->CR |= DAC_CR_EN1;      // enable
    // Turn off buffer to make vaules more precise
    DAC->CR |= DAC_CR_BOFF1;    // buffer off
}



void init_ADC(void)
{
    // Disable ADC if it was already on
    if (ADC1->CR & ADC_CR_ADEN) {
        ADC1->CR |= ADC_CR_ADDIS; // disable ADC
        while (ADC1->CR & ADC_CR_ADEN); // wait till fully disabled
    }


    ADC1->CFGR1 &= ~ADC_CFGR1_RES;      // 12-bit resolution (00 = 12 bit)
    ADC1->CFGR1 &= ~ADC_CFGR1_ALIGN;    // right alignment (data stored in lower 12 bits of the 16-bit register)
    ADC1->CFGR1 |=  ADC_CFGR1_CONT;     // continuous mode, ADC moves on to next conversion right after last

    // WHY DO WE NEED THIS
    ADC1->CFGR1 &= ~ADC_CFGR1_OVRMOD;   // keep old data on overrun

    // Clear channels and pick channel 1 (PA1)
    ADC1->CHSELR = 0;
    ADC1->CHSELR |= ADC_CHSELR_CHSEL1;

    // Config sampling time to 111(7 in decimal) tells how long ADC should look at voltage
    ADC1->SMPR |= (7U << 0);


    ADC1->CR |= ADC_CR_ADEN;            // enable ADC
    while (!(ADC1->ISR & ADC_ISR_ADRDY));   // wait for ready

    // Start the conversion
    ADC1->CR |= ADC_CR_ADSTART;
}
