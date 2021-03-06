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

#include "argv.h"

/*!
    \file argv.c
    \brief Implementation of argv_t
*/

#include "memory.h"
#include "util.h"

#include <ctype.h>
#include <glob.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct _argv {
    size_t n;
    size_t a;
    char **d;
};

static void argv_grow(argv_t *argv, size_t n) {
    if (argv->a - argv->n > n)
        return;
    argv->a *= 2;
    argv->d = (char**)yas_realloc(argv->d, argv->a * sizeof(char*));
}

/*!
    \brief Create a new argv_t
*/
argv_t* argv_new() {
    argv_t *argv = (argv_t*)yas_malloc(sizeof(argv_t));
    argv->n = 0;
    argv->a = 1;
    argv->d = (char**)yas_malloc(argv->a * sizeof(char*));
    argv->d[0] = 0;
    return argv;
}

/*!
    \brief Destroy an argv_t
*/
void argv_destroy(argv_t *argv) {
    if (!argv)
        return;
    yas_free(argv->d);
    yas_free(argv);
}

/*!
    \return the number of arguments of an argv_t
*/
size_t argv_get_argc(argv_t *argv) {
    return argv ? argv->n : 0;
}

/*!
    \return the arguments of an argv_t
*/
char** argv_get_argv(argv_t *argv) {
    return argv ? argv->d : 0;
}

/*!
    \brief Add a string argument to an argv_t
*/
int argv_add(argv_t *argv, const char *s) {
    if (!argv || !s)
        return 0;
    argv_grow(argv, 1);
    argv->d[argv->n] = (char*)yas_malloc((strlen(s)+1) * sizeof(char));
    strcpy(argv->d[argv->n], s);
    argv->d[++argv->n] = 0;
    return 0;
}

/*!
    \brief Glob-expand and field-split a string and add the resulting string arguments to an argv_t
*/
int argv_add_split(argv_t *argv, const char *s) {
    if (!argv || !s)
        return 0;
    /* field splitting & glob expansion */
    const char *last = s, *current = s;
    do {
        if (isspace(*current) || !*current) {
            if (current != last) {
                char *tmp = (char*)yas_malloc((current - last + 1) * sizeof(char));
                strncpy(tmp, last, current - last);
                tmp[current - last] = 0;
                
                glob_t globs;
                int err = glob(tmp,
                               GLOB_NOMAGIC | GLOB_TILDE_CHECK | GLOB_ERR,
                               NULL, &globs);
                if (err) {
                    fprintf(stderr, "Wildcard/tilde expansion failed.\n");
                    fprintf(stderr, "%s\n", tmp);
                    return 1;
                }
                size_t j;
                for (j = 0; j < globs.gl_pathc; ++j)
                    argv_add(argv, globs.gl_pathv[j]);
                globfree(&globs);
                yas_free(tmp);
            }
            last = current + 1;
        }
    } while (*(current++));
    return 0;
}

/*!
    \brief Print the content of an argv_t for debugging purpose
*/
void argv_inspect(argv_t *argv) {
    size_t i;
    for (i = 0; i < argv->n; ++i)
        fprintf(stderr, "%s\n", argv->d[i]);
}
