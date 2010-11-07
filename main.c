/*******************************************************************************
** YetAnotherShell
** Copyright (c) 2010 Hugues Bruant & Nicolas Paglieri. All rights reserved
** 
** This file may be used under the terms of the GNU General Public License
** version 3 as published by the Free Software Foundation.
** See <http://www.gnu.org/licenses/> or GPL.txt included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*******************************************************************************/

#include "memory.h"
#include "input.h"
#include "command.h"
#include "exec.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/resource.h>

int get_cpu_count() {
    FILE *f = popen("cat /proc/cpuinfo | grep processor | wc -l", "r");
    char buffer[16];
    size_t size = fread(buffer, 1, 16, f);
    pclose(f);
    return size ? atoi(buffer) : 0;
}

static const char* sigchld_reason(int code) {
    if (code == CLD_EXITED)
        return "Exited";
    else if (code == CLD_KILLED)
        return "Killed";
    else if (code == CLD_DUMPED)
        return "Dumped";
    else if (code == CLD_TRAPPED)
        return "Trapped";
    else if (code == CLD_STOPPED)
        return "Stopped";
    else if (code == CLD_CONTINUED)
        return "Continued";
    return "";
}

static struct rusage yas_ru;
static task_list_t *tasklist = 0;

static long long timeval_diff_millis(const struct timeval *current,
                                     const struct timeval *last) {
    long long diff = current->tv_sec - last->tv_sec;
    diff *= 1000;
    diff += (current->tv_usec - last->tv_usec) / 1000;
    return diff;
}

static void sigchld_handler(int sig, siginfo_t *info, void *context) {
    (void)context;
    if (sig != SIGCHLD || info->si_signo != SIGCHLD)
        return;
    size_t i;
    size_t n = task_list_get_size(tasklist);
    for (i = 0; i < n; ++i) {
        task_t *task = task_list_get_task(tasklist, i);
        if (task_get_pid(task) == info->si_pid) {
            task_list_remove(tasklist, i);
            yas_readline_pre_signal();
            struct rusage ru;
            getrusage(RUSAGE_CHILDREN, &ru);
            long long utime = timeval_diff_millis(&ru.ru_utime, &yas_ru.ru_utime);
            long long stime = timeval_diff_millis(&ru.ru_stime, &yas_ru.ru_stime);
            long long wall = task_get_elapsed_millis(task);
            yas_ru = ru;
            fprintf(stderr,
                    "[%u] %s after %lli ms [usr=%llu, sys=%llu, cpu=%.2lf%%]\n",
                    info->si_pid,
                    sigchld_reason(info->si_code),
                    wall,
                    utime,
                    stime,
                    ((double)(utime + stime) / (double)(wall * get_cpu_count())));
            fflush(stderr);
            yas_readline_post_signal();
            break;
        }
    }
}

static void install_sigchld_handler() {
    static struct sigaction act;
    act.sa_flags = SA_RESTART | SA_SIGINFO;
    act.sa_sigaction = sigchld_handler;
    if (sigaction(SIGCHLD, &act, NULL)) {
        fprintf(stderr, "Failed to install SIGCHLD handler.\n");
    }
}

/*
    trivial = empty line, line made of whitspaces, comments
*/
int is_nontrivial(const char *s) {
    if (!s)
        return 0;
    while (*s) {
        if (!isspace(*s))
            return *s != '#';
        ++s;
    }
    return 0;
}

int main(int argc, char **argv) {
    (void) argc;
    (void) argv;
    getrusage(RUSAGE_CHILDREN, &yas_ru);
    tasklist = task_list_new();
    install_sigchld_handler();
    int eof = 0;
    while (!eof) {
        char *line = yas_readline("yas> ", &eof);
        if (is_nontrivial(line)) {
            size_t line_sz = strlen(line);
            command_t *command = command_create(line, line_sz);
            yas_free(line);
            if (!command) {
                size_t i, n = command_error_position() + 5;
                for (i = 0; i < n; ++i) 
                    fprintf(stderr, " ");
                fprintf(stderr, "^\nsyntax error @ %zu : ", command_error_position());
                fprintf(stderr, "%s\n", command_error_string());
            } else {
                /* command_inspect(command, 0); */
                exec_command(command, tasklist);
                command_destroy(command);
            }
        }
    }
    return 0;
}
