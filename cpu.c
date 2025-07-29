#include "logger.h"
#include "cpu.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* --------------------------- forward declaration -------------------------- */
static void chip8_load_fontset(chip8_t *chip8);

static void op_00E0(chip8_t *chip8);
static void op_00EE(chip8_t *chip8);
static void op_1nnn(chip8_t *chip8, uint16_t opcode);
static void op_2nnn(chip8_t *chip8, uint16_t opcode);
static void op_3xkk(chip8_t *chip8, uint16_t opcode);
static void op_4xkk(chip8_t *chip8, uint16_t opcode);
static void op_5xy0(chip8_t *chip8, uint16_t opcode);
static void op_6xkk(chip8_t *chip8, uint16_t opcode);
static void op_7xkk(chip8_t *chip8, uint16_t opcode);
static void op_8xy0(chip8_t *chip8, uint16_t opcode);
static void op_8xy1(chip8_t *chip8, uint16_t opcode);
static void op_8xy2(chip8_t *chip8, uint16_t opcode);
static void op_8xy3(chip8_t *chip8, uint16_t opcode);
static void op_8xy4(chip8_t *chip8, uint16_t opcode);
static void op_8xy5(chip8_t *chip8, uint16_t opcode);
static void op_8xy6(chip8_t *chip8, uint16_t opcode);
static void op_8xy7(chip8_t *chip8, uint16_t opcode);
static void op_8xyE(chip8_t *chip8, uint16_t opcode);
static void op_9xy0(chip8_t *chip8, uint16_t opcode);
static void op_Annn(chip8_t *chip8, uint16_t opcode);
static void op_Bnnn(chip8_t *chip8, uint16_t opcode);
static void op_Cxkk(chip8_t *chip8, uint16_t opcode);
static void op_Dxyn(chip8_t *chip8, uint16_t opcode);
static void op_Ex9E(chip8_t *chip8, uint16_t opcode);
static void op_ExA1(chip8_t *chip8, uint16_t opcode);
static void op_Fx07(chip8_t *chip8, uint16_t opcode);
static void op_Fx0A(chip8_t *chip8, uint16_t opcode);
static void op_Fx15(chip8_t *chip8, uint16_t opcode);
static void op_Fx18(chip8_t *chip8, uint16_t opcode);
static void op_Fx1E(chip8_t *chip8, uint16_t opcode);
static void op_Fx29(chip8_t *chip8, uint16_t opcode);
static void op_Fx33(chip8_t *chip8, uint16_t opcode);
static void op_Fx55(chip8_t *chip8, uint16_t opcode);
static void op_Fx65(chip8_t *chip8, uint16_t opcode);

static uint8_t fontset[FONTSET_SIZE] = {
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

/* ---------------------------- initialisation/cycle functions ---------------------------- */
/**
 * @brief loads fontset into memory, starting from 0x50
 *
 * @param chip8 pointer to chip8 struct
 */
static void chip8_load_fontset(chip8_t *chip8)
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
    LOG_ERROR("Could not open %s", rom_path);
    return 1;
  }

  fseek(rom_file, 0, SEEK_END);
  long rom_size = ftell(rom_file); // size of file in bytes
  rewind(rom_file);

  if (rom_size > MEMORY_SIZE - START_ADDRESS)
  {
    LOG_ERROR("%s is too large to fit in memory", rom_path);
    fclose(rom_file);
    return 1;
  }

  size_t bytes_read = fread(&chip8->memory[START_ADDRESS], 1, rom_size, rom_file);

  if (bytes_read != rom_size)
  {
    // number of bytes read doesnt match the file size, ROM not loaded properly
    LOG_ERROR("Failed to read from %s", rom_path);
    fclose(rom_file);
    return 1;
  }
  else
  {
    LOG_OK("Read from %s successfully", rom_path);
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
  case 0x0000:
    switch (opcode & 0x00FF)
    {
    case 0x00E0:
      LOG_INFO("00E0 - CLS");
      op_00E0(chip8);
      break;
    case 0x00EE:
      LOG_INFO("00EE - RET");
      op_00EE(chip8);
      break;
    }
    break;
  case 0x1000:
    LOG_INFO("1nnn - JP 0x%03X", opcode & 0x0FFF);
    op_1nnn(chip8, opcode);
    break;
  case 0x2000:
    LOG_INFO("2nnn - CALL 0x%03X", opcode & 0x0FFF);
    op_2nnn(chip8, opcode);
    break;
  case 0x3000:
    LOG_INFO("3xkk - SE V%X, 0x%02X", (opcode & 0x0F00) >> 8, opcode & 0x00FF);
    op_3xkk(chip8, opcode);
    break;
  case 0x4000:
    LOG_INFO("4xkk - SNE V%X, 0x%02X", (opcode & 0x0F00) >> 8, opcode & 0x00FF);
    op_4xkk(chip8, opcode);
    break;
  case 0x5000:
    LOG_INFO("5xy0 - SE V%X, V%X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
    op_5xy0(chip8, opcode);
    break;
  case 0x6000:
    LOG_INFO("6xkk - LD V%X, 0x%02X", (opcode & 0x0F00) >> 8, opcode & 0x00FF);
    op_6xkk(chip8, opcode);
    break;
  case 0x7000:
    LOG_INFO("7xkk - ADD V%X, 0x%02X", (opcode & 0x0F00) >> 8, opcode & 0x00FF);
    op_7xkk(chip8, opcode);
    break;
  case 0x8000:
    switch (opcode & 0x000F)
    {
    case 0x0000:
      LOG_INFO("8xy0 - LD V%X, V%X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
      op_8xy0(chip8, opcode);
      break;
    case 0x0001:
      LOG_INFO("8xy1 - OR V%X, V%X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
      op_8xy1(chip8, opcode);
      break;
    case 0x0002:
      LOG_INFO("8xy2 - AND V%X, V%X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
      op_8xy2(chip8, opcode);
      break;
    case 0x0003:
      LOG_INFO("8xy3 - XOR V%X, V%X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
      op_8xy3(chip8, opcode);
      break;
    case 0x0004:
      LOG_INFO("8xy4 - ADD V%X, V%X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
      op_8xy4(chip8, opcode);
      break;
    case 0x0005:
      LOG_INFO("8xy5 - SUB V%X, V%X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
      op_8xy5(chip8, opcode);
      break;
    case 0x0006:
      LOG_INFO("8xy6 - SHR V%X", (opcode & 0x0F00) >> 8);
      op_8xy6(chip8, opcode);
      break;
    case 0x0007:
      LOG_INFO("8xy7 - SUBN V%X, V%X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
      op_8xy7(chip8, opcode);
      break;
    case 0x000E:
      LOG_INFO("8xyE - SHL V%X", (opcode & 0x0F00) >> 8);
      op_8xyE(chip8, opcode);
      break;
    }
    break;
  case 0x9000:
    LOG_INFO("9xy0 - SNE V%X, V%X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
    op_9xy0(chip8, opcode);
    break;
  case 0xA000:
    LOG_INFO("Annn - LD I, 0x%03X", opcode & 0x0FFF);
    op_Annn(chip8, opcode);
    break;
  case 0xB000:
    LOG_INFO("Bnnn - JP V0, 0x%03X", opcode & 0x0FFF);
    op_Bnnn(chip8, opcode);
    break;
  case 0xC000:
    LOG_INFO("Cxkk - RND V%X, 0x%02X", (opcode & 0x0F00) >> 8, opcode & 0x00FF);
    op_Cxkk(chip8, opcode);
    break;
  case 0xD000:
    LOG_INFO("Dxyn - DRW V%X, V%X, %d", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4, opcode & 0x000F);
    op_Dxyn(chip8, opcode);
    break;
  case 0xE000:
    switch (opcode & 0x00FF)
    {
    case 0x009E:
      LOG_INFO("Ex9E - SKP V%X", (opcode & 0x0F00) >> 8);
      op_Ex9E(chip8, opcode);
      break;
    case 0x00A1:
      LOG_INFO("ExA1 - SKNP V%X", (opcode & 0x0F00) >> 8);
      op_ExA1(chip8, opcode);
      break;
    }
    break;
  case 0xF000:
    switch (opcode & 0x00FF)
    {
    case 0x0007:
      LOG_INFO("Fx07 - LD V%X, DT", (opcode & 0x0F00) >> 8);
      op_Fx07(chip8, opcode);
      break;
    case 0x000A:
      LOG_INFO("Fx0A - LD V%X, K", (opcode & 0x0F00) >> 8);
      op_Fx0A(chip8, opcode);
      break;
    case 0x0015:
      LOG_INFO("Fx15 - LD DT, V%X", (opcode & 0x0F00) >> 8);
      op_Fx15(chip8, opcode);
      break;
    case 0x0018:
      LOG_INFO("Fx18 - LD ST, V%X", (opcode & 0x0F00) >> 8);
      op_Fx18(chip8, opcode);
      break;
    case 0x001E:
      LOG_INFO("Fx1E - ADD I, V%X", (opcode & 0x0F00) >> 8);
      op_Fx1E(chip8, opcode);
      break;
    case 0x0029:
      LOG_INFO("Fx29 - LD F, V%X", (opcode & 0x0F00) >> 8);
      op_Fx29(chip8, opcode);
      break;
    case 0x0033:
      LOG_INFO("Fx33 - LD B, V%X", (opcode & 0x0F00) >> 8);
      op_Fx33(chip8, opcode);
      break;
    case 0x0055:
      LOG_INFO("Fx55 - LD [I], V%X", (opcode & 0x0F00) >> 8);
      op_Fx55(chip8, opcode);
      break;
    case 0x0065:
      LOG_INFO("Fx65 - LD V%X, [I]", (opcode & 0x0F00) >> 8);
      op_Fx65(chip8, opcode);
      break;
    }
    break;
  default:
    // invalid opcode
    LOG_ERROR("Invalid opcode: 0x%X", opcode);
    break;
  }
}

/* ------------------------- opcode implementations ------------------------- */

/**
 * @brief 00E0 - CLS
 *
 * Clear the display.
 *
 * @param chip8 pointer to chip8 struct
 */
static void op_00E0(chip8_t *chip8)
{
  memset(chip8->display, 0, sizeof(chip8->display));
}

/**
 * @brief 00EE - RET
 *
 * Returns from a subroutine.
 *
 * @param chip8 pointer to chip8 struct
 */
static void op_00EE(chip8_t *chip8)
{
  chip8->pc = chip8->stack[--chip8->sp];
}

/**
 * @brief 1nnn - JP addr
 *
 * Jump to location nnn.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_1nnn(chip8_t *chip8, uint16_t opcode)
{
  uint16_t nnn = opcode & 0x0FFF;
  chip8->pc = nnn;
}

/**
 * @brief 2nnn - CALL addr
 *
 * Call subroutine at nnn.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_2nnn(chip8_t *chip8, uint16_t opcode)
{
  uint16_t nnn = opcode & 0x0FFF;

  chip8->stack[chip8->sp++] = chip8->pc;
  chip8->pc = nnn;
}

/**
 * @brief 3xkk - SE Vx, byte
 *
 * Skip next instruction if Vx = kk.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_3xkk(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t kk = opcode & 0x00FF;

  if (chip8->registers[x] == kk)
  {
    chip8->pc += 2;
  }
}

/**
 * @brief 4xkk - SNE Vx, byte
 *
 * Skip next instruction if Vx != kk.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_4xkk(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t kk = opcode & 0x00FF;

  if (chip8->registers[x] != kk)
  {
    chip8->pc += 2;
  }
}

/**
 * @brief 5xy0 - SE Vx, Vy
 *
 * Skip next instruction if Vx = Vy.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_5xy0(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t y = (opcode & 0x00F0) >> 4;

  if (chip8->registers[x] == chip8->registers[y])
  {
    chip8->pc += 2;
  }
}

/**
 * @brief 6xkk - LD Vx, byte
 *
 * Set Vx = kk.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_6xkk(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t kk = opcode & 0x00FF;

  chip8->registers[x] = kk;
}

/**
 * @brief 7xkk - ADD Vx, byte
 *
 * Set Vx = Vx + kk.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_7xkk(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t kk = opcode & 0x00FF;

  chip8->registers[x] += kk;
}

/**
 * @brief 8xy0 - LD Vx, Vy
 *
 * Set Vx = Vy.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_8xy0(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t y = (opcode & 0x00F0) >> 4;

  chip8->registers[x] = chip8->registers[y];
}

/**
 * @brief 8xy1 - OR Vx, Vy
 *
 * Set Vx = Vx OR Vy.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_8xy1(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t y = (opcode & 0x00F0) >> 4;

  chip8->registers[x] |= chip8->registers[y];
}

/**
 * @brief 8xy2 - AND Vx, Vy
 *
 * Set Vx = Vx AND Vy.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_8xy2(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t y = (opcode & 0x00F0) >> 4;

  chip8->registers[x] &= chip8->registers[y];
}

/**
 * @brief 8xy3 - XOR Vx, Vy
 *
 * Set Vx = Vx XOR Vy.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_8xy3(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t y = (opcode & 0x00F0) >> 4;

  chip8->registers[x] ^= chip8->registers[y];
}

/**
 * @brief 8xy4 - ADD Vx, Vy
 *
 * Set Vx = Vx + Vy, set VF = carry.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_8xy4(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t y = (opcode & 0x00F0) >> 4;

  uint16_t sum = chip8->registers[x] + chip8->registers[y];

  if (sum > 255)
  {
    chip8->registers[0xF] = 1;
  }
  else
  {
    chip8->registers[0xF] = 0;
  }

  chip8->registers[x] = sum & 0xFF; // only the lowest 8 bits of the result are kept, and stored in Vx
}

/**
 * @brief 8xy5 - SUB Vx, Vy
 *
 * Set Vx = Vx - Vy, set VF = NOT borrow.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_8xy5(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t y = (opcode & 0x00F0) >> 4;

  if (chip8->registers[x] > chip8->registers[y])
  {
    chip8->registers[0xF] = 1;
  }
  else
  {
    chip8->registers[0xF] = 0;
  }

  chip8->registers[x] -= chip8->registers[y];
}

/**
 * @brief 8xy6 - SHR Vx {, Vy}
 *
 * Set Vx = Vx SHR 1.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_8xy6(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;

  chip8->registers[0xF] = chip8->registers[x] & 0x01; // store LSB in VF

  chip8->registers[x] >>= 1; // shift right by 1 is same as dividing by 2 on unsigned ints
}

/**
 * @brief 8xy7 - SUBN Vx, Vy
 *
 * Set Vx = Vy - Vx, set VF = NOT borrow.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_8xy7(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t y = (opcode & 0x00F0) >> 4;

  if (chip8->registers[y] > chip8->registers[x])
  {
    chip8->registers[0xF] = 1;
  }
  else
  {
    chip8->registers[0xF] = 0;
  }

  chip8->registers[x] = chip8->registers[y] - chip8->registers[x];
}

/**
 * @brief 8xyE - SHL Vx {, Vy}
 *
 * Set Vx = Vx SHL 1.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_8xyE(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;

  chip8->registers[0xF] = (chip8->registers[x] & 0x80) >> 7; // store MSB in VF

  chip8->registers[x] <<= 1; // shift left by 1 is same as multiplying by 2 on unsigned ints
}

/**
 * @brief 9xy0 - SNE Vx, Vy
 *
 * Skip next instruction if Vx != Vy.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_9xy0(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t y = (opcode & 0x00F0) >> 4;

  if (chip8->registers[x] != chip8->registers[y])
  {
    chip8->pc += 2;
  }
}

/**
 * @brief Annn - LD I, addr
 *
 * Set I = nnn.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_Annn(chip8_t *chip8, uint16_t opcode)
{
  uint16_t nnn = opcode & 0x0FFF;
  chip8->index = nnn;
}

/**
 * @brief Bnnn - JP V0, addr
 *
 * Jump to location nnn + V0.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_Bnnn(chip8_t *chip8, uint16_t opcode)
{
  uint16_t nnn = opcode & 0x0FFF;
  chip8->pc = nnn + chip8->registers[0];
}

/**
 * @brief Cxkk - RND Vx, byte
 *
 * Set Vx = random byte AND kk.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_Cxkk(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t kk = opcode & 0x00FF;

  // generate a random byte and AND it with kk
  chip8->registers[x] = (rand() % 256) & kk;
}

/**
 * @brief Dxyn - DRW Vx, Vy, nibble
 *
 * Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_Dxyn(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t y = (opcode & 0x00F0) >> 4;
  uint8_t n = opcode & 0x000F; // height of sprite in pixels; sprites are ALWAYS 8 pixels wide

  uint8_t pos_x = chip8->registers[x] % DISPLAY_WIDTH;
  uint8_t pos_y = chip8->registers[y] % DISPLAY_HEIGHT;

  chip8->registers[0xF] = 0; // reset collision to false

  for (int row = 0; row < n; ++row)
  {
    uint8_t sprite_byte = chip8->memory[chip8->index + row];

    for (int col = 0; col < 8; ++col)
    {
      uint8_t sprite_pixel = sprite_byte & (0x80 >> col);
      uint16_t screen_index = ((pos_y + row) * DISPLAY_WIDTH) + (pos_x + col);
      uint16_t screen_pixel = chip8->display[screen_index];

      if (sprite_pixel != 0)
      {
        if (screen_pixel)
        { // set collision to true
          chip8->registers[0xF] = 1;
        }

        chip8->display[screen_index] ^= 1; // flip screen pixel with XOR
      }
    }
  }
}

/**
 * @brief Ex9E - SKP Vx
 *
 * Skip next instruction if key with the value of Vx is pressed.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_Ex9E(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;

  if (chip8->keypad[chip8->registers[x]])
  { // key is activated
    chip8->pc += 2;
  }
}

/**
 * @brief ExA1 - SKNP Vx
 *
 * Skip next instruction if key with the value of Vx is not pressed.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_ExA1(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;

  if (!chip8->keypad[chip8->registers[x]])
  { // key is not activated
    chip8->pc += 2;
  }
}

/**
 * @brief Fx07 - LD Vx, DT
 *
 * Set Vx = delay timer value.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_Fx07(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  chip8->registers[x] = chip8->delay_timer;
}

/**
 * @brief Fx0A - LD Vx, K
 *
 * Wait for a key press, store the value of the key in Vx.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_Fx0A(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;

  for (int key = 0; key < KEY_COUNT; ++key)
  {
    if (chip8->keypad[key] != 0)
    {
      // key was pressed
      chip8->registers[x] = key;
      return;
    }
  }
  // exit loop when no key press detected

  chip8->pc -= 2; // decrement PC so this instruction runs infinitely until a key is pressed
}

/**
 * @brief Fx15 - LD DT, Vx
 *
 * Set delay timer = Vx.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_Fx15(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  chip8->delay_timer = chip8->registers[x];
}

/**
 * @brief Fx18 - LD ST, Vx
 *
 * Set sound timer = Vx.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_Fx18(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  chip8->sound_timer = chip8->registers[x];
}

/**
 * @brief Fx1E - ADD I, Vx
 *
 * Set I = I + Vx.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_Fx1E(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  chip8->index += chip8->registers[x];
}

/**
 * @brief Fx29 - LD F, Vx
 *
 * Set I = location of sprite for digit Vx.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_Fx29(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t digit = chip8->registers[x];

  // fontset start at 0x50 and each character is 5 bytes
  // location = 0x50 + (digit * 5)
  chip8->index = FONTSET_START_ADDRESS + (digit * 5);
}

/**
 * @brief Fx33 - LD B, Vx
 *
 * Store BCD representation of Vx in memory locations I, I+1, and I+2.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_Fx33(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t value = chip8->registers[x];

  // hundreds digit
  chip8->memory[chip8->index] = value / 100;

  // tens digit
  chip8->memory[chip8->index + 1] = (value / 10) % 10;

  // ones digit
  chip8->memory[chip8->index + 2] = value % 10;
}

/**
 * @brief Fx55 - LD [I], Vx
 *
 * Store registers V0 through Vx in memory starting at location I.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_Fx55(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;

  for (int i = 0; i <= x; ++i)
  {
    chip8->memory[chip8->index + i] = chip8->registers[i];
  }
}

/**
 * @brief Fx65 - LD Vx, [I]
 *
 * Read registers V0 through Vx from memory starting at location I.
 *
 * @param chip8 pointer to chip8 struct
 * @param opcode the current opcode
 */
static void op_Fx65(chip8_t *chip8, uint16_t opcode)
{
  uint8_t x = (opcode & 0x0F00) >> 8;

  for (int i = 0; i <= x; ++i)
  {
    chip8->registers[i] = chip8->memory[chip8->index + i];
  }
}
