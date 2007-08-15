/**
 * Various string functions mainly used for controlling text output

 * Copyright (C) 2003  Shawn Betts
 * Copyright (C) 2004, 2007  Sylvain Beucler

 * This file is part of GNU FreeDink

 * GNU FreeDink is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.

 * GNU FreeDink is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include "str_util.h"

/**
 * Compare two strings ignoring case
 * TODO: autoconfiscate(strcasecmp)
 */
int
string_icompare (char *s1, char *s2)
{
  while (*s1 && *s2 && toupper (*s1) == toupper (*s2))
    {
      s1++;
      s2++;
    }

  return *s1 - *s2;
}

/**
 * Upcase the string
 */
void
strupr (char *s)
{
  for (; *s; s++)
    *s = toupper (*s);
}
