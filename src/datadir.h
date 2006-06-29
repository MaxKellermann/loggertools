/*
 * loggertools
 * Copyright (C) 2004-2006 Max Kellermann <max@duempel.org>
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
 *
 * $Id$
 */

#ifndef __DATADIR_H
#define __DATADIR_H

struct datadir;

struct datadir *datadir_open(const char *path);

void datadir_close(struct datadir *dir);

void *datadir_read(struct datadir *dir,
                   const char *filename,
                   size_t length);

int datadir_write(struct datadir *dir,
                  const char *filename,
                  void *data, size_t length);

#endif
