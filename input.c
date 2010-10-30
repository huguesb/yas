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

#include "input.h"

#include "memory.h"

#include <stdio.h>
#include <string.h>

#ifdef YAS_USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

char* yas_readline(const char *prompt) {
#ifdef YAS_USE_READLINE
    char *s = readline(prompt);
    if (s && *s)
        add_history(s);
    /* NOTE : If yas_malloc/yas_free were to diverge from std malloc/free it
     will be necessary to yas_alloc a new buffer here and free the old one
     after having copied content from old to new for external consistency */
    return s;
#else
    printf("%s", prompt);
    fflush(stdout);
    char *buffer = 0;
    size_t buffer_sz = 32, offset = 0;
    while (1) {
        if (buffer_sz * 2 <= buffer_sz)
            yas_mem_error();
        buffer_sz *= 2;
        buffer = yas_realloc(buffer, buffer_sz);
        if (fgets(buffer + offset, buffer_sz - offset, stdin) == NULL) {
            if (offset <= 0) {
                yas_free(buffer);
                return 0;
            }
            return buffer;
        }
        offset += strlen(buffer + offset);
        if (offset && buffer[offset - 1] == '\n') {
            buffer[offset - 1] = '\0';
            return buffer;
        }
    }
#endif
}
