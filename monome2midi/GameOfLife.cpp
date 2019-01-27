#include "GameOfLife.h"

GameOfLife::GameOfLife(Interface *interface, uint8_t gridDevice, uint8_t arcDevice) : App(interface) {
    mainGrid = gridDevice;
    mainArc = arcDevice;
    arcValues[0] = arcValues[1] = 0;
    arcShow[0] = arcShow[1] = 0;
}

void GameOfLife::clock(bool phase) {
    if (!phase) {
        for (int i = 0; i < NOTES; i++) interface->noteOff(i, notes[i]);
        return;
    }

    uint8_t nextgen = gen ? 0 : 1;
    int count = 0;

    for (int x = 0; x < MAXX; x++)
        for (int y = 0; y < MAXY; y++)
        {
            uint8_t n = neighbours(x, y, gen);
            if (states[x][y][gen] && (n == 2 || n == 3)) {
                states[x][y][nextgen] = 1;
                count += x + y;
            } else if (!states[x][y][gen] && (n == 3)) {
                states[x][y][nextgen] = 1;
                count += x - y;
            } else {
                states[x][y][nextgen] = 0;
            }
        }
    gen = nextgen;
    renderGrid();
    
    for (int i = NOTES - 1; i > 0; i--) notes[i] = notes[i - 1];
    notes[0] = scale[abs(count) % 8] + 80;
    
    for (int i = 0; i < NOTES; i++) interface->noteOn(i, notes[i], 80 - i * 10);
    interface->refreshGrid(mainGrid);
}

void GameOfLife::gridEvent(uint8_t device, uint8_t x, uint8_t y, uint8_t pressed) {
    if (pressed && x < MAXX && y < MAXY) {
        states[x][y][gen] = !states[x][y][gen];
        interface->setGridLed(device, x, y, 15);
        interface->refreshGrid(device);
    }
}

void GameOfLife::arcEvent(uint8_t device, uint8_t encoder, int8_t delta) {
    if (encoder > 1) return;
    if (delta > 0){
        if (arcValues[encoder] > 0) arcValues[encoder] += delta; else arcValues[encoder] = delta;
    } else {
        if (arcValues[encoder] < 0) arcValues[encoder] += delta; else arcValues[encoder] = delta;
    }
    if (abs(arcValues[encoder]) < 40) return;
    arcValues[encoder] = 0;

    uint8_t temp;
    if (encoder == 0) {
        if (delta < 0) {
            arcShow[0] = (arcShow[0] + 63) & 63;
            for (int x = 0; x < MAXX; x++) {
                temp = states[x][0][gen];
                for (int y = 0; y < MAXY - 1; y++)
                    states[x][y][gen] = states[x][y + 1][gen];
                states[x][MAXY - 1][gen] = temp;
            }
        } else {
            arcShow[0] = (arcShow[0] + 1) & 63;
            for (int x = 0; x < MAXX; x++) {
                temp = states[x][MAXY - 1][gen];
                for (int y = MAXY - 1; y > 0; y--)
                    states[x][y][gen] = states[x][y - 1][gen];
                states[x][0][gen] = temp;
            }
        }
        interface->clearArcRing(device, 0);
        interface->setArcLed(device, 0, arcShow[0], 15);
    } else if (encoder == 1) {
        if (delta < 0) {
            arcShow[1] = (arcShow[1] + 63) & 63;
            for (int y = 0; y < MAXY; y++) {
                temp = states[0][y][gen];
                for (int x = 0; x < MAXX - 1; x++)
                    states[x][y][gen] = states[x + 1][y][gen];
                states[MAXX - 1][y][gen] = temp;
            }
        } else {
            arcShow[1] = (arcShow[1] + 1) & 63;
            for (int y = 0; y < MAXY; y++) {
                temp = states[MAXX - 1][y][gen];
                for (int x = MAXX - 1; x > 0; x--)
                    states[x][y][gen] = states[x - 1][y][gen];
                states[0][y][gen] = temp;
            }
        }
        interface->clearArcRing(device, 1);
        interface->setArcLed(device, 1, arcShow[1], 15);
    }

    interface->refreshArc(device);
    renderGrid();
}

void GameOfLife::renderGrid() {
    interface->clearAllLeds(mainGrid);
    for (int x = 0; x < MAXX; x++)
        for (int y = 0; y < MAXY; y++)
            interface->setGridLed(mainGrid, x, y, states[x][y][gen] ? 12 : 0);
    interface->refreshGrid(mainGrid);
}

uint8_t GameOfLife::neighbours(uint8_t x, uint8_t y, uint8_t gen) {
    return 
        states[(x+MAXX-1)%MAXX][(y+MAXY-1)%MAXY][gen] +
        states[(x+0)%MAXX][(y+MAXY-1)%MAXY][gen] +
        states[(x+1)%MAXX][(y+MAXY-1)%MAXY][gen] +
        states[(x+MAXX-1)%MAXX][(y+0)%MAXY][gen] +
        states[(x+1)%MAXX][(y+0)%MAXY][gen] +
        states[(x+MAXX-1)%MAXX][(y+1)%MAXY][gen] +
        states[(x+0)%MAXX][(y+1)%MAXY][gen] +
        states[(x+1)%MAXX][(y+1)%MAXY][gen];
}
