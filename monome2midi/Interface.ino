void Interface::setGridLed(uint8_t device, uint8_t x, uint8_t y, uint8_t level) {
    for (int i = 0; i < MONOMEDEVICECOUNT; i++) {
        if (monomeDevices[i].active && monomeDevices[i].isGrid) {
            monomeDevices[i].setGridLed(x, y, level);
            break;
        }
    }
}

void Interface::setArcLed(uint8_t device, uint8_t enc, uint8_t led, uint8_t level) {
    for (int i = 0; i < MONOMEDEVICECOUNT; i++) {
        if (monomeDevices[i].active && !monomeDevices[i].isGrid) {
            monomeDevices[i].setArcLed(enc, led, level);
            break;
        }
    }
}

void Interface::clearAllLeds(uint8_t device) {
    for (int i = 0; i < MONOMEDEVICECOUNT; i++) {
        if (monomeDevices[i].active) {
            monomeDevices[device].clearAllLeds();
            // break;
        }
    }
}

void Interface::clearArcRing(uint8_t device, uint8_t ring) {
    for (int i = 0; i < MONOMEDEVICECOUNT; i++) {
        if (monomeDevices[i].active && !monomeDevices[i].isGrid) {
            monomeDevices[i].clearArcRing(ring);
            break;
        }
    }
}

void Interface::refreshGrid(uint8_t device) {
    for (int i = 0; i < MONOMEDEVICECOUNT; i++) {
        if (monomeDevices[i].active && monomeDevices[i].isGrid) {
            monomeDevices[i].refreshGrid();
            break;
        }
    }
}

void Interface::refreshArc(uint8_t device) {
    for (int i = 0; i < MONOMEDEVICECOUNT; i++) {
        if (monomeDevices[i].active && !monomeDevices[i].isGrid) {
            monomeDevices[i].refreshArc();
            break;
        }
    }
}

void Interface::noteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    midiNoteOn(channel, note, velocity);
}

void Interface::noteOff(uint8_t channel, uint8_t note) {
    midiNoteOff(channel, note, 0);
}

void Interface::controlChange(byte channel, byte control, byte value) {
    midiControlChange(channel, control, value);
}
