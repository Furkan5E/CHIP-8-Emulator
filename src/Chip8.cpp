#include "Chip8.h"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstring>

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

bool Chip8::loadROM(const char* filename) {
    std::ifstream file(filename,std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open ROM: " << filename << std::endl;
        return false;
    }

    //ROM is loaded at 0x200 so it can use at most the remaining memory
    const std::streamoff maxSize = sizeof(memory) - 0x200;
    std::streamoff size = file.tellg();
    if (size <= 0) {
        std::cerr << "Failed to read ROM or ROM is empty: " << filename << std::endl;
        return false;
    }
    if (size > maxSize) {
        std::cerr << "ROM is too large (" << size << " bytes, max " << maxSize << "): " << filename << std::endl;
        return false;
    }

    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(&memory[0x200]), size)) {
        std::cerr << "Failed to read ROM: " << filename << std::endl;
        return false;
    }
    return true;
}

void Chip8::cycle() {
    //fetch opcode
    //shift the first byte left by 8 bits then OR it with the second byte
    //wrap addresses to 12 bits so a runaway PC can't read past memory
    opcode = (memory[pc & 0xFFF] << 8) | memory[(pc + 1) & 0xFFF];
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
                    //nothing to return to so skip instead of underflowing sp
                    if (sp == 0) {
                        std::cerr << "Stack underflow at 0x" << std::hex << pc << std::endl;
                        pc += 2;
                        break;
                    }
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
            //stack is full so skip the call instead of writing past it
            if (sp >= 16) {
                std::cerr << "Stack overflow at 0x" << std::hex << pc << std::endl;
                pc += 2;
                break;
            }
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

                //for the flag setting ops below VF is written after the result
                //so that when Vx is VF the flag wins over the result

                case 0x0004: { // 8XY4: add Vy to Vx, set VF = carry
                    uint8_t carry = (registers[Vy] > (0xFF - registers[Vx])) ? 1 : 0;
                    registers[Vx] += registers[Vy];
                    registers[0xF] = carry;
                    pc += 2;
                    break;
                }

                case 0x0005: { // 8XY5: subtract Vy from Vx, set VF = NOT borrow
                    uint8_t notBorrow = (registers[Vx] >= registers[Vy]) ? 1 : 0;
                    registers[Vx] -= registers[Vy];
                    registers[0xF] = notBorrow;
                    pc += 2;
                    break;
                }

                case 0x0006: { // 8XY6: shift Vx right by 1, set VF = LSB before shift
                    uint8_t lsb = registers[Vx] & 0x1;
                    registers[Vx] >>= 1;
                    registers[0xF] = lsb;
                    pc += 2;
                    break;
                }

                case 0x0007: { // 8XY7: set Vx = Vy - Vx, set VF = NOT borrow
                    uint8_t notBorrow = (registers[Vy] >= registers[Vx]) ? 1 : 0;
                    registers[Vx] = registers[Vy] - registers[Vx];
                    registers[0xF] = notBorrow;
                    pc += 2;
                    break;
                }

                case 0x000E: { // 8XYE: shift Vx left by 1, set VF = MSB before shift
                    uint8_t msb = (registers[Vx] & 0x80) >> 7;
                    registers[Vx] <<= 1;
                    registers[0xF] = msb;
                    pc += 2;
                    break;
                }

                default:
                    std::cerr << "Unknown 8-series opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
                    break;
            }
            break;
        }
        case 0xE000: {
            uint8_t Vx = (opcode & 0x0F00) >> 8;
            //only the low nibble is a valid key (keypad has 16 keys)
            uint8_t key = registers[Vx] & 0xF;

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

        case 0xB000: // BNNN: jump to address NNN + V0
            pc = (opcode & 0x0FFF) + registers[0];
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
}