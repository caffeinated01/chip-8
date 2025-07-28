#include <stdio.h>
#include <string.h>
#include "cpu.h"
#include "logger.h"
#include "config.h"

static void print_usage(FILE *out, const char *program);

static void print_usage(FILE *out, const char *program)
{
  fprintf(out, "Usage: %s [-v] -r <rom_path>\n", program);
}

int main(int argc, char **argv)
{
  const char *program = argv[0];
  char *rom_path = NULL;

  // parse arguments
  for (int i = 1; i < argc; ++i)
  {
    if (strcmp(argv[i], "-v") == 0)
    {
      g_config.verbose_logging = true;
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
        return 1;
      }
      continue;
    }

    fprintf(stderr, "Unknown argument: %s\n", argv[i]);
    print_usage(stderr, program);
    return 1;
  }

  if (rom_path == NULL)
  {
    fprintf(stderr, "ROM path not provided\n");
    print_usage(stderr, program);
    return 1;
  }

  LOG_INFO("Verbose logging enabled");
  LOG_INFO("ROM path: %s", rom_path);

  // initialise chip8 system
  chip8_t chip8;
  chip8_initialise(&chip8);

  // load rom
  if (chip8_load_rom(&chip8, rom_path) != 0)
  {
    return 1;
  }

  // main emulator loop
  while (1)
  {
    chip8_cycle(&chip8);

    // todo: draw graphics
    // todo: handle user input
    // todo: update timers
  }

  return 0;
}