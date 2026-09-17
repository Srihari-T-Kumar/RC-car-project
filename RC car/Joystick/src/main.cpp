#include "../../libs/adc.h"
#include "../../libs/vector.h"
#include <nRF24L01.h>
#include <RF24.h>

#define DEADZONE 20
#define ZERO {0, 0}

static volatile uint8_t x_val = 0, y_val = 0;
static volatile bool x_read = true;

static RF24 radio = RF24(9, 10);
static const uint8_t* address = (uint8_t *)"00001";

void setup() {
    adc_enable(AT8, _BV(REFS0), _BV(REFS1), true); //Enable the adc
    mux_set(ADC0);

    radio.begin(); //Enable radio transmission
    radio.openWritingPipe(address);
    radio.setPALevel(RF24_PA_MIN);
    radio.stopListening();
}

void loop() {
    Vec_32_t speeds;

    if(x_val > 127 - DEADZONE && x_val < 127 + DEADZONE && y_val > 127 - DEADZONE && y_val < 127 + DEADZONE) speeds = ZERO;
    else {
        int16_t x = x_val - 128;
        int16_t y = y_val - 128;

        speeds = transform({x, y});
    }

    radio.write(&speeds, sizeof(speeds));
}

ISR(ADC_vect) {
    const volatile uint8_t* adc = (volatile uint8_t*)adc_configs[adc_mode].adc;

    if(x_read) {
        x_val = *adc;
        x_read = false;
        mux_set(ADC1);
    }
    else {
        y_val = *adc;
        x_read = true;
        mux_set(ADC0);
    }
}