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

#ifndef _MEMORY_H_
#define _MEMORY_H_

/*!
    \file memory.h
    \brief Definition of memory allocation wrappers.
*/

#include <stddef.h>

void* yas_malloc(size_t sz);
void* yas_realloc(void *d, size_t sz);
void yas_free(void *d);

void yas_mem_error();

typedef void (*yas_mem_error_handler_t)();
void yas_set_mem_error_handler(yas_mem_error_handler_t handler);

#endif /* _MEMORY_H_ */
