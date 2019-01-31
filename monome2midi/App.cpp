#include "App.h"

App::App(Interface *interface) {
    this->interface = interface;
}

void App::clock(bool phase) { }
void App::gridEvent(uint8_t device, uint8_t x, uint8_t y, uint8_t pressed) { }
void App::arcEvent(uint8_t device, uint8_t encoder, int8_t delta) { }
void App::noteOnEvent(uint8_t channel, uint8_t note, uint8_t velocity) { }
void App::noteOffEvent(uint8_t channel, uint8_t note) { }
