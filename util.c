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

/*!
    \return The number of available CPU cores
*/
int get_cpu_count() {
    FILE *f = popen("cat /proc/cpuinfo | grep processor | wc -l", "r");
    char buffer[16];
    size_t size = fread(buffer, 1, 16, f);
    pclose(f);
    return size ? atoi(buffer) : 0;
}

/*!
    \return the home dir of the current user, 0 on error
    The caller is responsible for freeing the data
*/
char* get_homedir() {
    glob_t globs;
    int err = glob("~/",
                   GLOB_TILDE_CHECK | GLOB_ERR,
                   NULL, &globs);
    char *homedir = 0;
    if (err) {
        return 0;
    } else if (globs.gl_pathc == 1) {
        homedir = yas_malloc(strlen(*globs.gl_pathv) + 1);
        strcpy(homedir, *globs.gl_pathv);
    }
    globfree(&globs);
    return homedir;
}

/*!
    \return the current working directory
    The caller is responsible for freeing the data
*/
char* get_pwd() {
    char *pwd = 0;
    char *buffer = 0;
    size_t size = 16;
    do {
        size *= 2;
        buffer = (char*)yas_realloc(buffer, size * sizeof(char));
        pwd = getcwd(buffer, size);
    } while (!pwd && errno == ERANGE);
    if (!pwd)
        yas_free(buffer);
    return pwd;
}

/*!
    \return the current username
    \return the data is statically allocated
*/
char* get_username() {
    struct passwd *pw = getpwuid(geteuid());
    return pw ? pw->pw_name : 0;
}
