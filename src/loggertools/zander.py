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

import struct
from loggertools.earth import SurfacePosition

def load_angle(x):
    assert isinstance(x, str)
    assert len(x) == 4
    x = struct.unpack('BBBB', x)
    assert(x[3] == 0 or x[3] == 1)
    value = reduce(lambda a, b: a * 60 + b, x[0:3])
    if x[3] != 0:
        value = -value
    return value

def save_angle(value):
    assert isinstance(value, int)
    assert value >= -180 * 3600
    assert value <= 180 * 3600
    if value < 0:
        sign = 1
        value = -value
    else:
        sign = 0
    return struct.pack('BBBB', value / 3600, (value / 60) % 60,
                       value % 60, sign)

def load_surface_position(x):
    assert isinstance(x, str)
    assert len(x) == 8
    return SurfacePosition(load_angle(x[0:4]), load_angle(x[4:8]))

def save_surface_position(position):
    assert isinstance(position, SurfacePosition)
    return save_angle(position.latitude) + save_angle(position.longitude)
