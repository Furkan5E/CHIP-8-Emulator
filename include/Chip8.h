#pragma once
#include <cstdint>

class Chip8 {
public:
    Chip8();
    void cycle();

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
};