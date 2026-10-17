#ifndef LUNA_RESOURCE_MANAGER_OPEN_H
#define LUNA_RESOURCE_MANAGER_OPEN_H

#include "state.h"

void action_menu_interactive(const char *path, const char *start_dir, AppState *state);
void open_file_interactive(const char *path);
void show_about_dialog(void);
void show_help_dialog(void);

#endif
