#define SDL_MAIN_HANDLED
#include "Chip8.h"
#include <SDL.h>
#include <iostream>

int main(int argc, char* argv[]) {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << "\n";
        return -1;
    }
    //create window
    const int SCALE = 10;
    SDL_Window* window = SDL_CreateWindow(
        "CHIP-8 Emulator", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        64 * SCALE, 32 * SCALE, 
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << "\n";
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    // create texture
    SDL_Texture* texture = SDL_CreateTexture(
        renderer, 
        SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING, 
        64, 32
    );

    Chip8 cpu;
    //cpu.loadROM("roms/test_opcode.ch8");

    //game loop
    bool quit = false;
    SDL_Event e;
    while (!quit) {
        //handle input events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_x: cpu.keypad[0] = 1; break;
                    case SDLK_1: cpu.keypad[1] = 1; break;
                    case SDLK_2: cpu.keypad[2] = 1; break;
                    case SDLK_3: cpu.keypad[3] = 1; break;
                    case SDLK_q: cpu.keypad[4] = 1; break;
                    case SDLK_w: cpu.keypad[5] = 1; break;
                    case SDLK_e: cpu.keypad[6] = 1; break;
                    case SDLK_a: cpu.keypad[7] = 1; break;
                    case SDLK_s: cpu.keypad[8] = 1; break;
                    case SDLK_d: cpu.keypad[9] = 1; break;
                    case SDLK_z: cpu.keypad[0xA] = 1; break;
                    case SDLK_c: cpu.keypad[0xB] = 1; break;
                    case SDLK_4: cpu.keypad[0xC] = 1; break;
                    case SDLK_r: cpu.keypad[0xD] = 1; break;
                    case SDLK_f: cpu.keypad[0xE] = 1; break;
                    case SDLK_v: cpu.keypad[0xF] = 1; break;
                }
            } else if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.sym) {
                    case SDLK_x: cpu.keypad[0] = 0; break;
                    case SDLK_1: cpu.keypad[1] = 0; break;
                    case SDLK_2: cpu.keypad[2] = 0; break;
                    case SDLK_3: cpu.keypad[3] = 0; break;
                    case SDLK_q: cpu.keypad[4] = 0; break;
                    case SDLK_w: cpu.keypad[5] = 0; break;
                    case SDLK_e: cpu.keypad[6] = 0; break;
                    case SDLK_a: cpu.keypad[7] = 0; break;
                    case SDLK_s: cpu.keypad[8] = 0; break;
                    case SDLK_d: cpu.keypad[9] = 0; break;
                    case SDLK_z: cpu.keypad[0xA] = 0; break;
                    case SDLK_c: cpu.keypad[0xB] = 0; break;
                    case SDLK_4: cpu.keypad[0xC] = 0; break;
                    case SDLK_r: cpu.keypad[0xD] = 0; break;
                    case SDLK_f: cpu.keypad[0xE] = 0; break;
                    case SDLK_v: cpu.keypad[0xF] = 0; break;
                }
            }
        }

        //draw CPU display array to screen
        SDL_UpdateTexture(texture, nullptr, cpu.display, sizeof(cpu.display[0]) * 64);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    //clean up
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}