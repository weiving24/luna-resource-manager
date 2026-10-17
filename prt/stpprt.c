#include <curses.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "stpprt.h"

static int g_initialized;

static short curses_color(StpColor color)
{
    switch (color) {
    case STP_COLOR_BLACK:
    case STP_COLOR_BRIGHT_BLACK:
        return COLOR_BLACK;
    case STP_COLOR_RED:
        return COLOR_RED;
    case STP_COLOR_GREEN:
        return COLOR_GREEN;
    case STP_COLOR_YELLOW:
        return COLOR_YELLOW;
    case STP_COLOR_BLUE:
        return COLOR_BLUE;
    case STP_COLOR_MAGENTA:
        return COLOR_MAGENTA;
    case STP_COLOR_CYAN:
        return COLOR_CYAN;
    case STP_COLOR_WHITE:
    case STP_COLOR_BRIGHT_WHITE:
        return COLOR_WHITE;
    case STP_COLOR_DEFAULT:
#ifdef NCURSES_VERSION
        return -1;
#else
        return COLOR_BLACK;
#endif
    default:
        return COLOR_BLACK;
    }
}

static short pair_number(StpColor foreground, StpColor background)
{
    return (short)((int)foreground * STP_COLOR_COUNT + (int)background + 1);
}

static void apply_style(const StpStyle *style)
{
    int attributes = A_NORMAL;
    short pair;
    if (style == NULL) {
        (void)attrset(A_NORMAL);
        return;
    }
    if (style->bold || style->fg == STP_COLOR_BRIGHT_WHITE ||
        style->fg == STP_COLOR_BRIGHT_BLACK)
        attributes |= A_BOLD;
    if (style->reverse)
        attributes |= A_REVERSE;
    if (style->underline)
        attributes |= A_UNDERLINE;

    pair = pair_number(style->fg, style->bg);
    if (has_colors() && pair > 0 && pair < COLOR_PAIRS)
        attributes |= COLOR_PAIR(pair);
    (void)attrset(attributes);
}

static void initialize_colors(void)
{
    int foreground;
    int background;
    if (!has_colors() || start_color() == ERR)
        return;
#ifdef NCURSES_VERSION
    (void)use_default_colors();
#endif
    for (foreground = 0; foreground < STP_COLOR_COUNT; ++foreground) {
        for (background = 0; background < STP_COLOR_COUNT; ++background) {
            short pair = pair_number((StpColor)foreground, (StpColor)background);
            if (pair <= 0 || pair >= COLOR_PAIRS)
                continue;
            (void)init_pair(pair, curses_color((StpColor)foreground),
                            curses_color((StpColor)background));
        }
    }
}

int stpprt_init(void)
{
    if (g_initialized)
        return 0;
    if (initscr() == NULL) {
        errno = ENOTTY;
        return -1;
    }
    (void)cbreak();
    (void)noecho();
    (void)keypad(stdscr, TRUE);
#ifdef NCURSES_MOUSE_VERSION
    (void)mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
#endif
    (void)curs_set(0);
    initialize_colors();
    g_initialized = 1;
    return 0;
}

void stpprt_shutdown(void)
{
    if (g_initialized) {
        (void)endwin();
        g_initialized = 0;
    }
}

void stpprt_resize(int width, int height)
{
#ifdef NCURSES_VERSION
    if (g_initialized && width > 0 && height > 0)
        (void)resizeterm(height, width);
#else
    (void)width;
    (void)height;
#endif
}

void stpprt_clear(void)
{
    if (g_initialized)
        (void)erase();
}

void stpprt_get_size(int *width, int *height)
{
    int rows = 0;
    int columns = 0;
    if (g_initialized)
        getmaxyx(stdscr, rows, columns);
    if (width != NULL)
        *width = columns;
    if (height != NULL)
        *height = rows;
}

int stpprt_read_key(void)
{
    return g_initialized ? getch() : ERR;
}

static void draw_text_line(int y, int x, int width, const char *text,
                           const StpStyle *style)
{
    int rows, columns, length;
    if (!g_initialized || width <= 0)
        return;
    getmaxyx(stdscr, rows, columns);
    if (y < 0 || y >= rows || x < 0 || x >= columns)
        return;
    if (width > columns - x)
        width = columns - x;
    apply_style(style);
    (void)mvhline(y, x, ' ', width);
    if (text != NULL) {
        length = (int)strlen(text);
        if (length > width)
            length = width;
        if (length > 0)
            (void)mvaddnstr(y, x, text, length);
    }
}

static void draw_separator(int y, int x, int height, const StpStyle *style)
{
    int row;
    int rows, columns;
    if (!g_initialized)
        return;
    getmaxyx(stdscr, rows, columns);
    if (x < 0 || x >= columns)
        return;
    apply_style(style);
    for (row = 0; row < height && y + row < rows; ++row) {
        if (y + row >= 0)
            (void)mvaddch(y + row, x, ACS_VLINE);
    }
}

static void draw_list(const StpWidget *widget)
{
    int row;
    for (row = 0; row < widget->h; ++row) {
        int index = widget->scroll_offset + row;
        char line[1024];
        const char *name;
        StpStyle style;
        int visible_width = widget->w;

        if (visible_width <= 0)
            continue;
        if (index >= widget->item_count) {
            draw_text_line(widget->y + row, widget->x, visible_width, "", &widget->style);
            continue;
        }
        name = widget->items[index].text != NULL ? widget->items[index].text : "";
        style = index == widget->cursor ? (StpStyle){
            STP_COLOR_BLACK, STP_COLOR_GREEN, 0, 0, 0
        } : widget->items[index].style;
        (void)snprintf(line, sizeof(line), "%s %.*s",
                       widget->items[index].is_dir ? "[D]" : "   ",
                       visible_width > 4 ? visible_width - 4 : 0, name);
        draw_text_line(widget->y + row, widget->x, visible_width, line, &style);
    }
}

static void draw_multiline(const StpWidget *widget)
{
    const char *text = widget->text != NULL ? widget->text : "";
    int row;
    const char *line = text;
    for (row = 0; row < widget->h; ++row) {
        const char *end = strchr(line, '\n');
        size_t length = end != NULL ? (size_t)(end - line) : strlen(line);
        char *copy;
        if (length > (size_t)(widget->w > 0 ? widget->w : 1))
            length = (size_t)(widget->w > 0 ? widget->w : 1);
        copy = malloc(length + 1);
        if (copy == NULL)
            break;
        memcpy(copy, line, length);
        copy[length] = '\0';
        draw_text_line(widget->y + row, widget->x, widget->w, copy, &widget->style);
        free(copy);
        if (end == NULL)
            line += strlen(line);
        else
            line = end + 1;
        if (*line == '\0' && end != NULL) {
            int rest;
            for (rest = row + 1; rest < widget->h; ++rest)
                draw_text_line(widget->y + rest, widget->x, widget->w, "", &widget->style);
            break;
        }
    }
}

static void draw_widget(const StpWidget *widget)
{
    if (widget == NULL || !widget->visible)
        return;
    switch (widget->type) {
    case STP_WIDGET_HEADER:
    case STP_WIDGET_STATUSBAR:
        draw_text_line(widget->y, widget->x, widget->w, widget->text, &widget->style);
        break;
    case STP_WIDGET_SEPARATOR:
        draw_separator(widget->y, widget->x, widget->h, &widget->style);
        break;
    case STP_WIDGET_LIST:
        draw_list(widget);
        break;
    case STP_WIDGET_INPUT:
        draw_text_line(widget->y, widget->x, widget->w, widget->text, &widget->style);
        break;
    case STP_WIDGET_TEXT:
        draw_multiline(widget);
        break;
    case STP_WIDGET_PANEL:
    default:
        break;
    }
}

void stpprt_draw_frame(const UiFrame *frame)
{
    int i;
    if (!g_initialized || frame == NULL)
        return;
    (void)erase();
    for (i = 0; i < frame->widget_count; ++i)
        draw_widget(frame->widgets[i]);
    (void)refresh();
}
