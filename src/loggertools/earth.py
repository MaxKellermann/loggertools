#
#  loggertools
#  Copyright (C) 2004-2008 Max Kellermann <max@duempel.org>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; version 2 of the License.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

class SurfacePosition:
    def __init__(self, latitude, longitude):
        assert isinstance(latitude, int)
        assert isinstance(longitude, int)
        assert latitude >= -90 * 3600
        assert latitude <= 90 * 3600
        assert longitude >= -180 * 3600
        assert longitude <= 180 * 3600

        self.latitude = latitude
        self.longitude = longitude

    def __str__(self):
        a = abs(self.latitude)
        if self.latitude < 0:
            sign = 'S'
        else:
            sign = 'N'
        latitude = '%02u.%02u.%02u %c' % (a / 3600, (a / 60) % 60,
                                    a % 60, sign)

        a = abs(self.longitude)
        if self.longitude < 0:
            sign = 'W'
        else:
            sign = 'E'
        longitude = '%03u.%02u.%02u %c' % (a / 3600, (a / 60) % 60,
                                     a % 60, sign)

        return latitude + ' ' + longitude

def convert_angle(value):
    assert isinstance(value, int)
    assert value >= -180 * 3600
    assert value <= 180 * 3600

    import math
    return value * math.pi / (180. * 3600.)

def great_circle_distance(a, b):
    # formula from http://en.wikipedia.org/wiki/Great-circle_distance
    lat1 = convert_angle(a.latitude)
    lon1 = convert_angle(a.longitude)
    lat2 = convert_angle(b.latitude)
    lon2 = convert_angle(b.longitude)

    from math import atan2, hypot, cos, sin
    return atan2(hypot(cos(lat2) * sin(lon2 - lon1),
                       cos(lat1) * sin(lat2) -
                       sin(lat1) * cos(lat2) * cos(lon2 - lon1)),
                 (sin(lat1) * sin(lat2) +
                  cos(lat1) * cos(lat2) * cos(lon2 - lon1))) * 6372.795;
