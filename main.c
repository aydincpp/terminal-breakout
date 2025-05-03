#include "ncurses.h"

void init_ncurses();
void test_ncurses();

int main()
{
    init_ncurses();
    test_ncurses();

    endwin();

    return 0;
}

void init_ncurses()
{
    initscr();
    cbreak();
    noecho();
}

void test_ncurses() {
    mvprintw(2, 2, "Hello from ncurses!");
    refresh();

    getch();
}
