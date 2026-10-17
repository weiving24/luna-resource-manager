#ifndef LUNA_RESOURCE_MANAGER_PROCESS_H
#define LUNA_RESOURCE_MANAGER_PROCESS_H

#include <stddef.h>

int process_run_external(const char *program, const char *argument);
int process_run_file_capture(const char *path, char **output, size_t *length,
                             int *exit_code);

#endif
