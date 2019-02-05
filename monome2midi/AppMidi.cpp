#include "AppMidi.h"

AppMidi::AppMidi(Interface *interface, uint8_t gridDevice, uint8_t arcDevice) : App(interface) {
    mainGrid = gridDevice;
    mainArc = arcDevice;
    
    for (int i = 0; i < 12; i++) noteMap[i] = i;
}

void AppMidi::appOnEvent() {
    Serial.println("AppMidi app ON");
    appName = "AppMidi";
    renderGrid();
}

void AppMidi::appOffEvent() {
}

void AppMidi::clock(bool phase) {
    // FIXME once appOnEvent is fixed
    renderGrid();
}

void AppMidi::gridEvent(uint8_t device, uint8_t x, uint8_t y, uint8_t pressed) {
    if (pressed && x == 0 && y < 5) {
        octaveShift = 2 - y;
    } else if (pressed && y == 0 && x > 1 && x < 14) {
        selectedScale = x - 2;
    } else if (pressed && y == 1 && x > 1 && x < 14) {
        noteMap[selectedScale] = x - 2;
    } else if (pressed && y == 3 && x > 1 && x < 14) {
        transpose = x - 2;
    } else if (y == 5 && x > 1 && x < 14) {
        if (gridPressed) {
            if (gridX == x && gridY == y)
                gridPressed = 0;
            else if (pressed) {
                velocityLow = min(gridX, x) - 2;
                velocityHigh = max(gridX, x) - 2;
            }
        } else {
            gridPressed = pressed;
            gridX = x;
            gridY = y;
            if (pressed) velocityLow = velocityHigh = x - 2;
        }
    } else if (y == 7 && x > 1 && x < 14) {
        if (gridPressed) {
            if (gridX == x && gridY == y)
                gridPressed = 0;
            else if (pressed) {
                ccLow = min(gridX, x) - 2;
                ccHigh = max(gridX, x) - 2;
            }
        } else {
            gridPressed = pressed;
            gridX = x;
            gridY = y;
            if (pressed) ccLow = ccHigh = x - 2;
        }
    }

    renderGrid();
}

void AppMidi::noteOnEvent(uint8_t channel, uint8_t note, uint8_t velocity) {
    lastRealNote = note;
    lastNote = noteMap[note % 12] + (note / 12) * 12 + octaveShift * 12 + transpose;
    notePlaying = 1;
    interface->noteOn(channel, lastNote, random(velocityLow, velocityHigh + 1) * 8 + 20);
    interface->controlChange(channel, 91, random(ccLow, ccHigh + 1));
    renderGrid();
}

void AppMidi::noteOffEvent(uint8_t channel, uint8_t note) {
    notePlaying = 0;
    interface->noteOff(channel, lastNote);
    renderGrid();
}

void AppMidi::renderGrid() {
    interface->clearAllLeds(mainGrid);

    for (int y = 0; y < 5; y++)
        interface->setGridLed(mainGrid, 0, y, y == 2 - octaveShift ? 15 : 4);

    for (int x = 0; x < 12; x++) {
        interface->setGridLed(mainGrid, 2 + x, 0, (x == (lastRealNote % 12) ? 11 : 2)+ (x == selectedScale ? 4 : 0));
        interface->setGridLed(mainGrid, 2 + x, 1, (x == ((lastNote - transpose) % 12) ? 11 : 2)+ (x == noteMap[selectedScale] ? 4 : 0));
        interface->setGridLed(mainGrid, 2 + x, 3, (x == transpose ? 11 : 2));
        interface->setGridLed(mainGrid, 2 + x, 5, x >= velocityLow && x <= velocityHigh ? 11 : 2);
        interface->setGridLed(mainGrid, 2 + x, 7, x >= ccLow && x <= ccHigh ? 11 : 2);
    }
    
    interface->refreshGrid(mainGrid);
}
