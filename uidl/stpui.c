#include "stpui_ui.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static LayoutParams g_params;
static SetupCallbacks g_callbacks;
static StpTheme g_theme;
static int g_initialized;

static char *string_duplicate(const char *text)
{
    size_t length;
    char *copy;

    if (text == NULL)
        text = "";
    length = strlen(text) + 1;
    copy = malloc(length);
    if (copy != NULL)
        memcpy(copy, text, length);
    return copy;
}

static StpWidget *widget_new(StpWidgetType type, int x, int y, int width, int height)
{
    StpWidget *widget = calloc(1, sizeof(*widget));
    if (widget == NULL)
        return NULL;
    widget->type = type;
    widget->x = x;
    widget->y = y;
    widget->w = width > 0 ? width : 1;
    widget->h = height > 0 ? height : 1;
    widget->visible = 1;
    return widget;
}

static void widget_free(StpWidget *widget)
{
    int i;
    if (widget == NULL)
        return;

    free(widget->text);
    free(widget->tag);
    if (widget->items != NULL) {
        if (g_callbacks.free_items != NULL) {
            g_callbacks.free_items(widget->items, widget->item_count);
        } else {
            for (i = 0; i < widget->item_count; ++i) {
                free(widget->items[i].text);
                free(widget->items[i].meta);
            }
            free(widget->items);
        }
    }
    free(widget);
}

static int frame_add(UiFrame *frame, StpWidget *widget)
{
    StpWidget **grown;
    if (frame == NULL || widget == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (frame->widget_count == 2147483647) {
        errno = ENOMEM;
        return -1;
    }
    grown = realloc(frame->widgets,
                    ((size_t)frame->widget_count + 1U) * sizeof(*frame->widgets));
    if (grown == NULL)
        return -1;
    frame->widgets = grown;
    frame->widgets[frame->widget_count++] = widget;
    return 0;
}

static int set_widget_text(StpWidget *widget, const char *text)
{
    char *copy = string_duplicate(text);
    if (copy == NULL)
        return -1;
    free(widget->text);
    widget->text = copy;
    return 0;
}

int stpui_init(const LayoutParams *params, const SetupCallbacks *callbacks)
{
    if (params == NULL || callbacks == NULL) {
        errno = EINVAL;
        return -1;
    }
    g_params = *params;
    if (g_params.panel_ratio < 20)
        g_params.panel_ratio = 20;
    if (g_params.panel_ratio > 80)
        g_params.panel_ratio = 80;
    if (g_params.screen_w < 1)
        g_params.screen_w = 1;
    if (g_params.screen_h < 1)
        g_params.screen_h = 1;
    g_callbacks = *callbacks;
    stpui_theme_default(&g_theme);
    g_initialized = 1;
    return 0;
}

void stpui_shutdown(void)
{
    g_initialized = 0;
    memset(&g_callbacks, 0, sizeof(g_callbacks));
}

void stpui_set_layout(const LayoutParams *params)
{
    if (params == NULL)
        return;
    g_params = *params;
    if (g_params.panel_ratio < 20)
        g_params.panel_ratio = 20;
    if (g_params.panel_ratio > 80)
        g_params.panel_ratio = 80;
}

void stpui_resize(int width, int height)
{
    g_params.screen_w = width > 0 ? width : 1;
    g_params.screen_h = height > 0 ? height : 1;
}

void stpui_set_theme(const StpTheme *theme)
{
    if (theme != NULL)
        g_theme = *theme;
}

static StpWidget *build_header(UiFrame *frame, const char *cwd)
{
    StpWidget *widget = widget_new(STP_WIDGET_HEADER, 0, 0, frame->screen_w, 1);
    char text[PATH_MAX + 64];
    const char *time_text = g_callbacks.get_time != NULL ? g_callbacks.get_time() : "";

    if (widget == NULL)
        return NULL;
    (void)snprintf(text, sizeof(text), " Luna Resource Manager | %.3900s | %.32s ",
                   cwd != NULL ? cwd : ".", time_text != NULL ? time_text : "");
    if (set_widget_text(widget, text) != 0 ||
        (widget->tag = string_duplicate("header")) == NULL) {
        widget_free(widget);
        return NULL;
    }
    widget->style = g_theme.header;
    widget->focused = 1;
    return widget;
}

static StpWidget *build_list(const char *cwd,
                             int cursor, int scroll, int x, int y,
                             int width, int height)
{
    StpWidget *widget = widget_new(STP_WIDGET_LIST, x, y, width, height);
    int i;

    if (widget == NULL)
        return NULL;
    widget->tag = string_duplicate("left");
    if (widget->tag == NULL) {
        widget_free(widget);
        return NULL;
    }
    widget->cursor = cursor;
    widget->scroll_offset = scroll;
    widget->style = g_theme.file;
    if (g_callbacks.list_dir != NULL) {
        StpListItem *items = NULL;
        int count = 0;
        if (g_callbacks.list_dir(cwd != NULL ? cwd : ".", &items, &count) == 0) {
            widget->items = items;
            widget->item_count = count;
            for (i = 0; i < count; ++i)
                widget->items[i].style = widget->items[i].is_dir ? g_theme.dir : g_theme.file;
        } else {
            widget->items = NULL;
            widget->item_count = 0;
        }
    }
    if (widget->cursor < 0)
        widget->cursor = 0;
    if (widget->item_count > 0 && widget->cursor >= widget->item_count)
        widget->cursor = widget->item_count - 1;
    if (widget->item_count == 0)
        widget->cursor = 0;
    return widget;
}

static StpWidget *build_preview(UiFrame *frame, const char *cwd,
                                const StpWidget *list, int x, int y,
                                int width, int height)
{
    StpWidget *widget = widget_new(STP_WIDGET_TEXT, x, y, width, height);
    char path[PATH_MAX];
    char *contents = NULL;
    int length = 0;

    (void)frame;
    if (widget == NULL)
        return NULL;
    widget->tag = string_duplicate("preview");
    widget->style = g_theme.preview;
    if (widget->tag == NULL) {
        widget_free(widget);
        return NULL;
    }
    if (list == NULL || list->item_count == 0 || list->cursor < 0 ||
        list->cursor >= list->item_count) {
        if (set_widget_text(widget, "No item selected.") != 0) {
            widget_free(widget);
            return NULL;
        }
        return widget;
    }
    if (list->items[list->cursor].is_dir) {
        char message[PATH_MAX + 32];
        (void)snprintf(message, sizeof(message), "Directory: %s",
                       list->items[list->cursor].text);
        if (set_widget_text(widget, message) != 0) {
            widget_free(widget);
            return NULL;
        }
        return widget;
    }
    if (cwd == NULL || list->items[list->cursor].text == NULL ||
        snprintf(path, sizeof(path), "%s%s%s", cwd,
                 (cwd[0] != '\0' && cwd[strlen(cwd) - 1] == '/') ? "" : "/",
                 list->items[list->cursor].text) >= (int)sizeof(path)) {
        if (set_widget_text(widget, "Path is too long to preview.") != 0) {
            widget_free(widget);
            return NULL;
        }
        return widget;
    }
    if (g_callbacks.read_file_preview == NULL ||
        g_callbacks.read_file_preview(path, &contents, &length) != 0 || contents == NULL) {
        if (set_widget_text(widget, "Preview unavailable.") != 0) {
            widget_free(widget);
            return NULL;
        }
        free(contents);
        return widget;
    }
    {
        int binary = 0;
        int i;
        for (i = 0; i < length; ++i) {
            unsigned char c = (unsigned char)contents[i];
            if (c == '\0') {
                binary = 1;
                break;
            }
            if ((c < 32 && c != '\n' && c != '\t') || c == 127)
                contents[i] = '.';
        }
        if (binary) {
            free(contents);
            contents = string_duplicate("Binary file; preview disabled.");
            if (contents == NULL) {
                widget_free(widget);
                return NULL;
            }
        }
    }
    widget->text = contents;
    return widget;
}

static StpWidget *build_status(UiFrame *frame, const char *cwd,
                               int count, const char *message)
{
    StpWidget *widget = widget_new(STP_WIDGET_STATUSBAR, 0,
                                   frame->screen_h - 1, frame->screen_w, 1);
    char text[PATH_MAX + 512];
    if (widget == NULL)
        return NULL;
    (void)snprintf(text, sizeof(text), " %.3000s | %d items | %.400s | a:About ?:Help q:Quit ",
                   cwd != NULL ? cwd : ".", count, message != NULL ? message : "");
    if (set_widget_text(widget, text) != 0 ||
        (widget->tag = string_duplicate("status")) == NULL) {
        widget_free(widget);
        return NULL;
    }
    widget->style = g_theme.status;
    return widget;
}

static StpWidget *build_input(UiFrame *frame, const char *text)
{
    StpWidget *widget = widget_new(STP_WIDGET_INPUT, 0,
                                   frame->screen_h - 2, frame->screen_w, 1);
    char display[512];
    if (widget == NULL)
        return NULL;
    (void)snprintf(display, sizeof(display), " Search: %.480s", text != NULL ? text : "");
    if (set_widget_text(widget, display) != 0 ||
        (widget->tag = string_duplicate("search")) == NULL) {
        widget_free(widget);
        return NULL;
    }
    widget->style = g_theme.input;
    return widget;
}

UiFrame *stpui_build_frame(const char *cwd, int cursor, int scroll,
                           const char *search_text, const char *status_msg)
{
    UiFrame *frame;
    StpWidget *list = NULL;
    int y = 0;
    int left_width;
    int right_width;
    int body_height;

    if (!g_initialized) {
        errno = EINVAL;
        return NULL;
    }
    frame = calloc(1, sizeof(*frame));
    if (frame == NULL)
        return NULL;
    frame->screen_w = g_params.screen_w > 0 ? g_params.screen_w : 1;
    frame->screen_h = g_params.screen_h > 0 ? g_params.screen_h : 1;

    if (g_params.show_header) {
        StpWidget *header = build_header(frame, cwd);
        if (header == NULL || frame_add(frame, header) != 0) {
            widget_free(header);
            goto fail;
        }
        ++y;
    }

    body_height = frame->screen_h - y - (g_params.show_statusbar ? 1 : 0);
    if (search_text != NULL)
        --body_height;
    if (body_height < 1)
        body_height = 1;
    left_width = g_params.show_preview ?
        (frame->screen_w * g_params.panel_ratio) / 100 : frame->screen_w;
    if (left_width < 1)
        left_width = 1;
    if (left_width > frame->screen_w)
        left_width = frame->screen_w;
    right_width = frame->screen_w - left_width - (g_params.show_preview ? 1 : 0);

    list = build_list(cwd, cursor, scroll, 0, y, left_width, body_height);
    if (list == NULL || frame_add(frame, list) != 0) {
        widget_free(list);
        goto fail;
    }
    if (g_params.show_preview && right_width > 0 && frame->screen_w > 2) {
        StpWidget *separator = widget_new(STP_WIDGET_SEPARATOR, left_width, y, 1, body_height);
        StpWidget *preview;
        if (separator == NULL)
            goto fail;
        separator->style = g_theme.separator;
        separator->tag = string_duplicate("separator");
        if (separator->tag == NULL || frame_add(frame, separator) != 0) {
            widget_free(separator);
            goto fail;
        }
        preview = build_preview(frame, cwd, list, left_width + 1, y, right_width, body_height);
        if (preview == NULL || frame_add(frame, preview) != 0) {
            widget_free(preview);
            goto fail;
        }
    }
    if (search_text != NULL) {
        StpWidget *input = build_input(frame, search_text);
        if (input == NULL || frame_add(frame, input) != 0) {
            widget_free(input);
            goto fail;
        }
    }
    if (g_params.show_statusbar) {
        StpWidget *status = build_status(frame, cwd, list->item_count, status_msg);
        if (status == NULL || frame_add(frame, status) != 0) {
            widget_free(status);
            goto fail;
        }
    }
    return frame;

fail:
    stpui_frame_free(frame);
    return NULL;
}

void stpui_frame_free(UiFrame *frame)
{
    int i;
    if (frame == NULL)
        return;
    for (i = 0; i < frame->widget_count; ++i)
        widget_free(frame->widgets[i]);
    free(frame->widgets);
    free(frame);
}

static const StpWidget *find_by_tag(const UiFrame *frame, const char *tag)
{
    int i;
    if (frame == NULL || tag == NULL)
        return NULL;
    for (i = 0; i < frame->widget_count; ++i) {
        const StpWidget *widget = frame->widgets[i];
        if (widget != NULL && widget->tag != NULL && strcmp(widget->tag, tag) == 0)
            return widget;
    }
    return NULL;
}

int stpui_list_count(const UiFrame *frame, const char *tag)
{
    const StpWidget *widget = find_by_tag(frame, tag);
    return widget != NULL ? widget->item_count : 0;
}

int stpui_list_cursor(const UiFrame *frame, const char *tag)
{
    const StpWidget *widget = find_by_tag(frame, tag);
    return widget != NULL ? widget->cursor : 0;
}

int stpui_list_scroll(const UiFrame *frame, const char *tag)
{
    const StpWidget *widget = find_by_tag(frame, tag);
    return widget != NULL ? widget->scroll_offset : 0;
}
