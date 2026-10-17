#ifndef LUNA_RESOURCE_MANAGER_STATE_H
#define LUNA_RESOURCE_MANAGER_STATE_H

#include "stpui.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct {
    char cwd[PATH_MAX];
    int cursor;
    int scroll;
    char search[256];
    char status[512];
    int running;
} AppState;

int state_init(AppState *state, const char *start_path);
int state_set_cwd(AppState *state, const char *path);
void state_enter(AppState *state);
void state_back(AppState *state);

#endif
