#include "GameOfLife.h"

GameOfLife::GameOfLife(Interface *interface, uint8_t gridDevice, uint8_t arcDevice) : App(interface) {
    mainGrid = gridDevice;
    mainArc = arcDevice;
    arcValues[0] = arcValues[1] = 0;
    arcShow[0] = arcShow[1] = 0;
}

void GameOfLife::appOnEvent() {
    Serial.println("GameOfLife app ON");
    renderGrid();
}

void GameOfLife::appOffEvent() {
}

void GameOfLife::clock(bool phase) {
    if (++counter >= 24) counter = 0;

    /*
    if (!phase) {
        for (int i = 0; i < NOTES; i++) interface->noteOff(i + 1, notes[i]);
        return;
    }
    */

    if (counter % 6 == 0) {
        uint8_t nextgen = gen ? 0 : 1;
        int count = 0;
    
        for (int x = 0; x < MAXX; x++)
            for (int y = 0; y < MAXY; y++)
            {
                uint8_t n = neighbours(x, y, gen);
                if (states[x][y][gen] && (n == 2 || n == 3)) {
                    states[x][y][nextgen] = states[x][y][gen] > 5 ? states[x][y][gen] - 1 : 5;
                    count += x + states[x][y][nextgen];
                } else if (!states[x][y][gen] && (n == 3)) {
                    states[x][y][nextgen] = 15;
                    count += y + states[x][y][nextgen];
                } else {
                    states[x][y][nextgen] = 0;
                }
            }
    
        gen = nextgen;
        prevCount = count;
    }
    
    if (counter % 8 == 0) updateCCs(prevCount);
    playNotes(counter);

    renderGrid();
}

void GameOfLife::gridEvent(uint8_t device, uint8_t x, uint8_t y, uint8_t pressed) {
    if (pressed && y == MAXY - 1) {
        scale[x] = !scale[x];
        renderGrid();
    } else if (pressed && x < MAXX && y < MAXY) {
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

void GameOfLife::playNotes(int counter) {
    /*
    for (int i = NOTES - 1; i > 0; i--) notes[i] = notes[i - 1];
    notes[0] = scale[abs(count) % 8] + 70 + transpose;
    if (count) for (int i = 0; i < NOTES; i++) interface->noteOn(i + 1, notes[i], 80 - i * 5);
    */

    int scaleCount = 0;
    int scaleArray[MAXX];
    for (int i = 0; i < MAXX; i++) {
        if (scale[i]) scaleArray[scaleCount++] = i;
    }

    if (scaleCount == 0) {
        scaleCount = 1;
        scaleArray[0] = 0;
    }

    int count = 0;
    if (counter % 24 == 0) {
        for (int x = 0; x < MAXX; x++) count += states[x][0][gen] + states[x][1][gen];
        notes[0] = scaleArray[abs(count) % scaleCount] + 58 + transpose;
        //notes[0] = scaleArray[0] + 58 + transpose;
        interface->noteOn(1, notes[0], 80);
    }
    else if (counter % 24 == 18) interface->noteOff(1, notes[0]);

    if (counter % 24 == 6) {
        count = 0;
        for (int x = 0; x < MAXX; x++) count += states[x][2][gen] + states[x][3][gen];
        notes[1] = scaleArray[abs(count) % scaleCount] + 46 + transpose;
        interface->noteOn(2, notes[1], 80);
    }
    else if (counter % 24 == 22) interface->noteOff(2, notes[1]);

    if (counter % 24 == 12) {
        count = 0;
        for (int x = 0; x < MAXX; x++) count += states[x][4][gen] + states[x][5][gen];
        notes[2] = scaleArray[abs(count) % scaleCount] + 46 + transpose;
        interface->noteOn(3, notes[2], 80);
    }
    else if (counter % 24 == 20) interface->noteOff(3, notes[2]);

    if (counter % 24 == 18) {
        count = 0;
        for (int x = 0; x < MAXX; x++) count += states[x][6][gen] + states[x][7][gen];
        notes[3] = scaleArray[abs(count) % scaleCount] + 58 + transpose;
        interface->noteOn(4, notes[3], 80);
    }
    else if (counter == 6) interface->noteOff(4, notes[3]);

    // for (int i = 0; i < NOTES; i++) interface->noteOn(i + 1, notes[i], 80);
}

void GameOfLife::renderGrid() {
    interface->clearAllLeds(mainGrid);
    for (int x = 0; x < MAXX; x++)
        for (int y = 0; y < MAXY - 1; y++)
            interface->setGridLed(mainGrid, x, y, states[x][y][gen]);

    for (int x = 0; x < MAXX; x++)
        interface->setGridLed(mainGrid, x, MAXY - 1, scale[x] ? 8 : 0);
    interface->refreshGrid(mainGrid);
}

uint8_t GameOfLife::neighbours(uint8_t x, uint8_t y, uint8_t gen) {
    return 
        (states[(x+MAXX-1)%MAXX][(y+MAXY-1)%MAXY][gen] > 0) +
        (states[(x+0)%MAXX][(y+MAXY-1)%MAXY][gen] > 0) +
        (states[(x+1)%MAXX][(y+MAXY-1)%MAXY][gen] > 0) +
        (states[(x+MAXX-1)%MAXX][(y+0)%MAXY][gen] > 0) +
        (states[(x+1)%MAXX][(y+0)%MAXY][gen] > 0) +
        (states[(x+MAXX-1)%MAXX][(y+1)%MAXY][gen] > 0) +
        (states[(x+0)%MAXX][(y+1)%MAXY][gen] > 0) +
        (states[(x+1)%MAXX][(y+1)%MAXY][gen] > 0);
}

void GameOfLife::updateCCs(int count) {
    uint8_t x1 = 0, x2 = 15, x3 = 0, x4 = 15;
    for (int i = 0; i < NOTES; i++) {
        
        for (int x = 0; x < MAXX; x++) {
            if (states[x][i << 1][gen]) x1 = x;
            if (states[MAXX-1-x][i][gen]) x2 = x;
            if (states[x][(i << 1) + 1][gen]) x3 = x;
            if (states[MAXX-1-x][(i << 1) + 1][gen]) x4 = x;
        }
        interface->controlChange(i + 1, 91, x1);
        interface->controlChange(i + 1, 92, x2);
        interface->controlChange(i + 1, 17, x3);
        interface->controlChange(i + 1, 23, x4 + 80);
        interface->controlChange(i + 1, 19, (abs(count) % 10) + i * 5);
        interface->controlChange(i + 1, 18, (abs(count) % 20) + i * 3);
        /*
        interface->controlChange(i + 1, 90, (abs(count) % 10));
        interface->controlChange(i + 1, 18, (abs(count) % 20) + i * 3);
        interface->controlChange(i + 1, 19, (abs(count) % 10) + i * 5);
        */
    }
}
