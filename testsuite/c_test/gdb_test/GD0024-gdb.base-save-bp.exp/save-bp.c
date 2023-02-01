/* Copyright 2011-2012 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

void
break_me (void)
{
}

int
main (void)
{
  int i;
  break_me (); /* BREAK HERE.  */
  break_me (); /* Try a thread-specific breakpoint.  */

  for (i = 0; i < 5; i++)
    break_me (); /* Try a condition-specific breakpoint.  */

  break_me (); /* Finally, try a breakpoint with commands.  */
  return 0;
}

