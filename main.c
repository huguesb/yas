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
#include "input.h"
#include "command.h"

#include <string.h>

int main(int argc, char **argv) {
    (void) argc;
    (void) argv;
    while (1) {
        char *line = yas_readline("yas> ");
        if (line) {
            size_t line_sz = strlen(line);
            command_t *command = command_create(line, line_sz);
            yas_free(line);
            
            
            command_destroy(command);
        }
    }
    return 0;
}
