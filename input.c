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
#include "dstring.h"

#include <stdio.h>

#ifdef YAS_USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#else
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>

struct termios saved_attributes;

static const char *_yas_readline_prompt = 0;
static string_t *_yas_readline_buffer = 0;

static void _yas_prompt_redisplay() {
    fputc('\r', stdout);
    if (_yas_readline_prompt)
        fputs(_yas_readline_prompt, stdout);
    if (string_get_length(_yas_readline_buffer))
        fputs(string_get_cstr(_yas_readline_buffer), stdout);
    fflush(stdout);
}
#endif

static int _yas_readline_busy = 0;

static void yas_readline_setup() {
    _yas_readline_busy = 1;
#ifdef YAS_USE_READLINE
    
#else
    struct termios tattr;

    /* Make sure stdin is a terminal.  */
    if (!isatty (STDIN_FILENO)) {
        fprintf (stderr, "Not a terminal.\n");
        exit (EXIT_FAILURE);
    }

    /* Save the terminal attributes so we can restore them later.  */
    tcgetattr (STDIN_FILENO, &saved_attributes);

    /* Set the funny terminal modes.  */
    tcgetattr (STDIN_FILENO, &tattr);
    tattr.c_lflag &= ~(ICANON|ECHO); /* Clear ICANON and ECHO.   */
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
#endif
}

static void yas_readline_cleanup() {
#ifdef YAS_USE_READLINE
    
#else
    tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
#endif
    _yas_readline_busy = 0;
}

int yas_readline_is_busy() {
    return _yas_readline_busy;
}

void yas_readline_pre_signal() {
    if (!_yas_readline_busy)
        return;
#ifdef YAS_USE_READLINE
    fprintf(stdout, "\r");
    fflush(stdout);
#else
    fprintf(stdout, "\r");
    fflush(stdout);
#endif
}

void yas_readline_post_signal() {
    if (!_yas_readline_busy)
        return;
#ifdef YAS_USE_READLINE
    rl_forced_update_display();
#else
    if (_yas_readline_prompt && _yas_readline_buffer)
        _yas_prompt_redisplay();
#endif
}

char* yas_readline(const char *prompt, int *eof) {
    if (eof)
        *eof = 0;
    yas_readline_setup();
#ifdef YAS_USE_READLINE
    char *s = readline(prompt);
    if (s && *s)
        add_history(s);
    /* NOTE : If yas_malloc/yas_free were to diverge from std malloc/free it
     will be necessary to yas_alloc a new buffer here and free the old one
     after having copied content from old to new for external consistency */
#else
    _yas_readline_prompt = prompt;
    if (_yas_readline_buffer)
        string_destroy(_yas_readline_buffer);
    _yas_readline_buffer = string_new();
    fprintf(stdout, "%s", prompt);
    fflush(stdout);
    while (1) {
        char c;
        size_t n = read(STDIN_FILENO, &c, 1);
        if (n == 1) {
            if (c == 0x04 || c == '\n') {
                if (eof)
                    *eof = (c == 0x04);
                fputc('\n', stdout);
                fflush(stdout);
                break;
            } else if (c == 127) {
                // backspace
                string_shrink(_yas_readline_buffer, 1);
                // TODO: refactor below...
                _yas_prompt_redisplay();
                fputc(' ', stdout);
                fflush(stdout);
                _yas_prompt_redisplay();
            } else if (c >= ' ' && c < 127) {
                fputc(c, stdout);
                fflush(stdout);
                string_append_char(_yas_readline_buffer, c);
            } else if (c == 0x1B || c == (char)0x9B) {
                /* handle ANSI escape sequences described in ECMA-48 :
                    CSI p* i* f where :
                    CSI = 0x1B 0x5B
                    p in 0x30-0x3F
                    i in 0x20-0x2F
                    f in 0x40-0x7e [0x70-0x7e : private]
                */
                int csi = c == (char)0x9B;
                if (!csi && read(STDIN_FILENO, &c, 1) == 1)
                    csi = c == 0x5B;
                if (csi) {
                    do {
                        read(STDIN_FILENO, &c, 1);
                    } while (c < 64);
                    // TODO: handle cursor commands?
                }
            }
        }
    }
    char *s = string_get_cstr_copy(_yas_readline_buffer);
    string_destroy(_yas_readline_buffer);
    _yas_readline_buffer = 0;
    _yas_readline_prompt = 0;
#endif
    yas_readline_cleanup();
    return s;
}
