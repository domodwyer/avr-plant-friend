#include "button.h"
#include "../event.h"
#include "../pins.h"
#include "../wdt.h"
#include "watchdog.h"
#include <avr/eeprom.h>
#include <avr/power.h>
#include <stdbool.h>
#include <util/atomic.h>

/// The following timer configuration assumes a clock frequency of 8MHz
_Static_assert(F_CPU == 8000000);

/// Accumulator states for debouncing the button.
#define BUTTON_DOWN 0b00000000
#define BUTTON_UP 0b11111111

/// The value of TIMER_TICKS after ~1 second.
#define ONE_SECOND_TICKS 1000

/// @brief Enable the BUTTON_PIN change interrupt, assuming the Pin Change
/// Interrupt Enable bit is set in the General Interrupt Mask Register.
static void enable_pin_change_interrupt() { PCMSK |= (1 << BUTTON_PIN); }
/// @brief Disable the BUTTON_PIN change interrupt, leaving the other pin
/// interrupts unchanged.
static void disable_pin_change_interrupt() { PCMSK &= ~(1 << BUTTON_PIN); }

/// A timer incremented approximately once every millisecond.
static volatile uint32_t TIMER_TICKS = 0;
ISR(TIM0_COMPA_vect) { TIMER_TICKS += 1; } // ISR fires every 1ms.

/// @brief Configure the TIM0_COMPA_vect interrupt to fire every millisecond.
static void timer0_init() {
  power_timer0_enable();

  // Set the value of A that when reached, causes an interrupt at
  // 1ms/1000Hz.
  //
  // At 8MHz, and a timer pre-scaler divisor of 64, a total of 125 ticks is
  // exactly 1ms (125*64 = 8000, which is 1/1000th of 8,000,000).
  OCR0A = 125;

  TCCR0B = (1 << CS01) | (1 << CS00); // Pre-scaler: DIV64
  TCCR0A = (1 << WGM01);              // CTC mode, int. on value match of OCR0A
  TIMSK |= (1 << OCIE0A);             // Enable a compare-match interrupt on A

  TIMER_TICKS = 0;
}

static void debounce() {
  // Now de-bounce the pin and wait for one of two events:
  //
  // * The pin goes back to 0 -> pump test
  // * 1 second elapses -> start pump, recording the button depressed time

  // Start the 1ms interrupt timer.
  timer0_init();

  uint8_t accumulator = 0;
  uint8_t LAST_TICK = 0; // The LSB of TIMER_TICKS to save memory
  bool started = false;

  while (1) {
    // Wait for a tick event every ~1ms
    while (TIMER_TICKS == LAST_TICK) {
    }
    LAST_TICK = TIMER_TICKS;

    // Read the button state
    uint8_t state = (PINB >> BUTTON_PIN) & 1;

    // Add it to the accumulator
    accumulator <<= 1;
    accumulator |= state;

    // Process the debounced state
    switch (accumulator) {
    case BUTTON_UP: {
      // Always ensure the pump is now turned off.
      PORTB &= ~(1 << PUMP_PIN_1);

      // If the button debounce state never passed through the "on" state, then
      // simply return - the button was never "truly" pressed.
      if (!started)
        return;

      // The button has been released.
      //
      // If the time depressed was less than ~1 second, perform a pump test run.
      if (TIMER_TICKS < ONE_SECOND_TICKS) {
        handle_event_watchdog();
        return;
      }

      // Otherwise record the duration of the button being held down, and use it
      // as the new duration the pumps should be configured to run for.
      eeprom_write_word(&PUMP_ON_DURATION_SECONDS,
                        TIMER_TICKS / ONE_SECOND_TICKS);

      wdt_sleep_seconds(PUMP_INTERVAL_SECONDS);
      return;
    }

    case BUTTON_DOWN:
      // If the depress was already registered, do nothing.
      if (!started) {
        started = true;
        // The button has been depressed for the first time.
        //
        // Start counting the ticks to measure the duration of time spent
        // pressed.
        TIMER_TICKS = 0;
        // Reset LAST_TICK after TIMER_TICKS, so that if a tick happens
        // concurrently, the spin loop immediately unblocks.
        LAST_TICK = 0;
        continue;
      }

      // If more than one second, start the training routine by turning on the
      // pump.
      if (TIMER_TICKS >= ONE_SECOND_TICKS) {
        PORTB |= (1 << PUMP_PIN_1);
      }

      continue;

    default:
      continue;
    }
  }
}

void handle_event_button() {
  // First always stop the pumps, if running.
  PORTB &= ~(1 << PUMP_PIN_1);
  PORTB &= ~(1 << PUMP_PIN_2);

  // NOTE: If a pin-change interrupt occurred before this event handler disabled
  // the pin change interrupts, or a WDT interrupt and set the WDT event flag
  // before this handler disabled the WDT, then the respective interrupt flag
  // and/or event flag may be set already.
  //
  // Do not allow more pin change interrupts to fire. This does NOT clear any
  // queued interrupt.
  disable_pin_change_interrupt();

  // Disable the watchdog timer and clear any interrupt
  wdt_cancel();

  // Clear the event flags should either interrupt have fired before being
  // disabled.
  event_flag_reset();

  // Process the button change logic.
  debounce();

  // Disable the timer interrupt.
  TIMSK &= ~(1 << OCIE0A);

  // Disable the timer to minimise the power draw.
  power_timer0_disable();

  // Re-enable pin change interrupts to allow this code to be reached again.
  enable_pin_change_interrupt();
}

/// @brief Configure BUTTON_PIN as an input with pull ups, and enable pin
/// change interrupts.
void init_event_button() {
  // Set the button pin to input
  DDRB &= ~(1 << BUTTON_PIN);

  // Enable the pull-up for the button pin
  PORTB |= (1 << BUTTON_PIN);

  // Enable pin change interrupts for this pin
  GIMSK |= (1 << PCIE);
  enable_pin_change_interrupt();
}
