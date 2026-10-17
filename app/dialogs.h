#ifndef LUNA_RESOURCE_MANAGER_DIALOGS_H
#define LUNA_RESOURCE_MANAGER_DIALOGS_H

#include <stddef.h>

int dialog_input(const char *title, const char *prompt, char *output, size_t capacity);
int dialog_confirm(const char *title, const char *message);
void dialog_message(const char *title, const char *message);
void dialog_status(const char *message);
void dialog_text_view(const char *title, const char *text);

#endif
