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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void exec_internal(command_t *command);

char* eval_argument(argument_t *argument) {
    char *val = NULL;
    int type = argument_type(argument);
    if (type == ARGTYPE_STRING) {
        char *s = argument_get_string(argument);
        val = (char*)yas_malloc((strlen(s) + 1) * sizeof(char));
        strcpy(val, s);
    } else if (type == ARGTYPE_VARIABLE) {
        
    } else if (type == ARGTYPE_COMMAND) {
        int fd[2];
        if (pipe(fd)) {
            printf("Unable to open pipe.\n");
            return NULL;
        }
        pid_t pid = fork();
	switch (pid) {
	case -1:
            printf("Unable to fork.\n");
            return NULL;
	case 0:
            dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            close(fd[1]);
            exec_internal(argument_get_command(argument));
	    // no possible return
	default:
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
                val = NULL;
            }
        }
    }
    return val;
}

void exec_internal(command_t *command) {
    const size_t argc = command_argc(command);
    argument_t **argv = command_argv(command);
    char **str_argv = (char**)yas_malloc((argc + 1) * sizeof(char*));
    str_argv[argc] = NULL;
    size_t i;
    for (i = 0; i < argc; ++i) {
        char *s = eval_argument(argv[i]);
        if (s == NULL)
            break;
        str_argv[i] = s;
    }
    int errcode = i == argc ? execvp(*str_argv, str_argv) : 0;
    printf("Unable to start. \n");
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
            printf("unable to open pipe...\n");
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
        close(fd[1]);
        pfd = fd[0];
        ++i;
    }
    for (i = 0; i < n; ++i)
        waitpid(pid[i], NULL, 0);
}

void exec_command(command_t *command) {
    if (command_is_pipechain(command)) {
        exec_pipechain(command);
    } else {
        pid_t pid = fork();
        if (pid) {
            waitpid(pid, NULL, 0);
        } else {
            exec_internal(command);
        }
    }
}
