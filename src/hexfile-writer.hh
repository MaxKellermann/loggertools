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

#ifndef __HEXFILE_WRITER_HH
#define __HEXFILE_WRITER_HH

#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/operations.hpp>

namespace io = boost::iostreams;

class HexfileOutputFilterCategory
    : public io::multichar_output_filter_tag,
      public io::closable_tag {
};

class HexfileOutputFilter {
public:
    typedef char char_type;
    typedef HexfileOutputFilterCategory category;

public:
    unsigned segment, offset;

public:
    HexfileOutputFilter():segment(0), offset(0) {}

protected:
    void hexbyte(char *dest, unsigned char data) {
        static const char digits[] = "0123456789ABCDEF";
        dest[0] = digits[data / 16];
        dest[1] = digits[data % 16];
    }

    void hexshort(char *dest, unsigned short data) {
        hexbyte(dest, data / 256);
        hexbyte(dest + 2, data % 256);
    }

    template<typename Sink>
    std::streamsize write_record(Sink& snk, size_t length, unsigned address,
                                 unsigned type, const unsigned char *data) {
        char buffer[1 + (4 + 0x10 + 1) * 2 + 2], *p = buffer;
        size_t i;
        unsigned char checksum;

        if (length > 0x10)
            return 0;

        *p++ = ':';

        hexbyte(p, length);
        p += 2;

        hexshort(p, address);
        p += 4;

        hexbyte(p, type);
        p += 2;

        checksum = -(length + address / 256 + address + type);

        for (i = 0; i < length; ++i) {
            hexbyte(p, data[i]);
            checksum -= data[i];
            p += 2;
        }

        hexbyte(p, checksum);
        p += 2;

        *p++ = '\r';
        *p++ = '\n';

        return io::write(snk, buffer, p - buffer);
    }

public:
    template<typename Sink>
    std::streamsize write(Sink& snk, const char* s, std::streamsize n) { 
        std::streamsize rest = n, written = 0;

        while (rest > 0) {
            n = rest > 0x10 ? 0x10 : rest;
            if (offset >= 0x8000) {
                ++segment;
                offset = 0;

                write_record(snk, 0, 0, 0x10 + segment, NULL);
            }

            if (offset + n > 0x8000)
                n = 0x8000 - offset;

            std::streamsize ret = write_record(snk, n, offset, 0,
                                               (const unsigned char*)s);
            if (ret == 0)
                break;

            s += n;
            rest -= n;
            written += n;
            offset += n;
        }

        return written;
    }

    template<typename Sink>
    void close(Sink &snk) {
        write_record(snk, 0, 0, 0x01, NULL);
    }
};

#endif
