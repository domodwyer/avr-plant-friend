#ifndef WDT_H
#define WDT_H

#include <avr/io.h>

/// The callback to be invoked by the WDT interrupt service routine.
extern void wdt_tick();

/// Enable the WDT interrupt timer.
extern void wdt_sleep_seconds(uint32_t duration_seconds);

/// Cancel a running WDT sleep countdown, or NOP if not running.
extern void wdt_cancel();

#endif /* WDT_H */