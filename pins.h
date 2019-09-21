#ifndef PIN_DEF_H
#define PIN_DEF_H

#include <avr/io.h>

/// Pins in PORTB.
#define PUMP_PIN PINB4
#define BUTTON_PIN PINB0
#define OVERFLOW_SIGNAL_PIN PINB3

/// The const BUTTON_PIN is reused in the place of these values - they must be
/// equal. Change these as necessary when changing BUTTON_PIN.
_Static_assert(DDB0 == BUTTON_PIN);
_Static_assert(PORTB0 == BUTTON_PIN);
_Static_assert(PCINT0 == BUTTON_PIN);

/// As above, but for the overflow pin.
_Static_assert(DDB3 == OVERFLOW_SIGNAL_PIN);
_Static_assert(PORTB3 == OVERFLOW_SIGNAL_PIN);

/// All pins differ.
_Static_assert(PUMP_PIN != BUTTON_PIN);
_Static_assert(PUMP_PIN != OVERFLOW_SIGNAL_PIN);
_Static_assert(BUTTON_PIN != OVERFLOW_SIGNAL_PIN);

/// Helper macro returning true if pin is set high in PORTB.
#define IS_HIGH(pin) (((PINB >> pin) & 1) > 0)

#endif /* PIN_DEF_H */