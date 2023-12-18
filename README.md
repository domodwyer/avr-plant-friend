## Plant Friend

A plant friendly, attiny85-based AVR project.

* Event driven architecture
* Priority scheduling
* Minimal components
* Sampling software debounce
* Low power utilisation
* Watchdog timer sleeping
* Overflow detection / fail safe

2 pumps, same pump time, pumps every 24h.

## Water Overflow

Overflow detection is implemented per-pump, and is evaluated each prior to the
respective pump being activated. This allows water to build up during pumping,
but prevents further pumping until the water level has reduced.

## Programming

Ensure the button is not pressed, and neither overflow wire is connected.

