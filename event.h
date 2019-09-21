#ifndef EVENT_H
#define EVENT_H

#include <avr/io.h>

/// Flag bit definitions.
#define EVENT_STATE_WDT (1 << 0)    // Watchdog timer interrupt fired
#define EVENT_STATE_BUTTON (1 << 1) // PCINT interrupt fired

_Static_assert(EVENT_STATE_WDT != EVENT_STATE_BUTTON);

/// @brief Execute event loop.
///
/// Blocks forever.
extern void run_event_loop();

/// Set the specified `flag` in `EVENT_STATE`.
extern void event_flag_set(uint8_t flag);

/// Reset the event flags, clearing all pending flags.
extern void event_flag_reset();

#endif /* EVENT_H */