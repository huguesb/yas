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

#include "command.h"

#include "memory.h"

struct _command {
    int flags;
    int argc;
    argument_t **argv;
};

enum command_flags {
    COMMAND_IS_BACKGROUND = 1,
    COMMAND_IS_PIPECHAIN = 2
};

struct _argument {
    int type;
    union {
        char *str;
        command_t *cmd;
    } d;
};

enum argument_type {
    ARGTYPE_STRING,
    ARGTYPE_COMMAND
};

command_t* command_create(const char *str, size_t sz) {
    command_t* cmd = yas_malloc(sizeof(command_t));
    cmd->flags = 0;
    cmd->argc = 0;
    cmd->argv = 0;
    return cmd;
}

void command_destroy(command_t *command) {
    if (!command)
        return;
    for (int i = 0; i < command->argc; ++i)
        argument_destroy(command->argv[i]);
    yas_free(command->argv);
    yas_free(command);
}

int command_argc(command_t *command) {
    return command ? command->argc : 0;
}

argument_t** command_argv(command_t *command) {
    return command ? command->argv : 0;
}

int command_is_pipechain(command_t *command) {
    return command ? command->flags & COMMAND_IS_PIPECHAIN : 0;
}

int command_is_background(command_t *command) {
    return command ? command->flags & COMMAND_IS_BACKGROUND : 0;
}

argument_t* argument_create(const char *str, size_t sz) {
    argument_t *argument = (argument_t*)yas_malloc(sizeof(argument_t));
    argument->type = ARGTYPE_STRING;
    argument->d.str = 0;
    return argument;
}

void argument_destroy(argument_t *argument) {
    yas_free(argument);
}

int argument_is_command(argument_t *argument) {
    return argument ? argument->type == ARGTYPE_COMMAND : 0;
}

command_t* argument_get_command(argument_t *argument) {
    return argument && argument->type == ARGTYPE_COMMAND ? argument->d.cmd : 0;
}

char* argument_get_string(argument_t *argument) {
    return argument && argument->type == ARGTYPE_STRING ? argument->d.str : 0;
}
