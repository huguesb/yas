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

#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/*!
    \file memory.c
    \brief Implementation of memory allocation wrappers.
*/

static yas_mem_error_handler_t __yas_mem_error_handler = NULL;

/*!
    \brief Default memory error handler
*/
void yas_mem_error() {
    if (__yas_mem_error_handler != NULL) {
        __yas_mem_error_handler();
        return;
    }
    
    errno = ENOMEM;
    perror(0);
    exit(1);
}

/*!
    \brief Wrapper around malloc
*/
void* yas_malloc(size_t sz) {
    void *d = malloc(sz);
    if (d == NULL)
        yas_mem_error();
    return d;
}

/*!
    \brief Wrapper around realloc
*/
void* yas_realloc(void *d, size_t sz) {
    d = realloc(d, sz);
    if (d == NULL)
        yas_mem_error();
    return d;
}

/*!
    \brief Wrapper around free
*/
void yas_free(void *d) {
    free(d);
}

/*!
    \brief Set the memory error handler
    \param handler error handler
    If \a handler is NULL yas_mem_error will be called upon allocation error.
*/
void yas_set_mem_error_handler(yas_mem_error_handler_t handler) {
    __yas_mem_error_handler = handler;
}
