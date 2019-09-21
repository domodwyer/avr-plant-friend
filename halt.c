#include <assert.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>

// Set all PORTB low and stop the CPU.
void __attribute__((noinline)) halt() {
  PORTB = 0;           // Turn off all pumps
  MCUCR |= (1 << PUD); // Force off all pull-ups to minimise power draw
  cli();               // Disable interrupts to avoid being awoken
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_mode();
  assert(0);
}