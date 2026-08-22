#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <wiring_private.h>

#define ATMEGA328

//ADC prescaling is 128 for regular, 64 for fast
// Specs specify 10-bit acc for 50kHz - 200kHz - prescaler is between 80 and 320

#ifdef ATMEGA328

#define ADP_SET_REGULAR (_BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0))
#define ADP_CLEAR_REGULAR 0
#define ADP_SET_FAST (_BV(ADPS2) | _BV(ADPS1))
#define ADP_CLEAR_FAST  _BV(ADPS0)

enum ADC_pin {ADC0, ADC1, ADC2, ADC3, ADC4, ADC5};
enum ADC_mode {MT10, MT8, AT10, AT8}; //MT - manual trigger, AT - auto trigger

/*For manual trigger use the adc_start_conv() fn, 
else write your own irq handler and set the mux manually*/

typedef struct {
    volatile void *adc;
    bool adlar;
    bool adate;
    bool acme;
    uint8_t adts_set;
    uint8_t adts_clear;
} ADC_config;

static const ADC_config adc_configs[] = {
    {//Using full 10-bit, manual trigger
        &ADC,
        false,
        false,
        false,
        0, 0 //ADTS
    },
    {//Using only 8-bit, manual trigger
        &ADCH,
        true,
        false,
        false,
        0, 0 //ADTS
    },
    {//Using 10-bit, auto trigger, Timer0 overflow
        &ADC,
        false,
        true,
        false,
        _BV(ADTS2), _BV(ADTS0) | _BV(ADTS1)
    },
    {//Using 8-bit, auto trigger, Timer0 overflow
        &ADCH,
        true,
        true,
        false,
        _BV(ADTS2), _BV(ADTS0) | _BV(ADTS1)
    }
};

static ADC_mode adc_mode;
static volatile uint16_t ADC_val = 0;

void adc_enable(ADC_mode mode, uint8_t ref_s, uint8_t ref_c, bool int_en) {
    const ADC_config &config = adc_configs[mode];
    adc_mode = mode;
    
    ADMUX |= ref_s; //Reference Voltage setting
    ADMUX &= ~ref_c;

    if(config.adlar) { //8-bit resolution and fast conversion
        sbi(ADMUX, ADLAR);
        ADCSRA |= ADP_SET_FAST;
        ADCSRA &= ~ADP_CLEAR_FAST;
    }
    else { //10-bit resolution and slow conversion
        ADCSRA |= ADP_SET_REGULAR;
        ADCSRA &= ~ADP_CLEAR_REGULAR;
    }
    if(config.adate) {
        ADCSRB |= config.adts_set;
        ADCSRB &= ~config.adts_clear;
        sbi(ADCSRA, ADATE);
    }
    if(config.acme) sbi(ADCSRB, ACME);
    if(int_en) sbi(ADCSRA, ADIE);
    sbi(ADCSRA, ADEN);
}

void adc_disable() {
    cbi(ADCSRA, ADEN);
    cbi(ADCSRA, ADIE);
    cbi(ADCSRB, ACME);
}

void mux_set(ADC_pin pin) {
    ADMUX |= (ADMUX & 0xf0) | pin;
}

void adc_start_conv(ADC_pin pin) {
    const volatile void* adc = adc_configs[adc_mode].adc; //Load current configuration
    mux_set(pin);
    
    ADCSRA |= _BV(ADSC);

    if(adc_mode == MT10 || adc_mode == MT8) {
        while(bit_is_set(ADCSRA, ADSC)); //Busy wait for ADSC bit on ADCSRA
        if(adc_mode == MT10) ADC_val = *((volatile uint16_t*)adc);
        else ADC_val = *((volatile uint8_t*)adc);
    }
}

#endif