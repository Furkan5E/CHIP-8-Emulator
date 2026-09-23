#define SDL_MAIN_HANDLED
#include "Chip8.h"
#include <SDL.h>
#include <iostream>
#include <string>
#include <cerrno>
#include <cstdlib>

void audioCallback(void* /*userdata*/, Uint8* stream, int len) {
    static int phase = 0;
    int16_t* buffer = (int16_t*)stream;
    int length = len / 2;
    const int16_t volume = 3000;

    for (int i = 0; i < length; ++i) {
        //switch between positive and negative volume every 50 samples
        buffer[i] = (phase < 50) ? volume : static_cast<int16_t>(-volume);
        //wrap after one full period so the counter never overflows
        phase = (phase + 1) % 100;
    }
}

//physical keyboard key for each CHIP-8 key 0x0 to 0xF
//scancodes are positions, so the 4x4 block stays in the same place on
//AZERTY, QWERTZ, Dvorak etc (labels below are the QWERTY names)
//  1 2 3 C        1 2 3 4
//  4 5 6 D  --->  Q W E R
//  7 8 9 E        A S D F
//  A 0 B F        Z X C V
const SDL_Scancode keymap[16] = {
    SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, // 0 1 2 3
    SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A, // 4 5 6 7
    SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C, // 8 9 A B
    SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V  // C D E F
};

//return the CHIP-8 key for a keyboard position, or -1 if it isn't mapped
int mapKey(SDL_Scancode scancode) {
    for (int i = 0; i < 16; ++i) {
        if (keymap[i] == scancode) {
            return i;
        }
    }
    return -1;
}

//parse a whole string as an int within [min, max]
bool parseInt(const char* text, int min, int max, int& out) {
    char* end = nullptr;
    errno = 0;
    long value = std::strtol(text, &end, 10);
    if (end == text || *end != '\0' || errno == ERANGE || value < min || value > max) {
        return false;
    }
    out = static_cast<int>(value);
    return true;
}

int main(int argc, char* argv[]) {
    SDL_SetMainReady();

    //default config
    int scale = 10;
    int speed = 10;
    const char* romPath = nullptr;
    const std::string usage = std::string("Usage: ") + argv[0] + " <ROM_FILE_PATH> [--scale <1-100>] [--speed <1-1000>]\n";

    //parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--scale" || arg == "--speed") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for " << arg << "\n" << usage;
                return -1;
            }
            bool isScale = (arg == "--scale");
            if (!parseInt(argv[++i], 1, isScale ? 100 : 1000, isScale ? scale : speed)) {
                std::cerr << "Invalid value for " << arg << ": " << argv[i] << "\n" << usage;
                return -1;
            }
        } else if (arg.rfind("--", 0) == 0) {
            std::cerr << "Unknown option: " << arg << "\n" << usage;
            return -1;
        } else if (romPath) {
            std::cerr << "Only one ROM path can be given\n" << usage;
            return -1;
        } else {
            romPath = argv[i];
        }
    }

    if (!romPath) {
        std::cerr << usage;
        return -1;
    }

    //load the ROM before starting SDL so a bad path exits cleanly
    Chip8 cpu;
    if (!cpu.loadROM(romPath)) {
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
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // create texture
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        64, 32
    );
    if (!texture) {
        std::cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << "\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

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

    bool quit = false;
    bool isPaused = false;
    uint32_t fgColor = 0xFF33FF33;
    uint32_t bgColor = 0xFF111111;
    uint32_t pixelBuffer[64 * 32];

    SDL_Event e;
    const int FPS = 60;
    //schedule frames against fixed deadlines on the high resolution counter so
    //they average exactly 60 Hz (a whole ms delay per frame drifts off 60 Hz)
    const Uint64 perfFreq = SDL_GetPerformanceFrequency();
    const Uint64 frameTicks = perfFreq / FPS;
    Uint64 nextFrame = SDL_GetPerformanceCounter() + frameTicks;

    while (!quit) {
        //handle input events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    // QoL Controls
                    case SDLK_ESCAPE: 
                        cpu.reset(); //reset CPU state
                        if (!cpu.loadROM(romPath)) { //reload game
                            quit = true;
                        }
                        break;
                    case SDLK_p:
                        //ignore key repeat so holding P doesn't flicker pause
                        if (e.key.repeat) {
                            break;
                        }
                        isPaused = !isPaused;
                        SDL_SetWindowTitle(window, isPaused ? "CHIP-8 Emulator [Paused]" : "CHIP-8 Emulator");
                        break;

                    default: {
                        //standard CHIP-8 Keypad
                        int key = mapKey(e.key.keysym.scancode);
                        if (key >= 0) {
                            cpu.setKey(static_cast<uint8_t>(key), true);
                        }
                        break;
                    }
                }
            } else if (e.type == SDL_KEYUP) {
                int key = mapKey(e.key.keysym.scancode);
                if (key >= 0) {
                    cpu.setKey(static_cast<uint8_t>(key), false);
                }
            }
        }
        
        if (!isPaused) {
            for (int i = 0; i < speed; ++i) {
                cpu.cycle();
            }

            //play sound while the sound timer is running, then update timers
            SDL_PauseAudioDevice(audioDevice, cpu.isSoundOn() ? 0 : 1);
            cpu.tickTimers();
        } else {
            SDL_PauseAudioDevice(audioDevice, 1); //silence audio when paused
        }

        //map CPU display to custom colors
        const uint8_t* display = cpu.getDisplay();
        for (int i = 0; i < 64 * 32; ++i) {
            pixelBuffer[i] = (display[i] != 0) ? fgColor : bgColor;
        }

        //draw mapped pixel buffer to screen
        SDL_UpdateTexture(texture, nullptr, pixelBuffer, sizeof(pixelBuffer[0]) * 64);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        //wait until this frame's deadline then schedule the next one
        Uint64 now = SDL_GetPerformanceCounter();
        if (now < nextFrame) {
            SDL_Delay(static_cast<Uint32>((nextFrame - now) * 1000 / perfFreq));
        }
        nextFrame += frameTicks;
        //if we fell more than a frame behind (e.g. window dragged) resync instead of racing to catch up
        now = SDL_GetPerformanceCounter();
        if (now > nextFrame + frameTicks) {
            nextFrame = now + frameTicks;
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