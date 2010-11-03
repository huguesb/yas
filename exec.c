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

#include "memory.h"
#include "command.h"
#include "dstring.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <glob.h>
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

void exec_internal(command_t *command);

char* eval_argument(argument_t *argument) {
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
            // empty string for non-existent variables
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
            exec_internal(argument_get_command(argument));
            // no possible return
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
            char *tmp = eval_argument(*l);
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

typedef struct {
    size_t n;
    size_t a;
    char **d;
} argv_t;

void argv_grow(argv_t *argv, size_t n) {
    if (argv->a - argv->n > n)
        return;
    argv->a *= 2;
    argv->d = (char**)yas_realloc(argv->d, argv->a * sizeof(char*));
}

argv_t* argv_new() {
    argv_t *argv = (argv_t*)yas_malloc(sizeof(argv_t));
    argv->n = 0;
    argv->a = 1;
    argv->d = (char**)yas_malloc(argv->a * sizeof(char*));
    argv->d[0] = 0;
    return argv;
}

void argv_destroy(argv_t *argv) {
    yas_free(argv->d);
    yas_free(argv);
}

void argv_add(argv_t *argv, const char *s) {
//     fprintf(stderr, "=> %s\n", s);
    argv_grow(argv, 1);
    argv->d[argv->n] = (char*)yas_malloc((strlen(s)+1) * sizeof(char));
    strcpy(argv->d[argv->n], s);
    argv->d[++argv->n] = 0;
}

void argv_add_split(argv_t *argv, const char *s) {
    // space split & glob expansion
//     fprintf(stderr, "expanding %s\n", s);
    const char *last = s, *current = s;
    do {
        if (isspace(*current) || !*current) {
            if (current != last) {
                char *tmp = (char*)yas_malloc((current - last + 1) * sizeof(char));
                strncpy(tmp, last, current - last);
                tmp[current - last] = 0;
                
                glob_t globs;
                int err = glob(tmp,
                                GLOB_NOMAGIC | GLOB_TILDE_CHECK | GLOB_ERR,
                                NULL, &globs);
                if (err) {
                    fprintf(stderr, "Wildcard/tilde expansion failed.\n");
                    fprintf(stderr, "%s\n", tmp);
                    exit(1);
                }
//                 fprintf(stderr, "expanded %s into %u parts\n", tmp, globs.gl_pathc);
                for (size_t j = 0; j < globs.gl_pathc; ++j)
                    argv_add(argv, globs.gl_pathv[j]);
                globfree(&globs);
                yas_free(tmp);
            }
            last = current + 1;
        }
    } while (*(current++));
}

void argv_inspect(argv_t *argv) {
    for (size_t i = 0; i < argv->n; ++i)
        fprintf(stderr, "%s\n", argv->d[i]);
}

int argv_eval(argv_t *argv, command_t *command) {
    const size_t n = command_argc(command);
    argument_t **d = command_argv(command);
    for (size_t i = 0; i < n; ++i) {
        char *s = eval_argument(d[i]);
        if (s == NULL) {
            fprintf(stderr, "Argument evaluation failed.\n");
            argument_inspect(d[i], 0);
            return 1;
        }
        if (argument_flags(d[i]) & ARGTYPE_QUOTED)
            argv_add(argv, s);
        else
            argv_add_split(argv, s);
    }
    return 0;
}

int exec_setup_redir(command_t *command) {
    if (command_redir_in(command)) {
        char *s = eval_argument(command_redir_in(command));
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
        char *s = eval_argument(command_redir_out(command));
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

int exec_builtin(argv_t *argv) {
    // try builtin commands
    if (!strcmp(argv->d[0], "cd")) {
        if (argv->n) {
            if (chdir(argv->d[1]))
                fprintf(stderr, "No such directory : %s\n", argv->d[1]);
        } else {
            // TODO : find home dir
            chdir("");
        }
        return 0;
    } else if (!strcmp(argv->d[0], "exit")) {
        exit(0);
    }
    return 1;
}

void exec_internal(command_t *command) {
    argv_t *argv = argv_new();
    if (argv_eval(argv, command))
        exit(1);
    if (exec_setup_redir(command))
        exit(1);
    if (!exec_builtin(argv))
        exit(0);
    // exec external command
    int errcode = execvp(argv->d[0], argv->d);
    fprintf(stderr, "Command not found: %s\n", argv->d[0]);
    exit(errcode);
}

void exec_pipechain(command_t *command) {
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
            // exec_internal never returns...
            exec_internal(argument_get_command(d[i]));
        }
        if (command_is_background(argument_get_command(d[i]))) {
            pid[i] = 0;
            // TODO: add to list?
        }
        close(fd[1]);
        pfd = fd[0];
        ++i;
    }
    for (i = 0; i < n; ++i)
        if (pid[i])
            waitpid(pid[i], NULL, 0);
}

void exec_command(command_t *command) {
    if (command_is_pipechain(command)) {
        exec_pipechain(command);
    } else {
        argv_t *argv = argv_new();
        if (argv_eval(argv, command)) {
            argv_destroy(argv);
            return;
        }
        if (!exec_builtin(argv)) {
            argv_destroy(argv);
        } else {
            pid_t pid = fork();
            if (pid) {
                argv_destroy(argv);
                if (command_is_background(command)) {
                    
                } else {
                    waitpid(pid, NULL, 0);
                }
            } else {
                if (exec_setup_redir(command))
                    exit(1);
                int errcode = execvp(argv->d[0], argv->d);
                fprintf(stderr, "Command not found: %s\n", argv->d[0]);
                exit(errcode);
            }
        }
    }
}
