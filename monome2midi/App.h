#ifndef APP_H
#define APP_H

#include "Arduino.h"
#include "Interface.h"

class App {
    public:
        void virtual clock(bool phase);
        void virtual gridEvent(uint8_t device, uint8_t x, uint8_t y, uint8_t pressed);
        void virtual arcEvent(uint8_t device, uint8_t encoder, int8_t delta);

    protected:
        Interface *interface;
        App(Interface *interface);
};

#endif
