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

int command_argc(command_t *command);
argument_t** command_argv(command_t *command);

int command_is_pipechain(command_t *command);
int command_is_background(command_t *command);

argument_t* argument_create(const char *str, size_t sz);
void argument_destroy(argument_t *argument);

int argument_is_command(argument_t *argument);
command_t* argument_get_command(argument_t *argument);
char* argument_get_string(argument_t *argument);

#endif // _COMMAND_H_
