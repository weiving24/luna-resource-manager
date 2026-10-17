#ifndef LUNA_RESOURCE_MANAGER_MENU_H
#define LUNA_RESOURCE_MANAGER_MENU_H

typedef struct {
    const char *label;
    const char *value;
} MenuOption;

int menu_select(const char *title, const char *subtitle,
                const MenuOption *options, int count);

#endif
