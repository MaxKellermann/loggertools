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
#include "earth-parser.hh"

class AirfieldTurnPointReader : public TurnPointReader {
private:
    TurnPointReader *reader;
public:
    AirfieldTurnPointReader(TurnPointReader *_reader)
        :reader(_reader) {}
    virtual ~AirfieldTurnPointReader();
public:
    virtual const TurnPoint *read();
};

TurnPointReader *
AirfieldTurnPointFilter::createFilter(TurnPointReader *reader,
                                      const char *args) const
{
    if (args != NULL && *args != 0)
        throw malformed_input("No arguments supported");

    return new AirfieldTurnPointReader(reader);
}

AirfieldTurnPointReader::~AirfieldTurnPointReader()
{
    if (reader != NULL)
        delete reader;
}

static bool
is_airfield(TurnPoint::type_t type)
{
    return type == TurnPoint::TYPE_AIRFIELD ||
        type == TurnPoint::TYPE_MILITARY_AIRFIELD ||
        type == TurnPoint::TYPE_GLIDER_SITE ||
        type == TurnPoint::TYPE_ULTRALIGHT_FIELD ||
        type == TurnPoint::TYPE_OUTLANDING;
}

const TurnPoint *
AirfieldTurnPointReader::read()
{
    const TurnPoint *tp;

    if (reader == NULL)
        return NULL;

    while ((tp = reader->read()) != NULL) {
        if (is_airfield(tp->getType()))
            return tp;
        else
            delete tp;
    }

    delete reader;
    reader = NULL;
    return NULL;
}
