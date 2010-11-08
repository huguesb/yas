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

#include "dstring.h"

/*!
    \file dstring.c
    \brief Implementation of string_t
*/

#include "memory.h"
#include <string.h>

struct _string {
    size_t size;
    size_t alloc;
    char *data;
};

static void string_realloc(string_t *s, size_t sz) {
    s->alloc = sz + ((sz & 31) ? 32 - (sz & 31) : 0);
    if (s->alloc) {
        s->data = (char*)yas_realloc(s->data, s->alloc * sizeof(char));
    } else {
        yas_free(s->data);
        s->data = 0;
    }
}

static void string_grow(string_t *s, size_t n) {
    if (s->alloc - s->size > n)
        return;
    string_realloc(s, s->size ? 2 * s->size : 16);
}

/*!
    \brief Create a new string_t
    \return A new empty string_t
*/
string_t* string_new() {
    string_t *s = (string_t*)yas_malloc(sizeof(string_t));
    s->size = 0;
    s->alloc = 0;
    s->data = 0;
    return s;
}

/*!
    \brief Create a new string_t
    \param str Zero-terminated string
    \return A new string_t containing a copy of \a str
*/
string_t* string_from_cstr(const char *str) {
    string_t *s = string_new();
    string_append_cstr(s, str);
    return s;
}

/*!
    \brief Create a new string_t
    \param str String
    \param n String size
    \return A new string_t containing a copy of \a str
*/
string_t* string_from_cstrn(const char *str, size_t n) {
    string_t *s = string_new();
    string_append_cstrn(s, str, n);
    return s;
}

/*!
    \brief Create a new string_t
    \param str Zero-terminated string
    \return A new string_t using \a str as its internal storage
    \note Contrary to other ctors, this one takes ownership of \a str.
*/
string_t* string_from_cstr_own(char *str) {
    string_t *s = string_new();
    if (str) {
        s->size = strlen(str);
        s->alloc = s->size;
        s->data = str;
    }
    return s;
}

/*!
    \brief Destroy a string
*/
void string_destroy(string_t *s) {
    if (!s)
        return;
    yas_free(s->data);
    yas_free(s);
}

/*!
    \brief Clear a string
    \param s String to clear
*/
void string_clear(string_t *s) {
    if (!s)
        return;
    s->size = 0;
}

/*!
    \param s String
    \return string length
*/
size_t string_get_length(const string_t *s) {
    return s ? s->size : 0;
}

/*!
    \param s String
    \return string data as a zero-terminated char*
*/
char* string_get_cstr(const string_t *s) {
    return s ? s->data : 0;
}

/*!
    \param s String
    \return a yas_malloc'ed copy of string data
*/
char* string_get_cstr_copy(const string_t *s) {
    if (!s || !s->size || !s->data)
        return 0;
    char *str = (char*)yas_malloc((s->size + 1) * sizeof(char));
    strcpy(str, s->data);
    return str;
}

/*!
    \brief Append the content of a string_t to another
    \param dst String to append data to
    \param src String to append
*/
void string_append_string(string_t *dst, const string_t *src) {
    if (!dst || !src)
        return;
    string_append_cstrn(dst, src->data, src->size);
}

/*!
    \brief Append a character to a string_t
    \param dst String to append data to
    \param c Char to append
*/
void string_append_char(string_t *dst, char c) {
    if (!dst)
        return;
    string_grow(dst, 1);
    dst->data[dst->size++] = c;
    dst->data[dst->size] = 0;
}

/*!
    \brief Append a string to a string_t
    \param dst String to append data to
    \param str Zero-terminated string to append
*/
void string_append_cstr(string_t *dst, const char *str) {
    if (!dst || !str)
        return;
    string_append_cstrn(dst, str, strlen(str));
}

/*!
    \brief Append a string to a string_t
    \param dst String to append data to
    \param str String to append
    \param n Size of string to append
*/
void string_append_cstrn(string_t *dst, const char *str, size_t n) {
    if (!dst || !str || !n)
        return;
    string_grow(dst, n);
    strncpy(dst->data + dst->size, str, n);
    dst->size += n;
    dst->data[dst->size] = 0;
}

/*!
    \brief Shrink a string_t
    \param s String to shrink
    \param n Number of characters to remove at the end
*/
void string_shrink(string_t *s, size_t n) {
    if (!s)
        return;
    if (s->size > n)
        s->size -= n;
    else
        s->size = 0;
    if (s->data)
        s->data[s->size] = 0;
}
