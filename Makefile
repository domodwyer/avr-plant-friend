DEVICE 		= attiny85
F_CPU 		= 8000000 # 8MHz
PROGRAMMER 	= usbasp
FUSES      	= -U lfuse:w:0xe2:m -U hfuse:w:0xdf:m

# Fuse calculator: http://www.engbedded.com/fusecalc/

######################################################

AVRDUDE = avrdude -c $(PROGRAMMER) -p $(DEVICE)
COMPILE = avr-gcc -Wall -Os -g -DF_CPU=$(F_CPU) -mmcu=$(DEVICE)

######################################################

SHELL := /usr/bin/env bash
.SHELLFLAGS := -eu -o pipefail -c
.DEFAULT_GOAL := help

MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

# File lists
SRC = $(shell find . -type f -name '*.c')
OBJECTS = ${SRC:.c=.o}

.PRECIOUS: ${OBJECTS} main.elf

%.o: %.c
	${COMPILE} -c $< -O1 -o $@

%.hex: %.elf
	avr-objcopy -j .text -j .data -O ihex $< $@
	avr-size --format=avr --mcu=$(DEVICE) $<

%.eep: %.elf
	avr-objcopy -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@

%.elf: ${OBJECTS}
	${COMPILE} -o $@ ${OBJECTS}

#? build: compile source, producing a flash and EEPROM image
.PHONY: build
build: main.hex main.eep

#? flash: flash the firmware onto the mcu
.PHONY: flash
flash: main.hex main.eep
	$(AVRDUDE) -U flash:w:main.hex:i -U eeprom:w:main.eep:i

#? fuse: apply fuse configuration to the mcu
.PHONY: fuse
fuse:
	$(AVRDUDE) $(FUSES)

#? clean: remove any generated files
.PHONY: clean
clean:
	-rm -f main.hex main.elf main.eep $(OBJECTS)

#? help: prints this help message
.PHONY: help
help:
	@echo "Usage:"
	@sed -n 's/^#?//p' ${MAKEFILE_LIST} | column -t -s ':' |  sed -e 's/^/ /'