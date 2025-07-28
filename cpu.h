#pragma once

#include <stdint.h>

#define MEMORY_SIZE 4096
#define REGISTER_COUNT 16
#define STACK_DEPTH 16
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define KEY_COUNT 16
#define FONTSET_SIZE 80
#define FONTSET_START_ADDRESS 0x50
#define START_ADDRESS 0x200

typedef struct chip8
{
  uint8_t memory[MEMORY_SIZE];                     // 4KB of memory
  uint8_t registers[REGISTER_COUNT];               // 16 general-purpose registers
  uint16_t index;                                  // index register
  uint16_t pc;                                     // program counter
  uint16_t stack[STACK_DEPTH];                     // stack for subroutine calls
  uint8_t sp;                                      // stack pointer
  uint8_t keypad[KEY_COUNT];                       // keypad state for 16 keys
  uint8_t display[DISPLAY_WIDTH * DISPLAY_HEIGHT]; // 64 by 32 display
  uint8_t delay_timer;                             // delay timer
  uint8_t sound_timer;                             // sound timer
} chip8_t;

/* --------------------------- function prototypes -------------------------- */

/**
 * @brief loads fontset into memory, starting from 0x50
 *
 * @param chip8 pointer to chip8 struct
 */
void chip8_load_fontset(chip8_t *chip8);

/**
 * @brief loads a ROM into memory
 *
 * @param chip8 pointer to chip8 struct
 * @param rom_path path to the ROM file
 * @return `0` on success, `-1` on failure
 */
int chip8_load_rom(chip8_t *chip8, const char *rom_path);

/**
 * @brief initialises the chip8 struct
 *
 * @param chip8 pointer to chip8 struct
 */
void chip8_initialise(chip8_t *chip8);

/**
 * @brief executes a single chip8 cycle
 *
 * @param chip8 pointer to chip8 struct
 */
void chip8_cycle(chip8_t *chip8);
