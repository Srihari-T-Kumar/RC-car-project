#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <wiring_private.h>

//ADC prescaling is 128 for regular, 64 for fast
// Specs specify 10-bit acc for 50kHz - 200kHz - prescaler is between 80 and 320

#define ADP_SET_REGULAR (_BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0))
#define ADP_CLEAR_REGULAR 0
#define ADP_SET_FAST (_BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0))
#define ADP_CLEAR_FAST  0

enum ADC_pin {ADC0, ADC1, ADC2, ADC3, ADC4, ADC5};
enum ADC_mode {MT10, MT8, AT10, AT8};

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
    }
};

static ADC_mode adc_mode;
static uint16_t ADC_val = 0;

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
    if(config.adate) sbi(ADCSRA, ADATE);
    if(config.acme) sbi(ADCSRB, ACME);
    if(int_en) sbi(ADCSRA, ADIE);
}

void adc_disable() {
    cbi(ADCSRA, ADEN);
    cbi(ADCSRA, ADIE);
    cbi(ADCSRB, ACME);
}

void adc_start_conv(ADC_pin pin) {
    const volatile void* adc = adc_configs[adc_mode].adc; //Load current configuration

    ADMUX |= pin;
    ADMUX &= (pin | 0xF0);
    ADCSRA |= _BV(ADSC);

    if(adc_mode == MANT10 || adc_mode == MANT8) {
        while(bit_is_set(ADCSRA, ADSC)); //Busy wait for ADSC bit on ADCSRA
        if(adc_mode == MANT10) ADC_val = *((volatile uint16_t*)adc);
        else ADC_val = *((volatile uint8_t*)adc);
    }
}