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

#include "exec.h"

/*!
    \file exec.c
    \brief Implementation of command execution
*/

#include "memory.h"
#include "command.h"
#include "dstring.h"
#include "argv.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

char* get_pwd() {
    char *buffer = 0;
    size_t size = 16;
    do {
        size *= 2;
        buffer = (char*)yas_realloc(buffer, size * sizeof(char));
        getcwd(buffer, size);
    } while (errno == ERANGE);
    if (errno) {
        yas_free(buffer);
        buffer = 0;
    }
    return buffer;
}

typedef struct {
    task_list_t *tasklist;
} exec_context_t;

char* eval_argument(argument_t *argument, exec_context_t *cxt);
int argv_eval(argv_t *argv, command_t *command, exec_context_t *cxt);
int exec_setup_redir(command_t *command, exec_context_t *cxt);
void exec_internal(command_t *command, exec_context_t *cxt);
void exec_pipechain(command_t *command, exec_context_t *cxt);

char* eval_argument(argument_t *argument, exec_context_t *cxt) {
    char *val = 0;
    int type = argument_type(argument);
    if (type == ARGTYPE_STRING) {
        char *s = argument_get_string(argument);
        val = (char*)yas_malloc((strlen(s) + 1) * sizeof(char));
        strcpy(val, s);
    } else if (type == ARGTYPE_VARIABLE) {
        char *env = getenv(argument_get_variable(argument));
        if (env) {
            val = (char*)yas_malloc((strlen(env) + 1) * sizeof(char));
            strcpy(val, env);
        } else {
            /* empty string for non-existent variables */
            val = (char*)yas_malloc(sizeof(char));
            *val = 0;
        }
    } else if (type == ARGTYPE_COMMAND) {
        int fd[2];
        if (pipe(fd)) {
            fprintf(stderr, "Unable to open pipe.\n");
            return 0;
        }
        pid_t pid = fork();
        if (!pid) {
            dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            close(fd[1]);
            command_t *cmd = argument_get_command(argument);
            if (command_is_pipechain(cmd)) {
                exec_pipechain(cmd, cxt);
                exit(0);
            } else {
                exec_internal(cmd, cxt);
            }
            /* no possible return */
        } else if (pid == -1) {
            fprintf(stderr, "Unable to fork.\n");
            return NULL;
        }
        close(fd[1]);
        waitpid(pid, NULL, 0);
        size_t buffer_size = 0;
        while (1) {
            val = (char*)yas_realloc(val, buffer_size + 1024);
            ssize_t n = read(fd[0], val, 1024);
            buffer_size += n;
            if (n < 1024)
                break;
        }
        if (buffer_size) {
            val[buffer_size - 1] = 0;
        } else {
            yas_free(val);
            val = 0;
        }
    } else if (type == ARGTYPE_CAT) {
        string_t *s = string_new();
        argument_t **l = argument_get_arguments(argument);
        while (l && *l) {
            char *tmp = eval_argument(*l, cxt);
            if (!tmp) {
                string_destroy(s);
                return 0;
            }
            string_append_cstr(s, tmp);
            ++l;
        }
        val = string_get_cstr(s);
    }
    return val;
}


int argv_eval(argv_t *argv, command_t *command, exec_context_t *cxt) {
    const size_t n = command_argc(command);
    argument_t **d = command_argv(command);
    size_t i;
    for (i = 0; i < n; ++i) {
        char *s = eval_argument(d[i], cxt);
        if (s == NULL) {
            fprintf(stderr, "Argument evaluation failed.\n");
            argument_inspect(d[i], 0);
            return 1;
        }
        int ret = (argument_flags(d[i]) & ARGTYPE_QUOTED)
                ? argv_add(argv, s)
                : argv_add_split(argv, s);
        if (ret)
            return 1;
    }
    return 0;
}

int exec_setup_redir(command_t *command, exec_context_t *cxt) {
    if (command_redir_in(command)) {
        char *s = eval_argument(command_redir_in(command), cxt);
        if (s == NULL) {
            fprintf(stderr, "Argument evaluation failed.\n");
            argument_inspect(command_redir_in(command), 0);
            return 1;
        }
        int fd = open(s, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "Unable to read from %s.\n", s);
            return 1;
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    if (command_redir_out(command)) {
        char *s = eval_argument(command_redir_out(command), cxt);
        if (s == NULL) {
            fprintf(stderr, "Argument evaluation failed.\n");
            argument_inspect(command_redir_out(command), 0);
            return 1;
        }
        int fd = open(s, O_WRONLY | O_CREAT);
        if (fd == -1) {
            fprintf(stderr, "Unable to write into %s.\n", s);
            return 1;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    return 0;
}

int exec_builtin(argv_t *argv, exec_context_t *cxt) {
    size_t n = argv_get_argc(argv);
    char **d = argv_get_argv(argv);
    if (!n || !d)
        return 1;
    /* try builtin commands */
    if (!strcmp(*d, "cd")) {
        if (n > 1) {
            if (chdir(d[1]))
                fprintf(stderr, "No such directory : %s\n", d[1]);
        } else {
            char *homedir = get_homedir();
            if (homedir) {
                chdir(homedir);
                free(homedir);
            } else {
                fprintf(stderr, "Unable to find home directory\n");
            }
        }
        return 0;
    } else if (!strcmp(*d, "exit")) {
        exit(0);
    } else if (!strcmp(*d, "list_tasks") || !strcmp(*d, "liste_ps")) {
        size_t i, n = task_list_get_size(cxt->tasklist);
        for (i = 0; i < n; ++i)
            task_inspect(task_list_get_task(cxt->tasklist, i));
        return 0;
    }
    return 1;
}

void exec_internal(command_t *command, exec_context_t *cxt) {
    argv_t *argv = argv_new();
    if (argv_eval(argv, command, cxt))
        exit(1);
    if (exec_setup_redir(command, cxt))
        exit(1);
    if (!exec_builtin(argv, cxt))
        exit(0);
    /* exec external command */
    char **d = argv_get_argv(argv);
    int errcode = execvp(*d, d);
    fprintf(stderr, "Command not found: %s\n", *d);
    exit(errcode);
}

void exec_pipechain(command_t *command, exec_context_t *cxt) {
    size_t i = 0;
    const size_t n = command_argc(command);
    argument_t **d = command_argv(command);
    
    int fd[2], pfd = STDIN_FILENO;
    pid_t pid[n];
    
    while (i < n) {
        if (i + 1 < n && pipe(fd)) {
            fprintf(stderr, "unable to open pipe...\n");
            exit(1);
        }
        pid[i] = fork();
        if (!pid[i]) {
            dup2(pfd, STDIN_FILENO);
            close(pfd);
            if (i + 1 < n) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
            }
            /* exec_internal never returns... */
            exec_internal(argument_get_command(d[i]), cxt);
        }
        if (command_is_background(argument_get_command(d[i]))) {
            pid[i] = 0;
            /* TODO: add to list? */
        }
        close(fd[1]);
        pfd = fd[0];
        ++i;
    }
    for (i = 0; i < n; ++i)
        if (pid[i])
            waitpid(pid[i], NULL, 0);
}

void exec_command(command_t *command, task_list_t *tasklist) {
    exec_context_t cxt;
    cxt.tasklist = tasklist;
    if (command_is_pipechain(command)) {
        exec_pipechain(command, &cxt);
    } else {
        argv_t *argv = argv_new();
        if (argv_eval(argv, command, &cxt)) {
            argv_destroy(argv);
            return;
        }
        if (!exec_builtin(argv, &cxt)) {
            argv_destroy(argv);
        } else {
            task_t *task = task_new();
            pid_t pid = fork();
            if (pid) {
                if (command_is_background(command)) {
                    task_set_pid(task, pid);
                    task_set_argv(task, argv);
                    task_list_add(cxt.tasklist, task);
                    fprintf(stderr, "[%zu] %u\n", task_list_get_size(cxt.tasklist), pid);
                } else {
                    task_destroy(task);
                    argv_destroy(argv);
                    waitpid(pid, NULL, 0);
                }
            } else {
                if (exec_setup_redir(command, &cxt))
                    exit(1);
                char **d = argv_get_argv(argv);
                int errcode = execvp(*d, d);
                fprintf(stderr, "Command not found: %s\n", *d);
                exit(errcode);
            }
        }
    }
}
