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

#ifndef __LOGGERTOOLS_IO_COMPARE_HH
#define __LOGGERTOOLS_IO_COMPARE_HH

#include "io-rewind.hh"
#include "exception.hh"

/**
 * A Reader class which finds a specific object in the stream, and
 * returns all objects matching the Compare operation.
 */
template<class T, class Find, class Compare>
class FindCompareReader : public RewindReader<T> {
private:
    Find find;
    Compare compare;
    T *reference;

public:
    FindCompareReader(Reader<T> *_reader, Find _find, Compare _compare)
        :RewindReader<T>(_reader), find(_find), compare(_compare),
         reference(NULL) {}

    virtual ~FindCompareReader() {
        if (reference != NULL)
            delete reference;
    }

public:
    virtual const T *read() {
        const T *t;

        while (reference == NULL) {
            /* find the reference item */

            t = RewindReader<T>::read();
            if (t == NULL)
                throw malformed_input("reference item not found");

            if (find(*t)) {
                /* the reference has been fond: rewind the stream, so
                   all previous objects are being compared */
                reference = new T(*t);
                this->rewind();
            }

            delete t;
        }

        while (true) {
            t = RewindReader<T>::read();
            if (t == NULL || compare(*reference, *t))
                return t;

            /* this object didn't pass the filter: delete it */
            delete t;
        }
    }
};

#endif
