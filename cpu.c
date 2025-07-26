#include "log.h"
#include "cpu.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

uint8_t fontset[FONTSET_SIZE] = {
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
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void chip8_load_fontset(chip8_t *chip8)
{
  // load font into memory
  memcpy(&chip8->memory[FONTSET_START_ADDRESS], fontset, FONTSET_SIZE);

  LOG_OK("Fontset loaded into memory");
}

int chip8_load_rom(chip8_t *chip8, const char *rom_path)
{
  FILE *rom_file = fopen(rom_path, "rb");

  if (rom_file == NULL)
  {
    LOG_ERROR("Could not open ROM file %s", rom_path);
    return -1;
  }

  fseek(rom_file, 0, SEEK_END);
  long rom_size = ftell(rom_file); // size of file in bytes
  rewind(rom_file);

  if (rom_size > MEMORY_SIZE - START_ADDRESS)
  {
    LOG_ERROR("ROM file %s is too large to fit in memory", rom_path);
    fclose(rom_file);
    return -1;
  }

  size_t bytes_read = fread(&chip8->memory[START_ADDRESS], 1, rom_size, rom_file);

  if (bytes_read != rom_size)
  {
    // number of bytes read doesnt match the file size, ROM not loaded properly
    LOG_ERROR("Reading from ROM file at %s unsuccessful", rom_path);
    fclose(rom_file);
    return -1;
  }
  else
  {
    LOG_OK("Reading from ROM file at %s successful", rom_path);
  }

  fclose(rom_file);
  return 0;
}

void chip8_initialise(chip8_t *chip8)
{
  memset(chip8, 0, sizeof(chip8_t)); // clear memory

  chip8_load_fontset(chip8);

  // seed ONCE only. see https://stackoverflow.com/questions/7343833/srand-why-call-it-only-once
  srand((unsigned)time(NULL));

  chip8->pc = 0x200;
}

void chip8_cycle(chip8_t *chip8)
{
  uint16_t opcode = chip8->memory[chip8->pc] << 8 | chip8->memory[chip8->pc + 1]; // fetch opcode

  chip8->pc += 2; // increment PC before executing anything

  LOG_INFO("PC: %x", chip8->pc);
  LOG_INFO("Opcode: %x", opcode);

  // decode and execute
  switch (opcode & 0xF000)
  {
    // handle different opcode groups
  default:
    // invalid opcode
    break;
  }
}
