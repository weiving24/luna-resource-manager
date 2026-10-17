#ifndef LUNA_RESOURCE_MANAGER_CALLBACKS_H
#define LUNA_RESOURCE_MANAGER_CALLBACKS_H

#include "state.h"

void callbacks_bind(AppState *state);
int cb_list_dir(const char *path, StpListItem **items, int *count);
void cb_free_items(StpListItem *items, int count);
const char *cb_get_cwd(void);
const char *cb_get_time(void);
int cb_read_preview(const char *path, char **buf, int *len);

#endif
