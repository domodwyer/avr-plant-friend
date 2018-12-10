#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// MAX_WATERINGS defines the upper bound on the number of times water is
// dispensed.
const uint8_t MAX_WATERINGS = 10;

// DELAY_HOURS defines the duration of time (in hours) between each water cycle.
const uint8_t DELAY_HOURS = 34;

// PUMP pins
const uint8_t PUMP_1 = PC5;
const uint8_t PUMP_2 = PC4;
const uint8_t PUMP_3 = PC3;

// Pump turns on the pump connected to the pin on port C identified by pump for
// the duration specified by seconds. The specified pump pin must be configured
// as an output pin before calling pump.
//
// Once the duration has elapsed, all pins on port C are set low.
void pump(uint8_t pump, uint8_t seconds) {
    // Turn on the specified pump
    PORTC = (1 << pump);

    // Wait for seconds duration
    uint8_t i;
    for (i = 0; i < seconds; i++) {
        _delay_ms(1000);
    }

    // Turn off all pumps
    PORTC = 0;
}

// sleep_hour blocks for 1 hour, excluding interrupt handling.
void sleep_hour() {
    uint8_t minutes;
    uint8_t seconds;
    for (minutes = 0; minutes < 60; minutes++) {
        for (seconds = 0; seconds < 60; seconds++) {
            _delay_ms(1000); 
        }
    }
}

// halt puts the MCU into an uninterruptible sleep.
void halt() {
    // Disable ADC
    ADCSRA = 0;

    set_sleep_mode(SLEEP_MODE_STANDBY);
    cli();
    sleep_enable();
    sleep_bod_disable();
    sleep_cpu();
}

// int main(void) {
//     // Set all the port C pins to output
//     DDRC = 0xFF;

//     uint8_t cycles;
//     for (cycles = 0; cycles < MAX_WATERINGS; cycles++) {
//         // Wait the configured number of hours
//         uint8_t hours;
//         for (hours = 0; hours < DELAY_HOURS; hours++) {
//             sleep_hour();
//         }

//         pump(PUMP_1, 3);
//         pump(PUMP_2, 3);
//         pump(PUMP_3, 3);
//     }

//     while (1) halt();
// }

int main(void) {
    // Set all the port C pins to output
    DDRC = 0xFF;
    
    while (1) {
        pump(PUMP_1, 3);
        pump(PUMP_2, 3);
        pump(PUMP_3, 3);

        uint8_t seconds;
        for (seconds = 0; seconds < 30; seconds++) {
            _delay_ms(1000); 
        }
    }
}