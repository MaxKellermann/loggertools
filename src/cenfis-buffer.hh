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
 */

#ifndef __CENFIS_BUFFER_HH
#define __CENFIS_BUFFER_HH

#include "cenfis-airspace.h"
#include "airspace.hh"

#include <assert.h>
#include <string>
#include <ostream>

class CenfisBuffer {
private:
    char *buffer;
    size_t base, buffer_size, buffer_pos;
    unsigned num_vertices;
    static Latitude::value_t latitude_sum;
    static Longitude::value_t longitude_sum;

public:
    CenfisBuffer()
        :buffer(NULL), base(0), buffer_size(0), buffer_pos(0),
         num_vertices(0) {}

    CenfisBuffer(size_t _base)
        :buffer(NULL), base(_base), buffer_size(0), buffer_pos(0),
         num_vertices(0) {}

    ~CenfisBuffer()
    {
        if (buffer != NULL)
            free(buffer);
    }

protected:
    void need_buffer(size_t min_size)
    {
        if (buffer_size >= min_size)
            return;
        size_t new_size = buffer_size;
        if (new_size < 1024)
            new_size = 1024;
        do
            new_size *= 2;
        while (new_size < min_size);
        char *new_buffer = new char[new_size];
        if (new_buffer == NULL)
            throw "XXX out of memory";
        memcpy(new_buffer, buffer, buffer_pos);
        buffer = new_buffer;
        buffer_size = new_size;
    }

    const void *data() const
    {
        return buffer;
    }

public:
    size_t tell() const
    {
        return buffer_pos;
    }

    void fill(uint8_t ch, size_t length);
    void append(const void *buffer, size_t length);
    void append(const char *s);
    void append(const std::string &s);
    void append(const Altitude &alt);
    void append(const SurfacePosition &pos);
    void append_first(const SurfacePosition &pos);
    void append(const SurfacePosition &pos, const SurfacePosition &rel);

    void
    append_anchor(const SurfacePosition &rel);

private:
    void append_circle(const Edge &edge);
    void append_arc(const Edge &edge, const SurfacePosition &rel);

public:
    void append(const Edge &edge, const SurfacePosition &rel);

    void append(const Frequency &frequency);

    void append_byte(uint8_t ch)
    {
        append(&ch, sizeof(ch));
    }

    void append_short(uint16_t v)
    {
        append_byte(v >> 8);
        append_byte(v & 0xff);
    }

    void append_long(uint32_t v)
    {
        append_short(v >> 16);
        append_short(v & 0xffff);
    }

    void make_header() {
        assert(buffer_pos == 0);
        need_buffer(sizeof(header()));
        buffer_pos = sizeof(header());
        memset(buffer, 0xff, sizeof(header()));
        header().voice_ind = 0;
    }

    struct cenfis_airspace_header &header()
    {
        assert(buffer_pos >= sizeof(header()));
        return *(struct cenfis_airspace_header*)buffer;
    }

    unsigned char &operator [](size_t offset)
    {
        assert(offset < buffer_pos);
        return (unsigned char&)buffer[offset];
    }

    void auto_bank_switch(size_t length)
    {
        size_t pos = base + tell();
        if (pos / 0x8000 == (pos + length) / 0x8000)
            return;

        /* don't write across bank limit, insert 0xff padding
           instead */
        fill(0xff, ((pos + length) / 0x8000) * 0x8000 - pos);
    }

    void encrypt(size_t length);

    CenfisBuffer &operator <<(const CenfisBuffer &src)
    {
        auto_bank_switch(src.tell());
        append(src.data(), src.tell());
        return *this;
    }

    friend std::ostream &operator <<(std::ostream &os, const CenfisBuffer &src)
    {
        os.write((const std::ostream::char_type*)src.data(), src.tell());
        return os;
    }
} __attribute__((packed));

#endif
