#ifndef LUNA_RESOURCE_MANAGER_UI_H
#define LUNA_RESOURCE_MANAGER_UI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STP_COLOR_DEFAULT = 0,
    STP_COLOR_BLACK,
    STP_COLOR_RED,
    STP_COLOR_GREEN,
    STP_COLOR_YELLOW,
    STP_COLOR_BLUE,
    STP_COLOR_MAGENTA,
    STP_COLOR_CYAN,
    STP_COLOR_WHITE,
    STP_COLOR_BRIGHT_BLACK,
    STP_COLOR_BRIGHT_WHITE,
    STP_COLOR_COUNT
} StpColor;

typedef struct {
    StpColor fg;
    StpColor bg;
    uint8_t bold;
    uint8_t reverse;
    uint8_t underline;
} StpStyle;

typedef enum {
    STP_WIDGET_HEADER,
    STP_WIDGET_LIST,
    STP_WIDGET_INPUT,
    STP_WIDGET_STATUSBAR,
    STP_WIDGET_SEPARATOR,
    STP_WIDGET_TEXT,
    STP_WIDGET_PANEL
} StpWidgetType;

typedef struct {
    char *text;
    char *meta;
    StpStyle style;
    int is_dir;
    int64_t size;
    int64_t mtime;
} StpListItem;

typedef struct {
    StpWidgetType type;
    int x, y, w, h;
    StpStyle style;
    char *text;
    StpListItem *items;
    int item_count;
    int scroll_offset;
    int cursor;
    int focused;
    int visible;
    char *tag;
} StpWidget;

typedef struct {
    StpWidget **widgets;
    int widget_count;
    int screen_w;
    int screen_h;
    int dirty;
} UiFrame;

typedef struct {
    int screen_w;
    int screen_h;
    int panel_ratio;
    int show_header;
    int show_statusbar;
    int show_preview;
} LayoutParams;

typedef struct {
    int (*list_dir)(const char *path, StpListItem **items, int *count);
    void (*free_items)(StpListItem *items, int count);
    const char *(*get_cwd)(void);
    const char *(*get_time)(void);
    int (*read_file_preview)(const char *path, char **buf, int *len);
} SetupCallbacks;

typedef struct {
    StpStyle header;
    StpStyle status;
    StpStyle dir;
    StpStyle file;
    StpStyle focus;
    StpStyle separator;
    StpStyle input;
    StpStyle preview;
} StpTheme;

void stpui_theme_default(StpTheme *theme);
void stpui_theme_dark(StpTheme *theme);
void stpui_theme_light(StpTheme *theme);

#ifdef __cplusplus
}
#endif

#endif
