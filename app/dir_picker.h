#ifndef LUNA_RESOURCE_MANAGER_DIR_PICKER_H
#define LUNA_RESOURCE_MANAGER_DIR_PICKER_H

#include <stddef.h>

int dir_picker_select(const char *start_directory, char *output, size_t output_size);

#endif
