/* Enigma
 *

 *
 */

#include <Bounce.h>
#include <EEPROM.h>
#include <MIDI.h>
#include <USBHost_t36.h>

// OSC ??
#include <OSCBoards.h>
#include <OSCBundle.h>
#include <OSCData.h>
#include <OSCMatch.h>
#include <OSCMessage.h>
#include <OSCTiming.h>
#include <SLIPEncodedSerial.h>
#include <SLIPEncodedUSBSerial.h>

#define USBBAUD 115200
#define USBFORMAT USBHOST_SERIAL_8N1

// USER DEFINES

#define BUTTONCOUNT 2
#define BUTTONDEBOUNCE 10 // ms
#define BUTTON1 7
#define BUTTON2 8
#define LED 13   // teensy built-in LED
#define LED1 23  // enigma LED 1
#define LED2 22  // enigma LED 2
#define USBDRIVERCOUNT 10
#define HIDDEVICECOUNT 3

USBHost myusb;  // usb host mode
USBHub hub1(myusb);
USBHub hub2(myusb);
USBSerial userial1(myusb);
USBSerial userial2(myusb);
MIDIDevice midi01(myusb);
MIDIDevice midi02(myusb);
MIDIDevice midi03(myusb);
MIDIDevice midi04(myusb);

// for future use - see USBHost_t36 "mouse" example for more
USBHIDParser hid1(myusb);
KeyboardController keyboard1(myusb);
MouseController mouse1(myusb);
JoystickController joystick1(myusb);
RawHIDController rawhid1(myusb);

// SYSEX RELATED
// toggle indicates when toggle behavior in effect
int sysExToggled[3] = { false, false, false };

// an array of sysEx bytes, + defaults until EEPROM is loaded
int sysExMem[14] = { 1, 4, 7, 64, 0, 65, 0, 66, 0 };

long buttonval[BUTTONCOUNT]{};
Bounce pushbutton1 = Bounce(BUTTON1, BUTTONDEBOUNCE);
Bounce pushbutton2 = Bounce(BUTTON2, BUTTONDEBOUNCE);
Bounce *buttons[BUTTONCOUNT]{ &pushbutton1, &pushbutton2 };

// Create the Hardware MIDI-out port
MIDI_CREATE_DEFAULT_INSTANCE();

USBDriver *drivers[USBDRIVERCOUNT] = { &hub1,   &hub2,   &userial1, &userial2, &midi01,
                         &midi02, &midi03, &midi04,   &hid1,     &keyboard1 };
const char *driver_names[USBDRIVERCOUNT] = { "Hub1",     "Hub2",  "USERIAL1",
                                          "USERIAL2", "MIDI1", "MIDI2",
                                          "MIDI3",    "MIDI4", "HID",
                                          "Keyboard" };
bool driver_active[USBDRIVERCOUNT] = { false, false, false, false, false,
                         false, false, false, false, false };

// Lets also look at HID Input devices
USBHIDInput *hiddrivers[HIDDEVICECOUNT] = { &mouse1, &joystick1, &rawhid1 };
const char *hid_driver_names[HIDDEVICECOUNT] = { "Mouse1", "Joystick1",
                                              "RawHid1" };
bool hid_driver_active[HIDDEVICECOUNT] = { false, false, "false" };

// LED array
int leds[]{ 23, 22, 13 };  // 13 is teensy led

// map encoder # to a CC
int encoderCCs[]{ 16, 17, 18, 19, 20, 21, 22, 23 };

// MIDI CHANNEL
const byte midiChannel = 1;  // The MIDI Channel to send the commands over

// A variable to know how long the LED has been turned on
elapsedMillis ledOnMillis;

/* --- */
// kruft?
String deviceID = "monome";
String serialNum = "m1000010";  // this # does not get used -  serial number
                                // from usb_names is picked up instead
const uint8_t gridX = 16;       // Will be either 8 or 16
const uint8_t gridY = 8;        // standard for 128
/* --- */

// SETUP

void setup() {
    MIDI.begin(MIDI_CHANNEL_OMNI);
    Serial.begin(115200);

    // BUTTON SETUP
    pinMode(BUTTON1, INPUT_PULLUP);  // BUTTON 1
    pinMode(BUTTON2, INPUT_PULLUP);  // BUTTON 2

    // LED SETUP
    pinMode(LED, OUTPUT);  // LED pin
    digitalWrite(LED, LOW);
    pinMode(LED1, OUTPUT);  // LED1 pin
    digitalWrite(LED1, HIGH);
    pinMode(LED2, OUTPUT);  // LED2 pin
    digitalWrite(LED2, HIGH);


    // EEPROM - read EEPROM for sysEx data
    if (EEPROM.read(0) > 0 && EEPROM.read(0) < 17) {
        // EEPROM @ 0 is non-zer0 or not a viable MIDI channel
        for (int i = 0; i <= 8; i++) {  // read EEPROM for sysEx data
            sysExMem[i] = EEPROM.read(i);
            delay(5);  // give time for EEPROM read??
        }
        // int chnl = sysExMem[0];
    }

    // Wait 1.5 seconds before turning on USB Host.  If connected USB devices
    // use too much power, Teensy at least completes USB enumeration, which
    // makes isolating the power issue easier.
    delay(1500);
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    
    myusb.begin();
    Serial.println("\n\nUSB Host - Serial");

    // get device info
    deviceInfo();

    // USB connected midi device i/o
    midi01.setHandleNoteOn(myNoteOn);
    midi01.setHandleNoteOff(myNoteOff);
    midi01.setHandleControlChange(myControlChange);

    midi02.setHandleNoteOn(myNoteOn);
    midi02.setHandleNoteOff(myNoteOff);
    midi02.setHandleControlChange(myControlChange);

    midi03.setHandleNoteOn(myNoteOn);
    midi03.setHandleNoteOff(myNoteOff);
    midi03.setHandleControlChange(myControlChange);

    midi04.setHandleNoteOn(myNoteOn);
    midi04.setHandleNoteOff(myNoteOff);
    midi04.setHandleControlChange(myControlChange);

    // HARDWARE MIDI
    MIDI.setHandleNoteOn(myNoteOn);
    MIDI.setHandleNoteOff(myNoteOff);
    MIDI.setHandleControlChange(myControlChange);

    // USB MIDI
    usbMIDI.setHandleNoteOn(myNoteOn);
    usbMIDI.setHandleNoteOff(myNoteOff);
    usbMIDI.setHandleControlChange(myControlChange);
    usbMIDI.setHandleSystemExclusive(mySystemExclusive);

    // writeInt(0x12);

    delay(2000);
}

// MAIN LOOP

void loop() {
    bool activity = false;
    myusb.Task();

    // START BUTTONS LOOP
    for (byte z = 0; z < BUTTONCOUNT; z++) {
        buttons[z]->update();
        if (buttons[z]->risingEdge()) {  // release
            //  do release things - like turn off LED
            digitalWrite(leds[z], LOW);  // LED off
            buttonval[z] = 0;
            Serial.print("button:");
            Serial.print(z + 1);
            Serial.println(" released");
        }
        if (buttons[z]->fallingEdge()) {  // press
            // do press things - like turn on LED
            digitalWrite(leds[z], HIGH);  // LED on
            buttonval[z] = 1;
            Serial.print("button:");
            Serial.print(z + 1);
            Serial.println(" pressed");
        }
    }  // END BUTTONS LOOP


    // Print out information about different devices.
    deviceInfo();


    // Read MIDI from USB HUB connected MIDI Devices
    while (midi01.read() && driver_active[4]) { activity = true; }
    while (midi02.read() && driver_active[5]) { activity = true; }
    while (midi03.read() && driver_active[6]) { activity = true; }
    while (midi04.read() && driver_active[7]) { activity = true; }

    while (MIDI.read()) { activity = true; }

    // Read USB MIDI
    while (usbMIDI.read()) {
        // controllers must call .read() to keep the queue clear even if they
        // are not responding to MIDI
        activity = true;
    }


    // process incoming serial from Monomes
    if (userial1.available() > 0) {
        do {
            processSerial(userial1);
            activity = true;
        } while (userial1.available() > 16);
    }
    if (userial2.available() > 0) {
        do {
            processSerial(userial2);
            activity = true;
        } while (userial2.available() > 16);
    }

    // blink the LED when any activity has happened
    if (activity) {
        digitalWriteFast(leds[0], HIGH);  // LED on
        ledOnMillis = 0;
    }
    if (ledOnMillis > 15) {
        digitalWriteFast(leds[0], LOW);  // LED off
    }
}


// MISC FUNCTIONS

uint8_t readInt(USBSerial &thisSerial) {
    uint8_t val = thisSerial.read();
    return val;
}

void writeInt(uint8_t value, USBSerial &thisSerial) {
    // standard is to write out the 8 bit value on serial
    thisSerial.write(value);
}

void processSerial(USBSerial &thisSerial) {
    uint8_t identifierSent;  // command byte sent from controller to matrix
    uint8_t gridNum;  // for reading in data not used by the matrix
    uint8_t readX, readY;    // x and y values read from driver
    uint8_t devSect, devNum;  // x and y device size read from driver
    uint8_t i, n;
    int8_t d;
    int8_t note;

    identifierSent = thisSerial.read();  // get command identifier: first byte
                                         // of packet is identifier in the form:
                                         // [(a << 4) + b]
    // a = section (ie. system, key-grid, digital, encoder, led grid, tilt)
    // b = command (ie. query, enable, led, key, frame)
    switch (identifierSent) {
        case 0x00:  // device information
            // Serial.println("0x00");
            devSect =
                readInt(thisSerial);       // system/query response 0x00 -> 0x00
            devNum = readInt(thisSerial);  // grids
            Serial.print("section: ");
            Serial.print(devSect);
            Serial.print(", number: ");
            Serial.print(devNum);
            Serial.println(" ");
            break;

        case 0x01:  // system / ID
            // Serial.println("0x01");
            for (i = 0; i < 32; i++) {  // has to be 32
                Serial.print(readInt(thisSerial));
            }
            Serial.println(" ");
            break;

        case 0x02:  // system / report grid offset - 4 bytes
            // Serial.println("0x02");
            gridNum = readInt(thisSerial);  // n = grid number
            readX = readInt(
                thisSerial);  // an offset of 8 is valid only for 16 x 8 monome
            readY = readInt(
                thisSerial);  // an offset is invalid for y as it's only 8
            Serial.print("n: ");
            Serial.print(gridNum);
            Serial.print(", x: ");
            Serial.print(readX);
            Serial.print(" , y: ");
            Serial.print(readY);
            Serial.println(" ");
            break;

        case 0x03:  // system / report grid size
            // Serial.println("0x03");
            readX = readInt(
                thisSerial);  // an offset of 8 is valid only for 16 x 8 monome
            readY = readInt(
                thisSerial);  // an offset is invalid for y as it's only 8
            Serial.print("x: ");
            Serial.print(readX);
            Serial.print(" y: ");
            Serial.print(readY);
            Serial.println(" ");
            break;

        case 0x04:  // system / report ADDR
            // Serial.println("0x04");
            readX = readInt(thisSerial);  // a ADDR
            readY = readInt(thisSerial);  // b type
            break;

        case 0x0F:  // system / report firmware version
            // Serial.println("0x0F");
            for (i = 0; i < 8; i++) {  // 8 character string
                Serial.print(readInt(thisSerial));
            }
            break;

        case 0x20:
            /*
             * 0x20 key-grid / key up
             bytes: 3
             structure: [0x20, x, y]
             description: key up at (x,y)

             0x21 key-grid / key down
             bytes: 3
             structure: [0x21, x, y]
             description: key down at (x,y)
             */
            readX = readInt(thisSerial);
            readY = readInt(thisSerial);
            Serial.print("grid key: ");
            Serial.print(readX);
            Serial.print(" ");
            Serial.print(readY);
            Serial.print(" up - ");

            // turn off led
            writeInt(0x10, thisSerial);  // /prefix/led/set x y 0
            writeInt(readX, thisSerial);
            writeInt(readY, thisSerial);

            // note off
            note = readX + (readY << 4);
            myNoteOff(midiChannel, note, 0);
            Serial.print("Send note-off: ");
            Serial.println(note);

            break;
        case 0x21:
            readX = readInt(thisSerial);
            readY = readInt(thisSerial);
            Serial.print("grid key: ");
            Serial.print(readX);
            Serial.print(" ");
            Serial.print(readY);
            Serial.print(" dn - ");

            // turn on led
            writeInt(0x11, thisSerial);  // /prefix/led/set x y 1
            writeInt(readX, thisSerial);
            writeInt(readY, thisSerial);

            // note on
            note = readX + (readY << 4);
            myNoteOn(midiChannel, note, 60);
            Serial.print("Send note-on : ");
            Serial.println(note);

            break;

        case 0x40:  //   d-line-in / change to low
            break;
        case 0x41:  //   d-line-in / change to high
            break;

        // 0x5x are encoder
        case 0x50:  // /prefix/enc/delta n d - [0x50, n, d]
            // Serial.println("0x50");
            n = readInt(thisSerial);
            d = readInt(thisSerial);
            Serial.print("encoder: ");
            Serial.print(n);
            Serial.print(" : ");
            Serial.print(d);
            Serial.println();

            myControlChange(midiChannel, encoderCCs[n], d);

            // bytes: 3
            // structure: [0x50, n, d]
            // n = encoder number
            //  0-255
            // d = delta
            //  (-128)-127 (two's comp 8 bit)
            // description: encoder position change
            break;

        case 0x51:  // /prefix/enc/key n (key up)
            // Serial.println("0x51");
            n = readInt(thisSerial);
            Serial.print("key: ");
            Serial.print(n);
            Serial.println(" up");

            // bytes: 2
            // structure: [0x51, n]
            // n = encoder number
            //  0-255
            // description: encoder switch up
            break;

        case 0x52:  // /prefix/enc/key n (key down)
            // Serial.println("0x52");
            n = readInt(thisSerial);
            Serial.print("key: ");
            Serial.print(n);
            Serial.println(" down");

            // bytes: 2
            // structure: [0x52, n]
            // n = encoder number
            //  0-255
            // description: encoder switch down
            break;

        case 0x60:  //   analog / active response - 33 bytes [0x01, d0..31]
            break;
        case 0x61:  //   analog in - 4 bytes [0x61, n, dh, dl]
            break;
        case 0x80:  //   tilt / active response - 9 bytes [0x01, d]
            break;
        case 0x81:  //   tilt - 8 bytes [0x80, n, xh, xl, yh, yl, zh, zl]
            break;

        default: break;
    }
}


// FUNCTION TO PRINT DEVICE INFO
void deviceInfo() {
    int serialnum;
    char maker[6];
    int devicetype = 0;  // 1=40h, 2=series, 3=mext

    for (uint8_t i = 0; i < USBDRIVERCOUNT; i++) {
        if (*drivers[i] != driver_active[i]) {
            if (driver_active[i]) {
                Serial.printf("*** %s Device - disconnected ***\n",
                              driver_names[i]);
                driver_active[i] = false;
            }
            else {
                Serial.printf("*** %s Device %x:%x - connected ***\n",
                              driver_names[i], drivers[i]->idVendor(),
                              drivers[i]->idProduct());
                driver_active[i] = true;

                const uint8_t *psz = drivers[i]->manufacturer();
                if (psz && *psz) Serial.printf("  manufacturer: %s\n", psz);
                sprintf(maker, "%s", psz);

                const char *psp = (const char *)drivers[i]->product();
                if (psp && *psp) Serial.printf("  product: %s\n", psp);

                const char *pss = (const char *)drivers[i]->serialNumber();
                if (pss && *pss) Serial.printf("  Serial: %s\n", pss);

                // check for monome types
                if (String(maker) == "monome") {
                    //"m128%*1[-_]%d" = series, "mk%d" = kit, "m40h%d" = 40h,
                    //"m%d" = mext
                    if (sscanf(pss, "m40h%d", &serialnum)) {
                        Serial.println("  40h device");
                        devicetype = 1;
                    }
                    else if (sscanf(pss, "m256%*1[-_]%d", &serialnum)) {
                        Serial.println("  monome series 256 device");
                        devicetype = 2;
                    }
                    else if (sscanf(pss, "m128%*1[-_]%d", &serialnum)) {
                        Serial.println("  monome series 128 device");
                        devicetype = 2;
                    }
                    else if (sscanf(pss, "m64%*1[-_]%d", &serialnum)) {
                        Serial.println("  monome series 64 device");
                        devicetype = 2;
                    }
                    else if (sscanf(pss, "mk%d", &serialnum)) {
                        Serial.println("   monome kit device");
                        devicetype = 2;
                    }
                    else if (sscanf(pss, "m%d", &serialnum)) {
                        Serial.println("  mext device");
                        devicetype = 3;
                    }
                }
                //Serial.println(devicetype);

                // If this is a new Serial device.
                if (drivers[i] == &userial1) {
                    // Lets try first outputting something to our USerial to see
                    // if it will go out...
                    userial1.begin(USBBAUD, USBFORMAT);
                }
            }
        }
    }
}

// MIDI NOTE/CC HANDLERS

void myNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    // When using MIDIx4 or MIDIx16, usbMIDI.getCable() can be used
    // to read which of the virtual MIDI cables received this message.
    usbMIDI.sendNoteOn(note, velocity, channel);
    MIDI.sendNoteOn(note, velocity, channel);

    Serial.print("Note On, ch=");
    Serial.print(channel, DEC);
    Serial.print(", note=");
    Serial.print(note, DEC);
    Serial.print(", velocity=");
    Serial.println(velocity, DEC);

    // echo midi note-on back to midi device?
    midi01.sendNoteOn(note, velocity, channel);
    midi02.sendNoteOn(note, velocity, channel);
    midi03.sendNoteOn(note, velocity, channel);
    midi04.sendNoteOn(note, velocity, channel);

    // echo midi note-on back to grid
    writeInt(0x18, userial1);  // /prefix/led/level/set x y i
    writeInt(note & 15, userial1);
    writeInt(note >> 4, userial1);
    writeInt(velocity, userial1);
}

void myNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    usbMIDI.sendNoteOff(note, 0, channel);
    MIDI.sendNoteOff(note, 0, channel);

    Serial.print("Note Off, ch=");
    Serial.print(channel, DEC);
    Serial.print(", note=");
    Serial.print(note, DEC);
    Serial.print(", velocity=");
    Serial.println(velocity, DEC);

    // echo midi note-off back to midi device?
    midi01.sendNoteOff(note, velocity, channel);
    midi02.sendNoteOff(note, velocity, channel);
    midi03.sendNoteOff(note, velocity, channel);
    midi04.sendNoteOff(note, velocity, channel);

    // echo midi note-off back to grid
    writeInt(0x18, userial1);  // /prefix/led/set x y 0
    writeInt(note & 15, userial1);
    writeInt(note >> 4, userial1);
    writeInt(0, userial1);
}

void myControlChange(byte channel, byte control, byte value) {
    usbMIDI.sendControlChange(control, value, channel);
    MIDI.sendControlChange(control, value, channel);

    /*
      midi01.sendControlChange(control, value, channel);
      midi02.sendControlChange(control, value, channel);
      midi03.sendControlChange(control, value, channel);
      midi04.sendControlChange(control, value, channel);
    */
    Serial.print("Control Change, ch=");
    Serial.print(channel, DEC);
    Serial.print(", control=");
    Serial.print(control, DEC);
    Serial.print(", value=");
    Serial.println(value, DEC);
}

//************ SYSEX CALLBACKS **************
void mySystemExclusiveChunk(const byte *data, uint16_t length, bool last) {
    Serial.print("SysEx Message: ");
    // printBytes(data, length);
    if (last) { Serial.println(" (end)"); }
    else {
        Serial.println(" (to be continued)");
    }
}
void mySystemExclusive(byte *data, unsigned int length) {
    Serial.print("SysEx Message: ");
    // printBytes(data, length);
    Serial.println();
}

/* OLD WAY
sends a small (16 byte) sysex message to configure the box
-- only nine bytes are program data, the rest is to make sure it's the intended
message.
(I used the private-use (non-commercial) sysex ID '7D' with an added four bytes
as a 'key'.)
The code also sends back three bytes as an acknowledge message.
*/

void doSysEx() {
    byte *sysExBytes = usbMIDI.getSysExArray();
    if (sysExBytes[0] == 0xf0 && sysExBytes[15] == 0xf7 &&
        sysExBytes[1] == 0x7D  // 7D is private use (non-commercial)
        &&
        sysExBytes[2] ==
            0x4C  // 4-byte 'key' - not really needed if via USB but why not!
        && sysExBytes[3] == 0x65 && sysExBytes[4] == 0x69 &&
        sysExBytes[5] ==
            0x66) {  // read and compare static bytes to ensure valid msg
        for (int i = 0; i < 10; i++) {
            EEPROM.write(i, sysExBytes[i + 6]);
            sysExMem[i] = sysExBytes[i + 6];
        }
        byte data[] = {
            0xF0, 0x7D, 0xF7
        };  // ACK msg - should be safe for any device even if listening for 7D
        usbMIDI.sendSysEx(3, data);  // SEND
        for (int i = 0; i < 3; i++) {
            sysExToggled[i] = false;  // for consistant behaviour, start in OFF position
        }
    }
}
