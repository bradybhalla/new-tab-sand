#include "config.h"
#include "grid.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <emscripten.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct program {
  SDL_Window *window;
  SDL_Renderer *renderer;

  pos_t window_size;
  uint64_t last_grid_update_time;

  grid_t *grid;

  // red = color & 0xff0000 << 16
  // green = color & 0x00ff00 << 8
  // blue = color & 0x0000ff
  uint32_t sand_color;
  uint32_t empty_color;

} program_t;

/* Check if the window size has changed */
bool program_window_size_changed(program_t *program) {
  pos_t new_size;
  SDL_GetWindowSize(program->window, &new_size.x, &new_size.y);

  return program->window_size.x != new_size.x ||
         program->window_size.y != new_size.y;
}

int max(int x, int y) { return x >= y ? x : y; }

/* Resize the program to match the current window size */
void program_resize(program_t *program) {
  SDL_GetWindowSize(program->window, &program->window_size.x,
                    &program->window_size.y);

  if (program->grid != NULL) {
    int cell_size_x = program->window_size.x / GRID_COLS + 1;
    int cell_size_y = program->window_size.y / GRID_ROWS + 1;
    int cell_size = max(max(cell_size_x, cell_size_y), MIN_CELL_SIZE);

    pos_t new_grid_size = {program->window_size.x / cell_size,
                           program->window_size.y / cell_size};

    grid_resize(program->grid, new_grid_size);
  }
}

/* Allocate the program data */
program_t *program_init() {
  program_t *program = malloc(sizeof(program_t));
  assert(program != NULL);

  SDL_CreateWindowAndRenderer(500, 500, SDL_WINDOW_RESIZABLE, &program->window,
                              &program->renderer);

  program->last_grid_update_time = 0;

  program->grid = NULL;

  program_resize(program);

  getenv("asdf");
  program->sand_color = 0xaaaaaa;
  program->empty_color = 0x000000;

  return program;
}

/* Free the program data */
void program_destroy(program_t *program) {
  SDL_DestroyWindow(program->window);
  SDL_DestroyRenderer(program->renderer);
  if (program->grid != NULL) {
    grid_destroy(program->grid);
  }
  free(program);
}

/* Initialize the grid. This is not done immediately to
 * save time and space
 */
void program_initialize_grid(program_t *program, uint64_t time) {
  program->grid = grid_init(GRID_ROWS, GRID_COLS);
  program->last_grid_update_time = time;
  program_resize(program);
}

/* Main loop of the program */
void loop(void *data) {
  program_t *program = (program_t *)(data);
  uint64_t time = SDL_GetTicks64();

  // if it has been a long time since the last update,
  // skip to the current time. This prevents a delay
  // after a tab is out of focus.
  if (time - program->last_grid_update_time > 500) {
    program->last_grid_update_time = time;
  }

  // handle events
  SDL_Event e;
  while (SDL_PollEvent(&e) == 1) {
    switch (e.type) {
    case SDL_QUIT: {
      // quit event shouldn't happen in the browser
      return;
    }
    case SDL_KEYDOWN: {
      // only run if modifiers are not pressed
      if ((e.key.keysym.mod & (KMOD_CTRL | KMOD_ALT | KMOD_GUI)) == 0) {

        // initialize the grid if needed
        if (program->grid == NULL) {
          program_initialize_grid(program, time);
        }

        // remove sand if backspace or escape are
        // pressed, otherwise add sand
        if (e.key.keysym.sym == SDLK_BACKSPACE ||
            e.key.keysym.sym == SDLK_ESCAPE) {
          grid_remove_sand(program->grid);
        } else {
          grid_add_sand(program->grid);
        }
      }
      break;
    }
    case SDL_WINDOWEVENT: {
      // resize the window if needed
      if (program_window_size_changed(program)) {
        program_resize(program);
      }
      break;
    }
    }
  }

  // clear screen
  SDL_SetRenderDrawColor(program->renderer,
                         program->empty_color & 0xff0000 >> 16,
                         program->empty_color & 0x00ff00 >> 8,
                         program->empty_color & 0x0000ff, 255);
  SDL_RenderClear(program->renderer);

  if (program->grid != NULL) {
    // update grid
    while (program->last_grid_update_time < time) {
      grid_tick(program->grid);
      program->last_grid_update_time += GRID_TICK_MS;
    }

    // draw grid
    grid_draw(program->grid, program->renderer, program->window_size,
              program->sand_color, program->empty_color);
  }

  SDL_RenderPresent(program->renderer);
}

int main(int argc, char **argv) {
  SDL_Init(SDL_INIT_VIDEO);

  program_t *program = program_init();

  // run the main loop with program data
  emscripten_set_main_loop_arg(loop, program, 0, true);

  program_destroy(program);

  return 0;
}
