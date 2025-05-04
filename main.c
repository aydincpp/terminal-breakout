#include <locale.h>
#include <stdlib.h>
#include <unistd.h>

#define _XOPEN_SOURCE_EXTENDED 1
#include "ncursesw/curses.h"

// Define desired game window dimension
#define GAME_WIDTH 80
#define GAME_HEIGHT 24

#define MIN_PADDLE_SIZE 10
#define MAX_PADDLE_SIZE 14
#define PADDLE_CHAR L"█"

#define BALL_CHAR L"⚪"

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
} Paddle;

typedef struct {
  Rect rect;
  Vec2 dir;
  wchar_t* ch;
  int is_launched;
} Ball;

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

// Checks if terminal size can fit the game window dimensions
void check_terminal_size();

// Sets the background color and refreshes the screen to apply the changes
void setup_background_color();

// Initialize the game window configs
void init_game_win_Conf(WindowConfig* win_conf);

// Draws the window frame (border) and applies background color
void draw_window(WINDOW*);

// Initilize paddle
void init_paddle(Paddle* paddle, WindowConfig* win_conf);

// Draw the paddle
void draw_paddle(WINDOW* win, const Paddle* paddle);

// Keep the paddle within bounds
void clamp_paddle_bounds(WindowConfig* win_conf, Paddle* paddle);

// Initilize ball
void init_ball(Ball* ball, Paddle* paddle);

// Draw the ball
void draw_ball(WINDOW* win, Ball* ball, Paddle* paddle);

void keep_ball_within_bounds(WindowConfig* win_conf, Ball* ball);

// check if a point is coll
void isColliding();

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
  setlocale(LC_ALL, "");
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

  Paddle paddle;
  init_paddle(&paddle, &game_win_conf);

  Ball ball;
  init_ball(&ball, &paddle);

  // Draw the window frame and apply the background color
  draw_window(game_win);

  // draw the paddle at the start
  draw_paddle(game_win, &paddle);

  // Draw the ball at the start
  draw_ball(game_win, &ball, &paddle);

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
        ball.is_launched = 1;
        ball.dir.y = -1;
        ball.dir.x = 1;
      default:
        break;
    }

    paddle.rect.x += paddle.dir.x;
    clamp_paddle_bounds(&game_win_conf, &paddle);

    ball.rect.x += ball.dir.x;
    ball.rect.y += ball.dir.y;
    keep_ball_within_bounds(&game_win_conf, &ball);

    // Clear previous paddle position and redraw window
    wclear(game_win);
    draw_window(game_win);           // Redraw the window
    draw_paddle(game_win, &paddle);  // Draw paddle at new position
    draw_ball(game_win, &ball, &paddle);

    // Refresh the window to update screen
    wrefresh(game_win);

    // Small delay to improve smoothness (adjust as needed)
    napms(1000 / 30);  // Delay for 10 milliseconds
  }

  // End ncurses mode, performing any necessary cleanup
  kill_ncurses();

  // Return EXIT_SUCCESS to indicate the program ended successfully
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
  // init_pair(2, COLOR_BLUE, COLOR_BLACK);
  // init_pair(3, COLOR_YELLOW, COLOR_BLACK);
}

void kill_ncurses() {
  endwin();  // End ncurses mode
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
  bkgd(COLOR_PAIR(1));

  // Refresh the screen to apply the background color change
  refresh();
}

void init_game_win_Conf(WindowConfig* win_conf) {
  // Set the window padding
  win_conf->padding.x = 0;
  win_conf->padding.y = 0;

  win_conf->rect.w = GAME_WIDTH;   // Set the window width
  win_conf->rect.h = GAME_HEIGHT;  // Set the window height

  win_conf->inner_rect.w = get_inner_window_width(win_conf);
  win_conf->inner_rect.h = get_inner_window_height(win_conf);

  // Center the window horizontally and vertically in the terminal
  win_conf->rect.x = get_center_offset(COLS, win_conf->rect.w);
  win_conf->rect.y = get_center_offset(LINES, win_conf->rect.h);

  win_conf->inner_rect.x = win_conf->padding.x + 1;
  win_conf->inner_rect.y = win_conf->padding.y + 1;
}

void draw_window(WINDOW* win) {
  // Draw a border around the given window using default line characters
  box(win, 0, 0);

  // Set the window's background using color pair 1
  wbkgd(win, COLOR_PAIR(1));

  // Refresh the window to apply the border and background changes
  wrefresh(win);
}

void init_paddle(Paddle* paddle, WindowConfig* win_conf) {
  paddle->ch = PADDLE_CHAR;
  paddle->rect.w = MAX_PADDLE_SIZE;

  paddle->rect.x = get_center_offset(win_conf->inner_rect.w, paddle->rect.w);
  paddle->rect.y = win_conf->inner_rect.h;

  paddle->dir.x = 0;
  paddle->dir.y = 0;
}

void draw_paddle(WINDOW* win, const Paddle* paddle) {
  cchar_t ch;
  setcchar(&ch, paddle->ch, A_NORMAL, 0, NULL);

  for (int i = 0; i < paddle->rect.w; i++) {
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

  ball->rect.x = paddle->rect.x + (paddle->rect.w / 2);
  ball->rect.y = paddle->rect.y - 1;

  ball->is_launched = 0;
}

void draw_ball(WINDOW* win, Ball* ball, Paddle* paddle) {
  cchar_t ch;
  setcchar(&ch, ball->ch, A_NORMAL, 0, NULL);

  if (ball->is_launched == 0) {
    ball->rect.x = paddle->rect.x + (paddle->rect.w / 2) - 1;
    ball->rect.y = paddle->rect.y - 1;
  }

  mvwadd_wch(win, ball->rect.y, ball->rect.x, &ch);

  wrefresh(win);
}

void keep_ball_within_bounds(WindowConfig* win_conf, Ball* ball) {
  if (ball->rect.x <= win_conf->inner_rect.x ||
      ball->rect.x >= win_conf->inner_rect.w) {
    ball->dir.x *= -1;
  }

  if (ball->rect.y <= win_conf->inner_rect.y ||
      ball->rect.y >= win_conf->inner_rect.h) {
    ball->dir.y *= -1;
  }
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
  // Calculate the usable width inside the window by subtracting:
  // - 2 units for the left and right borders
  // - padding on both sides (padding * 2)
  return win_conf->rect.w - ((win_conf->padding.x * 2) + 2);
}

int get_inner_window_height(WindowConfig* win_conf) {
  // Calculate the usable height inside the window by subtracting:
  // - 2 units for the top and bottom borders
  // - padding on both sides (padding * 2)
  return win_conf->rect.h - ((win_conf->padding.y * 2) + 2);
}

int get_center_offset(int outer_len, int inner_len) {
  return ((outer_len - inner_len) / 2);
}
