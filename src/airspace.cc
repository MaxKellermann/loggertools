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

#include "airspace.hh"

Airspace::Airspace(const std::string &_name, type_t _type,
                   const Altitude &_bottom, const Altitude &_top,
                   const EdgeList &_edges)
    :name(_name), type(_type),
     bottom(_bottom), top(_top),
     edges(_edges),
     voice(0) {
}

Airspace::Airspace(const std::string &_name, type_t _type,
                   const Altitude &_bottom, const Altitude &_top,
                   const Altitude &_top2,
                   const EdgeList &_edges,
                   const Frequency &_frequency,
                   unsigned _voice)
    :name(_name), type(_type),
     bottom(_bottom), top(_top), top2(_top2),
     edges(_edges),
     frequency(_frequency),
     voice(_voice) {
}
