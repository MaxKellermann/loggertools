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

#ifndef __LOGGERTOOLS_IO_HH
#define __LOGGERTOOLS_IO_HH

template<class T>
class Reader {
public:
    virtual ~Reader() {}
public:
    virtual const T *read() = 0;
};

template<class T>
class Writer {
public:
    virtual ~Writer() {}
public:
    virtual void write(const T &tp) = 0;
    virtual void flush() = 0;
};

template<class RT, class WT>
class Format {
public:
    virtual ~Format() {}
public:
    virtual RT *createReader(std::istream *stream) const = 0;
    virtual WT *createWriter(std::ostream *stream) const = 0;
};

#endif
