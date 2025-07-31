#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "cpu.h"
#include "logger.h"
#include "config.h"
#include <SDL.h>

#define WINDOW_TITLE "CHIP-8"

#define SAMPLE_RATE 48000 // number of samples computer takes per second to represent the wave
#define AMPLITUDE 2000
#define FREQUENCY 440 // "440Hz is a middle C and pleasant as well" ~ https://forum.allaboutcircuits.com/threads/frequency-for-a-nice-beep-sound.116284/

typedef struct Emulator
{
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_AudioDeviceID audio_device;
  color_t bg_color;
  color_t fg_color;
} emulator_t;

/* --------------------------- forward declaration -------------------------- */

static void print_usage(FILE *out, const char *program);

static color_t hex_to_rgba(const char *hex);

static int initialise_sdl(emulator_t *emulator);
static void cleanup_sdl(emulator_t *emulator, int exit_status);

static void parse_arguments(int argc, char **argv, char **rom_path);

static void handle_input(chip8_t *chip8, bool *running);
static void draw_display(chip8_t *chip8, emulator_t *emulator);
static void audio_callback(void *userdata, uint8_t *stream, int len);

/* ---------------------------- helper functions ---------------------------- */

/**
 * @brief prints available flags
 *
 * @param out `stdout` or `stderr`
 * @param program program name, which is `argv[0]`
 */
static void print_usage(FILE *out, const char *program)
{
  fprintf(out, "Usage: %s [-v] [-s <scale>] [-d <delay>] [-c <bg_color> <fg_color>] -r <rom_path>\n", program);
}

/**
 * @brief converts hex color code to rgba
 *
 * @param hex string representing color code
 * @return color_t struct with r g b a properties
 */
static color_t hex_to_rgba(const char *hex)
{
  if (hex[0] == '#')
  {
    hex++;
  }

  char hex_component[3];
  hex_component[2] = '\0';

  // rr
  strncpy(hex_component, hex, 2);
  long r = strtol(hex_component, NULL, 16);

  // gg
  strncpy(hex_component, hex + 2, 2);
  long g = strtol(hex_component, NULL, 16);

  // bb
  strncpy(hex_component, hex + 4, 2);
  long b = strtol(hex_component, NULL, 16);

  color_t color = {
      .r = (uint8_t)r,
      .g = (uint8_t)g,
      .b = (uint8_t)b,
      .a = 255,
  };

  return color;
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

  SDL_AudioSpec want, have;
  SDL_zero(want);
  want.freq = SAMPLE_RATE;
  want.format = AUDIO_S16SYS;
  want.channels = 1;
  want.samples = 4096;
  want.callback = audio_callback;

  emulator->audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
  if (!emulator->audio_device)
  {
    LOG_ERROR("SDL_OpenAudioDevice failed: %s", SDL_GetError());
    return 1;
  }

  return 0;
}

/**
 * @brief releases SDL resources and exits with `exit_status`
 *
 * @param emulator pointer to emulator struct
 * @param exit_status `EXIT_SUCCESS` or `EXIT_FAILURE`
 */
static void cleanup_sdl(emulator_t *emulator, int exit_status)
{
  SDL_CloseAudioDevice(emulator->audio_device);
  SDL_DestroyRenderer(emulator->renderer);
  SDL_DestroyWindow(emulator->window);
  SDL_Quit();
  exit(exit_status);
}

/**
 * @brief parses cli arguments
 *
 * @param argc from main
 * @param argv from main
 * @param rom_path pointer to rom_path variable in main
 */
static void parse_arguments(int argc, char **argv, char **rom_path)
{
  const char *program = argv[0];

  for (int i = 1; i < argc; ++i)
  {
    if (strcmp(argv[i], "-r") == 0)
    {
      if (i + 1 < argc)
      {
        *rom_path = argv[++i];
      }
      else
      {
        fprintf(stderr, "ROM path not provided\n");
        print_usage(stderr, program);
        exit(EXIT_FAILURE);
      }
      continue;
    }

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

    if (strcmp(argv[i], "-d") == 0)
    {
      if (i + 1 < argc)
      {
        g_config.cycle_delay = atoi(argv[++i]);
      }
      else
      {
        fprintf(stderr, "Cycle delay not provided\n");
        print_usage(stderr, program);
        exit(EXIT_FAILURE);
      }
      continue;
    }

    if (strcmp(argv[i], "-c") == 0)
    {
      if (i + 2 < argc)
      {
        g_config.bg_color = hex_to_rgba(argv[++i]);
        g_config.fg_color = hex_to_rgba(argv[++i]);
      }
      else
      {
        fprintf(stderr, "Render colors not provided\n");
        print_usage(stderr, program);
        exit(EXIT_FAILURE);
      }
      continue;
    }

    fprintf(stderr, "Unknown argument: %s\n", argv[i]);
    print_usage(stderr, program);
    exit(EXIT_FAILURE);
  }
}

/**
 * @brief processes user input by handling SDL events
 *
 * @param chip8
 * @param running
 */
static void handle_input(chip8_t *chip8, bool *running)
{
  SDL_Event event;

  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
    case SDL_QUIT:
      *running = false;
      break;
    case SDL_KEYDOWN:
      switch (event.key.keysym.scancode)
      {
      case SDL_SCANCODE_ESCAPE:
        *running = false;
        break;
      case SDL_SCANCODE_1:
        chip8->keypad[0x1] = 1;
        break;
      case SDL_SCANCODE_2:
        chip8->keypad[0x2] = 1;
        break;
      case SDL_SCANCODE_3:
        chip8->keypad[0x3] = 1;
        break;
      case SDL_SCANCODE_4:
        chip8->keypad[0xC] = 1;
        break;
      case SDL_SCANCODE_Q:
        chip8->keypad[0x4] = 1;
        break;
      case SDL_SCANCODE_W:
        chip8->keypad[0x5] = 1;
        break;
      case SDL_SCANCODE_E:
        chip8->keypad[0x6] = 1;
        break;
      case SDL_SCANCODE_R:
        chip8->keypad[0xD] = 1;
        break;
      case SDL_SCANCODE_A:
        chip8->keypad[0x7] = 1;
        break;
      case SDL_SCANCODE_S:
        chip8->keypad[0x8] = 1;
        break;
      case SDL_SCANCODE_D:
        chip8->keypad[0x9] = 1;
        break;
      case SDL_SCANCODE_F:
        chip8->keypad[0xE] = 1;
        break;
      case SDL_SCANCODE_Z:
        chip8->keypad[0xA] = 1;
        break;
      case SDL_SCANCODE_X:
        chip8->keypad[0x0] = 1;
        break;
      case SDL_SCANCODE_C:
        chip8->keypad[0xB] = 1;
        break;
      case SDL_SCANCODE_V:
        chip8->keypad[0xF] = 1;
        break;
      default:
        break;
      }
      break;

    case SDL_KEYUP:
      switch (event.key.keysym.scancode)
      {
      case SDL_SCANCODE_1:
        chip8->keypad[0x1] = 0;
        break;
      case SDL_SCANCODE_2:
        chip8->keypad[0x2] = 0;
        break;
      case SDL_SCANCODE_3:
        chip8->keypad[0x3] = 0;
        break;
      case SDL_SCANCODE_4:
        chip8->keypad[0xC] = 0;
        break;
      case SDL_SCANCODE_Q:
        chip8->keypad[0x4] = 0;
        break;
      case SDL_SCANCODE_W:
        chip8->keypad[0x5] = 0;
        break;
      case SDL_SCANCODE_E:
        chip8->keypad[0x6] = 0;
        break;
      case SDL_SCANCODE_R:
        chip8->keypad[0xD] = 0;
        break;
      case SDL_SCANCODE_A:
        chip8->keypad[0x7] = 0;
        break;
      case SDL_SCANCODE_S:
        chip8->keypad[0x8] = 0;
        break;
      case SDL_SCANCODE_D:
        chip8->keypad[0x9] = 0;
        break;
      case SDL_SCANCODE_F:
        chip8->keypad[0xE] = 0;
        break;
      case SDL_SCANCODE_Z:
        chip8->keypad[0xA] = 0;
        break;
      case SDL_SCANCODE_X:
        chip8->keypad[0x0] = 0;
        break;
      case SDL_SCANCODE_C:
        chip8->keypad[0xB] = 0;
        break;
      case SDL_SCANCODE_V:
        chip8->keypad[0xF] = 0;
        break;
      default:
        break;
      }
      break;
    default:
      break;
    }
  }
}

/**
 * @brief draws pixels from the display buffer to the screen
 *
 * @param emulator pointer to emulator struct
 * @param chip8 pointer to chip8 struct
 */
static void draw_display(chip8_t *chip8, emulator_t *emulator)
{
  // bg color
  SDL_SetRenderDrawColor(emulator->renderer, emulator->bg_color.r, emulator->bg_color.g, emulator->bg_color.b, emulator->bg_color.a);
  SDL_RenderClear(emulator->renderer);

  // fg color
  SDL_SetRenderDrawColor(emulator->renderer, emulator->fg_color.r, emulator->fg_color.g, emulator->fg_color.b, emulator->fg_color.a);

  for (int row = 0; row < DISPLAY_HEIGHT; ++row)
  {
    for (int col = 0; col < DISPLAY_WIDTH; ++col)
    {

      if (chip8->display[row * DISPLAY_WIDTH + col])
      {
        SDL_Rect rect = {
            .x = col * g_config.window_scale,
            .y = row * g_config.window_scale,
            .w = g_config.window_scale,
            .h = g_config.window_scale,
        };
        SDL_RenderFillRect(emulator->renderer, &rect);
      }
    }
  }
  SDL_RenderPresent(emulator->renderer);
}

/**
 * @brief audio callback function to generate square wave
 *
 * @param userdata unused
 * @param stream buffer to write audio data to
 * @param len length of the stream buffer in bytes
 */
void audio_callback(void *userdata, uint8_t *stream, int len)
{
  // waves are represented in computers by a series of numbers called samples
  static double phase = 0.0; // Φ -- is the position of a point in time on a waveform cycle
  int16_t *buffer = (int16_t *)stream;
  int length = len / 2; // 2 bytes per sample

  // fₛ is the sample rate
  // f is number of cycles/s
  // time taken for one sample = Δt = 1 / fₛ
  // ΔΦ = f * Δt = f / fₛ

  double delta_phase = (double)FREQUENCY / SAMPLE_RATE;

  for (int i = 0; i < length; ++i)
  {
    // square wave
    buffer[i] = (sin(phase * 2.0 * M_PI) > 0 ? 1 : -1) * AMPLITUDE; // makes a square wave -- only alters between 2 values, 1 and -1
    phase += delta_phase;
    if (phase >= 1.0)
    {
      phase -= 1.0; // one cycle over, reset to beginning of next cycle
    }
  }
}

/* ---------------------------------- main ---------------------------------- */

int main(int argc, char **argv)
{
  const char *program = argv[0];
  char *rom_path = NULL;

  parse_arguments(argc, argv, &rom_path);

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
      .audio_device = 0,
      .bg_color = g_config.bg_color,
      .fg_color = g_config.fg_color,
  };

  if (initialise_sdl(&emulator) != 0)
  {
    cleanup_sdl(&emulator, EXIT_FAILURE);
  }

  bool running = true;
  uint32_t last_cycle_time = 0;
  uint32_t last_timer_time = 0;

  /*
    to ensure the timers decrement at a stable 60Hz rate, we
    cannot simply decrement them on every loop iteration

    instead a time-based approach is required. we measure the
    real time that has passed since the last timer update. if
    that elapsed time exceeds the target period for 60Hz (1000ms / 60 ≈ 16.667ms),
    we decrement the timers and record the current time as the
    new starting point for the next interval.

    this seperates the timer speed from the cpu's processing speed,
    ensuring consistent behavior on all machines.
  */

  while (running)
  {
    handle_input(&chip8, &running);

    uint32_t current_time = SDL_GetTicks();
    if (current_time - last_cycle_time > g_config.cycle_delay)
    {
      last_cycle_time = current_time;
      chip8_cycle(&chip8);
      draw_display(&chip8, &emulator);
    }

    if (current_time - last_timer_time > (1000 / 60))
    {
      last_timer_time = current_time;
      if (chip8.delay_timer > 0)
      {
        chip8.delay_timer--;
      }
      if (chip8.sound_timer > 0)
      {
        SDL_PauseAudioDevice(emulator.audio_device, 0); // play sound
        chip8.sound_timer--;
      }
      else
      {
        SDL_PauseAudioDevice(emulator.audio_device, 1); // pause sound
      }
    }
  }

  // cleanup
  cleanup_sdl(&emulator, EXIT_SUCCESS);

  return 0;
}