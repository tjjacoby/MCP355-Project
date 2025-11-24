#include <stdio.h>
#include <stdlib.h>
#include "diag/trace.h"
#include "stm32f0xx.h"
#include "cmsis/cmsis_device.h"

// Frequency measurement variables
volatile int timerTriggered = 0;
volatile uint32_t overflow_count = 0;
volatile uint32_t measured_frequency = 0;  // In Hz
volatile uint32_t measured_period = 0;     // In ms

// Source selection: 0 = Function Generator (PB2), 1 = 555 Timer (PB3)
volatile uint8_t frequency_source = 0;

// ADC/DAC variables
volatile uint16_t pot_value = 0;
volatile uint16_t pot_voltage = 0;  // In mV
volatile uint32_t pot_resistance = 0;  // In Ohms

// LED blinking
volatile uint16_t blinkingLED = ((uint16_t)0x0100);  // Start with blue LED (PC8)

// OLED refresh flag
volatile uint8_t oled_refresh_flag = 0;


// TIM2: Frequency measurement timer (no prescaling, max period)
#define myTIM2_PRESCALER ((uint16_t)0x0000)
#define myTIM2_PERIOD ((uint32_t)0xFFFFFFFF)

// TIM3: Debounce timer for USER button (1ms per tick, 50ms debounce)
#define myTIM3_PRESCALER ((uint16_t)47999)
#define myTIM3_PERIOD ((uint16_t)50)

// TIM4: OLED refresh timer (1ms per tick, 100ms refresh)
#define myTIM4_PRESCALER ((uint16_t)47999)
#define myTIM4_PERIOD ((uint16_t)100)

// ============================================================================
// OLED DISPLAY DEFINITIONS
// ============================================================================

SPI_HandleTypeDef SPI_Handle;

// OLED initialization commands
unsigned char oled_init_cmds[] = {
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

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

// System initialization
void SystemClock48MHz(void);

// GPIO initialization
void myGPIOA_Init(void);
void myGPIOB_Init(void);
void myGPIOC_Init(void);

// Peripheral initialization
void myADC_Init(void);
void myDAC_Init(void);
void myTIM2_Init(void);
void myTIM3_Init(void);
void myTIM4_Init(void);
void myEXTI_Init(void);
void mySPI_Init(void);

// OLED functions
void oled_Write(unsigned char Value);
void oled_Write_Cmd(unsigned char cmd);
void oled_Write_Data(unsigned char data);
void oled_config(void);
void refresh_OLED(void);

// Utility functions
void calculate_resistance(void);

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main(int argc, char* argv[])
{
    // Initialize system clock to 48 MHz
    SystemClock48MHz();

    trace_printf("ECE 355 Project: PWM Signal Monitoring System\n");
    trace_printf("System clock: %u Hz\n", SystemCoreClock);

    // Enable SYSCFG clock (needed for EXTI)
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN;

    // Initialize all peripherals
    myGPIOA_Init();    // PA0 (USER button), PA1 (ADC), PA4 (DAC)
    myGPIOB_Init();    // PB2 (Func Gen), PB3 (555), PB8/9/11/13/15 (SPI/OLED)
    myGPIOC_Init();    // PC8/PC9 (LEDs)
    
    myADC_Init();      // ADC for potentiometer
    myDAC_Init();      // DAC for optocoupler
    myTIM2_Init();     // Timer for frequency measurement
    myTIM3_Init();     // Timer for button debounce
    myTIM4_Init();     // Timer for OLED refresh
    myEXTI_Init();     // External interrupts (PA0, PB2, PB3)
    mySPI_Init();      // SPI for OLED display
    
    oled_config();     // Configure OLED display

    trace_printf("Initialization complete!\n");
    trace_printf("Press USER button to toggle frequency source\n");

    // Main loop
    while (1)
    {
        // Poll ADC (continuous conversion)
        if (ADC1->ISR & ADC_ISR_EOC)
        {
            pot_value = (uint16_t)ADC1->DR;
            pot_voltage = (pot_value * 3000) / 4095;  // Convert to mV
            
            // Calculate resistance
            calculate_resistance();
            
            // Send ADC value to DAC
            DAC->DHR12R1 = pot_value;
        }

        // Refresh OLED when timer flag is set
        if (oled_refresh_flag)
        {
            oled_refresh_flag = 0;
            refresh_OLED();
        }
    }

    return 0;
}


void SystemClock48MHz(void)
{
    // Disable the PLL
    RCC->CR &= ~(RCC_CR_PLLON);
    
    // Wait for the PLL to unlock
    while ((RCC->CR & RCC_CR_PLLRDY) != 0);
    
    // Configure the PLL for 48MHz system clock
    RCC->CFGR = 0x00280000;
    
    // Enable the PLL
    RCC->CR |= RCC_CR_PLLON;
    
    // Wait for the PLL to lock
    while ((RCC->CR & RCC_CR_PLLRDY) != RCC_CR_PLLRDY);
    
    // Switch the processor to the PLL clock source
    RCC->CFGR = (RCC->CFGR & (~RCC_CFGR_SW_Msk)) | RCC_CFGR_SW_PLL;
    
    // Update the system with the new clock frequency
    SystemCoreClockUpdate();
}

void myGPIOA_Init(void)
{
    // Enable GPIOA clock
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    
    // Enable ADC clock
    RCC->APB2ENR |= RCC_APB2ENR_ADCEN;
    
    // Enable DAC clock
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    
    // PA0: Input (USER button)
    GPIOA->MODER &= ~(GPIO_MODER_MODER0);
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR0);
    
    // PA1: Analog input (ADC - Potentiometer)
    GPIOA->MODER |= (3U << 2);
    
    // PA4: Analog output (DAC - Optocoupler control)
    GPIOA->MODER |= (3U << 8);
}

void myGPIOB_Init(void)
{
    // Enable GPIOB clock
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    
    // PB2: Input (Function Generator - EXTI2)
    GPIOB->MODER &= ~(GPIO_MODER_MODER2);
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR2);
    
    // PB3: Input (555 Timer - EXTI3)
    GPIOB->MODER &= ~(GPIO_MODER_MODER3);
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR3);
    
    // PB8: Output (CS# - Chip Select)
    GPIOB->MODER &= ~(GPIO_MODER_MODER8);
    GPIOB->MODER |= GPIO_MODER_MODER8_0;
    
    // PB9: Output (D/C# - Data/Command)
    GPIOB->MODER &= ~(GPIO_MODER_MODER9);
    GPIOB->MODER |= GPIO_MODER_MODER9_0;
    
    // PB11: Output (RES# - Reset)
    GPIOB->MODER &= ~(GPIO_MODER_MODER11);
    GPIOB->MODER |= GPIO_MODER_MODER11_0;
    
    // PB13: Alternate Function (SPI2 SCK)
    GPIOB->MODER &= ~(GPIO_MODER_MODER13);
    GPIOB->MODER |= GPIO_MODER_MODER13_1;
    GPIOB->AFR[1] &= ~(GPIO_AFRH_AFRH5);  // AF0
    
    // PB15: Alternate Function (SPI2 MOSI)
    GPIOB->MODER &= ~(GPIO_MODER_MODER15);
    GPIOB->MODER |= GPIO_MODER_MODER15_1;
    GPIOB->AFR[1] &= ~(GPIO_AFRH_AFRH7);  // AF0
}

void myGPIOC_Init(void)
{
    // Enable GPIOC clock
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    
    // PC8: Output (Blue LED)
    GPIOC->MODER |= GPIO_MODER_MODER8_0;
    GPIOC->OTYPER &= ~GPIO_OTYPER_OT_8;
    GPIOC->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR8;
    GPIOC->PUPDR &= ~GPIO_PUPDR_PUPDR8;
    
    // PC9: Output (Green LED)
    GPIOC->MODER |= GPIO_MODER_MODER9_0;
    GPIOC->OTYPER &= ~GPIO_OTYPER_OT_9;
    GPIOC->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR9;
    GPIOC->PUPDR &= ~GPIO_PUPDR_PUPDR9;
}

void myADC_Init(void)
{
    // Disable ADC if already enabled
    if (ADC1->CR & ADC_CR_ADEN)
    {
        ADC1->CR |= ADC_CR_ADDIS;
        while (ADC1->CR & ADC_CR_ADEN);
    }
    
    // Configure ADC
    ADC1->CFGR1 &= ~ADC_CFGR1_RES;      // 12-bit resolution
    ADC1->CFGR1 &= ~ADC_CFGR1_ALIGN;    // Right alignment
    ADC1->CFGR1 |= ADC_CFGR1_CONT;      // Continuous mode
    
    // Select channel 1 (PA1)
    ADC1->CHSELR = 0;
    ADC1->CHSELR |= ADC_CHSELR_CHSEL1;
    
    // Set sampling time
    ADC1->SMPR |= (7U << 0);
    
    // Enable and start ADC
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY));
    ADC1->CR |= ADC_CR_ADSTART;
}

void myDAC_Init(void)
{
    // Disable DAC if already enabled
    if (DAC->CR & DAC_CR_EN1)
    {
        DAC->CR &= ~DAC_CR_EN1;
    }
    
    // Initialize output to 0
    DAC->DHR12R1 = 0;
    
    // Enable DAC channel 1 with buffer off
    DAC->CR |= DAC_CR_EN1;
    DAC->CR |= DAC_CR_BOFF1;
}


void myTIM2_Init(void)
{
    // Enable TIM2 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    
    // Configure TIM2
    TIM2->CR1 = ((uint16_t)0x008C);
    TIM2->PSC = myTIM2_PRESCALER;
    TIM2->ARR = myTIM2_PERIOD;
    TIM2->EGR = ((uint16_t)0x0001);
    
    // Enable TIM2 update interrupt
    TIM2->DIER |= TIM_DIER_UIE;
    NVIC_SetPriority(TIM2_IRQn, 0);
    NVIC_EnableIRQ(TIM2_IRQn);
}

void myTIM3_Init(void)
{
    // Enable TIM3 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    
    // Configure TIM3 for one-shot mode (debounce)
    TIM3->CR1 = TIM_CR1_OPM;
    TIM3->PSC = myTIM3_PRESCALER;
    TIM3->ARR = myTIM3_PERIOD;
    
    // Enable TIM3 update interrupt
    TIM3->DIER |= TIM_DIER_UIE;
    NVIC_SetPriority(TIM3_IRQn, 2);
    NVIC_EnableIRQ(TIM3_IRQn);
}

void myTIM4_Init(void)
{
    // Enable TIM4 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;  // Note: Using TIM14, not TIM4
    
    // Configure TIM14 for OLED refresh
    TIM14->CR1 = ((uint16_t)0x008C);
    TIM14->PSC = myTIM4_PRESCALER;
    TIM14->ARR = myTIM4_PERIOD;
    TIM14->EGR = ((uint16_t)0x0001);
    
    // Enable TIM14 update interrupt
    TIM14->DIER |= TIM_DIER_UIE;
    NVIC_SetPriority(TIM14_IRQn, 3);
    NVIC_EnableIRQ(TIM14_IRQn);
    
    // Start timer
    TIM14->CR1 |= TIM_CR1_CEN;
}


void myEXTI_Init(void)
{
    // EXTI0: PA0 (USER button)
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PA;
    EXTI->IMR |= EXTI_IMR_MR0;
    EXTI->RTSR |= EXTI_RTSR_TR0;
    NVIC_SetPriority(EXTI0_1_IRQn, 1);
    NVIC_EnableIRQ(EXTI0_1_IRQn);
    
    // EXTI2: PB2 (Function Generator)
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI2_PB;
    EXTI->IMR |= EXTI_IMR_MR2;
    EXTI->RTSR |= EXTI_RTSR_TR2;
    NVIC_SetPriority(EXTI2_3_IRQn, 0);
    NVIC_EnableIRQ(EXTI2_3_IRQn);
    
    // EXTI3: PB3 (555 Timer)
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI3_PB;
    EXTI->IMR |= EXTI_IMR_MR3;
    EXTI->RTSR |= EXTI_RTSR_TR3;
    // Uses same IRQ handler as EXTI2
}


void mySPI_Init(void)
{
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
}


void oled_config(void)
{
    // Reset OLED display (RES# = PB11)
    GPIOB->BRR = GPIO_PIN_11;   // Active low reset
   
    GPIOB->BSRR = GPIO_PIN_11;  // Release reset
   
    
    // Send initialization commands
    for (unsigned int i = 0; i < sizeof(oled_init_cmds); i++)
    {
        oled_Write_Cmd(oled_init_cmds[i]);
    }
    
    // Clear display memory
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

void oled_Write_Cmd(unsigned char cmd)
{
    GPIOB->BSRR = (1 << 8);   // CS# high
    GPIOB->BRR = (1 << 9);    // D/C# low (command)
    GPIOB->BRR = (1 << 8);    // CS# low
    oled_Write(cmd);
    GPIOB->BSRR = (1 << 8);   // CS# high
}

void oled_Write_Data(unsigned char data)
{
    GPIOB->BSRR = (1 << 8);   // CS# high
    GPIOB->BSRR = (1 << 9);   // D/C# high (data)
    GPIOB->BRR = (1 << 8);    // CS# low
    oled_Write(data);
    GPIOB->BSRR = (1 << 8);   // CS# high
}

void oled_Write(unsigned char Value)
{
    while ((SPI2->SR & SPI_SR_TXE) == 0);
    HAL_SPI_Transmit(&SPI_Handle, &Value, 1, HAL_MAX_DELAY);
    while ((SPI2->SR & SPI_SR_TXE) == 0);
}

void refresh_OLED(void)
{
    unsigned char Buffer[17];
    
    // Display resistance on PAGE 2
    snprintf(Buffer, sizeof(Buffer), "R:%5lu Ohms", pot_resistance);
    
    oled_Write_Cmd(0xB2);  // Select PAGE 2
    oled_Write_Cmd(0x02);  // Lower column
    oled_Write_Cmd(0x10);  // Upper column
    
    for (int i = 0; Buffer[i] != '\0'; i++)
    {
        unsigned char c = Buffer[i];
        for (int j = 0; j < 8; j++)
        {
            oled_Write_Data(Characters[c][j]);
        }
    }
    
    // Display frequency on PAGE 4
    snprintf(Buffer, sizeof(Buffer), "F:%6lu Hz", measured_frequency);
    
    oled_Write_Cmd(0xB4);  // Select PAGE 4
    oled_Write_Cmd(0x02);  // Lower column
    oled_Write_Cmd(0x10);  // Upper column
    
    for (int i = 0; Buffer[i] != '\0'; i++)
    {
        unsigned char c = Buffer[i];
        for (int j = 0; j < 8; j++)
        {
            oled_Write_Data(Characters[c][j]);
        }
    }
    
    // Display source on PAGE 6
    if (frequency_source == 0)
    {
        snprintf(Buffer, sizeof(Buffer), "Src: Func Gen");
    }
    else
    {
        snprintf(Buffer, sizeof(Buffer), "Src: 555 Timer");
    }
    
    oled_Write_Cmd(0xB6);  // Select PAGE 6
    oled_Write_Cmd(0x02);  // Lower column
    oled_Write_Cmd(0x10);  // Upper column
    
    for (int i = 0; Buffer[i] != '\0'; i++)
    {
        unsigned char c = Buffer[i];
        for (int j = 0; j < 8; j++)
        {
            oled_Write_Data(Characters[c][j]);
        }
    }
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void calculate_resistance(void)
{
    // Assuming a voltage divider: POT in series with fixed resistor
    // V_out = V_in * (R_pot / (R_pot + R_fixed))
    // Rearranging: R_pot = (V_out * R_fixed) / (V_in - V_out)
    
    // Example calculation (adjust based on your circuit):
    // If R_fixed = 10k ohms, V_in = 3.3V
    const uint32_t R_FIXED = 10000;  // 10k ohms
    const uint32_t V_IN = 3300;      // 3.3V in mV
    
    if (pot_voltage < V_IN)
    {
        pot_resistance = (pot_voltage * R_FIXED) / (V_IN - pot_voltage);
    }
    else
    {
        pot_resistance = 0xFFFF;  // Max value if error
    }
}

// ============================================================================
// INTERRUPT HANDLERS
// ============================================================================

// TIM2 Overflow Handler
void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & TIM_SR_UIF) != 0)
    {
        trace_printf("\n*** Timer Overflow! ***\n");
        overflow_count++;
        TIM2->SR &= ~(TIM_SR_UIF);
        TIM2->CR1 |= TIM_CR1_CEN;
    }
}

// TIM3 Debounce Handler
void TIM3_IRQHandler(void)
{
    if ((TIM3->SR & TIM_SR_UIF) != 0)
    {
        TIM3->SR &= ~(TIM_SR_UIF);
        
        // Verify button still pressed
        if ((GPIOA->IDR & GPIO_IDR_0) != 0)
        {
            // Toggle frequency source
            frequency_source ^= 1;
            
            // Disable current source interrupt
            if (frequency_source == 0)
            {
                EXTI->IMR &= ~EXTI_IMR_MR3;  // Disable 555
                EXTI->IMR |= EXTI_IMR_MR2;   // Enable Func Gen
                trace_printf("\nSwitched to Function Generator\n");
            }
            else
            {
                EXTI->IMR &= ~EXTI_IMR_MR2;  // Disable Func Gen
                EXTI->IMR |= EXTI_IMR_MR3;   // Enable 555
                trace_printf("\nSwitched to 555 Timer\n");
            }
            
            // Reset measurement state
            timerTriggered = 0;
            TIM2->CR1 &= ~TIM_CR1_CEN;
        }
        
        // Re-enable USER button interrupt
        EXTI->IMR |= EXTI_IMR_MR0;
    }
}

// TIM14 (OLED Refresh) Handler
void TIM14_IRQHandler(void)
{
    if ((TIM14->SR & TIM_SR_UIF) != 0)
    {
        TIM14->SR &= ~(TIM_SR_UIF);
        oled_refresh_flag = 1;  // Set flag for main loop
    }
}

// USER Button Handler (EXTI0)
void EXTI0_1_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR0)
    {
        // Disable interrupt to prevent bounces
        EXTI->IMR &= ~EXTI_IMR_MR0;
        
        // Clear flag
        EXTI->PR |= EXTI_PR_PR0;
        
        // Start debounce timer
        TIM3->CR1 |= TIM_CR1_CEN;
    }
}

// Frequency Measurement Handler (EXTI2 and EXTI3)
void EXTI2_3_IRQHandler(void)
{
    // Check which interrupt triggered
    uint8_t is_exti2 = (EXTI->PR & EXTI_PR_PR2) != 0;
    uint8_t is_exti3 = (EXTI->PR & EXTI_PR_PR3) != 0;
    
    // Only process if it matches current source
    if ((is_exti2 && frequency_source == 0) || (is_exti3 && frequency_source == 1))
    {
        if (timerTriggered == 0)
        {
            // First edge: start timer
            TIM2->CNT = 0x00;
            overflow_count = 0;
            TIM2->CR1 |= TIM_CR1_CEN;
            timerTriggered = 1;
        }
        else
        {
            // Second edge: stop timer and calculate
            TIM2->CR1 &= ~(TIM_CR1_CEN);
            uint32_t count = TIM2->CNT;
            
            uint64_t total_ticks = ((uint64_t)overflow_count * 0xFFFFFFFF) + count;
            
            // Calculate period in microseconds
            float period_us = (total_ticks / 48.0f);
            
            // Calculate frequency in Hz
            if (period_us > 0)
            {
                measured_frequency = (uint32_t)(1000000.0f / period_us);
                measured_period = (uint32_t)(period_us / 1000.0f);  // Convert to ms
            }
            else
            {
                measured_frequency = 0;
                measured_period = 0;
            }
            
            trace_printf("Period: %lu ms, Freq: %lu Hz\n", 
                         measured_period, measured_frequency);
            
            timerTriggered = 0;
        }
    }
    
    // Clear pending flags
    if (is_exti2) EXTI->PR |= EXTI_PR_PR2;
    if (is_exti3) EXTI->PR |= EXTI_PR_PR3;
}
