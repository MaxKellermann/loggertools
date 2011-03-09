/*
 * loggertools
 * Copyright (C) 2004-2011 Max Kellermann <max@duempel.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the License
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

/** \file
 *
 * Portability macros for opening files.
 */

#ifndef OPEN_H
#define OPEN_H

#include <fcntl.h>

/* On Windows, files are opened in "text" mode by default, and the C
   library will mangle data being read/written; this must be switched
   off by specifying the proprietary "O_BINARY" flag.  That sucks! */
#ifndef O_BINARY
#ifdef _O_BINARY
#define O_BINARY _O_BINARY
#else
#define O_BINARY 0
#endif
#endif

#endif
