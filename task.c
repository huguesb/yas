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

#include "task.h"

#include "memory.h"

struct _task {
    pid_t pid;
    argv_t *argv;
};

task_t* task_new() {
    task_t *task = (task_t*)yas_malloc(sizeof(task_t));
    task->pid = 0;
    task->argv = 0;
    return task;
}

void task_destroy(task_t *task) {
    if (!task)
        return;
    argv_destroy(task->argv);
    yas_free(task);
}

pid_t task_get_pid(task_t *task) {
    return task ? task->pid : 0;
}

void task_set_pid(task_t *task, pid_t pid) {
    if (task)
        task->pid = pid;
}

argv_t* task_get_argv(task_t *task) {
    return task ? task->argv : 0;
}

void task_set_argv(task_t *task, argv_t *argv) {
    if (task)
        task->argv = argv;
}

////////////////////////////////////////////////////////////////////////////////

struct _task_list {
    size_t n;
    size_t a;
    task_t **d;
};

task_list_t* task_list_new() {
    task_list_t *list = (task_list_t*)yas_malloc(sizeof(task_list_t));
    list->n = 0;
    list->a = 0;
    list->d = 0;
    return list;
}

void task_list_destroy(task_list_t *list) {
    if (!list)
        return;
    for (size_t i = 0; i < list->n; ++i)
        task_destroy(list->d[i]);
    yas_free(list->d);
    yas_free(list);
}

size_t task_list_get_size(task_list_t *list) {
    return list ? list->n : 0;
}

task_t* task_list_get_task(task_list_t *list, size_t index) {
    return list ? list->d[index] : 0;
}

void task_list_add(task_list_t *list, task_t *task) {

}

void task_list_remove(task_list_t *list, task_t *task) {

}
