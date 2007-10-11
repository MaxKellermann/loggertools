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

Latitude::value_t CenfisBuffer::latitude_sum = 0;
Longitude::value_t CenfisBuffer::longitude_sum = 0;

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

    switch (inFeet.getRef()) {
    case Altitude::REF_GND:
        append_byte('G');
        break;

    case Altitude::REF_1013:
        append_byte('S');
        break;

    default:
        append_byte('M');
        break;
    }
}

void
CenfisBuffer::append(const SurfacePosition &pos)
{
    append_long(pos.getLatitude().refactor(60));
    append_long(pos.getLongitude().refactor(60));

    latitude_sum += pos.getLatitude().refactor(60);
    longitude_sum += pos.getLongitude().refactor(60);
    ++num_vertices;
}

void
CenfisBuffer::append_first(const SurfacePosition &pos)
{
    assert(num_vertices == 0);

    latitude_sum = 0;
    longitude_sum = 0;

    append_byte(8);
    append(pos);
}

void
CenfisBuffer::append(const SurfacePosition &pos, const SurfacePosition &rel)
{
    append_short(pos.getLatitude().refactor(60) -
                 rel.getLatitude().refactor(60));
    append_short(pos.getLongitude().refactor(60) -
                 rel.getLongitude().refactor(60));

    latitude_sum += pos.getLatitude().refactor(60);
    longitude_sum += pos.getLongitude().refactor(60);
    ++num_vertices;
}

void
CenfisBuffer::append_anchor(const SurfacePosition &rel)
{
    latitude_sum /= num_vertices;
    latitude_sum -= rel.getLatitude().refactor(60);

    longitude_sum /= num_vertices;
    longitude_sum -= rel.getLongitude().refactor(60);

    append_byte(4);
    append_short(latitude_sum);
    append_short(longitude_sum);
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
CenfisBuffer::append(const Frequency &frequency)
{
    append_byte(2);
    append_short(frequency.getKiloHertz() / 25);
}

void
CenfisBuffer::encrypt(size_t length)
{
    assert(length <= buffer_pos);

    cenfis_encrypt(buffer + buffer_pos - length, length);
}
