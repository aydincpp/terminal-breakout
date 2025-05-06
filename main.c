#define _XOPEN_SOURCE_EXTENDED 1
#define _XOPEN_SOURCE 700

#include <locale.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

// Define desired game window dimension
#define GAME_WIDTH ((COLS % 2 == 0) ? COLS : COLS - 1)
#define GAME_HEIGHT LINES

#define PADDLE_CHAR L"🟪"
#define MIN_PADDLE_SIZE 10
#define MAX_PADDLE_SIZE 30

#define BALL_CHAR L"⚽"
#define COLS_NOBORDER (COLS - 2)

#define BRICK_STRONG L"🟥"
#define BRICK_MEDIUM L"🟧"
#define BRICK_WEAK L"🟨"
#define BRICK_COUTN 5
#define BRICK_ROWS 8
#define BRICK_H_GAP 1
#define BRICK_V_GAP 2

#define DROP_HEALT_CHAR L"❤️"
#define DROP_BULLET_CHAR L"🔫"
#define DROP_EXTRA_BALL_CHAR L"🎁"
#define DROP_BOMB_CHAR L"💣"

typedef struct {
  int x;
  int y;
} Vec2;

typedef struct {
  int x;
  int y;
  int w;
  int h;
} Rect;

typedef struct {
  Rect rect;
  Vec2 dir;
  wchar_t* ch;
  int char_width;
} Paddle;

typedef struct {
  Rect rect;
  Vec2 dir;
  wchar_t* ch;
  int char_width;
  int is_launched;
} Ball;

typedef struct {
  Ball** items;
  int count;
} BallArray;

typedef enum {
  DROP_NONE,
  DROP_HEALTH,
  // DROP_BULLET,
  DROP_EXTRA_BALL,
  DROP_BOMB
} DropType;

typedef struct {
  Rect rect;
  DropType type;
  wchar_t* ch;
  int char_width;
  int spawned;
  int life;
  int none;
} Drop;

typedef struct {
  Rect rect;
  Drop drop;
  wchar_t* ch;
  int char_width;
  int health;
} Brick;

// Represents the window's position, size, and optional padding
typedef struct {
  Vec2 padding;
  Rect rect;
  Rect inner_rect;
} WindowConfig;

// Initializes ncurses mode and sets up color, input behavior, and cursor
// visibility
void init_ncurses();

// Ends ncurses mode and cleans up any ncurses-specific resources
void kill_ncurses();

void cleanup(BallArray* balls);

// Checks if terminal size can fit the game window dimensions
void check_terminal_size();

// Sets the background color and refreshes the screen to apply the changes
void setup_background_color();

// Initialize the game window configs
void init_game_win_Conf(WindowConfig* win_conf);

// Draws the window frame (border) and applies background color
void draw_window(WINDOW*);

void draw_start_menu(WINDOW* win);

// Initilize paddle
void init_paddle(Paddle* paddle, WindowConfig* win_conf);

// Draw the paddle
void draw_paddle(WINDOW* win, const Paddle* paddle);

// Keep the paddle within bounds
void clamp_paddle_bounds(WindowConfig* win_conf, Paddle* paddle);

// Initilize ball
void init_ball(Ball* ball, Paddle* paddle);

// Draw the ball
void draw_balls(WINDOW* win, WindowConfig* win_conf, BallArray* balls,
                Paddle* paddle);

void keep_balls_within_bounds(WindowConfig* win_conf, BallArray* balls);

int get_random_direction();

void bounce_ball(Ball* balls, Rect* rect);

void init_bricks(WindowConfig* win_conf, Brick* bricks, int count);

void draw_bricks(WINDOW* win, Brick* bricks, int count);

void draw_drop(WINDOW* win, WindowConfig* win_conf, Brick* bricks,
               Paddle* paddle, int count, BallArray* balls);

void resolve_drop_paddle_collision(Drop* drop, Paddle* paddle,
                                   BallArray* balls);

void resolve_balls_brick_collision(Brick* bricks, int count, BallArray* balls);

// check if a point is coll
int is_colliding(const Rect* a, const Rect* b);

// Validates a pointer and prints an error message with the pointer's name if
// it's NULL
void validate_ptr(void* ptr, const char* name);

// Macro to validate a pointer and print its name if NULL.
#define VALIDATE(ptr) validate_ptr(ptr, #ptr)

// Returns the inner width of the window, excluding border and padding
int get_inner_window_width(WindowConfig* win_conf);

// Returns the inner height of the window, excluding border and padding
int get_inner_window_height(WindowConfig* win_conf);

// Returns the offset needed to center inner_len within outer_len
int get_center_offset(int outer_len, int inner_len);

int main() {
  // setenv("TERMINFO", "./vendor/ncurses/build/share/terminfo", 1);
  setlocale(LC_ALL, "");
  srand(time(NULL));

  init_ncurses();
  check_terminal_size();
  setup_background_color();

  // Create and configure the main game window settings
  WindowConfig game_win_conf;
  init_game_win_Conf(&game_win_conf);

  // Create a new window for the game with the specified height, width, pos
  WINDOW* game_win = newwin(game_win_conf.rect.h, game_win_conf.rect.w,
                            game_win_conf.rect.y, game_win_conf.rect.x);
  VALIDATE(game_win);

  // ── Draw and handle start menu ──
  draw_start_menu(game_win);
  int menu_choice = 0;

  while (1) {
    int ch = wgetch(game_win);
    if (ch == '1') {
      menu_choice = 1;  // Start game
      break;
    } else if (ch == '2' || ch == 'q') {
      kill_ncurses();
      return 0;
    }
  }

  // ── Start Game ──
  Paddle paddle;
  init_paddle(&paddle, &game_win_conf);

  BallArray balls;
  balls.count = 1;
  balls.items = malloc(sizeof(Ball*) * balls.count);
  for (int i = 0; i < balls.count; i++) {
    balls.items[i] = malloc(sizeof(Ball));
  }

  init_ball(balls.items[balls.count - 1], &paddle);

  Brick bricks[BRICK_COUTN * BRICK_ROWS] = {0};
  init_bricks(&game_win_conf, bricks, BRICK_COUTN);

  draw_bricks(game_win, bricks, BRICK_COUTN);

  // Draw the window frame and apply the background color
  draw_window(game_win);

  // draw the paddle at the start
  draw_paddle(game_win, &paddle);

  // Draw the ball at the start
  draw_balls(game_win, &game_win_conf, &balls, &paddle);

  int ch = 0;
  nodelay(game_win, 1);
  keypad(game_win, 1);

  while (ch != 'q') {
    ch = getch();  // Get input (non-blocking)

    switch (ch) {
      case KEY_LEFT:
        paddle.dir.x = -1;
        break;
      case KEY_RIGHT:
        paddle.dir.x = 1;
        break;
      case KEY_UP:

        for (int i = 0; i < balls.count; i++) {
          if (balls.items[i]->is_launched == 0) {
            balls.items[i]->is_launched = 1;
            balls.items[i]->dir.y = -1;
            balls.items[i]->dir.x = get_random_direction();
          }
        }
      default:
        break;
    }

    paddle.rect.x += paddle.dir.x;
    clamp_paddle_bounds(&game_win_conf, &paddle);
    resolve_balls_brick_collision(bricks, BRICK_COUTN, &balls);
    keep_balls_within_bounds(&game_win_conf, &balls);

    for (int i = 0; i < balls.count; i++) {
      balls.items[i]->rect.x += balls.items[i]->dir.x;
      balls.items[i]->rect.y += balls.items[i]->dir.y;

      if (is_colliding(&balls.items[i]->rect, &paddle.rect)) {
        bounce_ball(balls.items[i], &paddle.rect);
      }
    }

    // Clear
    wclear(game_win);
    draw_window(game_win);
    draw_paddle(game_win, &paddle);
    draw_balls(game_win, &game_win_conf, &balls, &paddle);
    draw_bricks(game_win, bricks, BRICK_COUTN);
    draw_drop(game_win, &game_win_conf, bricks, &paddle, BRICK_COUTN, &balls);

    // Refresh the window to update screen
    wrefresh(game_win);

    napms(1000 / 24);
  }

  cleanup(&balls);
  kill_ncurses();

  return EXIT_SUCCESS;
}

void init_ncurses() {
  initscr();
  start_color();
  if (!has_colors()) {
    kill_ncurses();
    fprintf(stderr, "Your terminal does not support color\n");
    exit(1);
  }

  cbreak();
  noecho();
  curs_set(0);
  nodelay(stdscr, 1);
  keypad(stdscr, 1);
  init_pair(1, COLOR_WHITE, COLOR_BLACK);
}

void kill_ncurses() {
  endwin();  // End ncurses mode
}

void cleanup(BallArray* balls) {
  for (int i = 0; i < balls->count; i++) {
    free(balls->items[i]);
    balls->items[i] = NULL;
  }
}

void check_terminal_size() {
  // Get the current terminal dimensions (height and width)
  int term_height, term_width;
  getmaxyx(stdscr, term_height, term_width);

  // Check if the terminal's width and height are smaller than the required size
  // for the game window
  if (term_width < GAME_WIDTH || term_height < GAME_HEIGHT) {
    // End ncurses mode before printing the error message
    endwin();

    // Print an error message to inform the user that the terminal size is too
    // small
    printf("Error: Terminal size is too small. The required size is %dx%d.\n",
           GAME_WIDTH, GAME_HEIGHT);

    // Exit the program with failure status (EXIT_FAILURE)
    exit(EXIT_FAILURE);
  }
}

void setup_background_color() {
  // Set the background color using color pair 1 (as defined with init_pair())
  bkgd(COLOR_PAIR(3));

  // Refresh the screen to apply the background color change
  refresh();
}

void init_game_win_Conf(WindowConfig* win_conf) {
  win_conf->padding.x = 0;
  win_conf->padding.y = 0;

  win_conf->rect.w = GAME_WIDTH;
  win_conf->rect.h = GAME_HEIGHT;

  win_conf->inner_rect.w = get_inner_window_width(win_conf);
  win_conf->inner_rect.h = get_inner_window_height(win_conf);

  win_conf->rect.x = get_center_offset(COLS, win_conf->rect.w);
  win_conf->rect.y = get_center_offset(LINES, win_conf->rect.h);

  win_conf->inner_rect.x = win_conf->padding.x + 1;
  win_conf->inner_rect.y = win_conf->padding.y + 1;
}

void draw_window(WINDOW* win) {
  wbkgd(win, COLOR_PAIR(1));
  // box(win, 0, 0);
  wrefresh(win);
}

void draw_start_menu(WINDOW* win) {
  const wchar_t* art[] = {
      L"▀█████████▄     ▄████████  ▄█   ▄████████    ▄█   ▄█▄  ▄██████▄  ███   "
      L" █▄      ███     ",
      L"  ███    ███   ███    ███ ███  ███    ███   ███ ▄███▀ ███    ███ ███   "
      L" ███ ▀█████████▄ ",
      L"  ███    ███   ███    ███ ███▌ ███    █▀    ███▐██▀   ███    ███ ███   "
      L" ███    ▀███▀▀██ ",
      L" ▄███▄▄▄██▀   ▄███▄▄▄▄██▀ ███▌ ███         ▄█████▀    ███    ███ ███   "
      L" ███     ███   ▀ ",
      L"▀▀███▀▀▀██▄  ▀▀███▀▀▀▀▀   ███▌ ███        ▀▀█████▄    ███    ███ ███   "
      L" ███     ███     ",
      L"  ███    ██▄ ▀███████████ ███  ███    █▄    ███▐██▄   ███    ███ ███   "
      L" ███     ███     ",
      L"  ███    ███   ███    ███ ███  ███    ███   ███ ▀███▄ ███    ███ ███   "
      L" ███     ███     ",
      L"▄█████████▀    ███    ███ █▀   ████████▀    ███   ▀█▀  ▀██████▀  "
      L"████████▀     ▄████▀   ",
      L"               ███    ███                   ▀                          "
      L"                  "};

  const int art_lines = sizeof(art) / sizeof(art[0]);

  const char* option1 = "1. Start Game";
  const char* option2 = "2. Quit      ";

  werase(win);

  int start_y = (GAME_HEIGHT - art_lines - 4) / 2;
  for (int i = 0; i < art_lines; ++i) {
    int art_width = wcswidth(art[i], wcslen(art[i]));
    int art_x = (GAME_WIDTH - art_width) / 2;
    mvwprintw(win, start_y + i, art_x, "%ls", art[i]);
  }

  int menu_y = start_y + art_lines;
  int opt1_x = (GAME_WIDTH - strlen(option1)) / 2;
  int opt2_x = (GAME_WIDTH - strlen(option2)) / 2;

  mvwprintw(win, menu_y + 1, opt1_x, "%s", option1);
  mvwprintw(win, menu_y + 2, opt2_x, "%s", option2);

  wrefresh(win);
}

void init_paddle(Paddle* paddle, WindowConfig* win_conf) {
  paddle->char_width = wcwidth(PADDLE_CHAR[0]);
  paddle->ch = PADDLE_CHAR;

  // paddle->rect.w = MAX_PADDLE_SIZE * paddle->char_width;
  paddle->rect.w =
      ((MAX_PADDLE_SIZE + MIN_PADDLE_SIZE) / 2) * paddle->char_width;
  paddle->rect.h = 1;

  paddle->rect.x = get_center_offset(win_conf->inner_rect.w, paddle->rect.w);
  paddle->rect.y = win_conf->inner_rect.h;

  paddle->dir.x = 0;
  paddle->dir.y = 0;
}

void draw_paddle(WINDOW* win, const Paddle* paddle) {
  cchar_t ch;
  setcchar(&ch, paddle->ch, A_NORMAL, 0, NULL);

  for (int i = 0; i < paddle->rect.w; i += paddle->char_width) {
    mvwadd_wch(win, paddle->rect.y, paddle->rect.x + i, &ch);
  }
  wrefresh(win);
}

void clamp_paddle_bounds(WindowConfig* win_conf, Paddle* paddle) {
  if (paddle->rect.x < win_conf->inner_rect.x) {
    paddle->rect.x = win_conf->inner_rect.x;
  }

  if (paddle->rect.x + paddle->rect.w > win_conf->inner_rect.w) {
    paddle->rect.x = win_conf->rect.w - paddle->rect.w - 1;
  }
}

void init_ball(Ball* ball, Paddle* paddle) {
  ball->ch = BALL_CHAR;

  ball->dir.x = 0;
  ball->dir.y = 0;

  ball->rect.x = paddle->rect.x + (paddle->rect.w / 3);
  ball->rect.y = paddle->rect.y - 1;
  ball->rect.w = wcwidth(BALL_CHAR[0]);
  ball->rect.h = 1;

  ball->is_launched = 0;
}

void draw_balls(WINDOW* win, WindowConfig* win_conf, BallArray* balls,
                Paddle* paddle) {
  for (int i = 0; i < balls->count; i++) {
    cchar_t ch;
    setcchar(&ch, balls->items[i]->ch, A_NORMAL, 0, NULL);

    if (balls->items[i]->is_launched == 0) {
      balls->items[i]->rect.x = paddle->rect.x + (paddle->rect.w / 2) - 1;
      balls->items[i]->rect.y = paddle->rect.y - 1;
    }

    if (balls->items[i]->rect.y >= win_conf->inner_rect.h) {
      free(balls->items[i]);
      for (int j = i; j < balls->count - 1; j++) {
        balls->items[j] = balls->items[j + 1];
      }
      balls->count--;
      i--;

      continue;
    }

    mvwadd_wch(win, balls->items[i]->rect.y, balls->items[i]->rect.x, &ch);

    wrefresh(win);
  }
}

void keep_balls_within_bounds(WindowConfig* win_conf, BallArray* balls) {
  for (int i = 0; i < balls->count; i++) {
    if (balls->items[i]->rect.x <= win_conf->inner_rect.x ||
        balls->items[i]->rect.x >= win_conf->inner_rect.w) {
      balls->items[i]->dir.x *= -1;
    }

    if (balls->items[i]->rect.y <= win_conf->inner_rect.y ||
        balls->items[i]->rect.y >= win_conf->inner_rect.h) {
      balls->items[i]->dir.y *= -1;
    }
  }
}

int get_random_direction() { return (rand() % 3) - 1; }

int get_random_drop() { return (rand() % 4); };

int get_random_health() { return (rand() % 3) + 1; }

void bounce_ball(Ball* ball, Rect* rect) {
  int ball_center = ball->rect.x + (ball->rect.w / 2);
  int zone_width = rect->w / 3;

  if (ball_center < rect->x + zone_width) {
    ball->dir.x = -1;
  } else if (ball_center < rect->x + (2 * zone_width)) {
    ball->dir.x = 0;
  } else {
    ball->dir.x = 1;
  }

  ball->dir.y *= -1;
}

void init_bricks(WindowConfig* win_conf, Brick* bricks, int count) {
  int brick_char_width = wcwidth(BRICK_STRONG[0]);
  int total_gap = (count - 1) * BRICK_H_GAP;
  int usable_width = win_conf->inner_rect.w - total_gap;
  int brick_width = (usable_width / count) / brick_char_width;

  for (int row = 0; row < BRICK_ROWS; row++) {
    for (int col = 0; col < count; col++) {
      int index = row * count + col;

      bricks[index].ch = BRICK_STRONG;
      bricks[index].char_width = wcwidth(BRICK_STRONG[0]);
      bricks[index].rect.w = brick_width * brick_char_width;
      bricks[index].rect.h = 1;
      bricks[index].rect.x =
          win_conf->inner_rect.x +
          (col * ((brick_width * brick_char_width) + BRICK_H_GAP));
      bricks[index].rect.y =
          win_conf->inner_rect.y + row + (BRICK_V_GAP * (row + 1));
      bricks[index].health = get_random_health();

      // initilizing drop
      bricks[index].drop.type = get_random_drop();
      bricks[index].drop.spawned = 0;
      bricks[index].drop.life = 1;
      bricks[index].drop.none = 0;
      switch (bricks[index].drop.type) {
        case DROP_HEALTH:
          bricks[index].drop.ch = DROP_HEALT_CHAR;
          bricks[index].drop.char_width = wcwidth(DROP_HEALT_CHAR[0]);
          break;

          // case DROP_BULLET:
          //   bricks[index].drop.ch = DROP_BULLET_CHAR;
          //   bricks[index].drop.char_width = wcwidth(DROP_BULLET_CHAR[0]);
          //   break;

        case DROP_EXTRA_BALL:
          bricks[index].drop.ch = DROP_EXTRA_BALL_CHAR;
          bricks[index].drop.char_width = wcwidth(DROP_EXTRA_BALL_CHAR[0]);
          break;

        case DROP_BOMB:
          bricks[index].drop.ch = DROP_BOMB_CHAR;
          bricks[index].drop.char_width = wcwidth(DROP_BOMB_CHAR[0]);
          break;

        case DROP_NONE:
          bricks[index].drop.none = 1;
          break;
      }

      if (!bricks[index].drop.none) {
        bricks[index].drop.rect.w = bricks[index].drop.char_width;
        bricks[index].drop.rect.h = 1;
        bricks[index].drop.rect.x =
            bricks[index].rect.x + (bricks[index].rect.w / 2);
        bricks[index].drop.rect.y = bricks[index].rect.y + bricks[index].rect.h;
      }
    }
  }
}

void resolve_balls_brick_collision(Brick* bricks, int count, BallArray* balls) {
  for (int i = 0; i < balls->count; i++) {
    for (int row = 0; row < BRICK_ROWS; row++) {
      for (int col = 0; col < count; col++) {
        int index = row * count + col;

        if (is_colliding(&bricks[index].rect, &balls->items[i]->rect)) {
          if (bricks[index].health != 0) {
            bounce_ball(balls->items[i], &bricks[index].rect);
            if (bricks[index].health > 0) {
              bricks[index].health--;
            } else {
              bricks[index].health = 0;
            }
          }

          if (bricks[index].health == 0 && bricks[index].drop.spawned == 0 &&
              bricks[index].drop.life == 1) {
            bricks[index].drop.spawned = 1;
          }
        }
      }
    }
  }
}

void draw_bricks(WINDOW* win, Brick* bricks, int count) {
  int total_count = count * BRICK_ROWS;
  for (int i = 0; i < total_count; i++) {
    const wchar_t* ch_str;
    switch (bricks[i].health) {
      case 3:
        ch_str = BRICK_STRONG;
        break;
      case 2:
        ch_str = BRICK_MEDIUM;
        break;
      case 1:
        ch_str = BRICK_WEAK;
        break;
      case 0:
        continue;
    }

    cchar_t ch;
    setcchar(&ch, ch_str, A_NORMAL, 0, NULL);

    for (int j = 0; j < bricks[i].rect.w; j += bricks[i].char_width) {
      mvwadd_wch(win, bricks[i].rect.y, bricks[i].rect.x + j, &ch);
    }
  }
  wrefresh(win);
}

void draw_drop(WINDOW* win, WindowConfig* win_conf, Brick* bricks,
               Paddle* paddle, int count, BallArray* balls) {
  int total_count = count * BRICK_ROWS;
  for (int i = 0; i < total_count; i++) {
    if (bricks[i].health == 0 && bricks[i].drop.spawned &&
        !bricks[i].drop.none && bricks[i].drop.life == 1) {
      cchar_t ch;
      setcchar(&ch, bricks[i].drop.ch, A_NORMAL, 0, NULL);

      for (int j = 0; j < bricks[i].drop.rect.w;
           j += bricks[i].drop.char_width) {
        mvwadd_wch(win, bricks[i].drop.rect.y, bricks[i].drop.rect.x + j, &ch);
      }
      bricks[i].drop.rect.y++;
      if (bricks[i].drop.rect.y >=
          win_conf->inner_rect.y + win_conf->inner_rect.h) {
        bricks[i].drop.life = 0;
      }
      resolve_drop_paddle_collision(&bricks[i].drop, paddle, balls);
    }
  }
  wrefresh(win);
}

void resolve_drop_paddle_collision(Drop* drop, Paddle* paddle,
                                   BallArray* balls) {
  if (drop->life == 1) {
    if (is_colliding(&drop->rect, &paddle->rect)) {
      drop->life = 0;
      switch (drop->type) {
        case DROP_HEALTH:
          if (paddle->rect.w < MAX_PADDLE_SIZE * paddle->char_width) {
            paddle->rect.w += 5;
          }
          break;

          // case DROP_BULLET:
          //   break;

        case DROP_EXTRA_BALL:
          balls->items =
              realloc(balls->items, sizeof(Ball*) * (balls->count + 1));
          VALIDATE(balls->items);

          balls->items[balls->count] = malloc(sizeof(Ball));
          VALIDATE(balls->items[balls->count]);
          init_ball(balls->items[balls->count], paddle);

          balls->count++;
          break;

        case DROP_BOMB:
          if (paddle->rect.w > MIN_PADDLE_SIZE * paddle->char_width) {
            paddle->rect.w -= 5;
          }
          break;

        default:
          break;
      }
    }
  }
}

int is_colliding(const Rect* a, const Rect* b) {
  return !(a->x + a->w < b->x ||  // a is left of b
           a->x > b->x + b->w ||  // a is right of b
           a->y + a->h < b->y ||  // a is above b
           a->y > b->y + b->h);   // a is below b
}

void validate_ptr(void* ptr, const char* name) {
  // Check if the pointer is NULL
  if (ptr == NULL) {
    // End ncurses mode before printing the error message
    endwin();

    // Print an error message to stderr indicating the pointer is NULL,
    // including the pointer's name
    fprintf(stderr, "Error: NULL pointer detected in %s\n", name);

    // Exit the program with failure status (EXIT_FAILURE)
    exit(EXIT_FAILURE);
  }
}

int get_inner_window_width(WindowConfig* win_conf) {
  return win_conf->rect.w - ((win_conf->padding.x * 2) + 2);
}

int get_inner_window_height(WindowConfig* win_conf) {
  return win_conf->rect.h - ((win_conf->padding.y * 2) + 2);
}

int get_center_offset(int outer_len, int inner_len) {
  return ((outer_len - inner_len) / 2);
}
