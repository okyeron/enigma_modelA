#ifndef MONOMESERIAL_H
#define MONOMESERIAL_H

#include "Arduino.h"
#include <USBHost_t36.h>

class MonomeEvent {
    public:
        uint8_t gridKeyX;
        uint8_t gridKeyY;
        uint8_t gridKeyPressed;
};

class MonomeEventQueue {
    public:
        bool keyPressAvailable();
        MonomeEvent pollKeyPress();
        
    protected:
        void addEvent(uint8_t x, uint8_t y, uint8_t pressed);
        
    private:
        static const int MAXEVENTCOUNT = 50;
        MonomeEvent emptyEvent;
        MonomeEvent events[MAXEVENTCOUNT];
        int eventCount = 0;
        int firstEvent = 0;
};

class MonomeSerial : public USBSerial, public MonomeEventQueue {
    public: 
        MonomeSerial(USBHost usbHost);
        void setLed(uint8_t x, uint8_t y, uint8_t level);
        void clearLed(uint8_t x, uint8_t y);
        void clearAllLeds();
        void poll();
        
    private : 
        void processSerial();
};

#endif
