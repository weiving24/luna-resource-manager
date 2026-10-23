#define _POSIX_C_SOURCE 200809L

#include <curses.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "stpprt.h"

#define RGB_CACHE_MAX 256
#define PAIR_CACHE_MAX 512

typedef struct {
    uint32_t rgb;
    short slot;
} RgbCacheEntry;

typedef struct {
    short fg;
    short bg;
    short pair;
} PairCacheEntry;

static int g_initialized;
static RgbCacheEntry g_rgb_cache[RGB_CACHE_MAX];
static int g_rgb_count;
static PairCacheEntry g_pair_cache[PAIR_CACHE_MAX];
static int g_pair_count;
static short g_next_pair = 1;
static short g_next_color = 16;

static short curses_color(StpColor color)
{
    switch (color) {
    case STP_COLOR_BLACK:
    case STP_COLOR_BRIGHT_BLACK: return COLOR_BLACK;
    case STP_COLOR_RED: return COLOR_RED;
    case STP_COLOR_GREEN: return COLOR_GREEN;
    case STP_COLOR_YELLOW: return COLOR_YELLOW;
    case STP_COLOR_BLUE: return COLOR_BLUE;
    case STP_COLOR_MAGENTA: return COLOR_MAGENTA;
    case STP_COLOR_CYAN: return COLOR_CYAN;
    case STP_COLOR_WHITE:
    case STP_COLOR_BRIGHT_WHITE: return COLOR_WHITE;
    case STP_COLOR_DEFAULT:
#ifdef NCURSES_VERSION
        return -1;
#else
        return COLOR_BLACK;
#endif
    default: return COLOR_BLACK;
    }
}

static int rgb_components(uint32_t rgb, int *r, int *g, int *b)
{
    *r = (int)((rgb >> 16) & 0xff);
    *g = (int)((rgb >> 8) & 0xff);
    *b = (int)(rgb & 0xff);
    return 0;
}

static int color_distance(uint32_t a, uint32_t b)
{
    int ar, ag, ab, br, bg, bb;
    rgb_components(a, &ar, &ag, &ab);
    rgb_components(b, &br, &bg, &bb);
    return (ar - br) * (ar - br) +
           (ag - bg) * (ag - bg) +
           (ab - bb) * (ab - bb);
}

static short nearest_basic_color(uint32_t rgb)
{
    static const uint32_t palette[] = {
        0x000000, 0xAA0000, 0x00AA00, 0xAA5500,
        0x0000AA, 0xAA00AA, 0x00AAAA, 0xAAAAAA
    };
    int i, best = 0, best_distance = 0x7fffffff;
    for (i = 0; i < 8; ++i) {
        int d = color_distance(rgb, palette[i]);
        if (d < best_distance) {
            best_distance = d;
            best = i;
        }
    }
    switch (best) {
    case 0: return COLOR_BLACK;
    case 1: return COLOR_RED;
    case 2: return COLOR_GREEN;
    case 3: return COLOR_YELLOW;
    case 4: return COLOR_BLUE;
    case 5: return COLOR_MAGENTA;
    case 6: return COLOR_CYAN;
    default: return COLOR_WHITE;
    }
}

static short rgb_color(uint32_t rgb)
{
    int i, r, g, b;
    if (!has_colors())
        return -1;

    for (i = 0; i < g_rgb_count; ++i) {
        if (g_rgb_cache[i].rgb == rgb)
            return g_rgb_cache[i].slot;
    }

    if (can_change_color() && COLORS > 16 &&
        g_rgb_count < RGB_CACHE_MAX && g_next_color < COLORS) {
        short slot = g_next_color++;
        rgb_components(rgb, &r, &g, &b);
        if (init_color(slot,
                       (short)((r * 1000 + 127) / 255),
                       (short)((g * 1000 + 127) / 255),
                       (short)((b * 1000 + 127) / 255)) != ERR) {
            g_rgb_cache[g_rgb_count].rgb = rgb;
            g_rgb_cache[g_rgb_count].slot = slot;
            ++g_rgb_count;
            return slot;
        }
    }

    return nearest_basic_color(rgb);
}

static short style_color(StpColor basic, uint32_t rgb, uint8_t use_rgb)
{
    if (use_rgb)
        return rgb_color(rgb);
    return curses_color(basic);
}

static short stpprt_find_pair(short fg, short bg)
{
    int i;
    for (i = 0; i < g_pair_count; ++i) {
        if (g_pair_cache[i].fg == fg && g_pair_cache[i].bg == bg)
            return g_pair_cache[i].pair;
    }
    if (g_next_pair <= 0 || g_next_pair >= COLOR_PAIRS ||
        g_pair_count >= PAIR_CACHE_MAX)
        return 0;

    {
        short pair = g_next_pair++;
        if (init_pair(pair, fg, bg) == ERR)
            return 0;
        g_pair_cache[g_pair_count].fg = fg;
        g_pair_cache[g_pair_count].bg = bg;
        g_pair_cache[g_pair_count].pair = pair;
        ++g_pair_count;
        return pair;
    }
}

static void apply_style(const StpStyle *style)
{
    int attributes = A_NORMAL;
    short fg, bg, pair;

    if (style == NULL) {
        (void)attrset(A_NORMAL);
        return;
    }

    if (style->bold)
        attributes |= A_BOLD;
    if (style->reverse)
        attributes |= A_REVERSE;
    if (style->underline)
        attributes |= A_UNDERLINE;

    fg = style_color(style->fg, style->fg_rgb, style->use_fg_rgb);
    bg = style_color(style->bg, style->bg_rgb, style->use_bg_rgb);

    if (has_colors()) {
        pair = stpprt_find_pair(fg, bg);
        if (pair > 0)
            attributes |= COLOR_PAIR(pair);
    }
    (void)attrset(attributes);
}

static void initialize_colors(void)
{
    if (!has_colors() || start_color() == ERR)
        return;
#ifdef NCURSES_VERSION
    (void)use_default_colors();
#endif
    g_rgb_count = 0;
    g_pair_count = 0;
    g_next_pair = 1;
    g_next_color = 16;
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
    int rows = 0, columns = 0;
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
    int row, rows, columns;
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
            draw_text_line(widget->y + row, widget->x, visible_width,
                           "", &widget->style);
            continue;
        }

        name = widget->items[index].text != NULL ?
               widget->items[index].text : "";
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
        draw_text_line(widget->y + row, widget->x, widget->w,
                       copy, &widget->style);
        free(copy);

        if (end == NULL) {
            line += strlen(line);
        } else {
            line = end + 1;
        }
        if (*line == '\0' && end != NULL) {
            int rest;
            for (rest = row + 1; rest < widget->h; ++rest)
                draw_text_line(widget->y + rest, widget->x, widget->w,
                               "", &widget->style);
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
        draw_text_line(widget->y, widget->x, widget->w,
                       widget->text, &widget->style);
        break;
    case STP_WIDGET_SEPARATOR:
        draw_separator(widget->y, widget->x, widget->h, &widget->style);
        break;
    case STP_WIDGET_LIST:
        draw_list(widget);
        break;
    case STP_WIDGET_INPUT:
        draw_text_line(widget->y, widget->x, widget->w,
                       widget->text, &widget->style);
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
