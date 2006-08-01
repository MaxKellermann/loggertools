/*
 * loggertools
 * Copyright (C) 2004 Max Kellermann (max@duempel.org)
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
 *
 * $Id$
 */

#include <stdio.h>

#include "earth.hh"

#include <string>

class Runway {
public:
    enum type_t {
        TYPE_UNKNOWN,
        TYPE_GRASS,
        TYPE_ASPHALT
    };
private:
    type_t type;
    unsigned direction, length;
public:
    Runway();
    Runway(type_t _type, unsigned _direction, unsigned _length);
public:
    bool defined() const;
    type_t getType() const {
        return type;
    }
    unsigned getDirection() const {
        return direction;
    }
    unsigned getLength() const {
        return length;
    }
};

class TurnPoint {
public:
    enum type_t {
        TYPE_UNKNOWN,
        TYPE_AIRFIELD,
        TYPE_MILITARY_AIRFIELD,
        TYPE_GLIDER_SITE,
        TYPE_OUTLANDING,
        TYPE_MOUNTAIN_PASS,
        TYPE_MOUNTAIN_TOP,
        TYPE_SENDER,
        TYPE_VOR,
        TYPE_NDB,
        TYPE_COOL_TOWER,
        TYPE_DAM,
        TYPE_TUNNEL,
        TYPE_BRIDGE,
        TYPE_POWER_PLANT,
        TYPE_CASTLE,
        TYPE_CHURCH,
        TYPE_INTERSECTION,
        TYPE_THERMIK
    };
private:
    std::string title, code, country;
    Position position;
    type_t type;
    Runway runway;
    unsigned frequency;
    std::string description;
public:
    TurnPoint();
    TurnPoint(const std::string &_title,
              const std::string &_code,
              const std::string &_country,
              const Position &_position,
              type_t _type,
              const Runway &_runway,
              unsigned _frequency,
              const std::string &_description);
    ~TurnPoint();
public:
    const std::string &getTitle() const {
        return title;
    }
    void setTitle(const std::string &_title);
    const std::string &getCode() const {
        return code;
    }
    void setCode(const std::string &_code);
    const std::string &getCountry() const {
        return country;
    }
    void setCountry(const std::string &_country);
    const Position &getPosition() const {
        return position;
    }
    void setPosition(const Position &_position);
    type_t getType() const {
        return type;
    }
    void setType(type_t _type);
    const Runway &getRunway() const {
        return runway;
    }
    void setRunway(const Runway &runway);
    unsigned getFrequency() const {
        return frequency;
    }
    void setFrequency(unsigned _freq);
    const std::string &getDescription() const {
        return description;
    }
    void setDescription(const std::string &_description);
};

class TurnPointReaderException {
private:
    char *msg;
public:
    TurnPointReaderException(const char *fmt, ...)
        __attribute__((format(printf, 2, 3)));
    TurnPointReaderException(const TurnPointReaderException &ex);
    virtual ~TurnPointReaderException();
public:
    const char *getMessage() const {
        return msg;
    }
};

class TurnPointWriterException {
private:
    char *msg;
public:
    TurnPointWriterException(const char *fmt, ...)
        __attribute__((format(printf, 2, 3)));
    TurnPointWriterException(const TurnPointWriterException &ex);
    virtual ~TurnPointWriterException();
public:
    const char *getMessage() const {
        return msg;
    }
};

class TurnPointReader {
public:
    virtual ~TurnPointReader();
public:
    virtual const TurnPoint *read() = 0;
};

class TurnPointWriter {
public:
    virtual ~TurnPointWriter();
public:
    virtual void write(const TurnPoint &tp) = 0;
    virtual void flush() = 0;
};

class TurnPointFormat {
public:
    virtual ~TurnPointFormat();
public:
    virtual TurnPointReader *createReader(FILE *file) = 0;
    virtual TurnPointWriter *createWriter(FILE *file) = 0;
};

class SeeYouTurnPointFormat : public TurnPointFormat {
public:
    virtual TurnPointReader *createReader(FILE *file);
    virtual TurnPointWriter *createWriter(FILE *file);
};

class CenfisTurnPointFormat : public TurnPointFormat {
public:
    virtual TurnPointReader *createReader(FILE *file);
    virtual TurnPointWriter *createWriter(FILE *file);
};

class CenfisDatabaseFormat : public TurnPointFormat {
public:
    virtual TurnPointReader *createReader(FILE *file);
    virtual TurnPointWriter *createWriter(FILE *file);
};

class FilserTurnPointFormat : public TurnPointFormat {
public:
    virtual TurnPointReader *createReader(FILE *file);
    virtual TurnPointWriter *createWriter(FILE *file);
};

class ZanderTurnPointFormat : public TurnPointFormat {
public:
    virtual TurnPointReader *createReader(FILE *file);
    virtual TurnPointWriter *createWriter(FILE *file);
};

TurnPointFormat *getTurnPointFormat(const char *ext);


class TurnPointFilter {
public:
    virtual ~TurnPointFilter();
public:
    virtual TurnPointReader *createFilter(TurnPointReader *reader,
                                          const char *args) const = 0;
};

class DistanceTurnPointFilter : public TurnPointFilter {
public:
    virtual TurnPointReader *createFilter(TurnPointReader *reader,
                                          const char *args) const;
};

const TurnPointFilter *getTurnPointFilter(const char *name);
