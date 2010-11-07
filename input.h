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

#ifndef _INPUT_H_
#define _INPUT_H_

/*!
    \file input.h
    \brief Definition of input abstraction layer.
*/

int yas_readline_is_busy();
void yas_readline_pre_signal();
void yas_readline_post_signal();

char* yas_readline(const char *prompt, int *eof);

#endif /* _INPUT_H_ */
