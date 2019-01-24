#include "Arduino.h"
#include <USBHost_t36.h>

class MonomeSerial : public USBSerial {
const byte midiChannel = 1;  // The MIDI Channel to send the commands over FIXME

    public: MonomeSerial(USBHost usbHost) : USBSerial(usbHost) {}

    void setLed(uint8_t x, uint8_t y, uint8_t level) {
        this->write(0x18);  // /prefix/led/level/set x y i
        this->write(x);
        this->write(y);
        this->write(level);
    }
    
    void clearLed(uint8_t x, uint8_t y) {
        setLed(x, y, 0);
    }
    
    void processSerial() {
        uint8_t identifierSent;  // command byte sent from controller to matrix
        uint8_t gridNum;  // for reading in data not used by the matrix
        uint8_t readX, readY;    // x and y values read from driver
        uint8_t devSect, devNum;  // x and y device size read from driver
        uint8_t i, n;
        int8_t d;
        int8_t note;
    
        identifierSent = this->read();  // get command identifier: first byte
                                             // of packet is identifier in the form:
                                             // [(a << 4) + b]
        // a = section (ie. system, key-grid, digital, encoder, led grid, tilt)
        // b = command (ie. query, enable, led, key, frame)
        switch (identifierSent) {
            case 0x00:  // device information
                // Serial.println("0x00");
                devSect = this->read(); // system/query response 0x00 -> 0x00
                devNum = this->read();  // grids
                Serial.print("section: ");
                Serial.print(devSect);
                Serial.print(", number: ");
                Serial.print(devNum);
                Serial.println(" ");
                break;
    
            case 0x01:  // system / ID
                // Serial.println("0x01");
                for (i = 0; i < 32; i++) {  // has to be 32
                    Serial.print(this->read());
                }
                Serial.println(" ");
                break;
    
            case 0x02:  // system / report grid offset - 4 bytes
                // Serial.println("0x02");
                gridNum = this->read();  // n = grid number
                readX = this->read(); // an offset of 8 is valid only for 16 x 8 monome
                readY = this->read();  // an offset is invalid for y as it's only 8
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
                readX = this->read();  // an offset of 8 is valid only for 16 x 8 monome
                readY = this->read();  // an offset is invalid for y as it's only 8
                Serial.print("x: ");
                Serial.print(readX);
                Serial.print(" y: ");
                Serial.print(readY);
                Serial.println(" ");
                break;
    
            case 0x04:  // system / report ADDR
                // Serial.println("0x04");
                readX = this->read();  // a ADDR
                readY = this->read();  // b type
                break;
    
            case 0x0F:  // system / report firmware version
                // Serial.println("0x0F");
                for (i = 0; i < 8; i++) {  // 8 character string
                    Serial.print(this->read());
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
                readX = this->read();
                readY = this->read();
                Serial.print("grid key: ");
                Serial.print(readX);
                Serial.print(" ");
                Serial.print(readY);
                Serial.print(" up - ");
    
                // turn off led
                this->write(0x10);  // /prefix/led/set x y 0
                this->write(readX);
                this->write(readY);
    
                // note off
                note = readX + (readY << 4);
                //myNoteOff(1, note, 0);
                Serial.print("Send note-off: ");
                Serial.println(note);
    
                break;
            case 0x21:
                readX = this->read();
                readY = this->read();
                Serial.print("grid key: ");
                Serial.print(readX);
                Serial.print(" ");
                Serial.print(readY);
                Serial.print(" dn - ");
    
                // turn on led
                this->write(0x11);  // /prefix/led/set x y 1
                this->write(readX);
                this->write(readY);
    
                // note on
                note = readX + (readY << 4);
                //myNoteOn(midiChannel, note, 60);
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
                n = this->read();
                d = this->read();
                Serial.print("encoder: ");
                Serial.print(n);
                Serial.print(" : ");
                Serial.print(d);
                Serial.println();
    
                // FIXME myControlChange(1, encoderCCs[n], d);
    
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
                n = this->read();
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
                n = this->read();
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
};
