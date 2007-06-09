/*
 * loggertools
 * Copyright (C) 2004-2007 Max Kellermann <max@duempel.org>
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

#ifndef __HEXFILE_WRITER_HH
#define __HEXFILE_WRITER_HH

#include <iosfwd>
#include <ostream>
#include <streambuf>

class HexfileOutputFilterBuf
    : public std::streambuf {
private:
    std::ostream *next;
    unsigned segment, offset;

public:
    HexfileOutputFilterBuf(std::ostream &_next)
        :next(&_next), segment(0), offset(0) {}
    virtual ~HexfileOutputFilterBuf();

private:
    std::streamsize
    write_record(size_t length, unsigned address,
                 unsigned type, const unsigned char *data);

public:
    virtual std::streamsize
    xsputn(const char_type* __s, std::streamsize __n);
};

class HexfileOutputFilter
    : public std::ostream {
public:
    typedef HexfileOutputFilterBuf __filebuf_type;
    typedef std::ostream __ostream_type;

private:
    __filebuf_type _M_filebuf;

public:
    HexfileOutputFilter(__ostream_type &_next)
        :__ostream_type(), _M_filebuf(_next)
    {
        this->init(&_M_filebuf);
    }
};

#endif
