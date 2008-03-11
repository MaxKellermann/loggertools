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

#include <math.h>
#include <assert.h>
#include <stdlib.h>

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

    arc_start = &pos;
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
CenfisBuffer::append_circle(const Edge &edge)
{
    assert(edge.getType() == Edge::TYPE_CIRCLE);
    assert(edge.getCenter().defined());
    assert(edge.getRadius().defined());

    append_byte(10);
    append(edge.getCenter());
    append_short((int)(edge.getRadius().toUnit(Distance::UNIT_NAUTICAL_MILES).getValue() * 10.));
}

static int
rad_to_deg10(double angle)
{
    static const double rad_to_deg = 57.2957795;
    return (int)round(angle * rad_to_deg / 10.0);
}

static double
deg10_to_rad(int angle)
{
    static const double rad_to_deg = 57.2957795;
    return angle * 10.0 / rad_to_deg;
}

static int
sin10(int angle, double arc_radius)
{
    return (int)round(sin(deg10_to_rad(angle)) * arc_radius);
}

static int
cos10(int angle, double arc_radius)
{
    return (int)round(cos(deg10_to_rad(angle)) * arc_radius);
}

static double
my_atan2(double xr, double yr)
{
    static const double pi_h = 1.57079633;

    if (xr > 0.0)
        xr += 0.5;
    else
        xr -= 0.5;

    if (yr > 0.0)
        yr += 0.5;
    else
        yr -= 0.5;

    if (xr > 0.0 && yr > 0.0)
        return atan(xr / yr);

    if (xr > 0.0 && yr < 0.0)
        return atan(yr / xr) * -1.0 + pi_h;

    if (xr < 0.0 && yr < 0.0)
        return (atan(yr / xr) + pi_h) * -1.0;

    if (xr < 0.0 && yr > 0.0)
        return atan(xr / yr);

    abort();
}

static double
arc_radius(const SurfacePosition &start, const SurfacePosition &center)
{
    const Latitude rel_lat = start.getLatitude() - center.getLatitude();
    const Longitude rel_lon = start.getLongitude() - center.getLongitude();
    return hypot(rel_lat.refactor(60), rel_lon.refactor(60) * cos(center.getLatitude()));
}

static double
arc_angle(const SurfacePosition &start, const SurfacePosition &center)
{
    const Latitude rel_lat = start.getLatitude() - center.getLatitude();
    const Longitude rel_lon = start.getLongitude() - center.getLongitude();
    double angle = my_atan2(rel_lon.refactor(60) * cos(center.getLatitude()), rel_lat.refactor(60));
    static const double pi_2 = 6.28318531;
    if (angle < 0.0)
        angle += pi_2;
    return angle;
}

static int
arc_angle_deg10(const SurfacePosition &start, const SurfacePosition &center)
{
    return rad_to_deg10(arc_angle(start, center));
}

static int
deg10_add(int angle, int add)
{
    angle += add;
    if (angle < 0)
        angle = 35;
    else if (angle > 35)
        angle = 0;
    return angle;
}

void
CenfisBuffer::append_arc(const Edge &edge, const SurfacePosition &rel)
{
    assert(edge.getType() == Edge::TYPE_ARC);
    assert(edge.getSign() == -1 || edge.getSign() == 1);
    assert(edge.getCenter().defined());
    assert(edge.getEnd().defined());

    if (arc_start == NULL)
        return; // XXX

    double arc_radius = ::arc_radius(*arc_start, edge.getCenter());

    int start_alfa_i = deg10_add(arc_angle_deg10(*arc_start, edge.getCenter()),
                                 edge.getSign());
    int end_alfa_i = deg10_add(arc_angle_deg10(edge.getEnd(), edge.getCenter()),
                               -edge.getSign());

    static int num_points = 0; /* static due to bug reproduction */
    if (edge.getSign() > 0) {
        if (start_alfa_i < end_alfa_i)
            num_points = end_alfa_i - start_alfa_i;
        else if (start_alfa_i > end_alfa_i)
            num_points = (end_alfa_i + 36) - start_alfa_i;
    } else {
        if  (start_alfa_i < end_alfa_i)
            num_points = (start_alfa_i + 36) - end_alfa_i;
        else if (start_alfa_i > end_alfa_i)
            num_points = start_alfa_i - end_alfa_i;
    }

    for (int i = 0; i <= num_points; ++i) {
        int angle = start_alfa_i + edge.getSign() * i;

        Latitude d_latitude(cos10(angle, arc_radius), 60);
        Longitude d_longitude((int)round(sin10(angle, arc_radius) /
                                         cos(edge.getCenter().getLatitude())),
                              60);

        SurfacePosition pos(edge.getCenter().getLatitude() + d_latitude,
                            edge.getCenter().getLongitude() + d_longitude);

        append(pos, rel);
    }

    append(edge.getEnd(), rel);

    ++num_points; /* bug reproduction */
}


void
CenfisBuffer::append(const Edge &edge, const SurfacePosition &rel)
{
    switch (edge.getType()) {
    case Edge::TYPE_VERTEX:
        append(edge.getEnd(), rel);

        arc_start = &edge.getEnd();
        break;

    case Edge::TYPE_CIRCLE:
        append_circle(edge);
        break;

    case Edge::TYPE_ARC:
        append_arc(edge, rel);
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
