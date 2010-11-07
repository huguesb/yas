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

#ifndef _ARGV_H_
#define _ARGV_H_

/*!
    \file argv.h
    \brief Definition of argv_t
*/

#include <stddef.h>

char* get_homedir();

typedef struct _argv argv_t;

argv_t* argv_new();
void argv_destroy(argv_t *argv);

int argv_add(argv_t *argv, const char *s);
int argv_add_split(argv_t *argv, const char *s);

size_t argv_get_argc(argv_t *argv);
char** argv_get_argv(argv_t *argv);

void argv_inspect(argv_t *argv);

#endif /* _ARGV_H_ */
