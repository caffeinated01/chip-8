# 👾 CHIP-8 Emulator

CHIP-8 is an interpreted programming language, developed by Joseph Weisbecker on his 1802 microprocessor ([Wikepedia](https://en.wikipedia.org/wiki/CHIP-8)). This project is a CHIP-8 emulator written in C. I wrote it with the goal of dipping my feet into C while building something fun.\
Fantastic references:

- [Cowgod's Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM)
- [Tobias's Guide](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/)

## Features

(to be added)

## Building

0. Make sure you have the following installed on your system

- A C compiler (like [GCC](https://gcc.gnu.org/) or [Clang](https://clang.llvm.org/) etc.)
- [CMake](https://cmake.org/)

1. Clone the repository

```sh
git clone https://github.com/caffeinated01/chip-8.git
cd chip-8
```

2. Create a build directory and run CMake

```sh
mkdir build
cd build
cmake ..
```

3. Compile (and hope it works)

```sh
make
```

This will create a `chip8` executable in `build/bin/`

## Running

`Usage: chip8 [-v] [-s <scale>] -r <rom_path>` \
`-v` is for verbose logging. ommit this to disable verbose logging \
`-s` is for scale. Scale is multiplied to original display height and width, 64 and 32. A scale of 10 would result in a window that is 640px by 320px large. Defaulted as 10 \
`-d` is for cycle delay. Defaulted as 1 \
`-r` is for path to rom. \
Example usage:

```sh
./chip8 -r ../../roms/ibm-logo.ch8 -s 10 -v
```

## TODO

- [x] Scaffolding
- [x] Opcode implementation
- [x] Emulator main loop
- [x] Deal with timers
- [x] Rendering with a graphics library
- [x] Add configuration for render colors
- [x] Play BEEP sound
- [x] Test against [BonCoder/BestCoder test](https://github.com/cj1128/chip8-emulator/tree/master/rom) and [corax89's CHIP-8 test ROM](https://github.com/corax89/chip8-test-rom)
