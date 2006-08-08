/*
 * loggertools
 * Copyright (C) 2004-2006 Max Kellermann <max@duempel.org>
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

#ifndef __LOGGERTOOLS_TP_IO_HH
#define __LOGGERTOOLS_TP_IO_HH

#include <exception>
#include <string>
#include <iosfwd>

class TurnPointReaderException : public std::exception {
private:
    std::string msg;
public:
    TurnPointReaderException(const char *fmt, ...)
        __attribute__((format(printf, 2, 3)));
    virtual ~TurnPointReaderException() throw();
public:
    virtual const char *what() const throw() {
        return msg.c_str();
    }
};

class TurnPointWriterException : public std::exception {
private:
    std::string msg;
public:
    TurnPointWriterException(const char *fmt, ...)
        __attribute__((format(printf, 2, 3)));
    virtual ~TurnPointWriterException() throw();
public:
    virtual const char *what() const throw() {
        return msg.c_str();
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
    virtual TurnPointReader *createReader(std::istream *stream) const = 0;
    virtual TurnPointWriter *createWriter(std::ostream *stream) const = 0;
};

class SeeYouTurnPointFormat : public TurnPointFormat {
public:
    virtual TurnPointReader *createReader(std::istream *stream) const;
    virtual TurnPointWriter *createWriter(std::ostream *stream) const;
};

class CenfisTurnPointFormat : public TurnPointFormat {
public:
    virtual TurnPointReader *createReader(std::istream *stream) const;
    virtual TurnPointWriter *createWriter(std::ostream *stream) const;
};

class CenfisDatabaseFormat : public TurnPointFormat {
public:
    virtual TurnPointReader *createReader(std::istream *stream) const;
    virtual TurnPointWriter *createWriter(std::ostream *stream) const;
};

class CenfisHexTurnPointFormat : public TurnPointFormat {
public:
    virtual TurnPointReader *createReader(std::istream *stream) const;
    virtual TurnPointWriter *createWriter(std::ostream *stream) const;
};

class FilserTurnPointFormat : public TurnPointFormat {
public:
    virtual TurnPointReader *createReader(std::istream *stream) const;
    virtual TurnPointWriter *createWriter(std::ostream *stream) const;
};

class ZanderTurnPointFormat : public TurnPointFormat {
public:
    virtual TurnPointReader *createReader(std::istream *stream) const;
    virtual TurnPointWriter *createWriter(std::ostream *stream) const;
};

const TurnPointFormat *getTurnPointFormat(const char *ext);


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

#endif
