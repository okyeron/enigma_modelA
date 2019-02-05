/* Enigma
 *

 *
 */

#include <Bounce.h>
#include <EEPROM.h>
#include <MIDI.h>
#include <USBHost_t36.h>
#include <i2c_t3.h>
#include <U8g2lib.h> // oled display


// OSC ??
#include <OSCBoards.h>
#include <OSCBundle.h>
#include <OSCData.h>
#include <OSCMatch.h>
#include <OSCMessage.h>
#include <OSCTiming.h>
#include <SLIPEncodedSerial.h>
#include <SLIPEncodedUSBSerial.h>

#include "MonomeSerial.h"
#include "Interface.h"
#include "App.h"
#include "AppMidi.h"
#include "GameOfLife.h"

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
#define MONOMEDEVICECOUNT 2
#define MONOMEARCENCOUDERCOUNT 8
#define I2CADDR 0x55  // THIS DEVICE I2C ADDRESS

// i2c
// LEADER MODE allows you to broadcast values
// set to 0 for follower mode

#define LEADER 1

// i2c Function prototypes
//void receivei2cEvent(size_t count);
//void requesti2cEvent(void);

// i2c Memory
#define MEM_LEN 256
char i2c_databuf[MEM_LEN];
int count;
volatile uint8_t i2c_received;


USBHost myusb;  // usb host mode
USBHub hub1(myusb);
USBHub hub2(myusb);
MonomeSerial monomeDevices[MONOMEDEVICECOUNT] = { MonomeSerial(myusb), MonomeSerial(myusb) };
elapsedMillis monomeRefresh;
int arcValues[MONOMEARCENCOUDERCOUNT];

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

USBDriver *drivers[USBDRIVERCOUNT] = { &hub1,   &hub2,   &monomeDevices[0], &monomeDevices[1], &midi01,
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

Interface *interface;
const int APPCOUNT = 2;
App *apps[APPCOUNT];
int activeApp = 0;
elapsedMillis mainClock;
bool mainClockPhase;

// i2c Scan Function prototypes
void print_scan_status(uint8_t target, uint8_t all);
uint8_t found, target, all;

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
int counter = 0;

// SETUP

void setup() {
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

    u8g2.begin();  // oled display

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB10_tf);
    u8g2.drawStr(0,16,"enigma, init...");
    u8g2.sendBuffer(); 

    MIDI.begin(MIDI_CHANNEL_OMNI);

    // Data init
    i2c_received = 0;
    memset(i2c_databuf, 0, sizeof(i2c_databuf));

    if (LEADER) {
        Wire.begin(I2C_MASTER, I2CADDR, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000); 
        Wire.setDefaultTimeout(10000); // 10ms
      	Wire.resetBus();
      	
        // register callbacks
        Wire.onTransmitDone(i2cTransmitDone);
        Wire.onReqFromDone(i2cRequestDone);
        delay(500);
      
        // SCAN i2c BUS TO GET ADDRESSES OF CONNECTED DEVICES
        Serial.print("---------------------\n");
        Serial.print("Starting i2c scan...\n");
        for(target = 1; target <= 0x7F; target++) // sweep addr, skip general call
        {
            Wire.beginTransmission(target);       // slave addr
            Wire.endTransmission();               // no data, just addr
            print_scan_status(target, all);
        }
        if(!found) Serial.print("No i2c devices found.\n");
    } else {
        // follower mode
        Wire.begin(I2C_SLAVE, I2CADDR, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000);
        Wire.setDefaultTimeout(10000); // 10ms
        // register events
        Wire.onReceive(receivei2cEvent);
        Wire.onRequest(requesti2cEvent);
    }
	
    Serial.begin(115200);

    // EEPROM - read EEPROM for sysEx data
    /*
    if (EEPROM.read(0) > 0 && EEPROM.read(0) < 17) {
        // EEPROM @ 0 is non-zer0 or not a viable MIDI channel
        for (int i = 0; i <= 8; i++) {  // read EEPROM for sysEx data
            sysExMem[i] = EEPROM.read(i);
            delay(5);  // give time for EEPROM read??
        }
        // int chnl = sysExMem[0];
    }
    */

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
    midi01.setHandleNoteOn(midiHandleNoteOn);
    midi01.setHandleNoteOff(midiHandleNoteOff);
    midi01.setHandleControlChange(midiHandleControlChange);
    midi01.setHandleClock(midiHandleClock);
    midi01.setHandleStart(midiHandleStart);

    midi02.setHandleNoteOn(midiHandleNoteOn);
    midi02.setHandleNoteOff(midiHandleNoteOff);
    midi02.setHandleControlChange(midiHandleControlChange);
    midi02.setHandleClock(midiHandleClock);
    midi02.setHandleStart(midiHandleStart);

    midi03.setHandleNoteOn(midiHandleNoteOn);
    midi03.setHandleNoteOff(midiHandleNoteOff);
    midi03.setHandleControlChange(midiHandleControlChange);
    midi03.setHandleClock(midiHandleClock);
    midi03.setHandleStart(midiHandleStart);

    midi04.setHandleNoteOn(midiHandleNoteOn);
    midi04.setHandleNoteOff(midiHandleNoteOff);
    midi04.setHandleControlChange(midiHandleControlChange);
    midi04.setHandleClock(midiHandleClock);
    midi04.setHandleStart(midiHandleStart);

    // HARDWARE MIDI
    MIDI.setHandleNoteOn(midiHandleNoteOn);
    MIDI.setHandleNoteOff(midiHandleNoteOff);
    MIDI.setHandleControlChange(midiHandleControlChange);
    MIDI.setHandleClock(midiHandleClock);
    MIDI.setHandleStart(midiHandleStart);

    // USB MIDI
    usbMIDI.setHandleNoteOn(midiHandleNoteOn);
    usbMIDI.setHandleNoteOff(midiHandleNoteOff);
    usbMIDI.setHandleControlChange(midiHandleControlChange);
    usbMIDI.setHandleSystemExclusive(mySystemExclusive);
    usbMIDI.setHandleClock(midiHandleClock);
    usbMIDI.setHandleStart(midiHandleStart);

    delay(2000);

    for (int i = 0; i < MONOMEDEVICECOUNT; i++) monomeDevices[i].clearAllLeds();
    for (int i = 0; i < MONOMEARCENCOUDERCOUNT; i++) arcValues[i] = 0;

    int arcDevice = 0, gridDevice = 0;
    for (int i = 0; i < MONOMEDEVICECOUNT; i++) {
        if (!monomeDevices[i].active) continue;
        if (monomeDevices[i].isGrid)
            gridDevice = i;
        else
            arcDevice = i;
    }

    interface = new Interface();
    apps[0] = new GameOfLife(interface, gridDevice, arcDevice);
    apps[1] = new AppMidi(interface, gridDevice, arcDevice);
    mainClock = 0;
    mainClockPhase = 0;
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
                    Serial.print("loaded app: ");
                    Serial.println(apps[activeApp]->appName);
        }
        if (buttons[z]->fallingEdge()) {  // press
            // do press things - like turn on LED
            digitalWrite(leds[z], HIGH);  // LED on
            buttonval[z] = 1;
            Serial.print("button:");
            Serial.print(z + 1);
            Serial.println(" pressed");
            if (z == 0) {
                apps[activeApp]->appOffEvent();
                if (++activeApp >= APPCOUNT) {
                    activeApp = 0;
                    apps[activeApp]->appOnEvent();
                }
            }
        }
    }  // END BUTTONS LOOP


    // Write to 16n to select a port/fader
    
	Wire.beginTransmission(0x34);
	i2c_databuf[0] = 2;
	Wire.write(i2c_databuf, 1);
	Wire.endTransmission();
	Wire.requestFrom(0x34, 2); // Read from Follower (string len unknown, request full buffer)

    int16_t value = (i2c_databuf[0] << 8) + i2c_databuf[1];
    // i2c print received data - this is done in main loop to keep time spent in I2C ISR to minimum
    if(i2c_received && value > 0)
    {
        Serial.printf("Follower received: '%d'\n", value);
        i2c_received = 0;
    }


 /*
        // Check if error occured
        if(Wire.getError())
            Serial.print("FAIL\n");
        else
        {
            // If no error then read Rx data into buffer and print
            Wire.read(i2c_databuf, Wire.available());
            Serial.printf("'%s' OK (main)\n",i2c_databuf);
        }
*/
    
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
    for (int i = 0; i < MONOMEDEVICECOUNT; i++) {
        monomeDevices[i].poll();
        while (monomeDevices[i].gridEventAvailable()) {
            MonomeGridEvent event = monomeDevices[i].readGridEvent();
            apps[activeApp]->gridEvent(i, event.x, event.y, event.pressed);

            /*
            if (event.pressed) {
                monomeDevices[i].setGridLed(event.x, event.y, 9);
                
                // note on
                uint8_t note = event.x + (event.y << 4);
                midiNoteOn(midiChannel, note, 60);
                Serial.print("Send MIDI note-on : ");
                Serial.println(note);
            } else {
                monomeDevices[i].clearGridLed(event.x, event.y);
    
                // note off
                uint8_t note = event.x + (event.y << 4);
                midiNoteOff(midiChannel, note, 0);
                Serial.print("Send note-off: ");
                Serial.println(note);
            }
            
            monomeDevices[i].refreshGrid();
            */
        }

        while (monomeDevices[i].arcEventAvailable()) {
            MonomeArcEvent event = monomeDevices[i].readArcEvent();
            apps[activeApp]->arcEvent(i, event.index, event.delta);

            /*
            if (event.index < MONOMEARCENCOUDERCOUNT) {
                arcValues[event.index] = (arcValues[event.index] + 64 + event.delta) & 63;
                monomeDevices[i].clearArcRing(event.index);
                monomeDevices[i].setArcLed(event.index, arcValues[event.index], 9);
                monomeDevices[i].refreshArc();
            }

            myControlChange(1, event.index, event.delta);
            Serial.print("ARC: ");
            Serial.print(event.index);
            Serial.print(" ");
            Serial.print(event.delta);
            Serial.print(" ");
            Serial.println(arcValues[event.index]);
            */
        }
    }


    // blink the LED when any activity has happened
    if (activity) {
        digitalWriteFast(leds[2], HIGH);  // LED on
        ledOnMillis = 0;
    }
    if (ledOnMillis > 15) {
        digitalWriteFast(leds[2], LOW);  // LED off
    }

    if (mainClock > 150) {
        mainClockPhase = !mainClockPhase;
        apps[activeApp]->clock(mainClockPhase);
        mainClock = 0;
    }
  
    if (monomeRefresh > 50) {
        for (int i = 0; i < MONOMEDEVICECOUNT; i++) monomeDevices[i].refresh();
        monomeRefresh = 0;
    }
    String valueString = String(counter++);
    char copy[50];
    valueString.toCharArray(copy, 50);
    
  u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_helvB12_tf);  // choose a suitable font
      //u8g2.drawStr(0,10,"value = "); 
      //u8g2.drawStr(64,10,copy);  
      u8g2.setFontPosCenter();
      u8g2.drawStr(10,16,apps[activeApp]->appName);
    } while ( u8g2.nextPage() );
    


}


// MISC FUNCTIONS


// FUNCTION TO PRINT DEVICE INFO
void deviceInfo() {
    int serialnum;
    char maker[6];
    int devicetype = 0;  // 1=40h, 2=series, 3=mext

    for (uint8_t i = 0; i < USBDRIVERCOUNT; i++) {
        MonomeSerial *monome = false;
        for (int m = 0; m < MONOMEDEVICECOUNT; m++)
            if (drivers[i] == &monomeDevices[m]) monome = &monomeDevices[m];
        
        if (*drivers[i] != driver_active[i]) {
            if (driver_active[i]) {
                Serial.printf("*** %s Device - disconnected ***\n",
                              driver_names[i]);
                driver_active[i] = false;
                if (monome) monome->active = 0;
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

                if (monome) monome->begin(USBBAUD, USBFORMAT);

                // check for monome types
                if (String(maker) == "monome") {
                    //"m128%*1[-_]%d" = series, "mk%d" = kit, "m40h%d" = 40h,
                    //"m%d" = mext
                    if (sscanf(pss, "m40h%d", &serialnum)) {
                        Serial.println("  40h device");
                        monome->rows = monome->columns = 8;
                        monome->isGrid = monome->active = 1;
                        devicetype = 1;
                    }
                    else if (sscanf(pss, "m256%*1[-_]%d", &serialnum)) {
                        Serial.println("  monome series 256 device");
                        monome->rows = monome->columns = 16;
                        monome->isGrid = monome->active = 1;
                        devicetype = 2;
                    }
                    else if (sscanf(pss, "m128%*1[-_]%d", &serialnum)) {
                        Serial.println("  monome series 128 device");
                        monome->rows = 8;
                        monome->columns = 16;
                        monome->isGrid = monome->active = 1;
                        devicetype = 2;
                    }
                    else if (sscanf(pss, "m64%*1[-_]%d", &serialnum)) {
                        Serial.println("  monome series 64 device");
                        monome->rows = monome->columns = 8;
                        monome->isGrid = monome->active = 1;
                        devicetype = 2;
                    }
                    else if (sscanf(pss, "mk%d", &serialnum)) {
                        Serial.println("   monome kit device");
                        monome->rows = monome->columns = 8; // FIXME?
                        monome->isGrid = monome->active = 1;
                        devicetype = 2;
                    }
                    else if (sscanf(pss, "m%d", &serialnum)) {
                        Serial.println("  mext device");
                        devicetype = 3;
                        // monome->getDeviceInfo();
                    }
                    if (devicetype != 3) {
                        if (monome->isGrid) {
                            Serial.print("GRID rows: ");
                            Serial.print(monome->rows);
                            Serial.print(" columns: ");
                            Serial.println(monome->columns);
                        } else {
                            Serial.print("ARC encoders: ");
                            Serial.println(monome->encoders);
                        }
                    }
                }
                //Serial.println(devicetype);
            }
        }
    }
}

//************ SYSEX CALLBACKS **************
void mySystemExclusiveChunk(const byte *data, uint16_t length, bool last) {
    Serial.print("SysEx Message Chunk: ");
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
