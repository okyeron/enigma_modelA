#include <USBHost_t36.h>
#define USBBAUD 115200
uint32_t baud = USBBAUD;
uint32_t format = USBHOST_SERIAL_8N1;

USBHost myusb; // usb host mode
USBHub hub1(myusb);
USBHub hub2(myusb);
USBSerial userial(myusb);

USBDriver *drivers[] = {&hub1, &hub2, &userial};
#define CNT_DEVICES (sizeof(drivers)/sizeof(drivers[0]))
const char * driver_names[CNT_DEVICES] = {"Hub1", "Hub2", "USERIAL1" };
bool driver_active[CNT_DEVICES] = {false, false, false};


String deviceID  = "monome";
String serialNum = "m1000010"; // this # does not get used -  serial number from usb_names is picked up instead
const uint8_t gridX    = 16;   // Will be either 8 or 16
const uint8_t gridY    = 8;                 // standard for 128

static const uint8_t PROGMEM
xy2i128[8][16] = {
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
    { 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 },
    { 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47 },
    { 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63 },
    { 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79 },
    { 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95 },
    { 96, 97, 98, 99, 100,101,102,103,104,105,106,107,108,109,110,111 },
    { 112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127}},
i2xy128[128][2] = {
{0, 0},{1, 0},{2, 0},{3, 0},{4, 0},{5, 0},{6, 0},{7, 0},{8, 0},{9, 0},{10, 0},{11, 0},{12, 0},{13, 0},{14, 0},{15, 0},
{0, 1},{1, 1},{2, 1},{3, 1},{4, 1},{5, 1},{6, 1},{7, 1},{8, 1},{9, 1},{10, 1},{11, 1},{12, 1},{13, 1},{14, 1},{15, 1},
{0, 2},{1, 2},{2, 2},{3, 2},{4, 2},{5, 2},{6, 2},{7, 2},{8, 2},{9, 2},{10, 2},{11, 2},{12, 2},{13, 2},{14, 2},{15, 2},
{0, 3},{1, 3},{2, 3},{3, 3},{4, 3},{5, 3},{6, 3},{7, 3},{8, 3},{9, 3},{10, 3},{11, 3},{12, 3},{13, 3},{14, 3},{15, 3},
{0, 4},{1, 4},{2, 4},{3, 4},{4, 4},{5, 4},{6, 4},{7, 4},{8, 4},{9, 4},{10, 4},{11, 4},{12, 4},{13, 4},{14, 4},{15, 4},
{0, 5},{1, 5},{2, 5},{3, 5},{4, 5},{5, 5},{6, 5},{7, 5},{8, 5},{9, 5},{10, 5},{11, 5},{12, 5},{13, 5},{14, 5},{15, 5},
{0, 6},{1, 6},{2, 6},{3, 6},{4, 6},{5, 6},{6, 6},{7, 6},{8, 6},{9, 6},{10, 6},{11, 6},{12, 6},{13, 6},{14, 6},{15, 6},
{0, 7},{1, 7},{2, 7},{3, 7},{4, 7},{5, 7},{6, 7},{7, 7},{8, 7},{9, 7},{10, 7},{11, 7},{12, 7},{13, 7},{14, 7},{15, 7}
};

uint8_t xy2i(uint8_t x, uint8_t y) {
    return pgm_read_byte(&xy2i128[y][x]);
}
uint8_t i2xy(uint8_t i) {
    return pgm_read_byte(&i2xy128[i][1]);
}



void setup() {

  Serial.begin(115200);
  myusb.begin();
  Serial.println("\n\nUSB Host - Serial");
  userial.begin(baud, format);

  usbMIDI.setHandleNoteOn(myNoteOn);
  usbMIDI.setHandleNoteOff(myNoteOff);
  usbMIDI.setHandleControlChange(myControlChange);
  //writeInt(0x12);
}


uint8_t readInt() {
  uint8_t val = userial.read();
  //Serial2.write(val); // send to serial 2 pins for debug
  return val; 
}

void writeInt(uint8_t value) {
#if DEBUG
   //Serial.print(value,HEX);       // For debug, values are written to the serial monitor in Hexidecimal
   Serial.print(value);       
   Serial.println(" ");
#else
   userial.write(value);           // standard is to write out the 8 bit value on serial
#endif
}

void processSerial() {
  uint8_t identifierSent;                     // command byte sent from controller to matrix
  uint8_t deviceAddress;                      // device address sent from controller
  uint8_t dummy, gridNum;                              // for reading in data not used by the matrix
  uint8_t intensity = 15;                     // default full led intensity
  uint8_t readX, readY;                       // x and y values read from driver
  uint8_t deviceX, deviceY, devSect, devNum;                   // x and y device size read from driver
  uint8_t i, x, y, z;


  identifierSent = userial.read();             // get command identifier: first byte of packet is identifier in the form: [(a << 4) + b]
                                              // a = section (ie. system, key-grid, digital, encoder, led grid, tilt)
                                              // b = command (ie. query, enable, led, key, frame)
                 
  switch (identifierSent) {
    case 0x00:                  // device information
      devSect = readInt();                // system/query response 0x00 -> 0x00
      devNum = readInt();                // grids
      Serial.print("section: ");
      Serial.print(devSect);
      Serial.print(", number: ");
      Serial.print(devNum);
      Serial.println(" ");

      break;

    case 0x01:                  // system / ID
      for (i = 0; i < 32; i++) {              // has to be 32
          Serial.print(readInt());          
      }
      Serial.println(" ");
      
//      for (i = 0; i < 32; i++) {
//        deviceID[i] = userial.read();
//      }

      break;

    case 0x02:      // system / report grid offset - 4 bytes
      gridNum = readInt();                      // n = grid number
      readX = readInt();                      // an offset of 8 is valid only for 16 x 8 monome
      readY = readInt();                      // an offset is invalid for y as it's only 8
      Serial.print("ofeset: ");
      Serial.print(gridNum);
      Serial.print(", : ");
      Serial.print(readX);
      Serial.print(" ");
      Serial.print(readY);
      Serial.println(" ");
     break;

    case 0x03:            // system / report grid size
      readX = readInt();                      // an offset of 8 is valid only for 16 x 8 monome
      readY = readInt();                      // an offset is invalid for y as it's only 8
      Serial.print("grid size: ");
      Serial.print(readX);
      Serial.print(" ");
      Serial.print(readY);
      Serial.println(" ");

      break;

    case 0x04:                                // system / report ADDR
      readX = readInt();                      // a ADDR
      readY = readInt();                      // b type
 
      break;


    case 0x0F:                               // system / report firmware version
      for (i = 0; i < 8; i++) {              // 8 character string
          Serial.print(readInt());          
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
      readX = readInt();
      readY = readInt();
      Serial.print("grid key: ");
      Serial.print(readX);
      Serial.print(" ");
      Serial.print(readY);
      Serial.print(" up - ");
      // turn off led
      writeInt(0x10); // /prefix/led/set x y 0
      writeInt(readX);
      writeInt(readY);
      // note off
      myNoteOff(1, xy2i(readX, readY), 0) ;
      Serial.print("Send note-off: ");
      Serial.print(xy2i(readX, readY));
      Serial.println(" ");
     
      break;
    case 0x21:                               
      readX = readInt();
      readY = readInt();
      Serial.print("grid key: ");
      Serial.print(readX);
      Serial.print(" ");
      Serial.print(readY);
      Serial.print(" dn - ");
      // turn on led
      writeInt(0x11); // /prefix/led/set x y 1
      writeInt(readX);
      writeInt(readY);
      // note on
      myNoteOn(1, xy2i(readX, readY), 60) ;
      Serial.print("Send note-on : ");
      Serial.print(xy2i(readX, readY));
      Serial.println(" ");
     
      break;
 
    case 0x40:        //   d-line-in / change to low  
      break;
    case 0x41:        //   d-line-in / change to high  
      break;
      
   // 0x5x are encoder
    case 0x50:    // /prefix/enc/delta n d
      //bytes: 3
      //structure: [0x50, n, d]
      //n = encoder number
      //  0-255
      //d = delta
      //  (-128)-127 (two's comp 8 bit)
      //description: encoder position change
      break;
      
    case 0x51:    // /prefix/enc/key n d (d=0 key up)
      //bytes: 2
      //structure: [0x51, n]
      //n = encoder number
      //  0-255
      //description: encoder switch up
      break;
      
    case 0x52:    // /prefix/enc/key n d (d=1 key down)
      //bytes: 2
      //structure: [0x52, n]
      //n = encoder number
      //  0-255
      //description: encoder switch down
      break;

    case 0x60:        //   analog / active response - 33 bytes [0x01, d0..31]
      break;
    case 0x61:        //   analog in - 4 bytes [0x61, n, dh, dl]
      break;
    case 0x80:        //   tilt / active response - 9 bytes [0x01, d]
      break;
    case 0x81:        //   tilt - 8 bytes [0x80, n, xh, xl, yh, yl, zh, zl]
      break;


    default:
      Serial.print(identifierSent);
      Serial.println(" ");     
      break;

  }
}


void loop() {
  myusb.Task();
  usbMIDI.read();

  // Print out information about different devices.
  for (uint8_t i = 0; i < CNT_DEVICES; i++) {
    if (*drivers[i] != driver_active[i]) {
      if (driver_active[i]) {
        Serial.printf("*** %s Device - disconnected ***\n", driver_names[i]);
        driver_active[i] = false;
      } else {
        Serial.printf("*** %s Device %x:%x - connected ***\n", driver_names[i], drivers[i]->idVendor(), drivers[i]->idProduct());
        driver_active[i] = true;

        const uint8_t *psz = drivers[i]->manufacturer();
        if (psz && *psz) Serial.printf("  manufacturer: %s\n", psz);
        psz = drivers[i]->product();
        if (psz && *psz) Serial.printf("  product: %s\n", psz);
        psz = drivers[i]->serialNumber();
        if (psz && *psz) Serial.printf("  Serial: %s\n", psz);

        // If this is a new Serial device.
        if (drivers[i] == &userial) {
          // Lets try first outputting something to our USerial to see if it will go out...
          userial.begin(baud);
        }
      }
    }
  }
  
  if (userial.available() > 0) {
    do { processSerial();  } 
    while (userial.available() > 16);
  }
  

}

void myNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  uint8_t i, x, y, z;
  // When using MIDIx4 or MIDIx16, usbMIDI.getCable() can be used
  // to read which of the virtual MIDI cables received this message.
  Serial.print("Note On, ch=");
  Serial.print(channel, DEC);
  Serial.print(", note=");
  Serial.print(note, DEC);
  Serial.print(", velocity=");
  Serial.println(velocity, DEC);
      writeInt(0x18); // /prefix/led/level/set x y i
      writeInt(pgm_read_byte(&i2xy128[note][0]));
      writeInt(pgm_read_byte(&i2xy128[note][1]));  
      writeInt(velocity);  
}

void myNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  uint8_t i, x, y, z;
  Serial.print("Note Off, ch=");
  Serial.print(channel, DEC);
  Serial.print(", note=");
  Serial.print(note, DEC);
  Serial.print(", velocity=");
  Serial.println(velocity, DEC);
      writeInt(0x18); // /prefix/led/set x y 0
      writeInt(pgm_read_byte(&i2xy128[note][0]));
      writeInt(pgm_read_byte(&i2xy128[note][1]));  
      writeInt(0);  
}

void myControlChange(byte channel, byte control, byte value) {
  Serial.print("Control Change, ch=");
  Serial.print(channel, DEC);
  Serial.print(", control=");
  Serial.print(control, DEC);
  Serial.print(", value=");
  Serial.println(value, DEC);
}
