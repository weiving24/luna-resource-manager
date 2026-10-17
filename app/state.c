#include "state.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "callbacks.h"
#include "fs.h"
#include "open.h"

int state_init(AppState *state, const char *start_path)
{
    if (state == NULL) {
        errno = EINVAL;
        return -1;
    }
    memset(state, 0, sizeof(*state));
    if (state_set_cwd(state, start_path != NULL ? start_path : ".") != 0) {
        int saved_errno = errno;
        if (getcwd(state->cwd, sizeof(state->cwd)) == NULL)
            return -1;
        (void)snprintf(state->status, sizeof(state->status),
                       "Cannot open '%s': %s", start_path != NULL ? start_path : ".",
                       strerror(saved_errno));
    } else {
        (void)snprintf(state->status, sizeof(state->status), "Ready");
    }
    state->running = 1;
    return 0;
}

int state_set_cwd(AppState *state, const char *path)
{
    char resolved[PATH_MAX];
    if (state == NULL || path == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (realpath(path, resolved) == NULL)
        return -1;
    if (!fs_is_dir(resolved)) {
        errno = ENOTDIR;
        return -1;
    }
    if (strlen(resolved) >= sizeof(state->cwd)) {
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(state->cwd, resolved);
    state->cursor = 0;
    state->scroll = 0;
    return 0;
}

void state_enter(AppState *state)
{
    StpListItem *items = NULL;
    int count = 0;
    char path[PATH_MAX];

    if (state == NULL || fs_list_dir(state->cwd, &items, &count) != 0) {
        if (state != NULL)
            (void)snprintf(state->status, sizeof(state->status),
                           "Cannot list directory: %s", strerror(errno));
        return;
    }
    if (state->cursor < 0 || state->cursor >= count) {
        fs_free_items(items, count);
        return;
    }
    if (fs_join(path, sizeof(path), state->cwd, items[state->cursor].text) != 0) {
        (void)snprintf(state->status, sizeof(state->status), "Path is too long");
        fs_free_items(items, count);
        return;
    }
    if (items[state->cursor].is_dir) {
        if (!fs_can_access(path)) {
            (void)snprintf(state->status, sizeof(state->status),
                           "Cannot enter %s: %s", items[state->cursor].text,
                           strerror(errno));
        } else if (state_set_cwd(state, path) != 0) {
            (void)snprintf(state->status, sizeof(state->status),
                           "Cannot enter directory: %s", strerror(errno));
        } else {
            (void)snprintf(state->status, sizeof(state->status),
                           "Entered %.300s", items[state->cursor].text);
        }
        fs_free_items(items, count);
        return;
    }
    fs_free_items(items, count);
    action_menu_interactive(path, state->cwd, state);
}

void state_back(AppState *state)
{
    char parent[PATH_MAX];
    char *slash;
    if (state == NULL || strcmp(state->cwd, "/") == 0)
        return;
    if (strlen(state->cwd) >= sizeof(parent))
        return;
    strcpy(parent, state->cwd);
    slash = strrchr(parent, '/');
    if (slash == NULL)
        strcpy(parent, ".");
    else if (slash == parent)
        parent[1] = '\0';
    else
        *slash = '\0';
    if (state_set_cwd(state, parent) != 0)
        (void)snprintf(state->status, sizeof(state->status),
                       "Cannot go to parent: %s", strerror(errno));
}
