#include "viewer.h"
#include "dialogs.h"

#include <curses.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VIEWER_FILE_LIMIT (32U * 1024U * 1024U)

static char *read_whole_file(const char *path, size_t *length)
{
    FILE *stream;
    char *buffer;
    size_t used = 0;
    size_t capacity = 8192;

    stream = fopen(path, "rb");
    if (stream == NULL)
        return NULL;
    buffer = malloc(capacity + 1);
    if (buffer == NULL) {
        (void)fclose(stream);
        return NULL;
    }
    for (;;) {
        size_t got;
        if (used == capacity) {
            size_t next;
            char *grown;
            if (capacity >= VIEWER_FILE_LIMIT) {
                (void)fclose(stream);
                free(buffer);
                errno = EFBIG;
                return NULL;
            }
            next = capacity * 2;
            if (next > VIEWER_FILE_LIMIT)
                next = VIEWER_FILE_LIMIT;
            grown = realloc(buffer, next + 1);
            if (grown == NULL) {
                int saved_errno = errno;
                (void)fclose(stream);
                free(buffer);
                errno = saved_errno;
                return NULL;
            }
            buffer = grown;
            capacity = next;
        }
        got = fread(buffer + used, 1, capacity - used, stream);
        used += got;
        if (got == 0) {
            if (ferror(stream)) {
                int saved_errno = errno != 0 ? errno : EIO;
                (void)fclose(stream);
                free(buffer);
                errno = saved_errno;
                return NULL;
            }
            break;
        }
    }
    if (fclose(stream) != 0) {
        int saved_errno = errno;
        free(buffer);
        errno = saved_errno;
        return NULL;
    }
    buffer[used] = '\0';
    *length = used;
    return buffer;
}

static int is_binary_text(const char *text, size_t length)
{
    size_t i;
    for (i = 0; i < length; ++i) {
        unsigned char byte = (unsigned char)text[i];
        if (byte == 0)
            return 1;
    }
    return 0;
}

static char printable(unsigned char value)
{
    if (value == '\t' || value == '\r')
        return ' ';
    if (value < 32 || value == 127)
        return '.';
    return (char)value;
}

static void draw_view(WINDOW *window, const char *title, const char *text,
                      size_t length, int exit_code, int show_exit_code)
{
    int height, width;
    int body_height;
    int line_number = 0;
    int scroll = 0;
    int key;

    (void)keypad(window, TRUE);
    getmaxyx(window, height, width);
    body_height = height - 3;
    if (body_height < 1)
        body_height = 1;
    for (;;) {
        int current_line = 0;
        size_t position = 0;
        (void)werase(window);
        (void)box(window, 0, 0);
        (void)wattrset(window, A_BOLD);
        (void)mvwaddnstr(window, 0, 2, title != NULL ? title : "Viewer", width - 4);
        (void)wattrset(window, A_NORMAL);
        while (position < length) {
            size_t end = position;
            int column = 0;
            char *line;
            size_t raw_length;
            while (end < length && text[end] != '\n' && column < width - 2) {
                ++end;
                ++column;
            }
            while (end < length && text[end] != '\n')
                ++end;
            raw_length = end - position;
            if (current_line >= scroll && current_line < scroll + body_height) {
                size_t clipped_length = raw_length;
                size_t i;
                if (clipped_length > (size_t)(width - 2))
                    clipped_length = (size_t)(width - 2);
                line = malloc(clipped_length + 1);
                if (line == NULL)
                    break;
                for (i = 0; i < clipped_length; ++i)
                    line[i] = printable((unsigned char)text[position + i]);
                line[clipped_length] = '\0';
                (void)mvwaddnstr(window, 1 + current_line - scroll, 1,
                                 line, (int)clipped_length);
                free(line);
            }
            if (end < length && text[end] == '\n')
                ++end;
            if (end <= position)
                break;
            position = end;
            ++current_line;
        }
        line_number = current_line;
        (void)wattrset(window, A_REVERSE);
        (void)mvwhline(window, height - 1, 1, ' ', width - 2);
        if (show_exit_code)
            (void)mvwprintw(window, height - 1, 2,
                            " Esc: Close  Up/Down: Scroll  exit=%d  lines=%d ", exit_code, line_number);
        else
            (void)mvwprintw(window, height - 1, 2,
                            " Esc: Close  Up/Down: Scroll  line %d/%d ",
                            scroll + 1, line_number > 0 ? line_number : 1);
        (void)wattrset(window, A_NORMAL);
        (void)wrefresh(window);

        key = wgetch(window);
        if (key == 27 || key == 'q')
            break;
        if (key == KEY_UP && scroll > 0)
            --scroll;
        else if (key == KEY_DOWN && scroll < line_number - 1)
            ++scroll;
        else if (key == KEY_NPAGE) {
            scroll += body_height;
            if (scroll > line_number - 1)
                scroll = line_number > 0 ? line_number - 1 : 0;
        } else if (key == KEY_PPAGE) {
            scroll -= body_height;
            if (scroll < 0)
                scroll = 0;
        }
    }
}

static WINDOW *viewer_window(void)
{
    int rows, columns;
    int height, width;
    getmaxyx(stdscr, rows, columns);
    if (rows < 4 || columns < 10)
        return NULL;
    height = rows * 3 / 4;
    width = columns * 4 / 5;
    if (height < 4)
        height = rows;
    if (width < 10)
        width = columns;
    if (height > rows)
        height = rows;
    if (width > columns)
        width = columns;
    return newwin(height, width, (rows - height) / 2, (columns - width) / 2);
}

void viewer_text_file(const char *path)
{
    size_t length = 0;
    char *contents;
    WINDOW *window;
    char title[512];
    if (path == NULL)
        return;
    contents = read_whole_file(path, &length);
    if (contents == NULL) {
        char message[1024];
        (void)snprintf(message, sizeof(message), "Cannot read file:\n%s\n\n%s", path, strerror(errno));
        dialog_text_view("Cannot open file", message);
        return;
    }
    if (is_binary_text(contents, length)) {
        free(contents);
        dialog_text_view("Binary file", "This file contains binary data and cannot be displayed as text.");
        return;
    }
    (void)snprintf(title, sizeof(title), " View: %.480s ", path);
    window = viewer_window();
    if (window != NULL) {
        draw_view(window, title, contents, length, 0, 0);
        (void)delwin(window);
        (void)touchwin(stdscr);
        (void)refresh();
    }
    free(contents);
}

void viewer_output(const char *title, const char *text, size_t length, int exit_code)
{
    WINDOW *window = viewer_window();
    if (window == NULL)
        return;
    draw_view(window, title, text != NULL ? text : "", length, exit_code, 1);
    (void)delwin(window);
    (void)touchwin(stdscr);
    (void)refresh();
}
