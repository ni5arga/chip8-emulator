# CHIP-8 Emulator

A CHIP-8 interpreter written in C++ with SDL2 graphics. This project implements the complete CHIP-8 instruction set with real-time rendering and keyboard input.

## About CHIP-8

CHIP-8 is an interpreted programming language from the mid-1970s, originally designed by Joseph Weisbecker for 8-bit microcomputers. It provides a simple virtual machine with 4KB of RAM, 16 registers, and a 64x32 monochrome display.

## Features

- Complete implementation of all 35 CHIP-8 opcodes
- Hardware-accelerated SDL2 rendering at 60 FPS
- Real-time keyboard input handling
- 960x480 window with 15x pixel scaling

## Building

You'll need SDL2 development libraries installed:

**macOS:**
```bash
brew install sdl2
g++ -o chip8 emulator.cpp -std=c++11 -I/opt/homebrew/include -L/opt/homebrew/lib -lSDL2
```

**Linux:**
```bash
sudo apt-get install libsdl2-dev
g++ -o chip8 emulator.cpp -std=c++11 -lSDL2
```

**Windows (MinGW):**
```bash
g++ -o chip8.exe emulator.cpp -std=c++11 -lmingw32 -lSDL2main -lSDL2
```

## Usage

```bash
./chip8 path/to/rom.ch8
```

## Controls

The CHIP-8 keypad is mapped to your keyboard as follows:

```
Keyboard:          CHIP-8:
1 2 3 4            1 2 3 C
Q W E R            4 5 6 D
A S D F            7 8 9 E
Z X C V            A 0 B F
```

Press ESC to exit.

## Getting ROMs

Public domain CHIP-8 ROMs are available from:
- [CHIP-8 Archive](https://johnearnest.github.io/chip8Archive/)
- [Zophar's Domain](https://www.zophar.net/pdroms/chip8.html)
- [GitHub: chip8-roms](https://github.com/kripod/chip8-roms)

Popular games to try: PONG, TETRIS, SPACE INVADERS, BREAKOUT

## Architecture

### Memory Map

The CHIP-8 uses 4KB of addressable memory:

```
0x000-0x1FF (0-511)      Reserved for interpreter
0x050-0x09F              Font data for hex digits 0-F
0x200-0xFFF (512-4095)   Program ROM and work RAM
```

Programs are loaded starting at address 0x200. The first 512 bytes are reserved, with font sprites stored at 0x050-0x09F.

### Registers

- **V0-VF:** 16 general-purpose 8-bit registers (VF doubles as a flag register)
- **I:** 16-bit index register used for memory operations
- **PC:** 16-bit program counter
- **SP:** 8-bit stack pointer
- **DT:** Delay timer (decrements at 60Hz)
- **ST:** Sound timer (decrements at 60Hz, beeps when non-zero)

### Display

The screen is 64x32 pixels, monochrome. Sprites are drawn using XOR logic, which means drawing the same sprite twice erases it. When pixels are erased during drawing, the VF register is set to 1 to indicate collision.

### Instruction Format

Each instruction is 2 bytes. The format varies by opcode:

```
General format: NXYN
  N   = Opcode nibble
  X   = Register identifier (4 bits)
  Y   = Register identifier (4 bits)
  NNN = 12-bit address
  NN  = 8-bit constant
```

Example instructions:
- `00E0` - Clear the display
- `1NNN` - Jump to address NNN
- `6XNN` - Set register VX to NN
- `7XNN` - Add NN to register VX
- `DXYN` - Draw sprite at coordinates (VX, VY) with height N

## How It Works

The emulator follows a standard fetch-decode-execute cycle:

1. **Fetch** - Read the 2-byte instruction from memory at the program counter
2. **Decode** - Parse the opcode and extract any operands (registers, addresses, constants)
3. **Execute** - Perform the operation: arithmetic, logic, memory access, control flow, graphics, or input
4. **Update** - Increment the program counter and decrement timers
5. **Repeat** - Continue at 60Hz

The graphics system uses SDL2 to render the 64x32 framebuffer to a larger window. Each frame, the emulator executes multiple instructions (currently 10 per frame) to approximate the original CHIP-8 speed. The delay and sound timers count down at 60Hz independent of instruction execution.

Input is handled through SDL's event system. Key presses and releases update the internal keypad state, which instructions can query to implement game controls.

## Implementation Details

The core emulator is encapsulated in a Chip8 class that maintains all system state. Memory is represented as a simple byte array. The screen is a 1D array of 2048 bytes representing the 64x32 pixel grid. Sprites are drawn by XORing pixel data with the framebuffer.

The instruction decoder uses a switch statement on the high nibble of each opcode, with nested switches for opcodes that share the same high nibble. This provides efficient dispatch while keeping the code readable.

Random number generation uses the modern C++ random library with a Mersenne Twister generator, properly seeded from a hardware random device.

## Resources

- [Cowgod's CHIP-8 Technical Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM) - Complete opcode reference
- [How to write an emulator](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/) - Detailed tutorial
- [SDL2 Documentation](https://wiki.libsdl.org/) - SDL library reference

