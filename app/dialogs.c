#include "dialogs.h"

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static WINDOW *centered_window(int preferred_height, int preferred_width)
{
    int rows, columns;
    int height, width;
    int y, x;
    getmaxyx(stdscr, rows, columns);
    if (rows < 3 || columns < 8)
        return NULL;
    height = preferred_height < rows - 1 ? preferred_height : rows - 1;
    width = preferred_width < columns - 1 ? preferred_width : columns - 1;
    if (height < 3)
        height = 3;
    if (width < 8)
        width = 8;
    y = (rows - height) / 2;
    x = (columns - width) / 2;
    return newwin(height, width, y, x);
}

int dialog_input(const char *title, const char *prompt, char *output, size_t capacity)
{
    int rows, columns;
    int key;
    WINDOW *window;
    size_t length;

    if (output == NULL || capacity == 0)
        return -1;
    length = strlen(output);
    if (length >= capacity) {
        output[capacity - 1] = '\0';
        length = capacity - 1;
    }
    getmaxyx(stdscr, rows, columns);
    (void)rows;
    window = centered_window(8, columns * 4 / 5);
    if (window == NULL)
        return -1;
    (void)keypad(window, TRUE);
    (void)curs_set(1);
    for (;;) {
        int height, width;
        getmaxyx(window, height, width);
        (void)werase(window);
        (void)box(window, 0, 0);
        (void)wattrset(window, A_BOLD);
        (void)mvwprintw(window, 0, 2, " %s ", title != NULL ? title : "Input");
        (void)wattrset(window, A_NORMAL);
        (void)mvwaddnstr(window, 2, 2, prompt != NULL ? prompt : "Value:", width - 4);
        (void)mvwprintw(window, 4, 2, "%.*s", width - 4, output);
        (void)mvwprintw(window, height - 2, 2, "Enter: OK  Esc: Cancel");
        (void)wmove(window, 4, 2 + (int)(length < (size_t)(width - 5) ? length : (size_t)(width - 5)));
        (void)wrefresh(window);
        key = wgetch(window);
        if (key == 27) {
            (void)curs_set(0);
            (void)delwin(window);
            (void)touchwin(stdscr);
            (void)refresh();
            return -1;
        }
        if (key == '\n' || key == '\r' || key == KEY_ENTER) {
            (void)curs_set(0);
            (void)delwin(window);
            (void)touchwin(stdscr);
            (void)refresh();
            return 0;
        }
        if (key == KEY_BACKSPACE || key == 127 || key == 8) {
            if (length > 0)
                output[--length] = '\0';
        } else if (key >= 32 && key < 127 && length + 1 < capacity) {
            output[length++] = (char)key;
            output[length] = '\0';
        }
    }
}

int dialog_confirm(const char *title, const char *message)
{
    WINDOW *window;
    int key;
    int selected = 0;
    int rows, columns;
    int height, width;
    int row = 2;
    const char *cursor;

    getmaxyx(stdscr, rows, columns);
    (void)rows;
    window = centered_window(9, columns * 4 / 5);
    if (window == NULL)
        return 0;
    (void)keypad(window, TRUE);
    getmaxyx(window, height, width);
    (void)werase(window);
    (void)box(window, 0, 0);
    (void)wattrset(window, A_BOLD);
    (void)mvwprintw(window, 0, 2, " %s ", title != NULL ? title : "Confirm");
    (void)wattrset(window, A_NORMAL);
    cursor = message != NULL ? message : "Are you sure?";
    while (*cursor != '\0' && row < height - 3) {
        const char *newline = strchr(cursor, '\n');
        size_t length = newline != NULL ? (size_t)(newline - cursor) : strlen(cursor);
        if (length > (size_t)(width - 4))
            length = (size_t)(width - 4);
        (void)mvwaddnstr(window, row++, 2, cursor, (int)length);
        if (newline == NULL)
            break;
        cursor = newline + 1;
    }
    for (;;) {
        (void)wattrset(window, selected == 0 ? A_REVERSE | A_BOLD : A_NORMAL);
        (void)mvwprintw(window, height - 2, 4, " [ Yes ] ");
        (void)wattrset(window, selected == 1 ? A_REVERSE | A_BOLD : A_NORMAL);
        (void)mvwprintw(window, height - 2, 18, " [ No ] ");
        (void)wattrset(window, A_NORMAL);
        (void)wrefresh(window);
        key = wgetch(window);
        if (key == 27 || key == 'n' || key == 'N') {
            selected = 1;
            break;
        }
        if (key == 'y' || key == 'Y') {
            selected = 0;
            break;
        }
        if (key == '\t' || key == KEY_LEFT || key == KEY_RIGHT)
            selected = 1 - selected;
        else if (key == '\n' || key == '\r' || key == KEY_ENTER)
            break;
    }
    (void)delwin(window);
    (void)touchwin(stdscr);
    (void)refresh();
    return selected == 0;
}

void dialog_message(const char *title, const char *message)
{
    dialog_text_view(title, message != NULL ? message : "");
}

void dialog_status(const char *message)
{
    int rows, columns;
    if (stdscr == NULL)
        return;
    getmaxyx(stdscr, rows, columns);
    if (rows <= 0 || columns <= 0)
        return;
    (void)attron(A_REVERSE);
    (void)mvhline(rows - 1, 0, ' ', columns);
    if (message != NULL)
        (void)mvaddnstr(rows - 1, 1, message, columns - 2);
    (void)attroff(A_REVERSE);
    (void)refresh();
}

void dialog_text_view(const char *title, const char *text)
{
    WINDOW *window;
    int rows, columns, height, width;
    const char *cursor;
    int row = 2;
    int key;

    getmaxyx(stdscr, rows, columns);
    window = centered_window(rows * 3 / 4, columns * 4 / 5);
    if (window == NULL)
        return;
    (void)keypad(window, TRUE);
    getmaxyx(window, height, width);
    (void)werase(window);
    (void)box(window, 0, 0);
    (void)wattrset(window, A_BOLD);
    (void)mvwprintw(window, 0, 2, " %s ", title != NULL ? title : "Information");
    (void)wattrset(window, A_NORMAL);
    cursor = text != NULL ? text : "";
    while (*cursor != '\0' && row < height - 2) {
        const char *newline = strchr(cursor, '\n');
        size_t length = newline != NULL ? (size_t)(newline - cursor) : strlen(cursor);
        if (length > (size_t)(width - 4))
            length = (size_t)(width - 4);
        (void)mvwaddnstr(window, row++, 2, cursor, (int)length);
        if (newline == NULL)
            break;
        cursor = newline + 1;
    }
    (void)wattrset(window, A_REVERSE);
    (void)mvwhline(window, height - 1, 1, ' ', width - 2);
    (void)mvwprintw(window, height - 1, 2, " Esc: Close ");
    (void)wattrset(window, A_NORMAL);
    (void)wrefresh(window);
    do {
        key = wgetch(window);
    } while (key != 27 && key != 'q' && key != '\n' && key != '\r');
    (void)delwin(window);
    (void)touchwin(stdscr);
    (void)refresh();
}
