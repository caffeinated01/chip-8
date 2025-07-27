#include "logger.h"
#include "cpu.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
  case 0x0000:
    switch (opcode & 0x00FF)
    {
    case 0x00E0:
      op_00E0(chip8);
      break;
    case 0x00EE:
      op_00EE(chip8);
      break;
    }
    break;
  case 0x1000:
    op_1nnn(chip8, opcode);
    break;
  case 0x2000:
    op_2nnn(chip8, opcode);
    break;
  case 0x3000:
    op_3xkk(chip8, opcode);
    break;
  case 0x4000:
    op_4xkk(chip8, opcode);
    break;
  case 0x5000:
    op_5xy0(chip8, opcode);
    break;
  case 0x6000:
    op_6xkk(chip8, opcode);
    break;
  case 0x7000:
    op_7xkk(chip8, opcode);
    break;
  case 0x8000:
    switch (opcode & 0x000F)
    {
    case 0x0000:
      op_8xy0(chip8, opcode);
      break;
    case 0x0001:
      op_8xy1(chip8, opcode);
      break;
    case 0x0002:
      op_8xy2(chip8, opcode);
      break;
    case 0x0003:
      op_8xy3(chip8, opcode);
      break;
    case 0x0004:
      op_8xy4(chip8, opcode);
      break;
    case 0x0005:
      op_8xy5(chip8, opcode);
      break;
    case 0x0006:
      op_8xy6(chip8, opcode);
      break;
    case 0x0007:
      op_8xy7(chip8, opcode);
      break;
    case 0x000E:
      op_8xyE(chip8, opcode);
      break;
    }
    break;
  case 0x9000:
    op_9xy0(chip8, opcode);
    break;
  case 0xA000:
    op_Annn(chip8, opcode);
    break;
  case 0xB000:
    op_Bnnn(chip8, opcode);
    break;
  case 0xC000:
    op_Cxkk(chip8, opcode);
    break;
  case 0xD000:
    op_Dxyn(chip8, opcode);
    break;
  case 0xE000:
    switch (opcode & 0x00FF)
    {
    case 0x009E:
      op_Ex9E(chip8, opcode);
      break;
    case 0x00A1:
      op_ExA1(chip8, opcode);
      break;
    }
    break;
  case 0xF000:
    switch (opcode & 0x00FF)
    {
    case 0x0007:
      op_Fx07(chip8, opcode);
      break;
    case 0x000A:
      op_Fx0A(chip8, opcode);
      break;
    case 0x0015:
      op_Fx15(chip8, opcode);
      break;
    case 0x0018:
      op_Fx18(chip8, opcode);
      break;
    case 0x001E:
      op_Fx1E(chip8, opcode);
      break;
    case 0x0029:
      op_Fx29(chip8, opcode);
      break;
    case 0x0033:
      op_Fx33(chip8, opcode);
      break;
    case 0x0055:
      op_Fx55(chip8, opcode);
      break;
    case 0x0065:
      op_Fx65(chip8, opcode);
      break;
    }
    break;
  default:
    // invalid opcode
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
  uint8_t Vx = (opcode & 0x0F00) >> 8;
  uint8_t kk = opcode & 0x00FF;

  if (chip8->registers[Vx] == kk)
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
  uint8_t Vx = (opcode & 0x0F00) >> 8;
  uint8_t kk = opcode & 0x00FF;

  if (chip8->registers[Vx] != kk)
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
  uint8_t Vx = (opcode & 0x0F00) >> 8;
  uint8_t Vy = (opcode & 0x00F0) >> 4;

  if (chip8->registers[Vx] == chip8->registers[Vy])
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
  // todo: implement opcode 6xkk
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
  // todo: implement opcode 7xkk
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
  // todo: implement opcode 8xy0
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
  // todo: implement opcode 8xy1
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
  // todo: implement opcode 8xy2
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
  // todo: implement opcode 8xy3
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
  // todo: implement opcode 8xy4
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
  // todo: implement opcode 8xy5
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
  // todo: implement opcode 8xy6
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
  // todo: implement opcode 8xy7
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
  // todo: implement opcode 8xyE
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
  // todo: implement opcode 9xy0
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
  // todo: implement opcode Annn
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
  // todo: implement opcode Bnnn
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
  // todo: implement opcode Cxkk
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
  // todo: implement opcode Dxyn
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
  // todo: implement opcode Ex9E
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
  // todo: implement opcode ExA1
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
  // todo: implement opcode Fx07
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
  // todo: implement opcode Fx0A
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
  // todo: implement opcode Fx15
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
  // todo: implement opcode Fx18
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
  // todo: implement opcode Fx1E
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
  // todo: implement opcode Fx29
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
  // todo: implement opcode Fx33
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
  // todo: implement opcode Fx55
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
  // todo: implement opcode Fx65
}
