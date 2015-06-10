#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <error.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include "config-parser.h"

#define DEFAULT_CONFIG ".hotcorners.conf"

static useconds_t delay = 500000;

static Display *display;
static Window window;

typedef struct {
  int x, y;
} coord_t;

typedef struct {
  char *name;
  int x, y;
  char *action;
} corner_t;

static corner_t corners[] = {
  { .name = "top_left", .action = NULL },
  { .name = "top_right", .action = NULL },
  { .name = "bottom_left", .action = NULL },
  { .name = "bottom_right", .action = NULL },
};

static pthread_mutex_t mutex;
static int screen_width, screen_height;
static coord_t mouse_pos = {0, 0};
static bool action_taken = false;

void *corner_wait(void *arg) {
  pthread_mutex_lock(&mutex);
  if (action_taken)
    goto exit;
  action_taken = true;
  const corner_t *corner = (corner_t *)arg;
  usleep(delay);
  if (action_taken && mouse_pos.x == corner->x &&
                      mouse_pos.y == corner->y) {
    if (fork() == 0) {
      char *shell = getenv("SHELL");
      if (shell == NULL) shell = "bash";
      execlp(shell, shell, "-c", corner->action, NULL);
    }
    mouse_pos.x = mouse_pos.y = -1;
  }
exit:
  action_taken = false;
  pthread_mutex_unlock(&mutex);
  return NULL;
}

void event_loop(void) {
  int ax = -1, ay = -1;
  for (XEvent event; ~XNextEvent(display, &event);) {
    if (event.type == MotionNotify) {
      int x = event.xmotion.x_root, y = event.xmotion.y_root;
      if (!action_taken && (x != ax || y != ay)) {
        mouse_pos.x = ax = x, mouse_pos.y = ay = y;
        for (size_t i = 0; i < sizeof corners / sizeof (corner_t); i++)
          if (corners[i].action && x == corners[i].x && y == corners[i].y) {
            pthread_t thread_id;
            pthread_create(&thread_id, NULL, corner_wait, corners + i);
            pthread_detach(thread_id);
          }
      } else if (ax != x || ay != y)
        ax = x, ay = y, action_taken = false;
    }
  }
}

static inline void set_corner(const char *name, int x, int y, char *action) {
  for (int i = 0; i < 4; i++)
    if (!strcmp(name, corners[i].name)) {
      if (x != -1) corners[i].x = x;
      if (y != -1) corners[i].y = y;
      if (action) corners[i].action = strdup(action);
    }
}

void setup(void) {
  if (!(display = XOpenDisplay(NULL)))
    error(EXIT_FAILURE, 0, "unable to open X display");
  window = DefaultRootWindow(display);
  XAllowEvents(display, AsyncPointer, CurrentTime);
  screen_width = DisplayWidth(display, DefaultScreen(display));
  screen_height = DisplayHeight(display, DefaultScreen(display));
  XSelectInput(display, window, PointerMotionMask);
  set_corner("top_left",      0,                 0,                  NULL);
  set_corner("top_right",     screen_width - 1,  0,                  NULL);
  set_corner("bottom_left",   0,                 screen_height - 1,  NULL);
  set_corner("bottom_right",  screen_width - 1,  screen_height - 1,  NULL);
}

void show_help(const char *program) {
static const char *help_line =
"Usage: %s [OPTION]...\n"
"Run shell commands by hovering the mouse pointer over screen corners.\n\n"
"    -C, --config=PATH            load commands from PATH.\n"
"                                   (default PATH is $HOME/"DEFAULT_CONFIG"\n"
"    -d, --delay=NUMBER           wait NUMBER seconds before executing COMMAND\n"
"                                   (default NUMBER is 0.5)\n\n"
"  Corner options:\n"
"    -l, --top-left=COMMAND       top-left corner action\n"
"    -r, --top-right=COMMAND      top-right corner action\n"
"    -L, --bottom-left=COMMAND    bottom-left corner action\n"
"    -R, --bottom-left=COMMAND    bottom-right corner action\n\n"
"    -h, --help                   display this help and exit\n\n"
"  Config file format:\n"
"    The config file consists of key/value pairs separated by \":\".\n"
"    Each entry should be on a single line\n"
;
  printf(help_line, program);
}

int set_delay(const char *_str) {
  char *str;
  double sec;
  if (sscanf(_str, "%m[0-9.]", &str) != 1 || strlen(str) != strlen(_str))
    goto fail;
  sec = strtod(str, NULL) * 1000000;
  if (errno == ERANGE)
    goto fail;
  if (sec > ((unsigned long) 1 << 32) - 1)
    sec = ((unsigned long) 1 << 32) - 1;
  delay = sec;
  free(str);
  return 0;
fail:
  if (str) free(str);
  return -1;
}

void parse_config_row(char **row) {
  set_corner(*row, -1, -1, row[1]);
}

void parse_args(int argc, char **argv) {
  static struct option long_options[] = {
    {"config-file",   required_argument,  NULL,  'C'},
    {"delay",         required_argument,  NULL,  'd'},
    {"top-left",      required_argument,  NULL,  'l'},
    {"top-right",     required_argument,  NULL,  'r'},
    {"bottom-left",   required_argument,  NULL,  'L'},
    {"bottom-right",  required_argument,  NULL,  'R'},
    {"help",          no_argument,        NULL,  'h'},
    {0,               0,                  0,     0}
  };
  int config_flag = 0, oi = 0, c;
  while (~(c = getopt_long(argc, argv, "C:d:l:r:L:R:h", long_options, &oi))) {
    switch (c) {
    case 'C':
      config_flag = 1;
      if (parse_config_file(optarg, parse_config_row) == -1)
        error(EXIT_FAILURE, errno, "cannot access %s", optarg);
      break;
    case 'd':
      if (set_delay(optarg) < 0)
        error(EXIT_FAILURE, 0, "invalid number: %s\n", optarg);
      break;
    case 'l': set_corner("top_left",     -1, -1, optarg); break;
    case 'r': set_corner("top_right",    -1, -1, optarg); break;
    case 'L': set_corner("bottom_left",  -1, -1, optarg); break;
    case 'R': set_corner("bottom_right", -1, -1, optarg); break;
    case 'h':
      show_help(*argv);
      exit(EXIT_SUCCESS);
      break;
    case '?':
      fprintf(stderr, "Try '%s --help' for more information.\n", *argv);
      exit(EXIT_FAILURE);
      break;
    }
  }
  if (!config_flag) {
    char *home = getenv("HOME");
    if (home) {
      char def_config[strlen(home) + strlen(DEFAULT_CONFIG) + 1];
      sprintf(def_config, "%s/"DEFAULT_CONFIG, home);
      parse_config_file(def_config, parse_config_row);
    }
  }
  if (optind < argc)
    error(EXIT_FAILURE, 0, "unexpected argument: \"%s\"", argv[optind]);
}

int main(int argc, char **argv) {
  parse_args(argc, argv);
  setup();
  /* printf("%s: \"%s\"\n", "top_left",      corners[0].action); */
  /* printf("%s: \"%s\"\n", "top_right",     corners[1].action); */
  /* printf("%s: \"%s\"\n", "bottom_left",   corners[2].action); */
  /* printf("%s: \"%s\"\n", "bottom_right",  corners[3].action); */
  event_loop();
  XCloseDisplay(display);
  return 0;
}
