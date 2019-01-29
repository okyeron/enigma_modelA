#ifndef INTERFACE_H
#define INTERFACE_H

#include "Midi.h"

class Interface {
    public:
        void setGridLed(uint8_t device, uint8_t x, uint8_t y, uint8_t level);
        void setArcLed(uint8_t device, uint8_t enc, uint8_t led, uint8_t level);
        void clearAllLeds(uint8_t device);
        void clearArcRing(uint8_t device, uint8_t ring);
        void refreshGrid(uint8_t device);
        void refreshArc(uint8_t device);
        void noteOn(uint8_t channel, uint8_t note, uint8_t velocity);
        void noteOff(uint8_t channel, uint8_t note);
        void controlChange(byte channel, byte control, byte value);
};

#endif
