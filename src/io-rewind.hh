/*
 * loggertools
 * Copyright (C) 2004-2008 Max Kellermann <max@duempel.org>
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

#ifndef __LOGGERTOOLS_IO_REWIND_HH
#define __LOGGERTOOLS_IO_REWIND_HH

#include "io.hh"

#include <list>

/**
 * A Reader class which lets the caller rewind the stream, by saving a
 * copy of all objects.
 */
template<class T>
class RewindReader : public Reader<T> {
private:
    Reader<T> *reader;
    std::list<T> buffer;
    bool from_buffer;

public:
    RewindReader(Reader<T> *_reader)
        :reader(_reader), buffer(), from_buffer(false) {}

    virtual ~RewindReader() {
        delete reader;
    }

public:
    virtual const T *read() {
        if (from_buffer) {
            /* rewind() has been called; get the next object from the
               buffer */
            const T *ret = new T(*buffer.begin());
            buffer.pop_front();
            if (buffer.empty())
                from_buffer = false;
            return ret;
        }

        const T *t = reader->read();
        if (t != NULL)
            /* save the object, just in case the caller wants to
               rewind */
            buffer.push_back(*t);
        return t;
    }

    void rewind() {
        if (!buffer.empty())
            from_buffer = true;
    }
};

#endif
