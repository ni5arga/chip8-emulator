#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <random>
#include <SDL2/SDL.h>

const int SCREEN_WIDTH = 64;
const int SCREEN_HEIGHT = 32;
const int SCALE = 15;
const int WINDOW_WIDTH = SCREEN_WIDTH * SCALE;
const int WINDOW_HEIGHT = SCREEN_HEIGHT * SCALE;

class Chip8 {
private:
    uint8_t memory[4096];
    uint8_t registers[16];
    uint16_t indexRegister;
    uint16_t programCounter;
    uint8_t stackPointer;
    uint16_t stack[16];
    uint8_t delayTimer;
    uint8_t soundTimer;
    uint8_t screen[64 * 32];
    uint8_t keys[16];
    
    std::mt19937 randomGenerator;
    std::uniform_int_distribution<int> randomDistribution;

    uint8_t fontData[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0,
        0x20, 0x60, 0x20, 0x20, 0x70,
        0xF0, 0x10, 0xF0, 0x80, 0xF0,
        0xF0, 0x10, 0xF0, 0x10, 0xF0,
        0x90, 0x90, 0xF0, 0x10, 0x10,
        0xF0, 0x80, 0xF0, 0x10, 0xF0,
        0xF0, 0x80, 0xF0, 0x90, 0xF0,
        0xF0, 0x10, 0x20, 0x40, 0x40,
        0xF0, 0x90, 0xF0, 0x90, 0xF0,
        0xF0, 0x90, 0xF0, 0x10, 0xF0,
        0xF0, 0x90, 0xF0, 0x90, 0x90,
        0xE0, 0x90, 0xE0, 0x90, 0xE0,
        0xF0, 0x80, 0x80, 0x80, 0xF0,
        0xE0, 0x90, 0x90, 0x90, 0xE0,
        0xF0, 0x80, 0xF0, 0x80, 0xF0,
        0xF0, 0x80, 0xF0, 0x80, 0x80
    };

public:
    Chip8() : randomGenerator(std::random_device{}()), randomDistribution(0, 255) {
        initialize();
    }

    void initialize() {
        programCounter = 0x200;
        indexRegister = 0;
        stackPointer = 0;
        
        memset(memory, 0, sizeof(memory));
        memset(registers, 0, sizeof(registers));
        memset(stack, 0, sizeof(stack));
        memset(screen, 0, sizeof(screen));
        memset(keys, 0, sizeof(keys));
        
        for (int i = 0; i < 80; ++i) {
            memory[i] = fontData[i];
        }
        
        delayTimer = 0;
        soundTimer = 0;
    }

    bool loadProgram(const char* filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Couldn't open the ROM file" << std::endl;
            return false;
        }
        
        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        
        if (fileSize > (4096 - 512)) {
            std::cerr << "ROM is too large to fit in memory" << std::endl;
            return false;
        }
        
        file.read((char*)(memory + 512), fileSize);
        file.close();
        return true;
    }

    void executeCycle() {
        uint16_t instruction = memory[programCounter] << 8 | memory[programCounter + 1];
        
        uint16_t address = instruction & 0x0FFF;
        uint8_t nibble = instruction & 0x000F;
        uint8_t regX = (instruction & 0x0F00) >> 8;
        uint8_t regY = (instruction & 0x00F0) >> 4;
        uint8_t byteValue = instruction & 0x00FF;
        
        switch (instruction & 0xF000) {
            case 0x0000:
                if (instruction == 0x00E0) {
                    memset(screen, 0, sizeof(screen));
                    programCounter += 2;
                } else if (instruction == 0x00EE) {
                    programCounter = stack[--stackPointer];
                    programCounter += 2;
                }
                break;
                
            case 0x1000:
                programCounter = address;
                break;
                
            case 0x2000:
                stack[stackPointer++] = programCounter;
                programCounter = address;
                break;
                
            case 0x3000:
                programCounter += (registers[regX] == byteValue) ? 4 : 2;
                break;
                
            case 0x4000:
                programCounter += (registers[regX] != byteValue) ? 4 : 2;
                break;
                
            case 0x5000:
                programCounter += (registers[regX] == registers[regY]) ? 4 : 2;
                break;
                
            case 0x6000:
                registers[regX] = byteValue;
                programCounter += 2;
                break;
                
            case 0x7000:
                registers[regX] += byteValue;
                programCounter += 2;
                break;
            
            case 0x8000:
                switch (nibble) {
                    case 0x0:
                        registers[regX] = registers[regY];
                        break;
                    case 0x1:
                        registers[regX] |= registers[regY];
                        break;
                    case 0x2:
                        registers[regX] &= registers[regY];
                        break;
                    case 0x3:
                        registers[regX] ^= registers[regY];
                        break;
                    case 0x4: {
                        uint16_t sum = registers[regX] + registers[regY];
                        registers[0xF] = (sum > 255) ? 1 : 0;
                        registers[regX] = sum & 0xFF;
                        break;
                    }
                    case 0x5:
                        registers[0xF] = (registers[regX] > registers[regY]) ? 1 : 0;
                        registers[regX] -= registers[regY];
                        break;
                    case 0x6:
                        registers[0xF] = registers[regX] & 0x1;
                        registers[regX] >>= 1;
                        break;
                    case 0x7:
                        registers[0xF] = (registers[regY] > registers[regX]) ? 1 : 0;
                        registers[regX] = registers[regY] - registers[regX];
                        break;
                    case 0xE:
                        registers[0xF] = (registers[regX] & 0x80) >> 7;
                        registers[regX] <<= 1;
                        break;
                }
                programCounter += 2;
                break;
                
            case 0x9000:
                programCounter += (registers[regX] != registers[regY]) ? 4 : 2;
                break;
                
            case 0xA000:
                indexRegister = address;
                programCounter += 2;
                break;
                
            case 0xB000:
                programCounter = address + registers[0];
                break;
                
            case 0xC000:
                registers[regX] = randomDistribution(randomGenerator) & byteValue;
                programCounter += 2;
                break;
            
            case 0xD000: {
                registers[0xF] = 0;
                for (int row = 0; row < nibble; ++row) {
                    uint8_t spriteRow = memory[indexRegister + row];
                    for (int col = 0; col < 8; ++col) {
                        if ((spriteRow & (0x80 >> col)) != 0) {
                            int pixelIndex = ((registers[regY] + row) % 32) * 64 + 
                                           ((registers[regX] + col) % 64);
                            if (screen[pixelIndex] == 1) {
                                registers[0xF] = 1;
                            }
                            screen[pixelIndex] ^= 1;
                        }
                    }
                }
                programCounter += 2;
                break;
            }
            
            case 0xE000:
                if (byteValue == 0x9E) {
                    programCounter += (keys[registers[regX]]) ? 4 : 2;
                } else if (byteValue == 0xA1) {
                    programCounter += (!keys[registers[regX]]) ? 4 : 2;
                }
                break;
                
            case 0xF000:
                switch (byteValue) {
                    case 0x07:
                        registers[regX] = delayTimer;
                        break;
                    case 0x15:
                        delayTimer = registers[regX];
                        break;
                    case 0x18:
                        soundTimer = registers[regX];
                        break;
                    case 0x1E:
                        indexRegister += registers[regX];
                        break;
                    case 0x29:
                        indexRegister = registers[regX] * 5;
                        break;
                    case 0x33:
                        memory[indexRegister] = registers[regX] / 100;
                        memory[indexRegister + 1] = (registers[regX] / 10) % 10;
                        memory[indexRegister + 2] = registers[regX] % 10;
                        break;
                    case 0x55:
                        for (int i = 0; i <= regX; ++i) {
                            memory[indexRegister + i] = registers[i];
                        }
                        break;
                    case 0x65:
                        for (int i = 0; i <= regX; ++i) {
                            registers[i] = memory[indexRegister + i];
                        }
                        break;
                }
                programCounter += 2;
                break;
                
            default:
                std::cerr << "Unknown instruction: 0x" << std::hex << instruction << std::endl;
                programCounter += 2;
        }
        
        if (delayTimer > 0) {
            --delayTimer;
        }
        if (soundTimer > 0) {
            --soundTimer;
        }
    }

    void setKey(int index, bool pressed) {
        if (index >= 0 && index < 16) {
            keys[index] = pressed ? 1 : 0;
        }
    }
    
    const uint8_t* getScreen() const {
        return screen;
    }
};

int mapSDLKeyToChip8(SDL_Keycode key) {
    switch(key) {
        case SDLK_1: return 0x1;
        case SDLK_2: return 0x2;
        case SDLK_3: return 0x3;
        case SDLK_4: return 0xC;
        case SDLK_q: return 0x4;
        case SDLK_w: return 0x5;
        case SDLK_e: return 0x6;
        case SDLK_r: return 0xD;
        case SDLK_a: return 0x7;
        case SDLK_s: return 0x8;
        case SDLK_d: return 0x9;
        case SDLK_f: return 0xE;
        case SDLK_z: return 0xA;
        case SDLK_x: return 0x0;
        case SDLK_c: return 0xB;
        case SDLK_v: return 0xF;
        default: return -1;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <ROM file>" << std::endl;
        return 1;
    }
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    SDL_Window* window = SDL_CreateWindow(
        "CHIP-8 Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );
    
    if (!texture) {
        std::cerr << "Texture creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    Chip8 emulator;
    
    if (!emulator.loadProgram(argv[1])) {
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    std::cout << "CHIP-8 Emulator" << std::endl;
    std::cout << "Controls (CHIP-8 keypad mapped to keyboard):" << std::endl;
    std::cout << "  1 2 3 4      maps to    1 2 3 C" << std::endl;
    std::cout << "  Q W E R                 4 5 6 D" << std::endl;
    std::cout << "  A S D F                 7 8 9 E" << std::endl;
    std::cout << "  Z X C V                 A 0 B F" << std::endl;
    std::cout << "\nPress ESC to exit\n" << std::endl;
    
    bool running = true;
    SDL_Event event;
    uint32_t pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
    
    const int cyclesPerFrame = 10;
    Uint32 lastTime = SDL_GetTicks();
    const int targetFPS = 60;
    const int frameDelay = 1000 / targetFPS;
    
    while (running) {
        Uint32 frameStart = SDL_GetTicks();
        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else {
                    int chip8Key = mapSDLKeyToChip8(event.key.keysym.sym);
                    if (chip8Key != -1) {
                        emulator.setKey(chip8Key, true);
                    }
                }
            } else if (event.type == SDL_KEYUP) {
                int chip8Key = mapSDLKeyToChip8(event.key.keysym.sym);
                if (chip8Key != -1) {
                    emulator.setKey(chip8Key, false);
                }
            }
        }
        
        for (int i = 0; i < cyclesPerFrame; ++i) {
            emulator.executeCycle();
        }
        
        const uint8_t* screen = emulator.getScreen();
        for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
            pixels[i] = screen[i] ? 0xFFFFFFFF : 0x000000FF;
        }
        
        SDL_UpdateTexture(texture, nullptr, pixels, SCREEN_WIDTH * sizeof(uint32_t));
        
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        
        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }
    
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    std::cout << "Emulator stopped." << std::endl;
     s
    return 0;
}