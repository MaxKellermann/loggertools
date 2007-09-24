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

#include "cenfis-crypto.h"
#include "cenfis-buffer.hh"
#include "exception.hh"

void
CenfisBuffer::fill(uint8_t ch, size_t length)
{
    need_buffer(buffer_pos + length);
    memset((char*)buffer + buffer_pos, ch, length);
    buffer_pos += length;
}

void
CenfisBuffer::append(const void *p, size_t length)
{
    need_buffer(buffer_pos + length);
    memcpy((char*)buffer + buffer_pos, p, length);
    buffer_pos += length;
}

void
CenfisBuffer::append(const char *s)
{
    const size_t length = strlen(s);
    if (length >= 0x100)
        throw container_full("Pascal string overflow");
    const unsigned char length_char = (unsigned char)length;

    append(&length_char, sizeof(length_char));
    append(s, length);
}

void
CenfisBuffer::append(const std::string &s)
{
    if (s.length() >= 0x100)
        throw container_full("Pascal string overflow");
    const unsigned char length_char = (unsigned char)s.length();

    append(&length_char, sizeof(length_char));
    append(s.data(), s.length());
}

void
CenfisBuffer::append(const Altitude &alt)
{
    const Altitude inFeet = alt.toUnit(Altitude::UNIT_FEET);
    append_byte(3);
    append_short((uint16_t)(inFeet.getValue() / 10));
    append_byte(inFeet.getRef() == Altitude::REF_GND ||
               inFeet.getRef() == Altitude::REF_AIRFIELD
               ? 'G' : 'M');
}

void
CenfisBuffer::append(const SurfacePosition &pos)
{
    append_byte(8);
    append_long(pos.getLatitude().refactor(60));
    append_long(pos.getLongitude().refactor(60));

    latitude_sum = pos.getLatitude().getValue();
    longitude_sum = pos.getLongitude().getValue();
    num_vertices = 1;
}

void
CenfisBuffer::append(const SurfacePosition &pos, const SurfacePosition &rel)
{
    append_short(pos.getLatitude().refactor(60) -
                 rel.getLatitude().refactor(60));
    append_short(pos.getLongitude().refactor(60) -
                 rel.getLongitude().refactor(60));

    latitude_sum += pos.getLatitude().getValue();
    longitude_sum += pos.getLongitude().getValue();
    ++num_vertices;
}

void
CenfisBuffer::append(const Edge &edge, const SurfacePosition &rel)
{
    switch (edge.getType()) {
    case Edge::TYPE_VERTEX:
        append(edge.getEnd(), rel);
        break;

    default: // XXX
        break;
    }
}

void
CenfisBuffer::encrypt(size_t length)
{
    assert(length <= buffer_pos);

    cenfis_encrypt(buffer + buffer_pos - length, length);
}
