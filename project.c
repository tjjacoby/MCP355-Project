#include <stdio.h>
#include "diag/Trace.h"
#include "cmsis/cmsis_device.h"
#include <string.h>

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

/* Clock prescaler for TIM2 timer: no prescaling */
#define myTIM2_PRESCALER ((uint16_t)0x0000)
/* Maximum possible setting for overflow */
#define myTIM2_PERIOD ((uint32_t)0xFFFFFFFF)

/* Prescaler for a 1kHz timer clock (48MHz / (47999 + 1)), 1 clock cycle per millisecond*/
#define myTIM3_PRESCALER ((uint16_t)47999)
/* 50ms debounce delay (50 * 1ms) */
#define myTIM3_PERIOD ((uint16_t)50)

/* Assume 10k Ohm potentiometer */
#define POT_MAX_RES 10000

/* Vref in mV */
#define VREF_MV 3300

volatile unsigned int Freq = 0; // Measured frequency value
volatile unsigned int Res = 0; // Measured resistance value
volatile uint8_t display_update_flag = 1; // Flag to trigger display update (start with 1 for initial display)
volatile int timer_started = 0;
volatile uint32_t overflow_count = 0;
volatile int current_source = 0; // 0: FG (PB2), 1: 555 (PB3)
SPI_HandleTypeDef SPI_Handle;

/* LED Display initialization commands */
unsigned char oled_init_cmds[] =
{
    0xAE,
    0x20, 0x00,
    0x40,
    0xA0 | 0x01,
    0xA8, 0x40 - 1,
    0xC0 | 0x08,
    0xD3, 0x00,
    0xDA, 0x32,
    0xD5, 0x80,
    0xD9, 0x22,
    0xDB, 0x30,
    0x81, 0xFF,
    0xA4,
    0xA6,
    0xAD, 0x30,
    0x8D, 0x10,
    0xAE | 0x01,
    0xC0,
    0xA0
};

/* Character specifications for LED Display */
unsigned char Characters[][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // SPACE (indices 0-31)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // SPACE (32)
    {0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x00, 0x00}, // ! (33)
    {0x00, 0x07, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00}, // " (34)
    {0x14, 0x7F, 0x14, 0x7F, 0x14, 0x00, 0x00, 0x00}, // # (35)
    {0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x00, 0x00, 0x00}, // $ (36)
    {0x23, 0x13, 0x08, 0x64, 0x62, 0x00, 0x00, 0x00}, // % (37)
    {0x36, 0x49, 0x55, 0x22, 0x50, 0x00, 0x00, 0x00}, // & (38)
    {0x00, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}, // ' (39)
    {0x00, 0x1C, 0x22, 0x41, 0x00, 0x00, 0x00, 0x00}, // ( (40)
    {0x00, 0x41, 0x22, 0x1C, 0x00, 0x00, 0x00, 0x00}, // ) (41)
    {0x14, 0x08, 0x3E, 0x08, 0x14, 0x00, 0x00, 0x00}, // * (42)
    {0x08, 0x08, 0x3E, 0x08, 0x08, 0x00, 0x00, 0x00}, // + (43)
    {0x00, 0x50, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00}, // , (44)
    {0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00}, // - (45)
    {0x00, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00}, // . (46)
    {0x20, 0x10, 0x08, 0x04, 0x02, 0x00, 0x00, 0x00}, // / (47)
    {0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, 0x00, 0x00}, // 0 (48)
    {0x00, 0x42, 0x7F, 0x40, 0x00, 0x00, 0x00, 0x00}, // 1 (49)
    {0x42, 0x61, 0x51, 0x49, 0x46, 0x00, 0x00, 0x00}, // 2 (50)
    {0x21, 0x41, 0x45, 0x4B, 0x31, 0x00, 0x00, 0x00}, // 3 (51)
    {0x18, 0x14, 0x12, 0x7F, 0x10, 0x00, 0x00, 0x00}, // 4 (52)
    {0x27, 0x45, 0x45, 0x45, 0x39, 0x00, 0x00, 0x00}, // 5 (53)
    {0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00, 0x00, 0x00}, // 6 (54)
    {0x03, 0x01, 0x71, 0x09, 0x07, 0x00, 0x00, 0x00}, // 7 (55)
    {0x36, 0x49, 0x49, 0x49, 0x36, 0x00, 0x00, 0x00}, // 8 (56)
    {0x06, 0x49, 0x49, 0x29, 0x1E, 0x00, 0x00, 0x00}, // 9 (57)
    {0x00, 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00}, // : (58)
    {0x00, 0x56, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00}, // ; (59)
    {0x08, 0x14, 0x22, 0x41, 0x00, 0x00, 0x00, 0x00}, // < (60)
    {0x14, 0x14, 0x14, 0x14, 0x14, 0x00, 0x00, 0x00}, // = (61)
    {0x00, 0x41, 0x22, 0x14, 0x08, 0x00, 0x00, 0x00}, // > (62)
    {0x02, 0x01, 0x51, 0x09, 0x06, 0x00, 0x00, 0x00}, // ? (63)
    {0x32, 0x49, 0x79, 0x41, 0x3E, 0x00, 0x00, 0x00}, // @ (64)
    {0x7E, 0x11, 0x11, 0x11, 0x7E, 0x00, 0x00, 0x00}, // A (65)
    {0x7F, 0x49, 0x49, 0x49, 0x36, 0x00, 0x00, 0x00}, // B (66)
    {0x3E, 0x41, 0x41, 0x41, 0x22, 0x00, 0x00, 0x00}, // C (67)
    {0x7F, 0x41, 0x41, 0x22, 0x1C, 0x00, 0x00, 0x00}, // D (68)
    {0x7F, 0x49, 0x49, 0x49, 0x41, 0x00, 0x00, 0x00}, // E (69)
    {0x7F, 0x09, 0x09, 0x09, 0x01, 0x00, 0x00, 0x00}, // F (70)
    {0x3E, 0x41, 0x49, 0x49, 0x7A, 0x00, 0x00, 0x00}, // G (71)
    {0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00, 0x00, 0x00}, // H (72)
    {0x40, 0x41, 0x7F, 0x41, 0x40, 0x00, 0x00, 0x00}, // I (73)
    {0x20, 0x40, 0x41, 0x3F, 0x01, 0x00, 0x00, 0x00}, // J (74)
    {0x7F, 0x08, 0x14, 0x22, 0x41, 0x00, 0x00, 0x00}, // K (75)
    {0x7F, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00}, // L (76)
    {0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x00, 0x00, 0x00}, // M (77)
    {0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00, 0x00, 0x00}, // N (78)
    {0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00, 0x00, 0x00}, // O (79)
    {0x7F, 0x09, 0x09, 0x09, 0x06, 0x00, 0x00, 0x00}, // P (80)
    {0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00, 0x00, 0x00}, // Q (81)
    {0x7F, 0x09, 0x19, 0x29, 0x46, 0x00, 0x00, 0x00}, // R (82)
    {0x46, 0x49, 0x49, 0x49, 0x31, 0x00, 0x00, 0x00}, // S (83)
    {0x01, 0x01, 0x7F, 0x01, 0x01, 0x00, 0x00, 0x00}, // T (84)
    {0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00, 0x00, 0x00}, // U (85)
    {0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00, 0x00, 0x00}, // V (86)
    {0x3F, 0x40, 0x38, 0x40, 0x3F, 0x00, 0x00, 0x00}, // W (87)
    {0x63, 0x14, 0x08, 0x14, 0x63, 0x00, 0x00, 0x00}, // X (88)
    {0x07, 0x08, 0x70, 0x08, 0x07, 0x00, 0x00, 0x00}, // Y (89)
    {0x61, 0x51, 0x49, 0x45, 0x43, 0x00, 0x00, 0x00}, // Z (90)
    {0x7F, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // [ (91)
    {0x15, 0x16, 0x7C, 0x16, 0x15, 0x00, 0x00, 0x00}, // backslash (92)
    {0x00, 0x00, 0x00, 0x41, 0x7F, 0x00, 0x00, 0x00}, // ] (93)
    {0x04, 0x02, 0x01, 0x02, 0x04, 0x00, 0x00, 0x00}, // ^ (94)
    {0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00}, // _ (95)
    {0x00, 0x01, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00}, // ` (96)
    {0x20, 0x54, 0x54, 0x54, 0x78, 0x00, 0x00, 0x00}, // a (97)
    {0x7F, 0x48, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00}, // b (98)
    {0x38, 0x44, 0x44, 0x44, 0x20, 0x00, 0x00, 0x00}, // c (99)
    {0x38, 0x44, 0x44, 0x48, 0x7F, 0x00, 0x00, 0x00}, // d (100)
    {0x38, 0x54, 0x54, 0x54, 0x18, 0x00, 0x00, 0x00}, // e (101)
    {0x08, 0x7E, 0x09, 0x01, 0x02, 0x00, 0x00, 0x00}, // f (102)
    {0x0C, 0x52, 0x52, 0x52, 0x3E, 0x00, 0x00, 0x00}, // g (103)
    {0x7F, 0x08, 0x04, 0x04, 0x78, 0x00, 0x00, 0x00}, // h (104)
    {0x00, 0x44, 0x7D, 0x40, 0x00, 0x00, 0x00, 0x00}, // i (105)
    {0x20, 0x40, 0x44, 0x3D, 0x00, 0x00, 0x00, 0x00}, // j (106)
    {0x7F, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00, 0x00}, // k (107)
    {0x00, 0x41, 0x7F, 0x40, 0x00, 0x00, 0x00, 0x00}, // l (108)
    {0x7C, 0x04, 0x18, 0x04, 0x78, 0x00, 0x00, 0x00}, // m (109)
    {0x7C, 0x08, 0x04, 0x04, 0x78, 0x00, 0x00, 0x00}, // n (110)
    {0x38, 0x44, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00}, // o (111)
    {0x7C, 0x14, 0x14, 0x14, 0x08, 0x00, 0x00, 0x00}, // p (112)
    {0x08, 0x14, 0x14, 0x18, 0x7C, 0x00, 0x00, 0x00}, // q (113)
    {0x7C, 0x08, 0x04, 0x04, 0x08, 0x00, 0x00, 0x00}, // r (114)
    {0x48, 0x54, 0x54, 0x54, 0x20, 0x00, 0x00, 0x00}, // s (115)
    {0x04, 0x3F, 0x44, 0x40, 0x20, 0x00, 0x00, 0x00}, // t (116)
    {0x3C, 0x40, 0x40, 0x20, 0x7C, 0x00, 0x00, 0x00}, // u (117)
    {0x1C, 0x20, 0x40, 0x20, 0x1C, 0x00, 0x00, 0x00}, // v (118)
    {0x3C, 0x40, 0x38, 0x40, 0x3C, 0x00, 0x00, 0x00}, // w (119)
    {0x44, 0x28, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00}, // x (120)
    {0x0C, 0x50, 0x50, 0x50, 0x3C, 0x00, 0x00, 0x00}, // y (121)
    {0x44, 0x64, 0x54, 0x4C, 0x44, 0x00, 0x00, 0x00}, // z (122)
    {0x00, 0x08, 0x36, 0x41, 0x00, 0x00, 0x00, 0x00}, // { (123)
    {0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00}, // | (124)
    {0x00, 0x41, 0x36, 0x08, 0x00, 0x00, 0x00, 0x00}, // } (125)
    {0x08, 0x08, 0x2A, 0x1C, 0x08, 0x00, 0x00, 0x00}, // ~ (126)
    {0x08, 0x1C, 0x2A, 0x08, 0x08, 0x00, 0x00, 0x00} // <- (127)
};

void oled_Write(unsigned char Value);
void oled_Write_Cmd(unsigned char cmd);
void oled_Write_Data(unsigned char data);
void oled_config(void);
void refresh_OLED(void);

void myGPIOA_Init(void);
void myGPIOB_Init(void);
void myGPIOC_Init(void);
void myTIM2_Init(void);
void myTIM3_Init(void);
void myEXTI_Init(void);
void init_ADC(void);
void init_DAC(void);

/*** Call this function to boost the STM32F0xx clock to 48 MHz ***/
void SystemClock48MHz( void )
{
    RCC->CR &= ~(RCC_CR_PLLON);
    while (( RCC->CR & RCC_CR_PLLRDY ) != 0 );
    RCC->CFGR = 0x00280000;
    RCC->CR |= RCC_CR_PLLON;
    while (( RCC->CR & RCC_CR_PLLRDY ) != RCC_CR_PLLRDY );
    RCC->CFGR = ( RCC->CFGR & (~RCC_CFGR_SW_Msk)) | RCC_CFGR_SW_PLL;
    SystemCoreClockUpdate();
}

int main(int argc, char* argv[])
{
    SystemClock48MHz();

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN; /* Enable SYSCFG clock */

    myGPIOA_Init(); // Initialize I/O port PA
    myGPIOB_Init(); // Initialize I/O port PB
    myGPIOC_Init(); // Initialize I/O port PC
    myTIM2_Init(); // Initialize timer TIM2 for frequency measurement
    myTIM3_Init(); // Initialize TIM3 for debouncing
    myEXTI_Init(); // Initialize EXTI for PA0, PB2, PB3
    init_DAC(); // Initialize DAC
    init_ADC(); // Initialize ADC

    oled_config();

    // Initial LED: blue for FG
    GPIOC->BSRR = ((uint32_t)0x0100);
    GPIOC->BRR = ((uint32_t)0x0200);

    // Display initial values
    refresh_OLED();

    while (1)
    {
        // Poll ADC for potentiometer value
        if (ADC1->ISR & ADC_ISR_EOC)
        {
            // Read ADC value
            uint16_t pot_value = (uint16_t)ADC1->DR;

            // Calculate voltage in mV
            uint16_t voltage = (pot_value * VREF_MV) / 4095;

            // Calculate resistance (assuming 10k pot, linear)
            Res = (pot_value * POT_MAX_RES) / 4095;

            trace_printf("Voltage: %d mV, Res: %u\n", voltage, Res);

            // Send ADC value to DAC to control optocoupler
            DAC->DHR12R1 = pot_value;

            // Trigger display update
            display_update_flag = 1;
        }

        /* Update display when flag is set */
        if (display_update_flag)
        {
            display_update_flag = 0;
            refresh_OLED();
        }
    }

    return 0;
}

// User Button, ADC, DAC
void myGPIOA_Init()
{
    /* Enable clock for GPIOA peripheral */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    /* Configure PA0 as input for button */
    GPIOA->MODER &= ~(GPIO_MODER_MODER0);

    /* Ensure no pull-up/pull-down for PA0 */
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR0);

    /* Configure PA1 as analog for ADC */
    GPIOA->MODER |= (3U << 2); // PA1 → analog (MODER[3:2] = 11)

    /* Configure PA4 as analog for DAC */
    GPIOA->MODER |= (3U << 8); // PA4 → analog (MODER[9:8] = 11)
}

// FG and 555 inputs
void myGPIOB_Init()
{
    /* Enable clock for GPIOB peripheral */
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

    /* Configure PB2 as input */
    GPIOB->MODER &= ~(GPIO_MODER_MODER2);

    /* Ensure no pull-up/pull-down for PB2 */
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR2);

    /* Configure PB3 as input */
    GPIOB->MODER &= ~(GPIO_MODER_MODER3);

    /* Ensure no pull-up/pull-down for PB3 */
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR3);
}

// LEDs
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

// Frequency measurement timer
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
}

// Debounce timer
void myTIM3_Init()
{
    /* Enable clock for TIM3 peripheral */
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    // Configure TIM3 for one-shot mode for debouncing
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
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PA;

    /* Map PB2 to EXTI2 */
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI2;
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI2_PB;

    /* Map PB3 to EXTI3 */
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI3;
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI3_PB;

    /* Enable EXTI0 interrupt (button, rising edge) */
    EXTI->IMR |= EXTI_IMR_MR0;
    EXTI->RTSR |= EXTI_RTSR_TR0;

    /* Enable EXTI2 interrupt initially (FG, rising edge) */
    EXTI->IMR |= EXTI_IMR_MR2;
    EXTI->RTSR |= EXTI_RTSR_TR2;

    /* EXTI3 initially disabled */
    EXTI->RTSR |= EXTI_RTSR_TR3; // Enable rising edge, but IMR off

    /* Configure NVIC for EXTI0_1 (PA0) */
    NVIC_SetPriority(EXTI0_1_IRQn, 1);
    NVIC_EnableIRQ(EXTI0_1_IRQn);

    /* Configure NVIC for EXTI2_3 (PB2/PB3) */
    NVIC_SetPriority(EXTI2_3_IRQn, 1);
    NVIC_EnableIRQ(EXTI2_3_IRQn);
}

void init_DAC(void)
{
    // Enable DAC clock
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;

    // Make sure DAC is disabled before configuring
    if (DAC->CR & DAC_CR_EN1) {
        DAC->CR &= ~DAC_CR_EN1;
    }

    // init output to 0
    DAC->DHR12R1 = 0;

    /* EN1 = 1 (enable channel 1)
     * BOFF1 = 0 (enable output buffer)
     * TEN1 = 0 (no hardware trigger)
     */
    DAC->CR |= DAC_CR_EN1; // enable
    DAC->CR &= ~DAC_CR_BOFF1; // buffer on
}

void init_ADC(void)
{
    // Enable ADC clock
    RCC->APB2ENR |= RCC_APB2ENR_ADCEN;

    // Disable ADC if it was already on
    if (ADC1->CR & ADC_CR_ADEN) {
        ADC1->CR |= ADC_CR_ADDIS;
        while (ADC1->CR & ADC_CR_ADEN);
    }

    ADC1->CFGR1 &= ~ADC_CFGR1_RES; // 12-bit resolution
    ADC1->CFGR1 &= ~ADC_CFGR1_ALIGN; // right alignment
    ADC1->CFGR1 |= ADC_CFGR1_CONT; // continuous mode
    ADC1->CFGR1 &= ~ADC_CFGR1_OVRMOD; // keep old data on overrun

    // Clear channels and pick channel 1 (PA1)
    ADC1->CHSELR = 0;
    ADC1->CHSELR |= ADC_CHSELR_CHSEL1;

    // Config sampling time
    ADC1->SMPR |= (7U << 0);

    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY));

    // Start the conversion
    ADC1->CR |= ADC_CR_ADSTART;
}

void refresh_OLED()
{
    unsigned char Buffer[17];

    // Display resistance on PAGE 2
    snprintf(Buffer, sizeof(Buffer), "R: %5u Ohms", Res);

    oled_Write_Cmd(0xB2);
    oled_Write_Cmd(0x02);
    oled_Write_Cmd(0x10);

    for (int i = 0; Buffer[i] != '\0'; i++)
    {
        unsigned char c = Buffer[i];
        for (int j = 0; j < 8; j++)
        {
            oled_Write_Data(Characters[c][j]);
        }
    }

    // Display frequency on PAGE 4
    snprintf(Buffer, sizeof(Buffer), "F: %5u Hz", Freq);

    oled_Write_Cmd(0xB4);
    oled_Write_Cmd(0x02);
    oled_Write_Cmd(0x10);

    for (int i = 0; Buffer[i] != '\0'; i++)
    {
        unsigned char c = Buffer[i];
        for (int j = 0; j < 8; j++)
        {
            oled_Write_Data(Characters[c][j]);
        }
    }

    // Approximate 100ms delay (adjust based on observation)
    for (volatile uint32_t i = 0; i < 4800000; i++);
}

void oled_Write_Cmd(unsigned char cmd)
{
    GPIOB->BSRR = (1 << 8);
    GPIOB->BRR = (1 << 9);
    GPIOB->BRR = (1 << 8);
    oled_Write(cmd);
    GPIOB->BSRR = (1 << 8);
}

void oled_Write_Data(unsigned char data)
{
    GPIOB->BSRR = (1 << 8);
    GPIOB->BSRR = (1 << 9);
    GPIOB->BRR = (1 << 8);
    oled_Write(data);
    GPIOB->BSRR = (1 << 8);
}

void oled_Write(unsigned char Value)
{
    while ((SPI2->SR & SPI_SR_TXE) == 0);
    HAL_SPI_Transmit(&SPI_Handle, &Value, 1, HAL_MAX_DELAY);
    while ((SPI2->SR & SPI_SR_TXE) == 0);
}

void oled_config(void)
{
    // Enable GPIOB clock
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

    // Configure PB8, PB9, PB11 as outputs
    GPIOB->MODER &= ~(GPIO_MODER_MODER8);
    GPIOB->MODER &= ~(GPIO_MODER_MODER9);
    GPIOB->MODER &= ~(GPIO_MODER_MODER11);

    GPIOB->MODER |= (GPIO_MODER_MODER8_0);
    GPIOB->MODER |= (GPIO_MODER_MODER9_0);
    GPIOB->MODER |= (GPIO_MODER_MODER11_0);

    // Configure PB13 and PB15 as alternate function
    GPIOB->MODER &= ~(GPIO_MODER_MODER13);
    GPIOB->MODER &= ~(GPIO_MODER_MODER15);

    GPIOB->MODER |= (GPIO_MODER_MODER13_1);
    GPIOB->MODER |= (GPIO_MODER_MODER15_1);

    // Set alternate function to AF0 for PB13 and PB15
    GPIOB->AFR[1] &= ~((GPIO_AFRH_AFRH5));
    GPIOB->AFR[1] &= ~((GPIO_AFRH_AFRH7));

    // Enable SPI2 clock
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

    // Initialize SPI2 structure
    SPI_Handle.Instance = SPI2;
    SPI_Handle.Init.Direction = SPI_DIRECTION_1LINE;
    SPI_Handle.Init.Mode = SPI_MODE_MASTER;
    SPI_Handle.Init.DataSize = SPI_DATASIZE_8BIT;
    SPI_Handle.Init.CLKPolarity = SPI_POLARITY_LOW;
    SPI_Handle.Init.CLKPhase = SPI_PHASE_1EDGE;
    SPI_Handle.Init.NSS = SPI_NSS_SOFT;
    SPI_Handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    SPI_Handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
    SPI_Handle.Init.CRCPolynomial = 7;

    HAL_SPI_Init(&SPI_Handle);
    __HAL_SPI_ENABLE(&SPI_Handle);

    // Reset LED Display
    GPIOB->BRR = GPIO_PIN_11;
    for (volatile int i = 0; i < 1000; i++); // Small delay
    GPIOB->BSRR = GPIO_PIN_11;
    for (volatile int i = 0; i < 1000; i++); // Small delay

    // Send initialization commands
    for (unsigned int i = 0; i < sizeof(oled_init_cmds); i++)
    {
        oled_Write_Cmd(oled_init_cmds[i]);
    }

    // Clear display
    for (unsigned int page = 0; page < 8; page++)
    {
        oled_Write_Cmd(0xB0 | page);
        oled_Write_Cmd(0x00);
        oled_Write_Cmd(0x10);

        for (unsigned int col = 0; col < 128; col++)
        {
            oled_Write_Data(0x00);
        }
    }
}

/* EXTI0 interrupt handler (button) */
void EXTI0_1_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR0)
    {
        EXTI->IMR &= ~EXTI_IMR_MR0;
        EXTI->PR |= EXTI_PR_PR0;
        TIM3->CR1 |= TIM_CR1_CEN;
    }
}

/* EXTI2/3 interrupt handler (signals) */
void EXTI2_3_IRQHandler(void)
{
    uint32_t pr = EXTI->PR & (EXTI_PR_PR2 | EXTI_PR_PR3);
    if (pr)
    {
        EXTI->PR |= pr;

        if (!timer_started)
        {
            TIM2->CNT = 0;
            overflow_count = 0;
            TIM2->CR1 |= TIM_CR1_CEN;
            timer_started = 1;
        }
        else
        {
            TIM2->CR1 &= ~TIM_CR1_CEN;
            uint32_t count = TIM2->CNT;
            uint64_t total_ticks = ((uint64_t)overflow_count << 32) + count;
            if (total_ticks > 0)
            {
                Freq = (unsigned int)(48000000ULL / total_ticks);
            }
            else
            {
                Freq = 0;
            }
            timer_started = 0;
            display_update_flag = 1;
        }
    }
}

/* TIM3 interrupt handler (debounce) */
void TIM3_IRQHandler(void)
{
    if ((TIM3->SR & TIM_SR_UIF) != 0)
    {
        TIM3->SR &= ~(TIM_SR_UIF);

        if ((GPIOA->IDR & GPIO_IDR_0) != 0)
        {
            current_source ^= 1;
            if (current_source)
            {
                // Switch to 555 (PB3), green LED
                EXTI->IMR &= ~EXTI_IMR_MR2;
                EXTI->IMR |= EXTI_IMR_MR3;
                GPIOC->BSRR = ((uint32_t)0x0200);
                GPIOC->BRR = ((uint32_t)0x0100);
            }
            else
            {
                // Switch to FG (PB2), blue LED
                EXTI->IMR &= ~EXTI_IMR_MR3;
                EXTI->IMR |= EXTI_IMR_MR2;
                GPIOC->BSRR = ((uint32_t)0x0100);
                GPIOC->BRR = ((uint32_t)0x0200);
            }
            trace_printf("\nButton pressed - Switching source...\n");

            // Reset frequency measurement on switch
            TIM2->CR1 &= ~TIM_CR1_CEN;
            timer_started = 0;
            Freq = 0; // Reset frequency until new measurement

            display_update_flag = 1;
        }

        EXTI->IMR |= EXTI_IMR_MR0;
    }
}

/* TIM2 interrupt handler (overflow for frequency) */
void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & TIM_SR_UIF) != 0)
    {
        overflow_count++;
        TIM2->SR &= ~(TIM_SR_UIF);
    }
}

#pragma GCC diagnostic pop
