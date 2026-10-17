#include "controller.h"

#include <curses.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "callbacks.h"
#include "archive_preview.h"
#include "fileops.h"
#include "fs.h"
#include "open.h"
#include "prt/stpprt.h"
#include "uidl/stpui_ui.h"

static int view_rows(const AppConfig *config)
{
    int width, height;
    int rows;
    stpprt_get_size(&width, &height);
    (void)width;
    rows = height - (config->show_header ? 1 : 0) -
           (config->show_statusbar ? 1 : 0);
    return rows > 1 ? rows : 1;
}

static void adjust_scroll(AppState *state, int count, int visible_rows)
{
    int maximum_scroll;
    if (count <= 0) {
        state->cursor = 0;
        state->scroll = 0;
        return;
    }
    if (state->cursor < 0)
        state->cursor = 0;
    if (state->cursor >= count)
        state->cursor = count - 1;
    if (state->cursor < state->scroll)
        state->scroll = state->cursor;
    else if (state->cursor >= state->scroll + visible_rows)
        state->scroll = state->cursor - visible_rows + 1;
    maximum_scroll = count - visible_rows;
    if (maximum_scroll < 0)
        maximum_scroll = 0;
    if (state->scroll > maximum_scroll)
        state->scroll = maximum_scroll;
    if (state->scroll < 0)
        state->scroll = 0;
}

static void selected_item_path(AppState *state, StpListItem *items, int count,
                               char *path, size_t path_size)
{
    if (state->cursor < 0 || state->cursor >= count) {
        errno = ENOENT;
        path[0] = '\0';
        return;
    }
    if (items[state->cursor].text == NULL ||
        strcmp(items[state->cursor].text, ".") == 0 ||
        strcmp(items[state->cursor].text, "..") == 0) {
        (void)snprintf(state->status, sizeof(state->status),
                       "Navigation entries cannot be modified");
        path[0] = '\0';
        return;
    }
    if (fs_join(path, path_size, state->cwd, items[state->cursor].text) != 0)
        path[0] = '\0';
}

static void handle_key(AppState *state, int key, const AppConfig *config)
{
    StpListItem *items = NULL;
    int count = 0;
    int rows = view_rows(config);
    char path[PATH_MAX];

    if (fs_list_dir(state->cwd, &items, &count) != 0) {
        (void)snprintf(state->status, sizeof(state->status),
                       "Cannot list directory: %s", strerror(errno));
        return;
    }
    if (key == KEY_MOUSE) {
#ifdef NCURSES_MOUSE_VERSION
        MEVENT event;
        if (getmouse(&event) == OK && (event.bstate & BUTTON3_PRESSED)) {
            selected_item_path(state, items, count, path, sizeof(path));
            if (path[0] != '\0') {
                if (archive_preview_is_supported(path))
                    archive_preview_open(path);
                else
                    action_menu_interactive(path, state->cwd, state);
            }
        }
#endif
        fs_free_items(items, count);
        return;
    }
    switch (key) {
    case 'q':
    case 'Q':
        state->running = 0;
        break;
    case 'a':
    case 'A':
        show_about_dialog();
        break;
    case '?':
    case KEY_F(1):
        show_help_dialog();
        break;
    case KEY_UP:
        if (state->cursor > 0)
            --state->cursor;
        break;
    case KEY_DOWN:
        if (state->cursor < count - 1)
            ++state->cursor;
        break;
    case KEY_PPAGE:
        state->cursor -= rows;
        if (state->cursor < 0)
            state->cursor = 0;
        break;
    case KEY_NPAGE:
        if (count <= 0) {
            state->cursor = 0;
        } else if (state->cursor >= count - rows) {
            state->cursor = count - 1;
        } else {
            state->cursor += rows;
        }
        break;
    case KEY_HOME:
        state->cursor = 0;
        break;
    case KEY_END:
        state->cursor = count > 0 ? count - 1 : 0;
        break;
    case KEY_LEFT:
        state_back(state);
        break;
    case KEY_RIGHT:
    case '\n':
    case '\r':
#ifdef KEY_ENTER
    case KEY_ENTER:
#endif
        state_enter(state);
        break;
    case KEY_F(2):
        selected_item_path(state, items, count, path, sizeof(path));
        if (path[0] != '\0')
            action_menu_interactive(path, state->cwd, state);
        break;
    case 'n':
    case 'N':
        fileops_newdir(state, state->cwd);
        break;
    default:
        break;
    }
    adjust_scroll(state, count, rows);
    fs_free_items(items, count);
}

int controller_run(AppState *state, const AppConfig *config)
{
    int width, height;
    LayoutParams layout;
    SetupCallbacks callbacks;

    if (state == NULL || config == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (stpprt_init() != 0)
        return -1;
    stpprt_get_size(&width, &height);
    layout.screen_w = width;
    layout.screen_h = height;
    layout.panel_ratio = config->panel_ratio;
    layout.show_header = config->show_header;
    layout.show_statusbar = config->show_statusbar;
    layout.show_preview = config->show_preview;

    callbacks.list_dir = cb_list_dir;
    callbacks.free_items = cb_free_items;
    callbacks.get_cwd = cb_get_cwd;
    callbacks.get_time = cb_get_time;
    callbacks.read_file_preview = cb_read_preview;
    callbacks_bind(state);
    if (stpui_init(&layout, &callbacks) != 0) {
        stpprt_shutdown();
        return -1;
    }
    config_apply_theme(config);

    while (state->running) {
        UiFrame *frame;
        StpListItem *items = NULL;
        int count = 0;
        int new_width, new_height;

        stpprt_get_size(&new_width, &new_height);
        if (new_width != layout.screen_w || new_height != layout.screen_h) {
            layout.screen_w = new_width;
            layout.screen_h = new_height;
            stpui_set_layout(&layout);
            stpui_resize(new_width, new_height);
        }
        if (fs_list_dir(state->cwd, &items, &count) == 0) {
            adjust_scroll(state, count, view_rows(config));
            fs_free_items(items, count);
        }
        frame = stpui_build_frame(state->cwd, state->cursor, state->scroll,
                                  state->search[0] != '\0' ? state->search : NULL,
                                  state->status);
        if (frame != NULL) {
            stpprt_draw_frame(frame);
            stpui_frame_free(frame);
        } else {
            (void)snprintf(state->status, sizeof(state->status),
                           "Cannot render screen: %s", strerror(errno));
        }
        handle_key(state, stpprt_read_key(), config);
    }

    stpui_shutdown();
    stpprt_shutdown();
    return 0;
}
