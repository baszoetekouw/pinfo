/***************************************************************************
 *  Pinfo is a ncurses based lynx style info documentation browser
 *
 *  Copyright (C) 1999  Przemek Borys <pborys@dione.ids.pl>
 *  Copyright (C) 2005  Bas Zoetekouw <bas@debian.org>
 *  Copyright 2005  Nathanael Nerode <neroden@gcc.gnu.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 *  USA
 ***************************************************************************/

#ifndef __INITIALIZELINKS_H
#define __INITIALIZELINKS_H
/* initializes node links.  */
void initializelinks (const std::string & line1,
                      const std::string & line2, 
                      int line);
/*
 * scans for url end in given url-string (from pos).
 * returns index of found place.
 */
std::string::size_type findurlend (const std::string & str,
																	 std::string::size_type pos = 0);

/* scans for the beginning of username. Returns its index.  */
std::string::size_type findemailstart (const std::string & str,
																			 std::string::size_type pos = 0);

#endif
