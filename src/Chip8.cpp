#include "Chip8.h"
#include <fstream>
#include <iostream>

//standard chip8 font set for characters 0 through F
const uint8_t fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80 // F
};

Chip8::Chip8() {
    //initialise program counter to standard ROM starting location
    pc = 0x200;
    opcode = 0;
    index = 0;
    sp = 0;

    //clear memory, registers, stack, display, and keypad arrays
    memset(memory, 0, sizeof(memory));
    memset(registers, 0, sizeof(registers));
    memset(stack, 0, sizeof(stack));
    memset(display, 0, sizeof(display));
    memset(keypad, 0, sizeof(keypad));

    delay_timer = 0;
    sound_timer = 0;

    //load the font set into interpreter memory starting at address 0x050
    for (unsigned int i = 0; i < 80; ++i) {
        memory[0x50 + i] = fontset[i];
    }
}

void Chip8::loadROM(const char* filename) {
    std::ifstream file(filename,std::ios::binary | std::ios::ate);
    if (file.is_open()) {
        std::streampos size = file.tellg();
        char* buffer = new char[size];

        file.seekg(0, std::ios::beg);
        file.read(buffer, size);
        file.close();
        for (long i = 0; i < size; ++i) {
            memory[0x200 + i] = buffer[i];
        }

        delete[] buffer;
    } else {
        std::cerr << "Failed to open ROM: " << filename << std::endl;
    }
}

void Chip8::cycle() {
}