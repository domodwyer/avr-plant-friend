#include "wdt.h"
#include "event.h"
#include "halt.h"
#include <assert.h>
#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/atomic.h>

// Ref table Table 8-2 attiny13A datasheet.
static const uint8_t WDT_1_SECOND = (1 << WDP2) | (1 << WDP1);
static const uint8_t WDT_2_SECOND = (1 << WDP2) | (1 << WDP1) | (1 << WDP0);
static const uint8_t WDT_4_SECOND = (1 << WDP3);
static const uint8_t WDT_8_SECOND = (1 << WDP3) | (1 << WDP0);

/// @brief The duration of time to count down before emitting EVENT_STATE_WDT.
static volatile uint32_t WDT_SLEEP_REMAINING_SECONDS = 0;

/// @brief The duration of time the current WDT sleep shall last.
///
/// After the WTD interrupt fires, this duration of time has elapsed.
static volatile uint8_t WDT_THIS_SLEEP_SECONDS = 0;

static void configure_sleep();
static uint8_t maximal_interval_mask(uint8_t duration_seconds);
static uint8_t interval_mask_to_seconds(uint8_t interval_mask);

/// @brief Process a WDT interrupt.
///
/// This MUST be called from an interrupt context in response to all WDT
/// interrupts.
inline void wdt_tick() {
  // Otherwise adjust the countdown timer, taking care to avoid an overflow by
  // perform a saturating subtraction.
  if ((uint8_t)WDT_SLEEP_REMAINING_SECONDS >= WDT_THIS_SLEEP_SECONDS) {
    WDT_SLEEP_REMAINING_SECONDS -= WDT_THIS_SLEEP_SECONDS;
  } else {
    WDT_SLEEP_REMAINING_SECONDS = 0;
  }

  configure_sleep();
}

/// @brief Configure the WDT to cause an EVENT_STATE_WDT to be emitted after the
/// configured duration, replacing any existing WDT sleep countdown.
/// @param duration_seconds Approximate number of seconds in the future to set
/// the EVENT_STATE_WDT flag.
///
/// Always enables interrupts before returning.
void wdt_sleep_seconds(uint32_t duration_seconds) {
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    // This sleep will last this long, woken at the maximal interval
    WDT_SLEEP_REMAINING_SECONDS = duration_seconds;
    // Configure the watchdog to sleep.
    configure_sleep();
  }
}

/// @brief Disable the watchdog timer, and clear any pending interrupts for it.
void wdt_cancel() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    WDTCR |= (1 << WDCE) | (1 << WDE);
    WDTCR = 0;
  }
}

/// @brief Configure the WDT interrupt for the maximal interval until
/// WDT_SLEEP_REMAINING_SECONDS.
///
/// Disables the WDT if WDT_SLEEP_REMAINING_SECONDS == 0 to prevent spurious
/// wakeups and immediately sets the EVENT_STATE_WDT flag.
///
/// MUST be called while interrupts are disabled.
static void configure_sleep() {
  // If there's nothing to sleep for, immediately emit the event and return.
  //
  // This prevents an invalid mask being used after this conditional.
  if (WDT_SLEEP_REMAINING_SECONDS == 0) {
    // Disable the watchdog interrupt when no sleep is required.
    WDTCR |= (1 << WDCE) | (1 << WDE);
    WDTCR = 0;

    event_flag_set(EVENT_STATE_WDT);
    return;
  }

  // Select the maximal sleep interval mask for the remaining duration.
  uint8_t interval_mask = maximal_interval_mask(WDT_SLEEP_REMAINING_SECONDS);

  WDT_THIS_SLEEP_SECONDS = interval_mask_to_seconds(interval_mask);

  // The sequence for clearing WDE and changing time-out configuration is as
  // follows:
  //
  //     1. In the same operation, write a logic one to the Watchdog change
  //     enable bit (WDCE) and WDE. A logic one must be written to WDE
  //     regardless of the previous value of the WDE bit.
  WDTCR |= (1 << WDCE) | (1 << WDE);

  //     2. Within the next four clock cycles, write the WDE and Watchdog
  //     prescaler bits (WDP) as desired, but with the WDCE bit cleared.
  //     This must be done in one operation.

  // Disable the watchdog reset by clearing the WDE bit set above, and enable
  // the watchdog interrupt by setting WDTIE.
  WDTCR = (1 << WDIE) | interval_mask;
}

/// @brief Return the largest WDT sleep interval that is less than or equal to
/// duration_seconds.
/// @param duration_seconds The number of seconds the caller would like to
/// sleep for.
/// @return The WDTCR duration bitmask.
static inline uint8_t maximal_interval_mask(uint8_t duration_seconds) {
  // Note the input truncates the u32 to a u8, which covers the full range of
  // the interval masks while saving a few bytes.

  if (duration_seconds >= 8) {
    return WDT_8_SECOND;
  }
  if (duration_seconds >= 4) {
    return WDT_4_SECOND;
  }
  if (duration_seconds >= 2) {
    return WDT_2_SECOND;
  }
  if (duration_seconds >= 1) {
    return WDT_1_SECOND;
  }

  halt();
}

/// @brief Convert the WDTCR duration bitmask to the number of seconds it sleeps
/// @param interval_mask The WDTCR duration bitmask.
/// @return The number of seconds the bitmask configures the watchdog to sleep.
static inline uint8_t interval_mask_to_seconds(uint8_t interval_mask) {
  switch (interval_mask) {
  case WDT_1_SECOND:
    return 1;
  case WDT_2_SECOND:
    return 2;
  case WDT_4_SECOND:
    return 4;
  case WDT_8_SECOND:
    return 8;
  default:
    halt();
  }
}