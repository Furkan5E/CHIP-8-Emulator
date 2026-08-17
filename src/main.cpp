#define SDL_MAIN_HANDLED
#include "Chip8.h"
#include <SDL.h>
#include <iostream>
#include <string>

void audioCallback(void* userdata, Uint8* stream, int len) {
    static int phase = 0;
    int16_t* buffer = (int16_t*)stream;
    int length = len / 2;
    int volume = 3000;

    for (int i = 0; i < length; ++i) {
        //switch between positive and negative volume every 50 samples
        buffer[i] = ((phase++ / 50) % 2 == 0) ? volume : -volume; 
    }
}

int main(int argc, char* argv[]) {
    SDL_SetMainReady();

    //default config
    int scale = 10;
    int speed = 10;
    const char* romPath = nullptr;

    //parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--scale" && i + 1 < argc) {
            scale = std::stoi(argv[++i]);
        } else if (arg == "--speed" && i + 1 < argc) {
            speed = std::stoi(argv[++i]);
        } else {
            romPath = argv[i];
        }
    }

    if (!romPath) {
        std::cerr << "Usage: " << argv[0] << " <ROM_FILE_PATH> [--scale <int>] [--speed <int>]\n";
        return -1;
    }
    
    //initialise SDL for VIDEO and AUDIO
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << "\n";
        return -1;
    }
    
    //create window
    SDL_Window* window = SDL_CreateWindow(
        "CHIP-8 Emulator", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        64 * scale, 32 * scale, 
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
    cpu.loadROM(romPath);
    bool quit = false;
    bool isPaused = false;
    uint32_t fgColor = 0xFF33FF33;
    uint32_t bgColor = 0xFF111111;
    uint32_t pixelBuffer[64 * 32];

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
                    // QoL Controls
                    case SDLK_ESCAPE: 
                        cpu = Chip8(); //reset CPU state
                        cpu.loadROM(romPath); //reload game
                        break;
                    case SDLK_p: 
                        isPaused = !isPaused; 
                        break;

                    //standard CHIP-8 Keypad
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
        
        if (!isPaused) {
            for (int i = 0; i < speed; ++i) {
                cpu.cycle();
            }

            //update timers and audio
            if (cpu.delay_timer > 0) {
                --cpu.delay_timer;
            }
            if (cpu.sound_timer > 0) {
                SDL_PauseAudioDevice(audioDevice, 0); //play sound
                --cpu.sound_timer;
            } else {
                SDL_PauseAudioDevice(audioDevice, 1); //pause sound
            }
        } else {
            SDL_PauseAudioDevice(audioDevice, 1); //silence audio when paused
        }

        //map CPU display to custom colors
        for (int i = 0; i < 64 * 32; ++i) {
            pixelBuffer[i] = (cpu.display[i] != 0) ? fgColor : bgColor;
        }

        //draw mapped pixel buffer to screen
        SDL_UpdateTexture(texture, nullptr, pixelBuffer, sizeof(pixelBuffer[0]) * 64);
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