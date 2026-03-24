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
#include <stdlib.h>

#define LED_GPIO_PORT (GPIOB)
#define LED_GPIO_PINS (GPIO_PIN_5)


#define PWM_MODE_1 (6 << 4)

#define OUT_SAMPLERATE 31372.5f
#define BUFFER_SIZE 24

// 8-bit sine-wave for a 392Hz tone (31372.5kHz / 80 samples)




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


volatile uint8_t audio_idx = 0;

volatile uint8_t buf_clr = 0;
volatile uint8_t audio_bufA[BUFFER_SIZE] = {
128, 191, 238, 255, 238, 191, 128, 64,
17, 0, 17, 64,128, 191, 238, 255, 238, 191, 128, 64,
17, 0, 17, 64,};
volatile uint8_t audio_bufB[BUFFER_SIZE] = {
128, 160, 191, 218, 238, 251, 255, 251,
238, 218, 191, 160, 128, 95, 64, 37,
17, 4, 0, 4, 17, 37, 64, 95};
volatile uint8_t buf_idx = 0;


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
    enum type type;
};

/* Timer 2 update interrupt */

void setup_sound(void);
void fillBuf(volatile uint8_t *buf, struct synthVoice *sv);
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

        //GPIO_WriteReverse(LED_GPIO_PORT, (GPIO_Pin_TypeDef)LED_GPIO_PINS);
        //DelayMs(64);
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

struct synthVoice* makeVoice(uint16_t freq, uint16_t len, uint8_t amp, enum type voiceType){
    struct synthVoice *sv = malloc(sizeof(struct synthVoice));
    sv->phase = 0;
    sv->period = (uint16_t)( OUT_SAMPLERATE /freq);
    sv->type = voiceType;
    return sv;
}

void fillBuf(volatile uint8_t *buf, struct synthVoice *sv){
    uint8_t i;
    uint16_t sample;
    uint16_t phaseIndex;
    
    for(i = 0; i < BUFFER_SIZE; i++){
        // Calculate phase index for lookup table (64 entries)
        // Using fixed-point: (phase * 64) / period
        phaseIndex = (uint16_t)((uint32_t)(sv->phase * 64UL) / sv->period);
        
        // Ensure phaseIndex wraps within lookup table bounds
        if(phaseIndex >= 64) phaseIndex = phaseIndex % 64;
        
        switch(sv->type){
            case Sin:
                // Use sine lookup table
                sample = sineLookupTable[phaseIndex];
                break;
                
            case Tri:
                // Triangle wave: ramp up then down
                if(sv->phase < (sv->period >> 1)){
                    // Rising: 0 -> 255
                    sample = (uint16_t)((uint32_t)(sv->phase * 510UL) / sv->period);
                } else {
                    // Falling: 255 -> 0
                    sample = 255 - (uint16_t)((uint32_t)((sv->phase - (sv->period >> 1)) * 510UL) / sv->period);
                }
                break;
                
            case Sq:
                // Square wave: 255 for first half, 0 for second half
                sample = (sv->phase < (sv->period >> 1)) ? 255 : 0;
                break;
                
            case Saw:
                // Sawtooth wave: linear ramp from 0 to 255
                sample = (uint16_t)((uint32_t)(sv->phase * 255UL) / sv->period);
                break;
                
            default:
                sample = 128; // DC offset (silence)
                break;
        }
        
        // Apply amplitude scaling (fixed-point: sample * amp / 255)
        sample = (uint16_t)((uint32_t)(sample * sv->amp) / 255UL);
        
        // Store in buffer
        buf[i] = (uint8_t)sample;
        
        // Increment phase with wrapping
        sv->phase++;
        if(sv->phase >= sv->period){
            sv->phase = 0;
        }
    }
}

void Synth(struct synthVoice *sv){
    // period = SampleRate / frequency in samples
    // Fill the buffer that's NOT currently being played
    if(buf_clr){
        fillBuf(audio_bufB, sv);
    } else {
        fillBuf(audio_bufA, sv);
    }
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

    // juggle two buffers (one is being played, other is being fed data)
    /*if(buf_clr){
        TIM2->CCR3L = audio_bufA[buf_idx];
        buf_idx++;
        if(buf_idx >= BUFFER_SIZE-1) {
            buf_idx = 0;
            buf_clr = 1;
        } 
    } else {
        TIM2->CCR3L = audio_bufB[buf_idx];
        buf_idx++;
        if(buf_idx >= BUFFER_SIZE-1) {
            buf_idx = 0;
            buf_clr = 0;
        }   
    }*/
    
    TIM2->CCR3L = sineLookupTable[20];    
    /* Clear update interrupt flag */
    

    audio_idx++;
    if (audio_idx >= sizeof(sineLookupTable)) {
        audio_idx = 0;
        GPIO_WriteReverse(LED_GPIO_PORT, (GPIO_Pin_TypeDef)LED_GPIO_PINS);
        TIM2->CCR3L = sineLookupTable[200];
    }
    TIM2->SR1 &= (uint8_t)(~TIM2_SR1_UIF);
}