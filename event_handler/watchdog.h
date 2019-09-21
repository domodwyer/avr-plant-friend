#ifndef HANDLER_WATCHDOG_H
#define HANDLER_WATCHDOG_H

#include <avr/eeprom.h>

/// How long to sleep between pump routines.
#define PUMP_INTERVAL_SECONDS ((uint16_t)60 * 60 * 24)

extern uint16_t EEMEM PUMP_ON_DURATION_SECONDS;

extern void handle_event_watchdog();
extern void init_overflow_sensor();

#endif /* HANDLER_WATCHDOG_H */