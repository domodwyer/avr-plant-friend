// Copyright 2024 Dominic Dwyer (dom@itsallbroken.com)
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//             http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#include "event.h"
#include "event_handler/button.h"
#include "event_handler/watchdog.h"
#include "halt.h"
#include "pins.h"
#include "wdt.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>

// Pin change interrupt service routine.
ISR(PCINT0_vect) { event_flag_set(EVENT_STATE_BUTTON); }

// Watchdog interrupt service routine.
//
// Delegates to the WDT event ticker.
ISR(WDT_vect) { wdt_tick(); }

int main(void) {
  // Disable all the peripherals (ADC, ACA, BOD, etc) to minimise current draw.
  power_all_disable();

  // Set all the pins to output.
  DDRB = 0xFF;

  // Configure the button pin & change interrupts.
  init_event_button();

  // Configure the water overflow sensor pin.
  init_overflow_sensor();

  // Start the watchdog timer to trigger an interrupt every WDT_INTERVAL
  // duration of time.
  wdt_sleep_seconds(PUMP_INTERVAL_SECONDS);

  // And drive the event loop.
  run_event_loop();

  // The event loop should never exit - turn off the pump (if on) and gracefully
  // explode.
  halt();
}
