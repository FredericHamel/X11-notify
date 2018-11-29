#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#define INIT_STATUS() \
  int __STATUS__ = 0
#define EXIT_STATUS(status, label) \
  do { \
    __STATUS__ = status; \
    goto label; \
  } while (1)

#define RETURN_STATUS() \
  return __STATUS__

typedef struct {
  Display *dpy;
  Window win;
  GC gc;
} Client;

float get_battery_level();

void notify(Client* cl, float percent) {
  char msg[128];
  size_t len;

  struct timespec start, end;

  clock_gettime(CLOCK_MONOTONIC, &start);

  len = sprintf(msg, "Charge: %0.1f%%", percent);
  XEvent ev;
  XMapWindow(cl->dpy, cl->win);
  for(;;) {

    XNextEvent(cl->dpy, &ev);
    switch(ev.type) {
      case Expose:
        XDrawString(cl->dpy, cl->win, cl->gc, 10, 50, msg, len);
        break;
      case ButtonPress:
        goto close;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
      printf("Test\n");
    if (end.tv_sec - start.tv_sec > 2) { // 2 sec
      percent = get_battery_level();
      len = sprintf(msg, "Charge: %0.1f%%", percent);
      start.tv_sec = end.tv_sec;
    }
  }
close:
  XUnmapWindow(cl->dpy, cl->win);
}

int create_client(Client *cl) {
  INIT_STATUS();

  Display *dpy = XOpenDisplay(NULL);
  int scr;

  Window win;
  XSetWindowAttributes wa;
  GC gc;
  if (!dpy) {
    EXIT_STATUS(1, end);
  }
  scr = DefaultScreen(dpy);
  gc =  DefaultGC(dpy, scr);

  win = XCreateSimpleWindow(dpy, RootWindow(dpy, scr), 10,10, 100, 100, 1,
      BlackPixel(dpy, scr), WhitePixel(dpy, scr));

  wa.override_redirect = 1;
  XChangeWindowAttributes(dpy, win, CWOverrideRedirect, &wa);
  XSelectInput(dpy, win, ExposureMask | ButtonPressMask);

  cl->dpy = dpy;
  cl->win = win;
  cl->gc = gc;
end:
  RETURN_STATUS();
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

int main(int argc, char **argv) {
  const float LOW_BATTERY = 15;
  const float HIGH_BATTERY = 85;
  Client cl;
  float battery_level;
  for(;;) {
    battery_level = get_battery_level();
    if (1 || battery_level < LOW_BATTERY || battery_level > HIGH_BATTERY) {
      create_client(&cl);
      notify(&cl, battery_level);
      XDestroyWindow(cl.dpy, cl.win);
    }
    XCloseDisplay(cl.dpy);
    sleep(2);
  }
  return 0;
}
