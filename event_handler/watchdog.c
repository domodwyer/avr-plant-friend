#include "watchdog.h"
#include "../pins.h"
#include "../wdt.h"
#include <avr/delay.h>
#include <avr/eeprom.h>

/// Specify the default pump value if not previously set by the user.
uint16_t EEMEM PUMP_ON_DURATION_SECONDS = 5;

/// @brief Configure OVERFLOW_SIGNAL_PIN_1 as an input with pull ups.
void init_overflow_sensor() {
  // Set the button pin to input
  DDRB &= ~(1 << OVERFLOW_SIGNAL_PIN_1);

  // Enable the pull-up for the button pin
  PORTB |= (1 << OVERFLOW_SIGNAL_PIN_1);
}

void handle_event_watchdog() {
  // First, check if the pump is on.
  //
  // If so, this wakeup is to turn it off.
  if (IS_HIGH(PUMP_PIN_1)) {
    // Turn off the pump
    PORTB &= ~(1 << PUMP_PIN_1);

    // And wait for the next watering time
    wdt_sleep_seconds(PUMP_INTERVAL_SECONDS);
    return;
  }

  // If the code reaches this point, the pump was off, and this wakeup is the
  // start of the watering routine.
  //
  // * First check if the water overflow signal is high, if so, skip this
  //   watering and sleep until the next watering time.
  // * Turn the pump on.
  // * Sleep for the configured pump time, before waking and taking the branch
  //   above, turning off the pump.

  // If the water overflow signal is set, skip this watering.
  if (!IS_HIGH(OVERFLOW_SIGNAL_PIN_1)) {
    triple_flash();
    wdt_sleep_seconds(PUMP_INTERVAL_SECONDS);
    return;
  }

  // Turn the pump on.
  PORTB |= (1 << PUMP_PIN_1);

  // And sleep for the configured pump-on duration
  wdt_sleep_seconds(eeprom_read_word(&PUMP_ON_DURATION_SECONDS));
}

void triple_flash() {
  for (uint8_t i = 3; i--; i != 0) {
    PORTB |= (1 << PUMP_PIN);
    _delay_ms(100);
    PORTB &= ~(1 << PUMP_PIN);
    _delay_ms(100);
  }
}