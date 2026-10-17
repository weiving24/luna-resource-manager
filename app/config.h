#ifndef LUNA_RESOURCE_MANAGER_CONFIG_H
#define LUNA_RESOURCE_MANAGER_CONFIG_H

typedef struct {
    int panel_ratio;
    int show_header;
    int show_statusbar;
    int show_preview;
    char theme[16];
    char config_path[4096];
} AppConfig;

void config_defaults(AppConfig *config);
int config_load(AppConfig *config, const char *explicit_path);
void config_apply_theme(const AppConfig *config);
void config_print_help(const char *program);
void config_print_version(void);

#endif
