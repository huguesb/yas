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

#include <stdio.h>
#include <stdarg.h>

void indent_printf(size_t indent, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    for (size_t i = 0; i < indent; ++i)
        fputc(' ', stdout);
    vprintf(fmt, va);
    va_end(va);
}

/*
    Command grammar (external textual representation) :
    command_line = command ( '|' command )*
    command = argument+ ( '<' argument )? ( '>' argument )? '&'?
    argument = string | '$' '(' command ')' | '$' variable
    string = '"' ([^"] | '\' '"')* '"' | ([^"<>|&] | '\' ["<>|&])+
*/

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

static char* stripped(const char *src, size_t length) {
    char *dst = yas_malloc((length + 1) * sizeof(char));
    int idx = 0;
    int quoted = 0, escaped = 0;
    while (length && *src <= ' ') {
        --length;
        ++src;
    }
    while (length) {
        if (*src == '\"' && !escaped) {
            quoted = !quoted;
        } else if (*src == '\\' && !escaped) {
            escaped = 1;
        } else {
            escaped = 0;
            dst[idx++] = *src;
        }
        --length;
        ++src;
    }
    while (idx > 0 && dst[idx - 1] <= ' ')
        --idx;
    dst[idx] = 0;
    return dst;
}

static size_t skip_ws(const char *str, size_t sz) {
    size_t n = 0;
    while (n < sz && str[n] <= ' ')
        ++n;
    return n;
}

static size_t find_next_argument(const char *str, size_t sz) {
    size_t n = 0;
    n += skip_ws(str, sz);
    if (n < sz && str[n] == '|')
        ++n;
    if (n < sz)
        n += skip_ws(str + n, sz - n);
    return n;
}

static size_t find_argument_end(const char *str, size_t sz) {
    size_t idx = 0;
    int quoted = 0, escaped = 0;
    while (idx < sz) {
        char c = str[idx];
        if (escaped) {
            escaped = 0;
        } else if (c == '\\') {
            escaped = 1;
        } else if (c == '\"') {
            quoted = !quoted;
        } else if (!quoted) {
            if (c <= ' ' || c == '|' || c == ')') {
                idx += skip_ws(str + idx, sz - idx);
                return idx;
            } else if (c == '$' && idx + 1 < sz && str[idx + 1] == '(') {
                idx += 2;
                do {
                    idx += find_argument_end(str + idx, sz - idx);
                    idx += find_next_argument(str + idx, sz - idx);
                } while (idx < sz && str[idx] != ')');
            }
        }
        ++idx;
    }
    return sz;
}

command_t* command_new() {
    command_t *command = (command_t*)yas_malloc(sizeof(command_t));
    command->flags = 0;
    command->argc = 0;
    command->argv = 0;
    return command;
}

void command_add_argument(command_t *command, argument_t *argument) {
    command->argv = (argument_t**)yas_realloc(command->argv,
                                              (command->argc + 1) * sizeof(argument_t*));
    command->argv[command->argc] = argument;
    ++command->argc;
}

void command_add_subcommand(command_t *command, command_t *subcommand) {
    argument_t *argument = (argument_t*)yas_malloc(sizeof(argument_t));
    argument->type = ARGTYPE_COMMAND;
    argument->d.cmd = subcommand;
    command_add_argument(command, argument);
}

command_t* command_create(const char *str, size_t sz) {
    size_t idx = 0;
    command_t *p = 0, *cmd = command_new();
    idx += skip_ws(str, sz);
    while (idx < sz) {
//         printf("%u:%u : start = %u\n", p ? p->argc : 0, cmd->argc, idx);
        size_t next = find_argument_end(str + idx, sz - idx);
//         printf("%u:%u : sz = %u\n", p ? p->argc : 0, cmd->argc, next);
        argument_t *arg = argument_create(str + idx, next);
        if (!arg) {
            command_destroy(cmd);
            return 0;
        }
        command_add_argument(cmd, arg);
        idx += next;
        if (idx < sz && str[idx] == '|') {
//             printf("adding subcommand\n");
            if (!p) {
                p = yas_malloc(sizeof(command_t));
                p->flags = COMMAND_IS_PIPECHAIN;
                p->argc = 0;
                p->argv = 0;
            }
            command_add_subcommand(p, cmd);
            cmd = command_new();
        }
        idx += find_next_argument(str + idx, sz - idx);
//         printf("%u:%u : next = %u\n", p ? p->argc : 0, cmd->argc, idx);
    }
    if (p && cmd->argc)
        command_add_subcommand(p, cmd);
    return p ? p : cmd;
}

void command_destroy(command_t *command) {
    if (!command)
        return;
    for (int i = 0; i < command->argc; ++i)
        argument_destroy(command->argv[i]);
    yas_free(command->argv);
    yas_free(command);
}

void command_inspect(command_t *command, size_t indent) {
    if (!command)
        return;
    indent_printf(indent, "flags = %u\n", command->flags);
    indent_printf(indent, "args = {\n");
    for (int i = 0; i < command->argc; ++i) {
        argument_inspect(command->argv[i], indent + 1);
    }
    indent_printf(indent, "}\n");
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
    if (sz <= 0)
        return 0;
    
    while (sz > 0 && str[sz - 1] <= ' ')
        --sz;
    
    argument_t *argument = (argument_t*)yas_malloc(sizeof(argument_t));
    argument->type = ARGTYPE_STRING;
    argument->d.str = 0;
    
    if (str[0] == '$') {
        if (sz > 1 && str[1] == '(') {
            if (str[sz - 1] != ')') {
                yas_free(argument);
                return 0;
            }
            argument->type = ARGTYPE_COMMAND;
            argument->d.cmd = command_create(str + 2, sz - 3);
        } else {
            argument->type = ARGTYPE_VARIABLE;
            argument->d.str = stripped(str + 1, sz - 1);
        }
    } else {
        argument->d.str = stripped(str, sz);
    }
    
    return argument;
}

void argument_destroy(argument_t *argument) {
    yas_free(argument);
}

void argument_inspect(argument_t *argument, size_t indent) {
    if (!argument)
        return;
    switch (argument->type) {
        case ARGTYPE_STRING:
            indent_printf(indent, "STRING = \"%s\"\n", argument->d.str);
            break;
        case ARGTYPE_COMMAND:
            indent_printf(indent, "COMMAND = {\n");
            command_inspect(argument->d.cmd, indent + 1);
            indent_printf(indent, "}\n");
            break;
        case ARGTYPE_VARIABLE:
            indent_printf(indent, "VARIABLE = \"%s\"\n", argument->d.str);
            break;
        default:
            break;
    }
}

int argument_type(argument_t *argument) {
    return argument ? argument->type : ARGTYPE_INVALID;
}

char* argument_get_string(argument_t *argument) {
    return argument && argument->type == ARGTYPE_STRING ? argument->d.str : 0;
}

char* argument_get_variable(argument_t *argument) {
    return argument && argument->type == ARGTYPE_VARIABLE ? argument->d.str : 0;
}

command_t* argument_get_command(argument_t *argument) {
    return argument && argument->type == ARGTYPE_COMMAND ? argument->d.cmd : 0;
}
