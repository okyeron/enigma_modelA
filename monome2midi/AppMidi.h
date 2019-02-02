#ifndef APPMIDI_H
#define APPMIDI_H

#include "App.h"

class AppMidi : public App {
    public:
        AppMidi(Interface *interface, uint8_t gridDevice, uint8_t arcDevice);
        void clock(bool phase);
        void gridEvent(uint8_t device, uint8_t x, uint8_t y, uint8_t pressed);
        void noteOnEvent(uint8_t channel, uint8_t note, uint8_t velocity);
        void noteOffEvent(uint8_t channel, uint8_t note);
        void appOnEvent();
        void appOffEvent();

    private:
        uint8_t mainGrid;
        uint8_t mainArc;
        uint8_t notePlaying;
        uint8_t lastNote;
        uint8_t lastRealNote;
        uint8_t selectedScale = 0;
        uint8_t gridPressed = 0;
        uint8_t gridX;
        uint8_t gridY;

        uint8_t noteMap[12];
        int8_t octaveShift = 0;
        int8_t transpose = 0;
        uint8_t velocityLow = 11;
        uint8_t velocityHigh = 11;
        uint8_t ccLow = 0;
        uint8_t ccHigh = 0;

        AppMidi();
        void renderGrid(void);
};

#endif
