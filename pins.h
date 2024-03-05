#ifndef PIN_DEF_H
#define PIN_DEF_H

#include <avr/io.h>

/// Pins in PORTB.
#define BUTTON_PIN PINB0

#define PUMP_PIN_1 PINB3
#define OVERFLOW_SIGNAL_PIN_1 PINB1

#define PUMP_PIN_2 PINB4
#define OVERFLOW_SIGNAL_PIN_2 PINB2

/// The const BUTTON_PIN is reused in the place of these values - they must be
/// equal. Change these as necessary when changing BUTTON_PIN.
_Static_assert(DDB0 == BUTTON_PIN);
_Static_assert(PORTB0 == BUTTON_PIN);
_Static_assert(PCINT0 == BUTTON_PIN);

/// As above, but for the overflow pin.
_Static_assert(DDB1 == OVERFLOW_SIGNAL_PIN_1);
_Static_assert(PORTB1 == OVERFLOW_SIGNAL_PIN_1);

/// All pins differ.
_Static_assert(PUMP_PIN_1 != BUTTON_PIN);
_Static_assert(PUMP_PIN_1 != OVERFLOW_SIGNAL_PIN_1);
_Static_assert(PUMP_PIN_1 != OVERFLOW_SIGNAL_PIN_2);
_Static_assert(PUMP_PIN_1 != PUMP_PIN_2);
_Static_assert(PUMP_PIN_2 != BUTTON_PIN);
_Static_assert(PUMP_PIN_2 != OVERFLOW_SIGNAL_PIN_1);
_Static_assert(PUMP_PIN_2 != OVERFLOW_SIGNAL_PIN_2);
_Static_assert(BUTTON_PIN != OVERFLOW_SIGNAL_PIN_1);
_Static_assert(BUTTON_PIN != OVERFLOW_SIGNAL_PIN_2);
_Static_assert(OVERFLOW_SIGNAL_PIN_1 != OVERFLOW_SIGNAL_PIN_2);

/// Helper macro returning true if pin is set high in PORTB.
#define IS_HIGH(pin) (((PINB >> pin) & 1) > 0)

#endif /* PIN_DEF_H */
