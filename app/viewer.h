#ifndef LUNA_RESOURCE_MANAGER_VIEWER_H
#define LUNA_RESOURCE_MANAGER_VIEWER_H

#include <stddef.h>

void viewer_text_file(const char *path);
void viewer_output(const char *title, const char *text, size_t length, int exit_code);

#endif
