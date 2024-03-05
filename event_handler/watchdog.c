#include "watchdog.h"
#include "../halt.h"
#include "../pins.h"
#include "../wdt.h"
#include <avr/eeprom.h>
#include <stdbool.h>
#include <util/delay.h>

/// Specify the default pump value if not previously set by the user.
uint16_t EEMEM PUMP_ON_DURATION_SECONDS = 5;

/// @brief A helper enum to describe the state of the pump FSM.
enum PumpRoutineState {
  Pump1_On = 0,       // Must be 0 (no set bits)
  Pump1_Off = 1 << 0, // Other values must be a single set bit value
  Pump2_On = 1 << 1,
  Pump2_Off = 1 << 2
};

_Static_assert(Pump1_On == 0);
_Static_assert(Pump1_On != Pump1_Off);
_Static_assert(Pump1_On != Pump2_On);
_Static_assert(Pump1_On != Pump2_Off);
_Static_assert(Pump1_Off != Pump2_Off);
_Static_assert(Pump1_Off != Pump2_On);
_Static_assert(Pump2_On != Pump2_Off);

/// @brief Configure OVERFLOW_SIGNAL_PIN_1 as an input with pull ups.
void init_overflow_sensor() {
  // Set the button pin to input
  DDRB &= ~(1 << OVERFLOW_SIGNAL_PIN_1);

  // Enable the pull-up for the button pin
  PORTB |= (1 << OVERFLOW_SIGNAL_PIN_1);
}

/// @brief Pulse the pin 3 times in quick succession, flashing the pump LED.
void triple_flash(uint8_t pin) {
  for (uint8_t i = 3; i > 0; i--) {
    PORTB |= (1 << pin);
    _delay_ms(100);
    PORTB &= ~(1 << pin);
    _delay_ms(100);
  }
}

/// @brief Check if the provided overflow pin is high, and if so, turn on the
/// pump pin.
/// @param overflow_pin The overflow detection pin, overflowed when low.
/// @param pump_pin Pump pin to enable
/// @return True if the pump was enabled, false if overflowed.
static bool check_and_pump(uint8_t overflow_pin, uint8_t pump_pin) {
  // Check if the pump can run.
  if (!IS_HIGH(overflow_pin)) {
    triple_flash(pump_pin);
    return false;
  }

  // Turn the pump on.
  PORTB |= (1 << pump_pin);

  return true;
}

void handle_event_watchdog() {

  // The watering routine is a simple state machine:
  //
  //            ┌────────────────┐
  //            │                │
  //            │                ▼
  //            │          ┌───────────┐
  //            │          │ Pump1_On  │
  //            │          └───────────┘
  //            │                │             ┌ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┐
  //            │                │                  Enable Pump 1
  //            │          Overflowed? ──No───▶│ Sleep for Pump Time │
  //            │                │              ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─
  //            │              Yes                        │
  //            │                │                        │
  //            │                ▼                        │
  //            │          ┌───────────┐                  │
  //            │          │ Pump1_Off │◀─────────────────┘
  //            │          └───────────┘
  //     ┌ ─ ─ ─ ─ ─ ─ ┐         │
  //        Sleep 24h            ▼
  //     └ ─ ─ ─ ─ ─ ─ ┘   ┌───────────┐
  //            ▲          │ Pump2_On  │
  //            │          └───────────┘
  //            │                │             ┌ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┐
  //            │                │                  Enable Pump 2
  //            │          Overflowed? ──No───▶│ Sleep for Pump Time │
  //            │                │              ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─
  //            │              Yes                        │
  //            │                │                        │
  //            │                ▼                        │
  //            │          ┌───────────┐                  │
  //            │          │ Pump2_Off │◀─────────────────┘
  //            │          └───────────┘
  //            │                │
  //            │                │
  //            └────────────────┘
  //
  // Where control is yielded back to the event loop at each "sleep" point, and
  // the FSM resumes the next time this function is called.
  //
  // The current FSM state can be inferred from the pump pin states:

  enum PumpRoutineState NEXT_STEP = Pump1_On; // Default to starting the routine

  if (IS_HIGH(PUMP_PIN_1))
    NEXT_STEP |= Pump1_Off;

  if (IS_HIGH(PUMP_PIN_2))
    NEXT_STEP |= Pump2_Off;

  // At this point, the FSM state has been restored, and it can now be advanced.
  //
  // If both pumps were inadvertently enabled prior to this function being
  // called, the NEXT_STEP state is now invalid, and the default handler will be
  // invoked to disable the pumps and halt execution.

  switch (NEXT_STEP) {
  case Pump1_On:
    if (check_and_pump(OVERFLOW_SIGNAL_PIN_1, PUMP_PIN_1)) {
      // The pump is now on.
      wdt_sleep_seconds(eeprom_read_word(&PUMP_ON_DURATION_SECONDS));
      return; // Sleep and wait to be woken into Pump1_Off handler
    }

    // Fallthrough

  case Pump1_Off:
    // Turn off PUMP_1
    PORTB &= ~(1 << PUMP_PIN_1);

    // Small delay to let the pump/current settle
    _delay_ms(200);

    // Fallthrough

  case Pump2_On:
    if (check_and_pump(OVERFLOW_SIGNAL_PIN_2, PUMP_PIN_2)) {
      // The pump is now on.
      wdt_sleep_seconds(eeprom_read_word(&PUMP_ON_DURATION_SECONDS));
      return; // Sleep and wait to be woken into Pump2_Off handler
    }

    // Fallthrough

  case Pump2_Off:
    PORTB &= ~(1 << PUMP_PIN_2);
    return;

  default:
    halt();
  }
}
