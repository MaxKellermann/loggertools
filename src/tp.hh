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

class Position {
private:
    char *latitude, *longitude;
    long altitude;
public:
    Position(void);
    Position(const char *_lat, const char *_long,
             long _alt);
    Position(const Position &position);
    ~Position(void);
    void operator =(const Position &pos);
public:
    const char *getLatitude(void) const {
        return latitude;
    }
    const char *getLongitude(void) const {
        return longitude;
    }
    long getAltitude(void) const {
        return altitude;
    }
};

class TurnPoint {
private:
    char *title, *code, *country;
    Position position;
    int style;
    char *direction;
    unsigned length;
    char *frequency, *description;
public:
    TurnPoint(void);
    TurnPoint(const char *_title, const char *_code,
              const char *_country,
              const Position &_position,
              int _style,
              const char *_direction,
              unsigned _length,
              const char *_frequency,
              const char *_description);
    ~TurnPoint(void);
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
    int getStyle() const {
        return style;
    }
    void setStyle(int _style);
    const char *getDirection() const {
        return direction;
    }
    void setDirection(const char *_direction);
    unsigned getLength() const {
        return length;
    }
    void setLength(unsigned _length);
    const char *getFrequency() const {
        return frequency;
    }
    void setFrequency(const char *_freq);
    const char *getDescription() const {
        return description;
    }
    void setDescription(const char *_description);
};

class TurnPointReaderException {
private:
    char *msg;
public:
    TurnPointReaderException(const char *fmt, ...);
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
    TurnPointWriterException(const char *fmt, ...);
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
    virtual void flush(void) = 0;
};

class TurnPointFormat {
public:
    virtual ~TurnPointFormat();
public:
    virtual TurnPointReader *createReader(FILE *file) = 0;
    virtual TurnPointWriter *createWriter(FILE *file) = 0;
};

class SeeYouTurnPointFormat {
public:
    virtual TurnPointReader *createReader(FILE *file);
    virtual TurnPointWriter *createWriter(FILE *file);
};
