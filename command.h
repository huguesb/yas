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

#ifndef _COMMAND_H_
#define _COMMAND_H_

#include <stddef.h>

typedef struct _command command_t;
typedef struct _argument argument_t;

command_t* command_create(const char *str, size_t sz);
void command_destroy(command_t *command);
void command_inspect(command_t *command, size_t indent);

size_t command_error_position();
const char* command_error_string();

int command_argc(command_t *command);
argument_t** command_argv(command_t *command);

argument_t* command_redir_in(command_t *command);
argument_t* command_redir_out(command_t *command);

int command_is_pipechain(command_t *command);
int command_is_background(command_t *command);

enum argument_type {
    ARGTYPE_INVALID,
    ARGTYPE_STRING,
    ARGTYPE_COMMAND,
    ARGTYPE_VARIABLE,
    ARGTYPE_CAT,
    ARGTYPE_TYPE_MASK = 0x0FFF,
    ARGTYPE_FLAGS_MASK = 0xF000,
    ARGTYPE_QUOTED = 0x8000
};

enum error_type {
    ERRTYPE_DUPLICATED_INPUT,
    ERRTYPE_DUPLICATED_OUTPUT,
    ERRTYPE_UNMATCHING_DELIMITERS,
    ERRTYPE_UNKNOWN_SYNTAX
};

void argument_destroy(argument_t *argument);
void argument_inspect(argument_t *argument, size_t indent);

int argument_type(argument_t *argument);
int argument_flags(argument_t *argument);
char* argument_get_string(argument_t *argument);
char* argument_get_variable(argument_t *argument);
command_t* argument_get_command(argument_t *argument);
argument_t** argument_get_arguments(argument_t *argument);

#endif /* _COMMAND_H_ */
