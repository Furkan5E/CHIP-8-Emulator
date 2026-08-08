#include "Chip8.h"
#include <fstream>
#include <iostream>
#include <cstdlib>

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

        case 0x2000: // 2NNN: call subroutine at NNN
            stack[sp] = pc;
            ++sp;
            pc = opcode & 0x0FFF;
            break;

        case 0x3000: // 3XNN: skip next instruction if Vx == NN
            if (registers[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;

        case 0x4000: // 4XNN: skip next instruction if Vx != NN
            if (registers[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;

        case 0x5000: // 5XY0: skip next instruction if Vx == Vy
            if (registers[(opcode & 0x0F00) >> 8] == registers[(opcode & 0x00F0) >> 4]) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;

        case 0x6000: // 6XNN: set Vx = NN
            registers[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF);
            pc += 2;
            break;

        case 0x7000: // 7XNN: add NN to Vx
            registers[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
            pc += 2;
            break;

        case 0x9000: // 9XY0: skip next instruction if Vx != Vy
            if (registers[(opcode & 0x0F00) >> 8] != registers[(opcode & 0x00F0) >> 4]) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;

        case 0xC000: // CXNN: set Vx = random byte AND NN
            registers[(opcode & 0x0F00) >> 8] = (rand() % 256) & (opcode & 0x00FF);
            pc += 2;
            break;

        case 0x8000: {
            //extract x and y register identifiers from opcode
            uint8_t Vx = (opcode & 0x0F00) >> 8;
            uint8_t Vy = (opcode & 0x00F0) >> 4;

            switch (opcode & 0x000F) {
                case 0x0000: // 8XY0: set Vx = Vy
                    registers[Vx] = registers[Vy];
                    pc += 2;
                    break;

                case 0x0001: // 8XY1: set Vx = Vx OR Vy
                    registers[Vx] |= registers[Vy];
                    pc += 2;
                    break;

                case 0x0002: // 8XY2: set Vx = Vx AND Vy
                    registers[Vx] &= registers[Vy];
                    pc += 2;
                    break;

                case 0x0003: // 8XY3: set Vx = Vx XOR Vy
                    registers[Vx] ^= registers[Vy];
                    pc += 2;
                    break;

                case 0x0004: // 8XY4: add Vy to Vx, set VF = carry
                    if (registers[Vy] > (0xFF - registers[Vx])) {
                        registers[0xF] = 1; // Overflow occurred
                    } else {
                        registers[0xF] = 0;
                    }
                    registers[Vx] += registers[Vy];
                    pc += 2;
                    break;

                case 0x0005: // 8XY5: subtract Vy from Vx, set VF = NOT borrow
                    if (registers[Vx] >= registers[Vy]) {
                        registers[0xF] = 1; //no borrow
                    } else {
                        registers[0xF] = 0; //borrow occurred
                    }
                    registers[Vx] -= registers[Vy];
                    pc += 2;
                    break;

                case 0x0006: // 8XY6: shift Vx right by 1, set VF = LSB before shift
                    registers[0xF] = (registers[Vx] & 0x1);
                    registers[Vx] >>= 1;
                    pc += 2;
                    break;

                case 0x0007: // 8XY7: set Vx = Vy - Vx, set VF = NOT borrow
                    if (registers[Vy] >= registers[Vx]) {
                        registers[0xF] = 1; //no borrow
                    } else {
                        registers[0xF] = 0; //borrow occurred
                    }
                    registers[Vx] = registers[Vy] - registers[Vx];
                    pc += 2;
                    break;

                case 0x000E: // 8XYE: shift Vx left by 1, set VF = MSB before shift
                    registers[0xF] = (registers[Vx] & 0x80) >> 7;
                    registers[Vx] <<= 1;
                    pc += 2;
                    break;

                default:
                    std::cerr << "Unknown 8-series opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
                    break;
            }
            break;
        }
        case 0xE000: {
            uint8_t Vx = (opcode & 0x0F00) >> 8;
            uint8_t key = registers[Vx];

            switch (opcode & 0x00FF) {
                case 0x009E: // EX9E: skip next instruction if key stored in Vx is pressed
                    if (keypad[key] != 0) {
                        pc += 4;
                    } else {
                        pc += 2;
                    }
                    break;

                case 0x00A1: // EXA1: skip next instruction if key stored in Vx is NOT pressed
                    if (keypad[key] == 0) {
                        pc += 4;
                    } else {
                        pc += 2;
                    }
                    break;

                default:
                    std::cerr << "Unknown E-series opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
                    break;
            }
            break;
        }
        case 0xF000: {
            uint8_t Vx = (opcode & 0x0F00) >> 8;
            switch (opcode & 0x00FF) {
                case 0x0007: // FX07: set Vx = delay timer value
                    registers[Vx] = delay_timer;
                    pc += 2;
                    break;

                case 0x000A: { // FX0A: wait for key press store the value of key in Vx
                    bool key_pressed = false;
                    for (int i = 0; i < 16; ++i) {
                        if (keypad[i] != 0) {
                            registers[Vx] = i;
                            key_pressed = true;
                        }
                    }
                    //if no key pressed return without incrementing PC
                    if (!key_pressed) {
                        return;
                    }
                    pc += 2;
                    break;
                }
                case 0x0015: // FX15: set delay timer = Vx
                    delay_timer = registers[Vx];
                    pc += 2;
                    break;

                case 0x0018: // FX18: set sound timer = Vx
                    sound_timer = registers[Vx];
                    pc += 2;
                    break;

                case 0x001E: // FX1E: add Vx to i
                    index += registers[Vx];
                    pc += 2;
                    break;

                case 0x0029: // FX29: set i = location of sprite for digit Vx
                    //characters are 5 bytes long
                    index = 0x50 + (5 * registers[Vx]);
                    pc += 2;
                    break;

                case 0x0033: // FX33: store BCD representation of Vx in memory
                    memory[index]     = registers[Vx] / 100; //hundreds digit
                    memory[index + 1] = (registers[Vx] / 10) % 10; //tens digit
                    memory[index + 2] = (registers[Vx] % 100) % 10; // ones digit
                    pc += 2;
                    break;

                case 0x0055: // FX55: store registers V0 to Vx in memory
                    for (uint8_t i = 0; i <= Vx; ++i) {
                        memory[index + i] = registers[i];
                    }
                    pc += 2;
                    break;

                case 0x0065: // FX65: read registers V0 to Vx from memory
                    for (uint8_t i = 0; i <= Vx; ++i) {
                        registers[i] = memory[index + i];
                    }
                    pc += 2;
                    break;

                default:
                    std::cerr << "Unknown F-series opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
                    break;
            }
            break;
        }
        case 0x1000: // 1NNN: jump to address NNN
            pc = opcode & 0x0FFF;
            break;

        case 0xA000: // ANNN: set index register i to the address NNN
            index = opcode & 0x0FFF;
            pc += 2;
            break;

        case 0xD000: { // DXYN: draw sprite
            uint8_t Vx = (opcode & 0x0F00) >> 8;
            uint8_t Vy = (opcode & 0x00F0) >> 4;
            uint8_t height = opcode & 0x000F;
            uint8_t xPos = registers[Vx] % 64;
            uint8_t yPos = registers[Vy] % 32;

            registers[0xF] = 0; //reset collision flag
            for (unsigned int row = 0; row < height; ++row) {
                //fetch the sprite data byte from memory at the current index
                uint8_t spriteByte = memory[index + row];

                for (unsigned int col = 0; col < 8; ++col) {
                    //isolate current bit in sprite byte
                    uint8_t spritePixel = spriteByte & (0x80 >> col);
                    //if edge it clips the sprite
                    if ((xPos + col) >= 64 || (yPos + row) >= 32) {
                        continue; 
                    }

                    //calculate array index for display array
                    unsigned int pixelIndex = ((yPos + row) * 64) + (xPos + col);
                    if (spritePixel != 0) {
                        //if the display pixel is already ON collision happens
                        if (display[pixelIndex] == 0xFFFFFFFF) {
                            registers[0xF] = 1;
                        }
                        //XOR display pixel 
                        display[pixelIndex] ^= 0xFFFFFFFF;
                    }
                }
            }
            pc += 2;
            break;
        }
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