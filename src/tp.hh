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

class Altitude {
public:
    enum unit_t {
        UNIT_UNKNOWN = 0,
        UNIT_METERS = 1,
        UNIT_FEET = 2
    };
    enum ref_t {
        REF_UNKNOWN = 0,
        REF_MSL = 1,
        REF_GND = 2,
        REF_1013 = 3,
        REF_AIRFIELD = 4
    };
private:
    long value;
    unit_t unit;
    ref_t ref;
public:
    Altitude();
    Altitude(long _value, unit_t _unit, ref_t _ref);
public:
    bool defined() const {
        return unit != UNIT_UNKNOWN && ref != REF_UNKNOWN;
    }
    long getValue() const {
        return value;
    }
    unit_t getUnit() const {
        return unit;
    }
    ref_t getRef() const {
        return ref;
    }
};

class Angle {
private:
    int value;
public:
    Angle():value(0) {}
    Angle(int _value):value(_value) {}
public:
    bool defined() const {
        /* XXX */
        return value != 0;
    }
    int getValue() const {
        return value;
    }
};

class Position {
private:
    Angle latitude, longitude;
    Altitude altitude;
public:
    Position() {}
    Position(const Angle &_lat, const Angle &_lon,
             const Altitude &_alt);
    Position(const Position &position);
    void operator =(const Position &pos);
public:
    bool defined() const {
        return latitude.defined() &&
            longitude.defined();
    }
    const Angle &getLatitude() const {
        return latitude;
    }
    const Angle &getLongitude() const {
        return longitude;
    }
    const Altitude &getAltitude() const {
        return altitude;
    }
};

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
    char *title, *code, *country;
    Position position;
    type_t type;
    Runway runway;
    unsigned frequency;
    char *description;
public:
    TurnPoint();
    TurnPoint(const char *_title, const char *_code,
              const char *_country,
              const Position &_position,
              type_t _type,
              const Runway &_runway,
              unsigned _frequency,
              const char *_description);
    ~TurnPoint();
public:
    const char *getTitle() const {
        return title;
    }
    void setTitle(const char *_title);
    const char *getCode() const {
        return code;
    }
    void setCode(const char *_code);
    const char *getCountry() const {
        return country;
    }
    void setCountry(const char *_country);
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
    const char *getDescription() const {
        return description;
    }
    void setDescription(const char *_description);
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

TurnPointFormat *getTurnPointFormat(const char *ext);
