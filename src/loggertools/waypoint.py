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

from loggertools.earth import SurfacePosition

class Waypoint:
    def __init__(self, name, position):
        assert isinstance(name, str) or isinstance(name, unicode)
        assert isinstance(position, SurfacePosition)

        self.name = name
        self.position = position

def load_wz(f):
    waypoints = list()
    for line in f:
        line = line.strip()
        if len(line) == 0 or line[0] == '#': continue
        name = line[0:12].strip()
        lat = line[13:20]
        latitude = int(lat[0:2]) * 3600 + int(lat[2:4]) * 60 + int(lat[4:6])
        if lat[6] != 'N': latitude = -latitude
        lon = line[21:29]
        longitude = int(lon[0:3]) * 3600 + int(lon[3:5]) * 60 + int(lon[5:7])
        if lon[7] != 'E': longitude = -longitude
        waypoints.append(Waypoint(name, SurfacePosition(latitude, longitude)))
    return waypoints
