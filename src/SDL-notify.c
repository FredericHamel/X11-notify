#include <stdio.h>
#include <stdlib.h>

#include <math.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define INIT_STATUS() \
  int __STATUS__ = 0
#define EXIT_STATUS(status, label) \
  do { \
    __STATUS__ = status; \
    goto label; \
  } while (1)

#define RETURN_STATUS() \
  return __STATUS__

static int POPUP_WIDTH = 160;
static int POPUP_HEIGHT = 80;


static int __interrupt = 0;

void sigusr1_handler(int signum) {
  __interrupt = 1;
}

float get_battery_level() {
  static int energy_full = 0;
  int energy_now;
  FILE *fd;
  if (!energy_full) {
    if(!(fd = fopen("/sys/class/power_supply/BAT0/energy_full", "r"))) {
      // Impossible value
      return -1.0f;
    }
    fscanf(fd, "%d", &energy_full);
    fclose(fd);
  }

  if(!(fd = fopen("/sys/class/power_supply/BAT0/energy_now", "r"))) {
    // Impossible value
    return -1.0f;
  }
  fscanf(fd, "%d", &energy_now);
  fclose(fd);

  return 100.0f * ((float)energy_now) / ((float)energy_full);
}

void loop(SDL_Window *win, SDL_Renderer *renderer, TTF_Font *font) {
  char msg[128];
  float oldv, newv;
  SDL_Surface *text_surface;
  SDL_Texture *texture = NULL;
  SDL_Event event;

  int win_w, win_h;
  int text_w, text_h;
  SDL_GetWindowSize(win, &win_w, &win_h);

  SDL_Rect pos;
  SDL_Color color = { .a = 255, .r = 255, .g = 0, .b = 0 };

  struct timespec start, end, wait_time = {.tv_sec = 0, .tv_nsec = 0};

  oldv = newv = get_battery_level();
  sprintf(msg, "Charge: %.1f%%", newv);

  text_surface = TTF_RenderText_Solid(font, msg, color);
  texture = SDL_CreateTextureFromSurface(renderer, text_surface);
  text_w = text_surface->w;
  text_h = text_surface->h;
  pos.x = (win_w - text_w) / 2;
  pos.y = (win_h - text_h) / 2;
  pos.w = text_w;
  pos.h = text_h;
  SDL_FreeSurface(text_surface);

  const int FPS = 60;
  const int MSEC_PER_FRAME = 1000 / FPS;
  const float UPPER_BATTERY_LEVEL = 85.1f;
  const float LOWER_BATTERY_LEVEL = 14.9f;

  int frame = 0;
  while(!__interrupt) {
    if (newv > LOWER_BATTERY_LEVEL && newv < UPPER_BATTERY_LEVEL) {
      start.tv_sec = 2;
      start.tv_nsec = 0;
      newv = get_battery_level();
      nanosleep(&start, NULL);
      continue;
    }


    SDL_ShowWindow(win);
    while(!__interrupt) {
      if (newv > LOWER_BATTERY_LEVEL && newv < UPPER_BATTERY_LEVEL) {
        SDL_HideWindow(win);
        break;
      }
      clock_gettime(CLOCK_MONOTONIC, &start);
      while(SDL_PollEvent(&event)) {
        switch(event.type) {
          case SDL_QUIT:
            __interrupt = 1;
            break;
          case SDL_MOUSEBUTTONDOWN:
            SDL_HideWindow(win);
            sleep(2);
            SDL_ShowWindow(win);
            break;
        }
      }

      if (frame >= 120) {
        newv = get_battery_level();
        if (fabsf(newv - oldv) > 0.05f) {
          oldv = newv;
          sprintf(msg, "Charge: %.1f%%", newv);
          text_surface = TTF_RenderText_Solid(font, msg, color);
          if (texture) {
            SDL_DestroyTexture(texture);
          }
          texture = SDL_CreateTextureFromSurface(renderer, text_surface);
          text_w = text_surface->w;
          text_h = text_surface->h;
          pos.x = (win_w - text_w) / 2;
          pos.y = (win_h - text_h) / 2;
          pos.w = text_w;
          pos.h = text_h;
          SDL_FreeSurface(text_surface);
        }
        frame = 0;
      }

      SDL_RenderClear(renderer);
      SDL_RenderCopy(renderer, texture, NULL, &pos);
      SDL_RenderPresent(renderer);
      clock_gettime(CLOCK_MONOTONIC, &end);

      int msec = (end.tv_nsec - start.tv_nsec) / 1000000;
      if (msec < MSEC_PER_FRAME) {
        wait_time.tv_nsec = (MSEC_PER_FRAME - msec) * 1000000;
        nanosleep(&wait_time, NULL);
      }
      frame++;
    }
  }
  if (texture) {
    SDL_DestroyTexture(texture);
  }
}

int main(int argc, char **argv) {
  int retcode = 0;
  SDL_DisplayMode dm;
  SDL_Window *win = NULL;
  SDL_Renderer *renderer = NULL;
  TTF_Font *font = NULL;

  signal(SIGUSR1, &sigusr1_handler);

  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
    fprintf(stderr, "[SDL] %s\n", SDL_GetError());
    return 1;
  }

  if(TTF_Init() < 0) {
    fprintf(stderr, "[TTF] %s\n", TTF_GetError());
    goto sdl_quit;
  }

  font = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans.ttf", 16);
  if(!font) {
    fprintf(stderr, "[TTF] %s\n", TTF_GetError());
    retcode = 1;
    goto ttf_quit;
  }

  SDL_GetCurrentDisplayMode(0, &dm);
  win = SDL_CreateWindow(NULL,
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      POPUP_WIDTH, POPUP_HEIGHT, SDL_WINDOW_POPUP_MENU | SDL_WINDOW_HIDDEN);

  if (!win) {
    fprintf(stderr, "[SDL] %s\n", SDL_GetError());
    retcode = 1;
    goto ttf_unload_font;
  }

  renderer = SDL_CreateRenderer(win, -1, 0);
  if (!renderer) {
    fprintf(stderr, "[SDL] %s\n", SDL_GetError());
    retcode = 1;
    goto sdl_destroy_win;
  }

  SDL_SetRenderDrawColor(renderer, 0, 0, 127, 255);

  loop(win, renderer, font);
  SDL_DestroyRenderer(renderer);
sdl_destroy_win:
  SDL_DestroyWindow(win);
ttf_unload_font:
  TTF_CloseFont(font);
ttf_quit:
  TTF_Quit();
sdl_quit:
  SDL_Quit();
  return retcode;
}
