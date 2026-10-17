#include "menu.h"

#include <curses.h>
#include <stdio.h>

int menu_select(const char *title, const char *subtitle,
                const MenuOption *options, int count)
{
    int rows, columns, height, width, y, x;
    int selected = 0;
    WINDOW *window;

    if (options == NULL || count <= 0)
        return -1;
    getmaxyx(stdscr, rows, columns);
    if (rows < 5 || columns < 10)
        return -1;
    height = count + 5;
    if (height > rows - 1)
        height = rows - 1;
    width = columns * 4 / 5;
    if (width > columns - 1)
        width = columns - 1;
    if (width < 10)
        width = columns;
    y = (rows - height) / 2;
    x = (columns - width) / 2;
    window = newwin(height, width, y, x);
    if (window == NULL)
        return -1;
    (void)keypad(window, TRUE);

    for (;;) {
        int key;
        int index;
        (void)werase(window);
        (void)box(window, 0, 0);
        (void)wattrset(window, A_BOLD);
        (void)mvwprintw(window, 0, 2, " %s ", title != NULL ? title : "Menu");
        (void)wattrset(window, A_NORMAL);
        if (subtitle != NULL)
            (void)mvwaddnstr(window, 1, 2, subtitle, width - 4);
        for (index = 0; index < count && index < height - 4; ++index) {
            (void)wattrset(window, index == selected ? A_REVERSE | A_BOLD : A_NORMAL);
            (void)mvwprintw(window, 2 + index, 3, "%.*s", width - 6,
                            options[index].label != NULL ? options[index].label : "");
        }
        (void)wattrset(window, A_REVERSE);
        (void)mvwhline(window, height - 1, 1, ' ', width - 2);
        (void)mvwprintw(window, height - 1, 2, " Up/Down Select  Enter OK  Esc Cancel ");
        (void)wattrset(window, A_NORMAL);
        (void)wrefresh(window);

        key = wgetch(window);
        if (key == 27) {
            selected = -1;
            break;
        }
        if (key == KEY_UP && selected > 0)
            --selected;
        else if (key == KEY_DOWN && selected < count - 1 && selected < height - 5)
            ++selected;
        else if (key == '\n' || key == '\r' || key == KEY_ENTER)
            break;
    }
    (void)delwin(window);
    (void)touchwin(stdscr);
    (void)refresh();
    return selected;
}
