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

#include "exception.hh"
#include "tp.hh"
#include "tp-io.hh"
#include "io-match.hh"

class TurnPointMatchName {
    std::string name;

public:
    TurnPointMatchName(const std::string &_name)
        :name(_name) {}

public:
    bool operator ()(const TurnPoint &tp) {
        return tp.getCode() == name || tp.getShortName() == name ||
            tp.getFullName() == name;
    }
};

TurnPointReader *
NameTurnPointFilter::createFilter(TurnPointReader *reader,
                                  const char *args) const {
    if (args == NULL || *args == 0)
        throw malformed_input("No name provided");

    return new MatchReader<TurnPoint, TurnPointMatchName>
        (reader, TurnPointMatchName(args));
}
