/**
 * ====================================================================
 * 
 * stm8s blink demo
 * 
 * MCU: STM8S103/003
 * 
 * FREQ: 16MHz
 * 
 * PIN: PB5
 * 
 * ====================================================================
*/
//#include <stdint.h>
#include "stm8s.h"

#define LED_GPIO_PORT (GPIOB)
#define LED_GPIO_PINS (GPIO_PIN_5)


#define PWM_MODE_1 (6 << 4)

// 8-bit sine-wave for a 392Hz tone (31372.5kHz / 80 samples)
/*

const uint8_t tone[] = {
    128,141,154,166,178,189,199,209,217,223,229,234,
    238,241,244,245,246,247,247,246,244,243,240,237,
    233,229,225,220,214,209,202,196,189,182,175,167,
    160,152,144,136,128,119,111,103,95,88,80,73,
    66,59,53,46,41,35,30,26,22,18,15,12,
    11,9,8,8,9,10,11,14,17,21,26,32,
    38,46,56,66,77,89,101,114
};*/

//64-poit sinewave
const uint16_t sineLookupTable[] = {
128, 140, 152, 165, 176, 188, 198, 208,
218, 226, 234, 240, 245, 250, 253, 254,
255, 254, 253, 250, 245, 240, 234, 226,
218, 208, 198, 188, 176, 165, 152, 140,
128, 115, 103, 90, 79, 67, 57, 47,
37, 29, 21, 15, 10, 5, 2, 1,
0, 1, 2, 5, 10, 15, 21, 29,
37, 47, 57, 67, 79, 90, 103, 115};


//volatile uint8_t audio_idx = 0;

volatile uint8_t buf_clr = 0;
volatile uint8_t audio_buf = 0;

enum type {
    Sin,
    Tri,
    Sq,
    Saw
};

struct synthVoice {
    uint16_t phase;
    uint16_t period;
    uint16_t len;
    uint8_t amp;
    enum type;
};

/* Timer 2 update interrupt */

void setup_sound(void);
void Synth(struct synthVoice *sv);
void DelayInit(void);
void DelayMs(uint16_t ms);

void main(void)
{
    // set system clock to 16Mhz
    //CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
    CLK->CKDIVR = 0x00;

    // init delay timer
    DelayInit();

    // init LED pin
    GPIO_Init(LED_GPIO_PORT, (GPIO_Pin_TypeDef)LED_GPIO_PINS, GPIO_MODE_OUT_PP_LOW_FAST);

    setup_sound();
    rim();
    while (1)
    {
        if(buf_clr){

        }
        //GPIO_WriteReverse(LED_GPIO_PORT, (GPIO_Pin_TypeDef)LED_GPIO_PINS);
        //DelayMs(50);
        wfi();
    }
}

void DelayInit(void)
{
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER4, ENABLE);
    TIM4_TimeBaseInit(TIM4_PRESCALER_64, 249); // 1ms
    TIM4_SelectOnePulseMode(TIM4_OPMODE_SINGLE);
}

void DelayMs(uint16_t ms)
{
    while (ms--)
    {
        TIM4->SR1 = (uint8_t)(~TIM4_FLAG_UPDATE);
        TIM4->CR1 |= TIM4_CR1_CEN;
        while ((TIM4->SR1 & (uint8_t)TIM4_FLAG_UPDATE) == 0)
            ;
    }
}

void setup_sound(void)
{
    //TIM2->PSCRH = 0x01;
    //TIM2->PSCRL = 0x00;
    /* Auto-reload = 255 steps (0–254) */
    TIM2->PSCR = 0x01;
    TIM2->ARRH = 0x00;
    TIM2->ARRL = 254;
    // That's 31372.5kHz sample rate
    // Should be good enough

    /* PWM mode 1 + preload enable */
    TIM2->CCMR3 = PWM_MODE_1 | TIM2_CCMR_OCxPE;

    /* Enable channel 3 output */
    TIM2->CCER2 |= TIM2_CCER2_CC3E;

    /* Enable update interrupt */
    TIM2->IER |= TIM2_IER_UIE;

    /* Start timer */
    TIM2->CR1 |= TIM2_CR1_CEN;
}

void Synth(struct synthVoice *sv){
    // period = SampleRate / frequency in samples

    
    
}

/**
 * =========================================================
 *                     interrupt functions
 * =========================================================
*/

/**
  * @brief  TRAP interrupt routine
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER_TRAP(TRAP_IRQHandler)
{
    while (1)
    {
        nop();
    }
}

/**
  * @brief  this is a example for interrupt function define
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(_this_is_a_example, EXTI_PORTA_IRQn)
{
    // TODO
}

INTERRUPT_HANDLER(TIM2_UPD_OVF_BRK_IRQHandler, 13)
{
    //TIM2->CCR3L = tone2[audio_idx];
    TIM2->CCR3L = audio_buf;
    buf_clr = 1;

    /* Clear update interrupt flag */
    TIM2->SR1 &= (uint8_t)(~TIM2_SR1_UIF);

    /*audio_idx++;
    if (audio_idx >= sizeof(tone2)) {
        audio_idx = 0;
    }*/
}
