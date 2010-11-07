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
#include "dstring.h"

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

void indent_printf(size_t indent, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    size_t i;
    for (i = 0; i < indent; ++i)
        fputc(' ', stdout);
    vprintf(fmt, va);
    va_end(va);
}

/*
    Command grammar (external textual representation) :
    command_line = command ( ('|' | '&') command )*
    command = argument+ ( '<' argument )? ( '>' argument )?
    argument = string | '$' '(' command ')' | '$' variable
    string = '"' ([^"] | '\' '"')* '"' | ([^"<>|&] | '\' ["<>|&])+
*/

struct _command {
    int flags;
    size_t argc;
    argument_t **argv;
    argument_t *in;
    argument_t *out;
};

enum command_flags {
    COMMAND_IS_BACKGROUND = 1,
    COMMAND_IS_PIPECHAIN = 2
};

struct _argument {
    int type;
    size_t n;
    union {
        char *str;
        command_t *cmd;
        argument_t **sub;
    } d;
};


command_t* command_new() {
    command_t *command = (command_t*)yas_malloc(sizeof(command_t));
    command->flags = 0;
    command->argc = 0;
    command->argv = 0;
    command->in = 0;
    command->out = 0;
    return command;
}

void command_add_argument(command_t *command, argument_t *argument) {
    command->argv = (argument_t**)yas_realloc(command->argv,
                                              (command->argc + 1) * sizeof(argument_t*));
    command->argv[command->argc] = argument;
    ++command->argc;
}

command_t* command_add_subcommand(command_t *command, command_t *subcommand) {
    if (!command)
        return subcommand;
    if (!(command->flags & COMMAND_IS_PIPECHAIN)) {
        command_t *prev = command;
        command = command_new();
        command->flags = COMMAND_IS_PIPECHAIN;
        command_add_subcommand(command, prev);
    }
    argument_t *argument = (argument_t*)yas_malloc(sizeof(argument_t));
    argument->type = ARGTYPE_COMMAND;
    argument->d.cmd = subcommand;
    command_add_argument(command, argument);
    return command;
}

argument_t* argument_new() {
    argument_t *argument = (argument_t*)yas_malloc(sizeof(argument_t));
    argument->type = ARGTYPE_INVALID;
    argument->n = 0;
    argument->d.str = 0;
    return argument;
}

argument_t* argument_add_sub(argument_t *parent, argument_t *child) {
    if (!parent)
        return child;
    if ((parent->type & ARGTYPE_TYPE_MASK) != ARGTYPE_CAT) {
        argument_t *tmp = parent;
        parent = argument_new();
        parent->type = ARGTYPE_CAT | (tmp->type & ARGTYPE_FLAGS_MASK);
        parent->n = 1;
        parent->d.sub = (argument_t**)yas_malloc(2 * sizeof(argument_t*));
        parent->d.sub[0] = tmp;
        parent->d.sub[1] = 0;
    }
    ++parent->n;
    parent->d.sub = (argument_t**)yas_realloc(parent->d.sub,
                                              (parent->n + 1) * sizeof(argument_t*));
    parent->d.sub[parent->n - 1] = child;
    parent->d.sub[parent->n] = 0;
    return parent;
}

argument_t* argument_add_sub_from_string(argument_t *parent, string_t *s, int quoted) {
    if (string_get_length(s)) {
        argument_t *arg = argument_new();
        arg->d.str = string_get_cstr_copy(s);
        arg->type = ARGTYPE_STRING;
        if (quoted)
            arg->type |= ARGTYPE_QUOTED;
        string_clear(s);
        parent = argument_add_sub(parent, arg);
    }
    return parent;
}

/******************************************************************************/

typedef struct {
    const char *data;
    size_t length;
    size_t position;
    int error;
    int substitution;
} parse_context_t;

int parser_at_end(parse_context_t *cxt) {
    return cxt->position >= cxt->length;
}

char parser_char(parse_context_t *cxt) {
    return cxt->data[cxt->position];
}

void parser_advance(parse_context_t *cxt, int n) {
    if (n < 0 && cxt->position < (size_t)(-n))
        cxt->position = 0;
    else
        cxt->position += n;
}

char parser_consume(parse_context_t *cxt) {
    return cxt->data[cxt->position++];
}

void parser_skip_ws(parse_context_t *cxt) {
    while (cxt->position < cxt->length && isspace(cxt->data[cxt->position]))
        ++cxt->position;
}

command_t* parse_command_line(parse_context_t *cxt);
command_t* parse_command(parse_context_t *cxt);
argument_t* parse_argument(parse_context_t *cxt);

/* #define YAS_DEBUG_PARSE */

#ifdef YAS_DEBUG_PARSE
#define dprintf(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#else
#define dprintf(fmt, ...)
#endif

command_t* parse_command_line(parse_context_t *cxt) {
    dprintf("parse_command_line : %i/%i\n", cxt->position, cxt->length);
    command_t *p = 0;
    while (!parser_at_end(cxt)) {
        command_t *cmd = parse_command(cxt);
        if (!cmd)
            break;
        p = command_add_subcommand(p, cmd);
    }
    if (cxt->error && p) {
        command_destroy(p);
        p = 0;
    }
    dprintf("=> %p\n", p);
    return p;
}

command_t* parse_command(parse_context_t *cxt) {
    dprintf("parse_command : %i/%i\n", cxt->position, cxt->length);
    command_t *cmd = 0;
    while (!parser_at_end(cxt)) {
        argument_t *arg = parse_argument(cxt);
        if (!arg)
            break;
        else if (!cmd)
            cmd = command_new();
        command_add_argument(cmd, arg);
        int long_break = 1;
        while (1) {
            char c = parser_char(cxt);
            if (c == '|' || c == '&') {
                parser_advance(cxt, 1);
                if (c == '&')
                    cmd->flags |= COMMAND_IS_BACKGROUND;
                break;
            } else if (c == ')' || (c == '`' && cxt->substitution)) {
                break;
            } else if (c == '<') {
                if (!cmd->in) {
                    parser_advance(cxt, 1);
                    cmd->in = parse_argument(cxt);
                    if (cmd->in)
                        continue;
                }
                cxt->error = ERRTYPE_DUPLICATED_INPUT;
                break;
            } else if (c == '>') {
                if (!cmd->out) {
                    parser_advance(cxt, 1);
                    cmd->out = parse_argument(cxt);
                    if (cmd->out)
                        continue;
                }
                cxt->error = ERRTYPE_DUPLICATED_OUTPUT;
                break;
            }
            long_break = 0;
            break;
        }
        if (long_break)
            break;
    }
    if (cxt->error && cmd) {
        command_destroy(cmd);
        cmd = 0;
    }
    dprintf("=> %p\n", cmd);
    return cmd;
}

argument_t* parse_argument(parse_context_t *cxt) {
    dprintf("parse_argument : %i/%i\n", cxt->position, cxt->length);
    string_t *tmp = string_new();
    argument_t *p = 0;
    parser_skip_ws(cxt);
    int quoted = 0;
    while (!parser_at_end(cxt)) {
        char c = parser_char(cxt);
        if (c == '\\') {
            parser_advance(cxt, 1);
            string_append_char(tmp, parser_consume(cxt));
        } else if (c == '\"') {
            p = argument_add_sub_from_string(p, tmp, quoted);
            quoted = !quoted;
            parser_advance(cxt, 1);
        } else if (c == '$' || (c == '`' && !quoted && !cxt->substitution)) {
            p = argument_add_sub_from_string(p, tmp, quoted);
            int is_sub = c == '`';
            if (!is_sub) {
                parser_advance(cxt, 1);
                c = parser_char(cxt);
                is_sub = c == '(';
            } else {
                cxt->substitution = 1;
            }
            if (is_sub) {
                parser_advance(cxt, 1);
                command_t *sub = parse_command_line(cxt);
                if (!sub) {
                    break;
                }
                argument_t *arg = argument_new();
                arg->type = ARGTYPE_COMMAND;
                arg->d.cmd = sub;
                if (quoted)
                    arg->type |= ARGTYPE_QUOTED;
                p = argument_add_sub(p, arg);
                char pc = parser_char(cxt);
                if ((c == '(' && pc == ')') || (c == '`' && pc == '`')) {
                    parser_advance(cxt, 1);
                    if (c == '`')
                        cxt->substitution = 0;
                } else {
                    cxt->error = ERRTYPE_UNMATCHING_DELIMITERS;
                }
            } else if (isalnum(c) || (c == '_')) {
                while (!parser_at_end(cxt) && (isalnum(c) || (c == '_'))) {
                    string_append_char(tmp, c);
                    parser_advance(cxt, 1);
                    c = parser_char(cxt);
                }
                argument_t *arg = argument_new();
                arg->type = ARGTYPE_VARIABLE;
                arg->d.str = string_get_cstr_copy(tmp);
                string_clear(tmp);
                if (quoted)
                    arg->type |= ARGTYPE_QUOTED;
                p = argument_add_sub(p, arg);
            } else {
                /* TODO: report a deeper analysis of the error */
                cxt->error = ERRTYPE_UNKNOWN_SYNTAX;
            }
        } else if (!quoted && (c <= ' ' || c == '|' || c == '<' || c == '>' || c == '&' || c == ')' || c == '`')) {
            parser_skip_ws(cxt);
            break;
        } else if (!quoted && c == '#') {
            cxt->position = cxt->length;
            break;
        } else {
            string_append_char(tmp, parser_consume(cxt));
        }
    }
    if (cxt->error && p) {
        argument_destroy(p);
        p = 0;
    } else {
        p = argument_add_sub_from_string(p, tmp, quoted);
    }
    string_destroy(tmp);
    dprintf("=> %p\n", p);
    return p;
}

/******************************************************************************/

static size_t _command_error_position = 0;
static string_t *_command_error_string = 0;

command_t* command_create(const char *str, size_t sz) {
    _command_error_position = (size_t)-1;
    if (_command_error_string)
        string_clear(_command_error_string);
    else
        _command_error_string = string_new();
    parse_context_t cxt;
    cxt.data = str;
    cxt.length = sz;
    cxt.position = 0;
    cxt.error = 0;
    cxt.substitution = 0;
    command_t* cmd = parse_command_line(&cxt);
    if (cxt.error) {
        _command_error_position = cxt.position;
        switch (cxt.error) {
            case ERRTYPE_DUPLICATED_INPUT:
                string_append_cstr(_command_error_string, "Duplicated input");
                break;
            case ERRTYPE_DUPLICATED_OUTPUT:
                string_append_cstr(_command_error_string, "Duplicated output");
                break;
            case ERRTYPE_UNMATCHING_DELIMITERS:
                string_append_cstr(_command_error_string, "Unmatching delimiters");
                break;
            default:
                string_append_cstr(_command_error_string, "Unknown");
                break;
        }
    } else if (cxt.position < cxt.length) {
        _command_error_position = cxt.position;
        string_append_cstr(_command_error_string, "Input left : ");
        string_append_cstr(_command_error_string, cxt.data + cxt.position);
        command_destroy(cmd);
        cmd = 0;
    }
    return cmd;
}

size_t command_error_position() {
    return _command_error_position;
}

const char* command_error_string() {
    return string_get_cstr(_command_error_string);
}

void command_destroy(command_t *command) {
    if (!command)
        return;
    size_t i;
    for (i = 0; i < command->argc; ++i)
        argument_destroy(command->argv[i]);
    yas_free(command->argv);
    yas_free(command);
}

void command_inspect(command_t *command, size_t indent) {
    if (!command)
        return;
    indent_printf(indent, "flags = %u\n", command->flags);
    indent_printf(indent, "args = {\n");
    size_t i;
    for (i = 0; i < command->argc; ++i)
        argument_inspect(command->argv[i], indent + 1);
    
    if (command->in) {
        indent_printf(indent, "<\n");
        argument_inspect(command->in, indent + 1);
    }
    if (command->out) {
        indent_printf(indent, ">\n");
        argument_inspect(command->out, indent + 1);
    }
    indent_printf(indent, "}\n");
}

argument_t* command_redir_in(command_t *command) {
    return command ? command->in : 0;
}

argument_t* command_redir_out(command_t *command) {
    return command ? command->out : 0;
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

void argument_destroy(argument_t *argument) {
    int type = argument->type & ARGTYPE_TYPE_MASK;
    if (type == ARGTYPE_COMMAND) {
        command_destroy(argument->d.cmd);
    } else if (type == ARGTYPE_CAT) {
        argument_t **l = argument->d.sub;
        while (l && *l)
            argument_destroy(*(l++));
        yas_free(argument->d.sub);
    } else if (type == ARGTYPE_STRING || type == ARGTYPE_VARIABLE) {
        yas_free(argument->d.str);
    }
    yas_free(argument);
}

void argument_inspect(argument_t *argument, size_t indent) {
    if (!argument)
        return;
    switch (argument->type & ARGTYPE_TYPE_MASK) {
        case ARGTYPE_STRING:
            indent_printf(indent,
                          "%cSTRING = \"%s\"\n",
                          argument->type & ARGTYPE_QUOTED ? '*' : ' ',
                          argument->d.str);
            break;
        case ARGTYPE_COMMAND:
            indent_printf(indent,
                          "%cCOMMAND = {\n",
                          argument->type & ARGTYPE_QUOTED ? '*' : ' ');
            command_inspect(argument->d.cmd, indent + 1);
            indent_printf(indent, "}\n");
            break;
        case ARGTYPE_VARIABLE:
            indent_printf(indent, "%cVARIABLE = \"%s\"\n",
                          argument->type & ARGTYPE_QUOTED ? '*' : ' ',
                          argument->d.str);
            break;
        case ARGTYPE_CAT:
        {
            indent_printf(indent, "%cCAT = {\n",
                          argument->type & ARGTYPE_QUOTED ? '*' : ' ');
            argument_t **l = argument->d.sub;
            while (l && *l)
                argument_inspect(*(l++), indent + 1);
            indent_printf(indent, "}\n");
            break;
        }
        default:
            indent_printf(indent, "??? [%p : %x]\n", argument, argument->type);
            break;
    }
}

int argument_type(argument_t *argument) {
    return argument ? argument->type & ARGTYPE_TYPE_MASK : ARGTYPE_INVALID;
}

int argument_flags(argument_t *argument) {
    return argument ? argument->type & ARGTYPE_FLAGS_MASK : 0;
}

char* argument_get_string(argument_t *argument) {
    return argument && (argument->type & ARGTYPE_TYPE_MASK) == ARGTYPE_STRING ? argument->d.str : 0;
}

char* argument_get_variable(argument_t *argument) {
    return argument && (argument->type & ARGTYPE_TYPE_MASK) == ARGTYPE_VARIABLE ? argument->d.str : 0;
}

command_t* argument_get_command(argument_t *argument) {
    return argument && (argument->type & ARGTYPE_TYPE_MASK) == ARGTYPE_COMMAND ? argument->d.cmd : 0;
}

argument_t** argument_get_arguments(argument_t *argument) {
    return argument && (argument->type & ARGTYPE_TYPE_MASK) == ARGTYPE_CAT ? argument->d.sub : 0;
}
