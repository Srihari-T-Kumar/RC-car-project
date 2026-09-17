#include "../../libs/pwm.h"
#include "../../libs/vector.h"
#include <nRF24L01.h>
#include <RF24.h>

enum DIR_pin {MLF, MLB, MRF, MRB};

#define EPL PWM_5
#define EPR PWM_6
#define MAX_OCR 230
#define RX_TIMEOUT_MS 200
#define ZERO {0, 0}

static RF24 radio(9, 10);
static const uint8_t* address = (uint8_t*)"00001";

static uint32_t last_rx_ms;
static Vec_32_t last_speeds;

uint32_t actual_millis() { return millis()/8;}

void set_motor_speed(Vec_32_t last_speeds) {
    PORTD &= ~(_BV(MLF) | _BV(MLB) | _BV(MRF) | (_BV(MRB)));

    int16_t left = (int16_t)last_speeds.x, right = (int16_t)last_speeds.y;

    PORTD |= (left > 0) ? (_BV(MLF)) : (left < 0) ? (_BV(MLB)) : 0;
    PORTD |= (right > 0) ? (_BV(MRF)) : (right < 0) ? (_BV(MRB)) : 0;

    left = abs(left);
    right = abs(right);

    ocr_set(EPL, left);
    ocr_set(EPR, right);
}

void setup() {
    DDRD |= _BV(MLF) | _BV(MLB) | _BV(MRF) | (_BV(MRB));
    PORTD &= ~(_BV(MLF) | _BV(MLB) | _BV(MRF) | (_BV(MRB)));

    enable_pwm(EPL);
    enable_pwm(EPR);

    radio.begin();
    radio.openReadingPipe(1, address);
    radio.setPALevel(RF24_PA_MIN);
    radio.startListening();
}

void loop() {
    if(radio.available()) {
        radio.read(&last_speeds, sizeof(last_speeds));

        last_speeds.x = ((MAX_OCR/2)*last_speeds.x)/128;
        last_speeds.y = ((MAX_OCR/2)*last_speeds.y)/128;

        last_rx_ms = actual_millis();
    }

    if(actual_millis() - last_rx_ms > RX_TIMEOUT_MS) set_motor_speed(ZERO);
    else set_motor_speed(last_speeds);
}