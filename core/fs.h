#ifndef LUNA_RESOURCE_MANAGER_FS_H
#define LUNA_RESOURCE_MANAGER_FS_H

#include <stddef.h>
#include "stpui.h"

int fs_list_dir(const char *path, StpListItem **items, int *count);
void fs_free_items(StpListItem *items, int count);
int fs_is_dir(const char *path);
int fs_join(char *out, size_t out_size, const char *base, const char *name);
int fs_can_access(const char *path);
int fs_read_preview(const char *path, char **buf, int *len);

#endif
