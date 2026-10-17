#ifndef LUNA_RESOURCE_MANAGER_FILEOPS_H
#define LUNA_RESOURCE_MANAGER_FILEOPS_H

#include "state.h"

void fileops_copy(AppState *state, const char *src, const char *start_dir);
void fileops_move(AppState *state, const char *src, const char *start_dir);
void fileops_delete(AppState *state, const char *path);
void fileops_rename(AppState *state, const char *src);
void fileops_newdir(AppState *state, const char *dir);

#endif
