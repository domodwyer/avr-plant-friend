#include "event.h"
#include "event_handler/button.h"
#include "event_handler/watchdog.h"
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <stdbool.h>
#include <util/atomic.h>

/// A set of bit flags set
static volatile uint8_t EVENT_STATE = 0;

/// Return true if `flag` is set in `EVENT_STATE`.
static bool event_flag_is_set(uint8_t flag) {
  return (EVENT_STATE & flag) != 0;
}

/// Set/unset the specified `flag` in `EVENT_STATE`.
///
/// MUST be called in an atomic context (ISR or ATOMIC_BLOCK - see
/// event_flag_clear()) to preserve atomicity of the change.
void event_flag_set(uint8_t flag) { EVENT_STATE |= flag; }

/// @brief Clear any pending event flags.
void event_flag_reset() {
  // Note that this is NOT atomic block, as it is an atomic store (setting 0)
  // and not a RMW.
  EVENT_STATE = 0;
}

/// Clear the event flag atomically.
///
/// Because mutating the volatile EVENT_STATE is not atomic (compiles down to a
/// load, modify, store) care must be taken to avoid overwriting flag bits set
/// by an interleaved ISR.
static void event_flag_clear(uint8_t flag) {
  ATOMIC_BLOCK(ATOMIC_FORCEON) { EVENT_STATE &= ~flag; }
}

void event_loop_tick() {
  // Priority is always given to the training pin
  if (event_flag_is_set(EVENT_STATE_BUTTON)) {
    handle_event_button();
    event_flag_clear(EVENT_STATE_BUTTON);
  }

  // If the pin is in being actively pressed, process watchdog timer events.
  if (event_flag_is_set(EVENT_STATE_WDT)) {
    handle_event_watchdog();
    event_flag_clear(EVENT_STATE_WDT);
  }

  // Configure the sleep mode to enter the lowest power state.
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  // Perform a conditional sleep to avoid racing an incoming interrupt
  // intended to wake the device with the actual sleep.
  //
  // Ref:
  // https://www.nongnu.org/avr-libc/user-manual/group__avr__sleep.html#details
  //
  // Disable servicing interrupts
  cli();
  // At this point no interrupt can fire - the system state can be evaluated and
  // an action taken atomically.
  if (EVENT_STATE == 0) {
    sleep_enable();

    // Re-enable interrupts.
    //
    // This guarantees next instruction is executed atomically (no interrupt can
    // happen before the CPU sleep). Any pending (but not yet serviced)
    // interrupts will immediately wake the device.
    sei();
    sleep_cpu();
    sleep_disable(); // Wake here and restore system
  }
  // Always re-enable interrupts before yielding control.
  sei();
}

void inline run_event_loop() {
  while (1) {
    event_loop_tick();
  }
}