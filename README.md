# piezoOsc

This repository contains a small sketch for turning an Arduino and a piezoelectrical sensor into a OSC capable drumpad. Along with it there is also a small PD patch for demo purpose.

The sketch depends on [CNMAT's OSC implementation](https://github.com/CNMAT/OSC).

## Build

Using [Arduino-IDE](http://www.arduino.org/downloads):

1. Download and extract (or clone) [CNMAT's OSC implementation](https://github.com/CNMAT/OSC) to your Arduino/libraries/ folder
1. Start Arduino IDE
1. Open 'piezoOsc.ino'
1. Set the Arduino parameter depending on your board (i.e. Board, Processor and Port)
1. Flash the sketch

## Default parameter

The following parameters are set as default:

Serial:

* Serial baudrate: 115200

OSC messages:

* /drum/<padNumber>/bang with one integer is emitted if a pad is hit. The integer tells the amplitude.
* /drum/<padNumber>/volume with one integer gives clues about the reverbration of the pad.
