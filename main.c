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
  wchar_t* ch;
  int x, y;
  int dx, dy;
  int len, min_len, max_len;
} Paddle;

typedef struct {
  wchar_t* ch;
  int x, y;
  int dx, dy;
  int is_launched;
} Ball;

// Represents the window's position, size, and optional padding
typedef struct {
  int padding;              // Space inside the window boundary
  int x, y;                 // Position of the window (top-left corner)
  int in_x, in_y;           // border and padding excluded
  int width, height;        // Dimensions of the window
  int in_width, in_height;  // border and padding excluded
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

// Initilize paddle
void init_paddle(Paddle* paddle, WindowConfig* win_conf);

// Initilize ball
void init_ball(Ball* ball, Paddle* paddle);

// Validates a pointer and prints an error message with the pointer's name if
// it's NULL
void validate_ptr(void* ptr, const char* name);

// Macro to validate a pointer and print its name if NULL.
#define VALIDATE(ptr) validate_ptr(ptr, #ptr)

// Draws the window frame (border) and applies background color
void draw_window(WINDOW*);

// Draw the paddle
void draw_paddle(WINDOW* win, const Paddle* paddle);

// Draw the ball
void draw_ball(WINDOW* win, Ball* ball, Paddle* paddle);

void clamp_paddle_bounds(WindowConfig* win_conf, Paddle* paddle);

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
  WINDOW* game_win = newwin(game_win_conf.height, game_win_conf.width,
                            game_win_conf.y, game_win_conf.x);
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
        paddle.dx = -1;
        break;
      case KEY_RIGHT:
        paddle.dx = 1;
        break;
      default:
        break;
    }

    paddle.x += paddle.dx;
    clamp_paddle_bounds(&game_win_conf, &paddle);

    // Clear previous paddle position and redraw window
    wclear(game_win);
    draw_window(game_win);           // Redraw the window
    draw_paddle(game_win, &paddle);  // Draw paddle at new position
    draw_ball(game_win, &ball, &paddle);

    // Refresh the window to update screen
    wrefresh(game_win);

    // Small delay to improve smoothness (adjust as needed)
    napms(1000 / 60);  // Delay for 10 milliseconds
  }

  // End ncurses mode, performing any necessary cleanup
  kill_ncurses();

  // Return EXIT_SUCCESS to indicate the program ended successfully
  return EXIT_SUCCESS;
}

void init_ncurses() {
  initscr();
  start_color();
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
  win_conf->padding = 0;           // Set the window padding
  win_conf->width = GAME_WIDTH;    // Set the window width
  win_conf->height = GAME_HEIGHT;  // Set the window height

  win_conf->in_width = get_inner_window_width(win_conf);
  win_conf->in_height = get_inner_window_height(win_conf);

  // Center the window horizontally and vertically in the terminal
  win_conf->x = get_center_offset(COLS, win_conf->width);
  win_conf->y = get_center_offset(LINES, win_conf->height);

  win_conf->in_x = win_conf->padding + 1;
  win_conf->in_y = win_conf->padding + 1;
}

void init_paddle(Paddle* paddle, WindowConfig* win_conf) {
  paddle->ch = PADDLE_CHAR;
  paddle->min_len = MIN_PADDLE_SIZE;
  paddle->max_len = MAX_PADDLE_SIZE;
  paddle->len = paddle->max_len;

  paddle->x = get_center_offset(win_conf->in_width, paddle->len);
  paddle->y = win_conf->in_height;

  paddle->dx = 0;
  paddle->dy = 0;
}

void init_ball(Ball* ball, Paddle* paddle) {
  ball->ch = BALL_CHAR;

  ball->dx = 0;
  ball->dy = 0;

  ball->x = paddle->x + (paddle->len / 2);
  ball->y = paddle->y - 1;

  ball->is_launched = 0;
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

void draw_window(WINDOW* win) {
  // Draw a border around the given window using default line characters
  box(win, 0, 0);

  // Set the window's background using color pair 1
  wbkgd(win, COLOR_PAIR(1));

  // Refresh the window to apply the border and background changes
  wrefresh(win);
}

void draw_paddle(WINDOW* win, const Paddle* paddle) {
  cchar_t ch;
  setcchar(&ch, paddle->ch, A_NORMAL, 0, NULL);

  for (int i = 0; i < paddle->len; i++) {
    mvwadd_wch(win, paddle->y, paddle->x + i, &ch);
  }
  wrefresh(win);
}

void draw_ball(WINDOW* win, Ball* ball, Paddle* paddle) {
  cchar_t ch;
  setcchar(&ch, ball->ch, A_NORMAL, 0, NULL);

  int ball_x_pos =
      (ball->is_launched == 0) ? paddle->x + (paddle->len / 2) - 1 : ball->x;
  int ball_y_pos = (ball->is_launched == 0) ? paddle->y - 1 : ball->y;

  mvwadd_wch(win, ball_y_pos, ball_x_pos, &ch);

  wrefresh(win);
}

void clamp_paddle_bounds(WindowConfig* win_conf, Paddle* paddle) {
  if (paddle->x < win_conf->in_x) {
    paddle->x = win_conf->in_x;
  }

  if (paddle->x + paddle->len > win_conf->in_width) {
    paddle->x = win_conf->width - paddle->len - 1;
  }
}

int get_inner_window_width(WindowConfig* win_conf) {
  // Calculate the usable width inside the window by subtracting:
  // - 2 units for the left and right borders
  // - padding on both sides (padding * 2)
  return win_conf->width - ((win_conf->padding * 2) + 2);
}

int get_inner_window_height(WindowConfig* win_conf) {
  // Calculate the usable height inside the window by subtracting:
  // - 2 units for the top and bottom borders
  // - padding on both sides (padding * 2)
  return win_conf->height - ((win_conf->padding * 2) + 2);
}

int get_center_offset(int outer_len, int inner_len) {
  return ((outer_len - inner_len) / 2);
}
