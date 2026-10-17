#include "process.h"

#include <curses.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define CAPTURE_LIMIT (16U * 1024U * 1024U)

static int wait_for_child(pid_t pid, int *status)
{
    pid_t result;
    do {
        result = waitpid(pid, status, 0);
    } while (result < 0 && errno == EINTR);
    return result < 0 ? -1 : 0;
}

static void restore_terminal(void)
{
    (void)refresh();
    (void)clearok(stdscr, TRUE);
}

int process_run_external(const char *program, const char *argument)
{
    pid_t child;
    int status = 0;
    if (program == NULL || program[0] == '\0') {
        errno = EINVAL;
        return -1;
    }
    (void)endwin();
    child = fork();
    if (child == 0) {
        if (argument != NULL)
            execlp(program, program, argument, (char *)NULL);
        else
            execlp(program, program, (char *)NULL);
        _exit(127);
    }
    if (child < 0) {
        int saved_errno = errno;
        restore_terminal();
        errno = saved_errno;
        return -1;
    }
    if (wait_for_child(child, &status) != 0) {
        int saved_errno = errno;
        restore_terminal();
        errno = saved_errno;
        return -1;
    }
    restore_terminal();
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) {
        errno = EINTR;
        return 128 + WTERMSIG(status);
    }
    return -1;
}

int process_run_file_capture(const char *path, char **output, size_t *length,
                             int *exit_code)
{
    int descriptors[2];
    pid_t child;
    char *buffer;
    size_t used = 0;
    size_t capacity = 4096;
    int status = 0;
    int truncated = 0;
    int saved_errno = 0;

    if (path == NULL || output == NULL || length == NULL || exit_code == NULL) {
        errno = EINVAL;
        return -1;
    }
    *output = NULL;
    *length = 0;
    *exit_code = -1;
    if (pipe(descriptors) != 0)
        return -1;

    (void)endwin();
    child = fork();
    if (child < 0) {
        saved_errno = errno;
        (void)close(descriptors[0]);
        (void)close(descriptors[1]);
        restore_terminal();
        errno = saved_errno;
        return -1;
    }
    if (child == 0) {
        (void)close(descriptors[0]);
        if (dup2(descriptors[1], STDOUT_FILENO) < 0 ||
            dup2(descriptors[1], STDERR_FILENO) < 0)
            _exit(126);
        (void)close(descriptors[1]);
        execl(path, path, (char *)NULL);
        if (errno == ENOEXEC)
            execlp("sh", "sh", path, (char *)NULL);
        (void)dprintf(STDERR_FILENO, "Cannot execute %s: %s\n", path, strerror(errno));
        _exit(127);
    }

    (void)close(descriptors[1]);
    buffer = malloc(capacity + 1);
    if (buffer == NULL) {
        saved_errno = errno;
        (void)close(descriptors[0]);
        (void)wait_for_child(child, &status);
        restore_terminal();
        errno = saved_errno;
        return -1;
    }
    for (;;) {
        char chunk[8192];
        ssize_t received = read(descriptors[0], chunk, sizeof(chunk));
        if (received == 0)
            break;
        if (received < 0) {
            if (errno == EINTR)
                continue;
            saved_errno = errno;
            break;
        }
        if (used < CAPTURE_LIMIT) {
            size_t room = CAPTURE_LIMIT - used;
            size_t copy_length = (size_t)received < room ? (size_t)received : room;
            if (used + copy_length + 1 > capacity) {
                size_t next = capacity;
                char *grown;
                while (next < used + copy_length + 1) {
                    if (next > CAPTURE_LIMIT / 2) {
                        next = CAPTURE_LIMIT + 1;
                        break;
                    }
                    next *= 2;
                }
                if (next > CAPTURE_LIMIT + 1)
                    next = CAPTURE_LIMIT + 1;
                grown = realloc(buffer, next);
                if (grown == NULL) {
                    saved_errno = errno != 0 ? errno : ENOMEM;
                    break;
                }
                buffer = grown;
                capacity = next - 1;
            }
            memcpy(buffer + used, chunk, copy_length);
            used += copy_length;
            buffer[used] = '\0';
            if (copy_length < (size_t)received)
                truncated = 1;
        } else {
            truncated = 1;
        }
    }
    (void)close(descriptors[0]);
    if (wait_for_child(child, &status) != 0 && saved_errno == 0)
        saved_errno = errno;
    restore_terminal();

    if (saved_errno != 0) {
        free(buffer);
        errno = saved_errno;
        return -1;
    }
    if (truncated) {
        static const char marker[] = "\n[output truncated at 16 MiB]\n";
        size_t marker_length = sizeof(marker) - 1;
        if (used + marker_length + 1 > capacity) {
            char *grown = realloc(buffer, used + marker_length + 1);
            if (grown != NULL) {
                buffer = grown;
                capacity = used + marker_length;
            }
        }
        if (used + marker_length + 1 <= capacity + 1) {
            memcpy(buffer + used, marker, marker_length);
            used += marker_length;
            buffer[used] = '\0';
        }
    }
    *exit_code = WIFEXITED(status) ? WEXITSTATUS(status) :
                 (WIFSIGNALED(status) ? 128 + WTERMSIG(status) : -1);
    *output = buffer;
    *length = used;
    return 0;
}
