#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <wiring_private.h>

#define ATMEGA328

#ifdef ATMEGA328

enum Timer_mode_pin {PWM_3, PWM_5, PWM_6, PWM_9, PWM_10, PWM_11};

typedef struct {
    int timer_id;
    volatile uint8_t *tccrA;
    volatile uint8_t *tccrB;
    volatile uint8_t *tccrC;
    volatile void *tcnt;
    volatile void *ocr;
    volatile uint8_t *timsk;
    volatile uint16_t *icr;
    uint8_t wgm_a_set;
    uint8_t wgm_a_clear;
    uint8_t wgm_b_set;
    uint8_t wgm_b_clear;
    uint8_t com_set;
    uint8_t com_clear;
    uint8_t cs_set;
    uint8_t cs_clear;
    uint8_t foc_set;
    uint8_t foc_clear;
    
    bool is_8_bit;
    volatile uint8_t *port;
    volatile uint8_t* ddr;
    uint8_t port_set;

} PWM_config;

static const PWM_config pwm_configs[] = {
    { //Timer 2, Fast-PWM, 8-bit, Freq = nearly 8Khz, prescaler 8, OC2B, Pin 3
        2, 
        &TCCR2A, &TCCR2B, nullptr, //TCCR
        &TCNT2, &OCR2B, //TCNT and OCR
        &TIMSK2, //TIMSK
        nullptr, //ICR
        _BV(WGM20) | _BV(WGM21), 0, //WGM_A
        0, _BV(WGM22), //WGM_B
        _BV(COM2B1), _BV(COM2B0), //COMB
        _BV(CS21), _BV(CS20) | _BV(CS22), //Clock Select
        0, 0,
        true,
        &PORTD, &DDRD, //PORT address
        _BV(3)
    },
    { //Timer 0, Fast-PWM, 8-bit, Freq = nearly 8Khz, prescaler 8, OC0B, Pin 5
        0, 
        &TCCR0A, &TCCR0B, nullptr, //TCCR
        &TCNT0, &OCR0B, //TCNT and OCR
        &TIMSK0, //TIMSK
        nullptr, //ICR
        _BV(WGM00) | _BV(WGM01), 0, //WGM_A
        0, _BV(WGM02), //WGM_B
        _BV(COM0B1), _BV(COM0B0), //COMB
        _BV(CS01), _BV(CS00) | _BV(CS02), //Clock Select
        0, 0,
        true,
        &PORTD, &DDRD,
        _BV(5)
    },
    {//Timer 0, Fast-PWM, 8-bit, Freq = nearly 8Khz, prescaler 8, OC0A, Pin 6
        0, 
        &TCCR0A, &TCCR0B, nullptr, //TCCR
        &TCNT0, &OCR0A, //TCNT and OCR
        &TIMSK0, //TIMSK
        nullptr, //ICR
        _BV(WGM00) | _BV(WGM01), 0, //WGM_A
        0, _BV(WGM02), //WGM_B
        _BV(COM0A1), _BV(COM0A0), //COMA
        _BV(CS01), _BV(CS00) | _BV(CS02), //Clock Select
        0, 0,
        true,
        &PORTD, &DDRD,
        _BV(6)
    },
    { //Timer 1, Fast-PWM, 10-bit, Freq = nearly 2Khz, prescaler 8, OC1A, Pin 9
        1, 
        &TCCR1A, &TCCR1B, nullptr, //TCCR
        &TCNT1, &OCR1A, //TCNT and OCR
        &TIMSK1, //TIMSK
        nullptr, //ICR
        _BV(WGM10) | _BV(WGM11), 0, //WGM_A
        _BV(WGM12), _BV(WGM13), //WGM_B
        _BV(COM1A1), _BV(COM1A0), //COMA
        _BV(CS11), _BV(CS10) | _BV(CS12), //Clock Select
        0, 0,
        false,
        &PORTB, &DDRB,
        _BV(1)
    },
    {//Timer 1, Fast-PWM, 10-bit, Freq = nearly 2Khz, prescaler 8, OC1B, Pin 10
        1, 
        &TCCR1A, &TCCR1B, nullptr, //TCCR
        &TCNT1, &OCR1B, //TCNT and OCR
        &TIMSK1, //TIMSK
        nullptr, //ICR
        _BV(WGM10) | _BV(WGM11), 0, //WGM_A
        _BV(WGM12), _BV(WGM13), //WGM_B
        _BV(COM1B1), _BV(COM1B0), //COMB
        _BV(CS11), _BV(CS10) | _BV(CS12), //Clock Select
        0, 0,
        false,
        &PORTB, &DDRB,
        _BV(2)
    },
    { //Timer 2, Fast-PWM, 8-bit, Freq = nearly 8KHz, prescaler 8, OC2A, Pin 11
        2, 
        &TCCR2A, &TCCR2B, nullptr, //TCCR
        &TCNT2, &OCR2A, //TCNT and OCR
        &TIMSK2, //TIMSK
        nullptr, //ICR
        _BV(WGM20) | _BV(WGM21), 0, //WGM_A
        0, _BV(WGM22), //WGM_B
        _BV(COM2A1), _BV(COM2A0), //COMA
        _BV(CS21), _BV(CS20) | _BV(CS22), //Clock Select
        0, 0,
        true,
        &PORTB, &DDRB,
        _BV(3)
    }
};

static bool timer_enabled[] = {false, false, false};
static bool pwm_pin_enabled[] = {false, false, false, false, false, false};

void enable_pwm(Timer_mode_pin pin) {
    const PWM_config &c = pwm_configs[pin];

    // Output compare setup
    *c.tccrA |= c.com_set;
    *c.tccrA &= ~c.com_clear;
    *c.ddr   |= c.port_set;
    // Timer mode + prescaler (only if this timer not already running)
    if (!timer_enabled[c.timer_id]) {
        *c.tccrA |= c.wgm_a_set;
        *c.tccrA |= c.wgm_a_clear;  
        *c.tccrB |= c.wgm_b_set | c.cs_set;
        *c.tccrB  &= ~(c.wgm_b_clear | c.cs_clear);
        timer_enabled[c.timer_id] = true;
    }
    pwm_pin_enabled[pin] = true;
}

void disable_pwm(Timer_mode_pin pin) {
    const PWM_config &c = pwm_configs[pin];

    *c.tccrA &= ~c.com_set; //Detaches OCnX
    *c.port &= ~c.port_set; //Drives the pin low

    uint8_t old_sreg = SREG;
    cli();

    if(c.is_8_bit) *((volatile uint8_t*)c.ocr) = 0; //Clears ocr
    else *((volatile uint16_t*)c.ocr) = 0;

    SREG = old_sreg;
    pwm_pin_enabled[pin] = false;

    bool timer_preserve = false;
    for(int i = 0; i < 6; i++) {
        if(pwm_configs[i].timer_id == c.timer_id && pwm_pin_enabled[i]) {
            timer_preserve = true;
            break;
        }
    }

    if(!timer_preserve) {
        *c.tccrB &= ~c.cs_set; //Stops the timer
        timer_enabled[c.timer_id] = false; 
    }
}

void ocr_set(Timer_mode_pin pin , uint16_t ocr_val) {
    if(!pwm_pin_enabled[pin]) return;
    const PWM_config &config = pwm_configs[pin];

    uint8_t old_sreg = SREG;
    cli();

    if(config.is_8_bit) *((volatile uint8_t*)config.ocr) = (uint8_t)ocr_val;
    else *((volatile uint16_t*)config.ocr) = ocr_val;

    SREG = old_sreg;
}


#endif