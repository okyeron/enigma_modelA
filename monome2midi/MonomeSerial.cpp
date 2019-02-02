#include "MonomeSerial.h"

MonomeSerial::MonomeSerial(USBHost usbHost) : USBSerial(usbHost) {
    active = 0;
    rows = 0;
    columns = 0;
    encoders = 0;
    isGrid = 1;
}
   
void MonomeSerial::setGridLed(uint8_t x, uint8_t y, uint8_t level) {
    int index = x + (y << 4);
    if (index < MAXLEDCOUNT) leds[index] = level;
}
        
void MonomeSerial::clearGridLed(uint8_t x, uint8_t y) {
    setGridLed(x, y, 0);
}

void MonomeSerial::setArcLed(uint8_t enc, uint8_t led, uint8_t level) {
    int index = led + (enc << 6);
    if (index < MAXLEDCOUNT) leds[index] = level;
}
        
void MonomeSerial::clearArcLed(uint8_t enc, uint8_t led) {
    setArcLed(enc, led, 0);
}

void MonomeSerial::clearAllLeds() {
    for (int i = 0; i < MAXLEDCOUNT; i++) leds[i] = 0;
}

void MonomeSerial::clearArcRing(uint8_t ring) {
    for (int i = ring << 6, upper = i + 64; i < upper; i++) leds[i] = 0;
}

void MonomeSerial::refreshGrid() {
    gridDirty = true;
}

void MonomeSerial::refreshArc() {
    arcDirty = true;
}

void MonomeSerial::refresh() {
    uint8_t buf[35];
    int ind, led;

    if (gridDirty) {
        buf[0] = 0x1A;
        buf[1] = 0;
        buf[2] = 0;

        ind = 3;
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x += 2) {
                led = (y << 4) + x;
                buf[ind++] = (leds[led] << 4) | leds[led + 1];
            }
        write(buf, 35);
        
        ind = 3;
        buf[1] = 8;
        for (int y = 0; y < 8; y++)
            for (int x = 8; x < 16; x += 2) {
                led = (y << 4) + x;
                buf[ind++] = (leds[led] << 4) | leds[led + 1];
            }
        write(buf, 35);
        
        ind = 3;
        buf[1] = 0;
        buf[2] = 8;
        for (int y = 8; y < 16; y++)
            for (int x = 0; x < 8; x += 2) {
                led = (y << 4) + x;
                buf[ind++] = (leds[led] << 4) | leds[led + 1];
            }
        write(buf, 35);

        ind = 3;
        buf[1] = 8;
        for (int y = 8; y < 16; y++)
            for (int x = 8; x < 16; x += 2) {
                led = (y << 4) + x;
                buf[ind++] = (leds[led] << 4) | leds[led + 1];
            }
        write(buf, 35);
        
        gridDirty = false;
    }

    if (arcDirty) {
        buf[0] = 0x92;

        buf[1] = 0;
        ind = 2;
        for (led = 0; led < 64; led += 2)
            buf[ind++] = (leds[led] << 4) | leds[led + 1];
        write(buf, 34);
        
        buf[1] = 1;
        ind = 2;
        for (led = 64; led < 128; led += 2)
            buf[ind++] = (leds[led] << 4) | leds[led + 1];
        write(buf, 34);

        buf[1] = 2;
        ind = 2;
        for (led = 128; led < 192; led += 2)
            buf[ind++] = (leds[led] << 4) | leds[led + 1];
        write(buf, 34);
        
        buf[1] = 3;
        ind = 2;
        for (led = 192; led < 256; led += 2)
            buf[ind++] = (leds[led] << 4) | leds[led + 1];
        write(buf, 34);
        
        buf[1] = 4;
        ind = 2;
        for (led = 256; led < 320; led += 2)
            buf[ind++] = (leds[led] << 4) | leds[led + 1];
        write(buf, 34);

        buf[1] = 5;
        ind = 2;
        for (led = 320; led < 384; led += 2)
            buf[ind++] = (leds[led] << 4) | leds[led + 1];
        write(buf, 34);

        buf[1] = 6;
        ind = 2;
        for (led = 384; led < 448; led += 2)
            buf[ind++] = (leds[led] << 4) | leds[led + 1];
        write(buf, 34);

        buf[1] = 7;
        ind = 2;
        for (led = 448; led < 512; led += 2)
            buf[ind++] = (leds[led] << 4) | leds[led + 1];
        write(buf, 34);

        arcDirty = 0;
    }
}

void MonomeSerial::poll() {
    // if (available()) do { processSerial(); } while (available() > 16);
    while (available()) { processSerial(); };
}

void MonomeSerial::getDeviceInfo() {
    Serial.println("MonomeSerial::getDeviceInfo");
    write(uint8_t(0));
    poll();
    /*
    write(1);
    poll();
    write(3);
    poll();
    write(5);
    poll();
    write(0x0F);
    poll();
    */
}

void MonomeSerial::processSerial() {
    uint8_t identifierSent;  // command byte sent from controller to matrix
    uint8_t gridNum;  // for reading in data not used by the matrix
    uint8_t readX, readY;    // x and y values read from driver
    uint8_t devSect, devNum;  // x and y device size read from driver
    uint8_t index, n;
    int8_t delta;
    uint8_t gridKeyX;
    uint8_t gridKeyY;
    String devID;
     
    identifierSent = read();  // get command identifier: first byte
                              // of packet is identifier in the form:
                              // [(a << 4) + b]
    // a = section (ie. system, key-grid, digital, encoder, led grid, tilt)
    // b = command (ie. query, enable, led, key, frame)
    switch (identifierSent) {
        case 0x00:  // device information
            devSect = read(); // system/query response 0x00 -> 0x00
            devNum = read();  // grids
            Serial.println("0x00 system / query ---------------------- ");
            /*
            Serial.print("section: ");
            Serial.print(devSect);
            Serial.print(", number: ");
            Serial.print(devNum);
            Serial.println(" ");
            */
            // [null, "led-grid", "key-grid", "digital-out", "digital-in", "encoder", "analog-in", "analog-out", "tilt", "led-ring"]
            if (devSect == 2) {
                rows = devNum > 2 ? 16 : 8;
                columns = devNum > 1 ? 16 : 8;
                isGrid = 1;
                Serial.print("GRID rows: ");
                Serial.print(rows);
                Serial.print(" columns: ");
                Serial.println(columns);
            } else if (devSect == 5) {
                encoders = devNum;
                isGrid = 0;
                Serial.print("ARC encoders: ");
                Serial.println(encoders);
            }
            active = 1;
            break;

        case 0x01:  // system / ID
            Serial.println("0x01 system / ID --------------- ");
            Serial.print("'");
            for (int i = 0; i < 32 && available(); i++) {
            //for (int i = 0; i < 32; i++) {  // has to be 32
                devID = read();
                Serial.print(devID);
            }
            Serial.print("'");
            Serial.println(" ");
            break;

        case 0x02:  // system / report grid offset - 4 bytes
            Serial.println("0x02");
            gridNum = read();  // n = grid number
            readX = read(); // an offset of 8 is valid only for 16 x 8 monome
            readY = read();  // an offset is invalid for y as it's only 8
            Serial.print("n: ");
            Serial.print(gridNum);
            Serial.print(", x: ");
            Serial.print(readX);
            Serial.print(" , y: ");
            Serial.print(readY);
            Serial.println(" ");
            break;

        case 0x03:  // system / report grid size
            Serial.println("0x03 system / grid offsets --------------- ");
            readX = read();  // an offset of 8 is valid only for 16 x 8 monome
            readY = read();  // an offset is invalid for y as it's only 8
            Serial.print("x: ");
            Serial.print(readX);
            Serial.print(" y: ");
            Serial.print(readY);
            Serial.println(" ");
            break;

        case 0x04:  // system / report ADDR
            Serial.println("0x04");
            readX = read();  // a ADDR
            readY = read();  // b type
            break;

        case 0x05:  // system / report ADDR
            Serial.println("0x05 system / grid size -------------- ");
            Serial.print("x size: ");
            Serial.print(read());
            Serial.print(" y size: ");
            Serial.println(read());

        case 0x0F:  // system / report firmware version
            Serial.println("0x0F system firmware version ");
            for (int i = 0; i < 8; i++) {  // 8 character string
                Serial.print(read());
            }
            break;

        case 0x20:
            /*
             0x20 key-grid / key up
             bytes: 3
             structure: [0x20, x, y]
             description: key up at (x,y)
             */

            gridKeyX = read();
            gridKeyY = read();
            addGridEvent(gridKeyX, gridKeyY, 0);
            
            Serial.print("grid key: ");
            Serial.print(gridKeyX);
            Serial.print(" ");
            Serial.print(gridKeyY);
            Serial.println(" up - ");
            break;
            
        case 0x21:
            /*
             0x21 key-grid / key down
             bytes: 3
             structure: [0x21, x, y]
             description: key down at (x,y)
             */

            gridKeyX = read();
            gridKeyY = read();
            addGridEvent(gridKeyX, gridKeyY, 1);

            Serial.print("grid key: ");
            Serial.print(gridKeyX);
            Serial.print(" ");
            Serial.print(gridKeyY);
            Serial.println(" dn - ");
            break;

        case 0x40:  //   d-line-in / change to low
            break;
        case 0x41:  //   d-line-in / change to high
            break;

        // 0x5x are encoder
        case 0x50:
            // bytes: 3
            // structure: [0x50, n, d]
            // n = encoder number
            //  0-255
            // d = delta
            //  (-128)-127 (two's comp 8 bit)
            // description: encoder position change

            index = read();
            delta = read();
            addArcEvent(index, delta);

            /*
            Serial.print("encoder: ");
            Serial.print(index);
            Serial.print(" : ");
            Serial.print(delta);
            Serial.println();
            */

            break;

        case 0x51:  // /prefix/enc/key n (key up)
            // Serial.println("0x51");
            n = read();
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
            n = read();
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

void MonomeEventQueue::addGridEvent(uint8_t x, uint8_t y, uint8_t pressed) {
    if (gridEventCount >= MAXEVENTCOUNT) return;
    uint8_t ind = (gridFirstEvent + gridEventCount) % MAXEVENTCOUNT;
    gridEvents[ind].x = x;
    gridEvents[ind].y = y;
    gridEvents[ind].pressed = pressed;
    gridEventCount++;
}

bool MonomeEventQueue::gridEventAvailable() {
    return gridEventCount > 0;
}

MonomeGridEvent MonomeEventQueue::readGridEvent() {
    if (gridEventCount == 0) return emptyGridEvent;
    gridEventCount--;
    uint8_t index = gridFirstEvent;
    gridFirstEvent = (gridFirstEvent + 1) % MAXEVENTCOUNT;
    return gridEvents[index];
}

void MonomeEventQueue::addArcEvent(uint8_t index, int8_t delta) {
    if (arcEventCount >= MAXEVENTCOUNT) return;
    uint8_t ind = (arcFirstEvent + arcEventCount) % MAXEVENTCOUNT;
    arcEvents[ind].index = index;
    arcEvents[ind].delta = delta;
    arcEventCount++;
}

bool MonomeEventQueue::arcEventAvailable() {
    return arcEventCount > 0;
}

MonomeArcEvent MonomeEventQueue::readArcEvent() {
    if (arcEventCount == 0) return emptyArcEvent;
    arcEventCount--;
    uint8_t index = arcFirstEvent;
    arcFirstEvent = (arcFirstEvent + 1) % MAXEVENTCOUNT;
    return arcEvents[index];
}
