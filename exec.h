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

#ifndef _EXEC_H_
#define _EXEC_H_

/*!
    \file exec.h
    \brief Definition of command execution
*/

#include "command.h"
#include "task.h"

enum {
    EXEC_OK,
    EXEC_EXIT,
    EXEC_ERROR
};

int exec_command(command_t *command, task_list_t *tasklist);

#endif /* _EXEC_H_ */
