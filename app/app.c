#define _POSIX_C_SOURCE 200809L

#include "app.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "controller.h"
#include "state.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int instance_lock_fd = -1;

enum instance_lock_result {
    INSTANCE_LOCK_ERROR = -1,
    INSTANCE_LOCK_BUSY = 0,
    INSTANCE_LOCK_ACQUIRED = 1
};

static int try_instance_lock(const char *path)
{
    struct stat st;
    struct flock lock;
    int fd;
    int saved_errno;

    fd = open(path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0)
        return INSTANCE_LOCK_ERROR;

    if (fstat(fd, &st) != 0) {
        saved_errno = errno;
        (void)close(fd);
        errno = saved_errno;
        return INSTANCE_LOCK_ERROR;
    }

    if (!S_ISREG(st.st_mode) || st.st_uid != getuid()) {
        (void)close(fd);
        errno = EACCES;
        return INSTANCE_LOCK_ERROR;
    }

    if (fchmod(fd, S_IRUSR | S_IWUSR) != 0) {
        saved_errno = errno;
        (void)close(fd);
        errno = saved_errno;
        return INSTANCE_LOCK_ERROR;
    }

    if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        saved_errno = errno;
        (void)close(fd);
        errno = saved_errno;
        return INSTANCE_LOCK_ERROR;
    }

    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(fd, F_SETLK, &lock) == -1) {
        saved_errno = errno;
        (void)close(fd);

        if (saved_errno == EACCES || saved_errno == EAGAIN)
            return INSTANCE_LOCK_BUSY;

        errno = saved_errno;
        return INSTANCE_LOCK_ERROR;
    }

    instance_lock_fd = fd;
    return INSTANCE_LOCK_ACQUIRED;
}

static int try_instance_lock_in_dir(const char *dir,
                                    const char *filename,
                                    int require_private)
{
    char path[PATH_MAX];
    struct stat st;
    int n;

    if (dir == NULL || dir[0] != '/') {
        errno = EINVAL;
        return INSTANCE_LOCK_ERROR;
    }

    if (stat(dir, &st) != 0)
        return INSTANCE_LOCK_ERROR;

    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return INSTANCE_LOCK_ERROR;
    }

    if (require_private &&
        (st.st_uid != getuid() || (st.st_mode & 077) != 0)) {
        errno = EACCES;
        return INSTANCE_LOCK_ERROR;
    }

    if (access(dir, W_OK | X_OK) != 0)
        return INSTANCE_LOCK_ERROR;

    n = snprintf(path, sizeof(path), "%s/%s", dir, filename);
    if (n < 0 || (size_t)n >= sizeof(path)) {
        errno = ENAMETOOLONG;
        return INSTANCE_LOCK_ERROR;
    }

    return try_instance_lock(path);
}

static int acquire_instance_lock(void)
{
    const char *home = getenv("HOME");
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    const char *tmpdir = getenv("TMPDIR");
    char filename[96];
    int result;
    int saved_errno = EACCES;

    if (instance_lock_fd >= 0)
        return INSTANCE_LOCK_ACQUIRED;

    /*
     * Prefer HOME so launches from different shells use the same
     * lock file, including typical Android/Termux environments.
     */
    if (home != NULL && home[0] == '/') {
        struct stat st;

        if (stat(home, &st) == 0 &&
            S_ISDIR(st.st_mode) &&
            st.st_uid == getuid()) {
            result = try_instance_lock_in_dir(
                home,
                ".luna-resource-manager.lock",
                0
            );

            if (result != INSTANCE_LOCK_ERROR)
                return result;

            saved_errno = errno;
        }
    }

    /*
     * XDG_RUNTIME_DIR is accepted only when it belongs to this user
     * and has no group/other permission bits set.
     */
    if (runtime_dir != NULL && runtime_dir[0] == '/') {
        result = try_instance_lock_in_dir(
            runtime_dir,
            "luna-resource-manager.lock",
            1
        );

        if (result != INSTANCE_LOCK_ERROR)
            return result;

        saved_errno = errno;
    }

    (void)snprintf(
        filename,
        sizeof(filename),
        "luna-resource-manager-%lu.lock",
        (unsigned long)getuid()
    );

    if (tmpdir != NULL && tmpdir[0] == '/') {
        result = try_instance_lock_in_dir(
            tmpdir,
            filename,
            0
        );

        if (result != INSTANCE_LOCK_ERROR)
            return result;

        saved_errno = errno;
    }

    result = try_instance_lock_in_dir("/tmp", filename, 0);
    if (result != INSTANCE_LOCK_ERROR)
        return result;

    saved_errno = errno;
    errno = saved_errno;
    return INSTANCE_LOCK_ERROR;
}

static void release_instance_lock(void)
{
    if (instance_lock_fd >= 0) {
        (void)close(instance_lock_fd);
        instance_lock_fd = -1;
    }
}

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
    int lock_result;

    for (i = 1; i < argc; ++i) {
        if (!options_finished && strcmp(argv[i], "--") == 0) {
            options_finished = 1;
            continue;
        }

        if (!options_finished &&
            (strcmp(argv[i], "-h") == 0 ||
             strcmp(argv[i], "--help") == 0)) {
            config_print_help(argv[0]);
            return 0;
        }

        if (!options_finished &&
            (strcmp(argv[i], "-V") == 0 ||
             strcmp(argv[i], "--version") == 0)) {
            config_print_version();
            return 0;
        }

        if (!options_finished &&
            (strcmp(argv[i], "-c") == 0 ||
             strcmp(argv[i], "--config") == 0)) {
            if (i + 1 >= argc) {
                (void)fprintf(
                    stderr,
                    "%s: %s requires a file path\n",
                    argv[0],
                    argv[i]
                );
                return 2;
            }

            config_path = argv[++i];
            continue;
        }

        if (!options_finished &&
            argv[i][0] == '-' &&
            strcmp(argv[i], "-") != 0) {
            (void)fprintf(
                stderr,
                "%s: unknown option: %s\n",
                argv[0],
                argv[i]
            );

            (void)fprintf(
                stderr,
                "Try '%s --help' for more information.\n",
                argv[0]
            );

            return 2;
        }

        if (directory_seen) {
            (void)fprintf(
                stderr,
                "%s: only one start directory may be specified\n",
                argv[0]
            );
            return 2;
        }

        start_directory = argv[i];
        directory_seen = 1;
    }

    if (config_load(&config, config_path) != 0) {
        (void)fprintf(
            stderr,
            "%s: cannot load configuration: %s\n",
            argv[0],
            strerror(errno)
        );
        return 2;
    }

    lock_result = acquire_instance_lock();

    if (lock_result == INSTANCE_LOCK_BUSY) {
        (void)fprintf(
            stderr,
            "%s: Luna Resource Manager is already running; "
            "no additional instance was started.\n",
            argv[0]
        );
        return 0;
    }

    if (lock_result == INSTANCE_LOCK_ERROR) {
        (void)fprintf(
            stderr,
            "%s: cannot acquire single-instance lock: %s\n",
            argv[0],
            strerror(errno)
        );
        return 1;
    }

    if (state_init(&state, start_directory) != 0) {
        int saved_errno = errno;

        (void)fprintf(
            stderr,
            "%s: cannot initialize state: %s\n",
            argv[0],
            strerror(saved_errno)
        );

        release_instance_lock();
        return 1;
    }

    result = controller_run(&state, &config);

    if (result != 0) {
        int saved_errno = errno;

        (void)fprintf(
            stderr,
            "%s: cannot initialize terminal interface: %s\n",
            argv[0],
            strerror(saved_errno)
        );

        release_instance_lock();
        return 1;
    }

    release_instance_lock();
    return 0;
}