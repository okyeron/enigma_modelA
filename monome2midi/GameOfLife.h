#ifndef GAMEOFLIFE_H
#define GAMEOFLIFE_H

#include "App.h"

class GameOfLife : public App {
    public:
        GameOfLife(Interface *interface, uint8_t appId);
        void clock(bool phase);
        void gridEvent(uint8_t device, uint8_t x, uint8_t y, uint8_t pressed);
        void arcEvent(uint8_t device, uint8_t encoder, int8_t delta);
        void appOnEvent();
        void appOffEvent();

    private:
        static const uint8_t MAXX = 16;
        static const uint8_t MAXY = 8;
        static const uint8_t NOTES = 4;
        uint8_t scale[MAXX]; // = {0, 3, 5, 8, 12, 10, 8, 15 };

        uint8_t states[MAXX][MAXY][2];
        uint8_t gen;
        uint8_t notes[NOTES];
        int8_t arcValues[2];
        int8_t arcShow[2];
        int16_t prevCount;
        uint8_t transpose;
        uint8_t counter;

        GameOfLife();
        uint8_t neighbours(uint8_t x, uint8_t y, uint8_t gen);
        void renderGrid(void);
        void updateCCs(int count);
        void playNotes(int counter);
};

#endif
