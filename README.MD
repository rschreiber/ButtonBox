# Button Box software

## Dev Env

- VSCode
- PlatformIO extension


## Intro

This is a work-in-progress project for a button box that simulates keyboard input.

At present, it makes use of 2 x Arduino Nano's and 1 x Leonardo Tiny Atmega32U4 (a clone of the Arduino Leonardo in a really small packaging).

The Leonardo acts as an I2C master and polls the Nano boards. The Nano boards act as controllers for two banks of buttons in the button box.

I originally started out with a multiplexer for the buttons (and could run this all on a single board), however, the addition of rotary encoders quickly made me run out of pins, so now each Nano runs a 'bank' of buttons on the button box.

