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

#include "memory.h"

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

argv_t* argv_new() {
    argv_t *argv = (argv_t*)yas_malloc(sizeof(argv_t));
    argv->n = 0;
    argv->a = 1;
    argv->d = (char**)yas_malloc(argv->a * sizeof(char*));
    argv->d[0] = 0;
    return argv;
}

void argv_destroy(argv_t *argv) {
    if (!argv)
        return;
    yas_free(argv->d);
    yas_free(argv);
}

size_t argv_get_argc(argv_t *argv) {
    return argv ? argv->n : 0;
}

char** argv_get_argv(argv_t *argv) {
    return argv ? argv->d : 0;
}

int argv_add(argv_t *argv, const char *s) {
//     fprintf(stderr, "=> %s\n", s);
    argv_grow(argv, 1);
    argv->d[argv->n] = (char*)yas_malloc((strlen(s)+1) * sizeof(char));
    strcpy(argv->d[argv->n], s);
    argv->d[++argv->n] = 0;
    return 0;
}

int argv_add_split(argv_t *argv, const char *s) {
    // space split & glob expansion
//     fprintf(stderr, "expanding %s\n", s);
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
//                 fprintf(stderr, "expanded %s into %u parts\n", tmp, globs.gl_pathc);
                for (size_t j = 0; j < globs.gl_pathc; ++j)
                    argv_add(argv, globs.gl_pathv[j]);
                globfree(&globs);
                yas_free(tmp);
            }
            last = current + 1;
        }
    } while (*(current++));
    return 0;
}

void argv_inspect(argv_t *argv) {
    for (size_t i = 0; i < argv->n; ++i)
        fprintf(stderr, "%s\n", argv->d[i]);
}
