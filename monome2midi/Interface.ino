void Interface::setGridLed(uint8_t device, uint8_t x, uint8_t y, uint8_t level) {
    if (device < MONOMEDEVICECOUNT) monomeDevices[device].setGridLed(x, y, level);
}

void Interface::setArcLed(uint8_t device, uint8_t enc, uint8_t led, uint8_t level) {
    if (device < MONOMEDEVICECOUNT) monomeDevices[device].setArcLed(enc, led, level);
}

void Interface::clearAllLeds(uint8_t device) {
    if (device < MONOMEDEVICECOUNT) monomeDevices[device].clearAllLeds();
}

void Interface::clearArcRing(uint8_t device, uint8_t ring) {
    if (device < MONOMEDEVICECOUNT) monomeDevices[device].clearArcRing(ring);
}

void Interface::refreshGrid(uint8_t device) {
    if (device < MONOMEDEVICECOUNT) monomeDevices[device].refreshGrid();
}

void Interface::refreshArc(uint8_t device) {
    if (device < MONOMEDEVICECOUNT) monomeDevices[device].refreshArc();
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
