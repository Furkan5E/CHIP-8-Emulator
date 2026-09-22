#pragma once
#include <cstdint>
#include <random>

class Chip8 {
public:
    Chip8();
    void reset(); //clear all state back to power on (keeps no ROM loaded)
    bool loadROM(const char* filename);
    void cycle();
    void tickTimers(); //decrement delay and sound timers, call at 60 Hz

    // I/O for the frontend
    void setKey(uint8_t key, bool pressed);
    const uint32_t* getDisplay() const { return display; }
    bool isSoundOn() const { return sound_timer > 0; }

private:
    //memory and registers
    uint8_t memory[4096];
    uint8_t registers[16];
    uint16_t index;
    uint16_t pc;

    //stack and timers
    uint16_t stack[16];
    uint8_t sp;
    uint8_t delay_timer;
    uint8_t sound_timer;

    // I/O
    uint8_t keypad[16]; //hex keypad
    uint32_t display[64 * 32]; //64x32 display pixels
    uint16_t opcode; //current executing opcode
    int8_t waitingKey; //key FX0A is waiting to be released, -1 if none

    //random source for CXNN, seeded differently every run
    std::mt19937 rng{std::random_device{}()};
};
