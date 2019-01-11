# enigma - a cipher machine

```
monome serial -> usbmidi  
monome serial -> midi  
midi -> usbmidi  
usbmidi -> midi 
controller -> app -> midi  
```
enigma is a teensy 3.6 based USBhost device for monome to MIDI translation, hosted applications (polyearthsea et.al) and more.

## Plug connections (Steckerverbindungen)
- usbmidi (USB A connector)
- usbmidi (USB micro B connector)
- midi minijack in
- midi minijack out
- i2c minijack

## Wheel order (Walzenlage)

## Ring settings (Ringstellung)

## Starting position (Grundstellung)

## Indicator


## Compilation
- You must compile this with Tools->USB type set to Serial+MIDI.
- Be sure that the board speed is set to 96mhz or 120mhz (overclock) for maximum repsonsiveness.
- Tested with Arduino 1.8.8 and Teensyduino 1.45
- Most recent Teensyduino is required (1.45)
- Ardiuno versions before 1.8.5 will likely error on compile (due to old midi libs)


## Input device support
- 2x monome devices (grid, arc)
- 4x MIDI Devices (USB or minijack)
- USB Hubs (to connect multiple devices)

Can also support these with some more coding
- Keyboard
- Mouse
- HID 

## Troubleshooting
- Upgrade to Arduino 1.8.8 and Teensyduino 1.45
- Update to the newest Arduino/Teensduino for USB hubs to work.
