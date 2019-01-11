# enigma - a cipher machine

monome serial -> usbmidi  
monome serial -> midi  
midi -> usbmidi  
usbmidi -> midi 
controller -> app -> midi  

enigma is a teensy 3.6 based USBhost device for monome to MIDI translation, hosted applications (polyearthsea et.al) and more.


## Compilation
* Upgrade to Arduino 1.8.8 and Teensyduino 1.45
* Ardiuno versions before 1.8.5 will likely error compile
* You must compile this with Tools->USB type set to Serial+MIDI.
* Be sure that the board speed is set to 120mhz (overclock) for maximum repsonsiveness.


## Input device support
* 2x monome devices (grid, arc)
* 4x MIDI Devices (USB or minijack)
* USB Hubs (to connect multiple devices)

Can also support these with some more coding
* Keyboard
* Mouse
* HID 

## Troubleshooting
* Update to the newest Arduino/Teensduino for USB hubs to work.
