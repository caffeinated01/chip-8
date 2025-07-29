#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cpu.h"
#include "logger.h"
#include "config.h"
#include <SDL.h>

#define WINDOW_TITLE "CHIP-8"

typedef struct Emulator
{
  SDL_Window *window;
  SDL_Renderer *renderer;
} emulator_t;

static void print_usage(FILE *out, const char *program);

static int initialise_sdl(emulator_t *emulator);
static void cleanup_sdl(emulator_t *emulator, int exit_status);

/**
 * @brief prints available flags
 *
 * @param out `stdout` or `stderr`
 * @param program program name, which is `argv[0]`
 */
static void print_usage(FILE *out, const char *program)
{
  fprintf(out, "Usage: %s [-v] [-s <scale>] -r <rom_path>\n", program);
}

/**
 * @brief initialise SDL
 *
 * @param emulator pointer to emulator struct
 * @return `0` on success, `1` on failure
 */
static int initialise_sdl(emulator_t *emulator)
{
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
  {
    LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
    return 1;
  }

  const int window_width = DISPLAY_WIDTH * g_config.window_scale;
  const int window_height = DISPLAY_HEIGHT * g_config.window_scale;

  emulator->window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, 0);

  if (!emulator->window)
  {
    LOG_ERROR("SDL_CreateWindow failed: %s", SDL_GetError());
    return 1;
  }

  emulator->renderer = SDL_CreateRenderer(emulator->window, -1, 0);

  if (!emulator->renderer)
  {
    LOG_ERROR("SDL_CreateRenderer failed: %s", SDL_GetError());
    return 1;
  }

  return 0;
}

static void cleanup_sdl(emulator_t *emulator, int exit_status)
{
  SDL_DestroyRenderer(emulator->renderer);
  SDL_DestroyWindow(emulator->window);
  SDL_Quit();
  exit(exit_status);
}

int main(int argc, char **argv)
{
  const char *program = argv[0];
  char *rom_path = NULL;

  for (int i = 1; i < argc; ++i)
  {
    if (strcmp(argv[i], "-v") == 0)
    {
      g_config.verbose_logging = true;
      continue;
    }

    if (strcmp(argv[i], "-s") == 0)
    {
      if (i + 1 < argc)
      {
        g_config.window_scale = atoi(argv[++i]);
      }
      else
      {
        fprintf(stderr, "Scale value not provided\n");
        print_usage(stderr, program);
        exit(EXIT_FAILURE);
      }
      continue;
    }

    if (strcmp(argv[i], "-r") == 0)
    {
      if (i + 1 < argc)
      {
        rom_path = argv[++i];
      }
      else
      {
        fprintf(stderr, "ROM path not provided\n");
        print_usage(stderr, program);
        exit(EXIT_FAILURE);
      }
      continue;
    }

    fprintf(stderr, "Unknown argument: %s\n", argv[i]);
    print_usage(stderr, program);
    exit(EXIT_FAILURE);
  }

  if (rom_path == NULL)
  {
    fprintf(stderr, "ROM path not provided\n");
    print_usage(stderr, program);
    exit(EXIT_FAILURE);
  }

  LOG_INFO("Verbose logging enabled");
  LOG_INFO("ROM path: %s", rom_path);
  LOG_INFO("Window scale: %d", g_config.window_scale);

  chip8_t chip8;
  chip8_initialise(&chip8);
  if (chip8_load_rom(&chip8, rom_path) != 0)
  {
    exit(EXIT_FAILURE);
  }

  emulator_t emulator = {
      .window = NULL,
      .renderer = NULL,
  };

  if (initialise_sdl(&emulator) != 0)
  {
    cleanup_sdl(&emulator, EXIT_FAILURE);
  }

  bool running = true;
  while (running)
  {
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
      case SDL_QUIT:
        running = false;
        break;
      case SDL_KEYDOWN:
        switch (event.key.keysym.scancode)
        {
        case SDL_SCANCODE_ESCAPE:
          running = false;
          break;
        case SDL_SCANCODE_1:
          chip8.keypad[0x1] = 1;
          break;
        case SDL_SCANCODE_2:
          chip8.keypad[0x2] = 1;
          break;
        case SDL_SCANCODE_3:
          chip8.keypad[0x3] = 1;
          break;
        case SDL_SCANCODE_4:
          chip8.keypad[0xC] = 1;
          break;
        case SDL_SCANCODE_Q:
          chip8.keypad[0x4] = 1;
          break;
        case SDL_SCANCODE_W:
          chip8.keypad[0x5] = 1;
          break;
        case SDL_SCANCODE_E:
          chip8.keypad[0x6] = 1;
          break;
        case SDL_SCANCODE_R:
          chip8.keypad[0xD] = 1;
          break;
        case SDL_SCANCODE_A:
          chip8.keypad[0x7] = 1;
          break;
        case SDL_SCANCODE_S:
          chip8.keypad[0x8] = 1;
          break;
        case SDL_SCANCODE_D:
          chip8.keypad[0x9] = 1;
          break;
        case SDL_SCANCODE_F:
          chip8.keypad[0xE] = 1;
          break;
        case SDL_SCANCODE_Z:
          chip8.keypad[0xA] = 1;
          break;
        case SDL_SCANCODE_X:
          chip8.keypad[0x0] = 1;
          break;
        case SDL_SCANCODE_C:
          chip8.keypad[0xB] = 1;
          break;
        case SDL_SCANCODE_V:
          chip8.keypad[0xF] = 1;
          break;
        default:
          break;
        }
        break;

      case SDL_KEYUP:
        switch (event.key.keysym.scancode)
        {
        case SDL_SCANCODE_1:
          chip8.keypad[0x1] = 0;
          break;
        case SDL_SCANCODE_2:
          chip8.keypad[0x2] = 0;
          break;
        case SDL_SCANCODE_3:
          chip8.keypad[0x3] = 0;
          break;
        case SDL_SCANCODE_4:
          chip8.keypad[0xC] = 0;
          break;
        case SDL_SCANCODE_Q:
          chip8.keypad[0x4] = 0;
          break;
        case SDL_SCANCODE_W:
          chip8.keypad[0x5] = 0;
          break;
        case SDL_SCANCODE_E:
          chip8.keypad[0x6] = 0;
          break;
        case SDL_SCANCODE_R:
          chip8.keypad[0xD] = 0;
          break;
        case SDL_SCANCODE_A:
          chip8.keypad[0x7] = 0;
          break;
        case SDL_SCANCODE_S:
          chip8.keypad[0x8] = 0;
          break;
        case SDL_SCANCODE_D:
          chip8.keypad[0x9] = 0;
          break;
        case SDL_SCANCODE_F:
          chip8.keypad[0xE] = 0;
          break;
        case SDL_SCANCODE_Z:
          chip8.keypad[0xA] = 0;
          break;
        case SDL_SCANCODE_X:
          chip8.keypad[0x0] = 0;
          break;
        case SDL_SCANCODE_C:
          chip8.keypad[0xB] = 0;
          break;
        case SDL_SCANCODE_V:
          chip8.keypad[0xF] = 0;
          break;
        default:
          break;
        }
        break;
      default:
        break;
      }
    }

    chip8_cycle(&chip8);

    SDL_RenderClear(emulator.renderer);
    SDL_SetRenderDrawColor(emulator.renderer, 0, 0, 0, 255);
    SDL_RenderPresent(emulator.renderer);

    // todo: draw graphics from display buffer

    // todo: update timers

    SDL_Delay(16);
  }

  // cleanup
  cleanup_sdl(&emulator, EXIT_SUCCESS);

  return 0;
}