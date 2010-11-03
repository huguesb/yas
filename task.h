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

#ifndef _TASK_H_
#define _TASK_H_

#include "command.h"
#include "argv.h"

#include <sys/types.h>

typedef struct _task task_t;

task_t* task_new();
void task_destroy(task_t *task);

pid_t task_get_pid(task_t *task);
void task_set_pid(task_t *task, pid_t pid);

argv_t* task_get_argv(task_t *task);
void task_set_argv(task_t *task, argv_t *argv);

long long task_get_elapsed_seconds(task_t *task);
long long task_get_elapsed_millis(task_t *task);
long long task_get_elapsed_micros(task_t *task);

void task_inspect(task_t *task);

typedef struct _task_list task_list_t;

task_list_t* task_list_new();
void task_list_destroy(task_list_t *list);

size_t task_list_get_size(task_list_t *list);
task_t* task_list_get_task(task_list_t *list, size_t index);

void task_list_add(task_list_t *list, task_t *task);
void task_list_remove(task_list_t *list, size_t index);

#endif /* _TASK_H_ */
