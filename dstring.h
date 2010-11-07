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

#ifndef _DSTRING_H_
#define _DSTRING_H_

/*!
    \file string.h
    \brief Definition of string_t
*/

#include <stddef.h>

typedef struct _string string_t;

string_t* string_new();
string_t* string_from_cstr(const char *str);
string_t* string_from_cstrn(const char *str, size_t n);

void string_destroy(string_t *s);

void string_clear(string_t *s);

size_t string_get_length(const string_t *s);
char* string_get_cstr(const string_t *s);
char* string_get_cstr_copy(const string_t *s);

void string_append_string(string_t *dst, const string_t *src);
void string_append_char(string_t *dst, char c);
void string_append_cstr(string_t *dst, const char *str);
void string_append_cstrn(string_t *dst, const char *str, size_t n);

void string_shrink(string_t *s, size_t n);

#endif /* _DSTRING_H_ */
