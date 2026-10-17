#include "callbacks.h"

#include <time.h>

#include "fs.h"

static AppState *g_state;

void callbacks_bind(AppState *state)
{
    g_state = state;
}

int cb_list_dir(const char *path, StpListItem **items, int *count)
{
    return fs_list_dir(path, items, count);
}

void cb_free_items(StpListItem *items, int count)
{
    fs_free_items(items, count);
}

const char *cb_get_cwd(void)
{
    return g_state != NULL ? g_state->cwd : "/";
}

const char *cb_get_time(void)
{
    static char buffer[16];
    time_t now = time(NULL);
    struct tm broken_down;
    if (now == (time_t)-1 || localtime_r(&now, &broken_down) == NULL ||
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &broken_down) == 0)
        buffer[0] = '\0';
    return buffer;
}

int cb_read_preview(const char *path, char **buf, int *len)
{
    return fs_read_preview(path, buf, len);
}
