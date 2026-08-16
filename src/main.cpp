#define SDL_MAIN_HANDLED
#include "Chip8.h"
#include <SDL.h>
#include <iostream>

// Audio callback to generate a simple square wave (approx 440Hz)
void audioCallback(void* userdata, Uint8* stream, int len) {
    static int phase = 0;
    int16_t* buffer = (int16_t*)stream;
    int length = len / 2; // 16-bit audio
    int volume = 3000;    // Adjust this to change the volume

    for (int i = 0; i < length; ++i) {
        // Switch between positive and negative volume every 50 samples
        buffer[i] = ((phase++ / 50) % 2 == 0) ? volume : -volume; 
    }
}

int main(int argc, char* argv[]) {
    SDL_SetMainReady();
    
    // Initialize SDL for both VIDEO and AUDIO
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
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

    // Configure Audio Device
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 2048;
    want.callback = audioCallback;

    SDL_AudioDeviceID audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audioDevice == 0) {
        std::cerr << "Failed to open audio: " << SDL_GetError() << "\n";
    }

    Chip8 cpu;
    if (argc > 1) {
        cpu.loadROM(argv[1]);
    } else {
        std::cerr << "Usage: " << argv[0] << " <ROM_FILE_PATH>\n";
        return -1;
    }

    //game loop
    bool quit = false;
    SDL_Event e;
    
    const int FPS = 60;
    const int frameDelay = 1000 / FPS;
    Uint32 frameStart;
    int frameTime;

    while (!quit) {
        frameStart = SDL_GetTicks();

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
        
        for (int i = 0; i < 10; ++i) {
            cpu.cycle();
        }

        //update timers and audio
        if (cpu.delay_timer > 0) {
            --cpu.delay_timer;
        }
        if (cpu.sound_timer > 0) {
            SDL_PauseAudioDevice(audioDevice, 0); // Play sound
            --cpu.sound_timer;
        } else {
            SDL_PauseAudioDevice(audioDevice, 1); // Pause sound
        }

        //draw CPU display array to screen
        SDL_UpdateTexture(texture, nullptr, cpu.display, sizeof(cpu.display[0]) * 64);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        //delay to get 60 FPS
        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    //clean up
    SDL_CloseAudioDevice(audioDevice);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}