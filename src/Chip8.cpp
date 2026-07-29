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
    //fetch opcode
    //shift the first byte left by 8 bits then OR it with the second byte
    opcode = (memory[pc] << 8) | memory[pc + 1];
    //decode and execute
    //bitwise AND with 0xF000 to isolate first nibble
    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x00FF) {
                case 0x00E0: // 00E0: clears screen
                    memset(display, 0, sizeof(display));
                    pc += 2;
                    break;
                case 0x00EE: // 00EE: returns from subroutine
                    --sp;
                    pc = stack[sp];
                    pc += 2;
                    break;
                default:
                    std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
                    break;
            }
            break;
        case 0x2000: // 2NNN: Calls subroutine at NNN.
            stack[sp] = pc;
            ++sp;
            pc = opcode & 0x0FFF;
            break;
        case 0x1000: // 1NNN: jump to address NNN
            pc = opcode & 0x0FFF;
            break;

        case 0xA000: // ANNN: set index register i to the address NNN
            index = opcode & 0x0FFF;
            pc += 2;
            break;

        default:
            std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
            pc += 2; //prevent infinite loops if bad opcode
            break;
    }

    //update timers
    if (delay_timer > 0) {
        --delay_timer;
    }
    if (sound_timer > 0) {
        if (sound_timer == 1) {
            std::cout << "sound\n";//placeholder
        }
        --sound_timer;
    }
}