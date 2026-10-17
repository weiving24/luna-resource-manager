#include "app.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "controller.h"
#include "state.h"

int app_run(int argc, char **argv)
{
    AppConfig config;
    AppState state;
    const char *start_directory = ".";
    const char *config_path = NULL;
    int directory_seen = 0;
    int options_finished = 0;
    int i;
    int result;

    for (i = 1; i < argc; ++i) {
        if (!options_finished && strcmp(argv[i], "--") == 0) {
            options_finished = 1;
            continue;
        }
        if (!options_finished && (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)) {
            config_print_help(argv[0]);
            return 0;
        }
        if (!options_finished && (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0)) {
            config_print_version();
            return 0;
        }
        if (!options_finished && (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0)) {
            if (i + 1 >= argc) {
                (void)fprintf(stderr, "%s: %s requires a file path\n", argv[0], argv[i]);
                return 2;
            }
            config_path = argv[++i];
            continue;
        }
        if (!options_finished && argv[i][0] == '-' && strcmp(argv[i], "-") != 0) {
            (void)fprintf(stderr, "%s: unknown option: %s\n", argv[0], argv[i]);
            (void)fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
            return 2;
        }
        if (directory_seen) {
            (void)fprintf(stderr, "%s: only one start directory may be specified\n", argv[0]);
            return 2;
        }
        start_directory = argv[i];
        directory_seen = 1;
    }

    if (config_load(&config, config_path) != 0) {
        (void)fprintf(stderr, "%s: cannot load configuration: %s\n", argv[0], strerror(errno));
        return 2;
    }
    if (state_init(&state, start_directory) != 0) {
        (void)fprintf(stderr, "%s: cannot initialize state: %s\n", argv[0], strerror(errno));
        return 1;
    }
    result = controller_run(&state, &config);
    if (result != 0) {
        (void)fprintf(stderr, "%s: cannot initialize terminal interface: %s\n",
                      argv[0], strerror(errno));
        return 1;
    }
    return 0;
}
