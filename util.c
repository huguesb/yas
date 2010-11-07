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

#include "util.h"

/*!
    \file util.c
    \brief Implementation of several utility functions
*/
#include "memory.h"
#include "dstring.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <pwd.h>
#include <glob.h>
#include <unistd.h>
#include <sys/resource.h>

int get_cpu_count() {
    FILE *f = popen("cat /proc/cpuinfo | grep processor | wc -l", "r");
    char buffer[16];
    size_t size = fread(buffer, 1, 16, f);
    pclose(f);
    return size ? atoi(buffer) : 0;
}

char* get_homedir() {
    glob_t globs;
    int err = glob("~/",
                   GLOB_TILDE_CHECK | GLOB_ERR,
                   NULL, &globs);
    if (err) {
        return 0;
    } else if (globs.gl_pathc != 1) {
        globfree(&globs);
        return 0;
    }
    return *globs.gl_pathv;
}

char* get_pwd() {
    char *buffer = 0;
    size_t size = 16;
    do {
        size *= 2;
        buffer = (char*)yas_realloc(buffer, size * sizeof(char));
        getcwd(buffer, size);
    } while (errno == ERANGE);
    if (errno) {
        yas_free(buffer);
        buffer = 0;
    }
    return buffer;
}

char* get_username() {
    struct passwd *pw = getpwuid(geteuid());
    return pw ? pw->pw_name : 0;
}