#include "../../libs/adc.h"
#include "../../libs/pwm.h"
// #include <nRF24L01.h>
// #include <RF24.h>

void setup() {
    adc_enable(AT8, _BV(REFS0), _BV(REFS1), true);
    mux_set(ADC0);
    enable_pwm(PWM_3);
    ocr_set(PWM_3, 0);
}

void loop() {
    ocr_set(PWM_3, ADC_val - 127);
}

ISR(ADC_vect) {
    const volatile uint8_t* adc = (volatile uint8_t*)adc_configs[adc_mode].adc;
    ADC_val = *adc;
}