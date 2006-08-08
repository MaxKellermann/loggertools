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

#ifndef __LOGGERTOOLS_AIRSPACE_IO_HH
#define __LOGGERTOOLS_AIRSPACE_IO_HH

#include <exception>
#include <string>
#include <iosfwd>

class AirspaceReaderException : public std::exception {
private:
    std::string msg;
public:
    AirspaceReaderException(const char *fmt, ...)
        __attribute__((format(printf, 2, 3)));
    virtual ~AirspaceReaderException() throw();
public:
    virtual const char *what() const throw() {
        return msg.c_str();
    }
};

class AirspaceWriterException : public std::exception {
private:
    std::string msg;
public:
    AirspaceWriterException(const char *fmt, ...)
        __attribute__((format(printf, 2, 3)));
    virtual ~AirspaceWriterException() throw();
public:
    virtual const char *what() const throw() {
        return msg.c_str();
    }
};

class AirspaceReader {
public:
    virtual ~AirspaceReader();
public:
    virtual const Airspace *read() = 0;
};

class AirspaceWriter {
public:
    virtual ~AirspaceWriter();
public:
    virtual void write(const Airspace &as) = 0;
    virtual void flush() = 0;
};

class AirspaceFormat {
public:
    virtual ~AirspaceFormat();
public:
    virtual AirspaceReader *createReader(std::istream *stream) = 0;
    virtual AirspaceWriter *createWriter(std::ostream *stream) = 0;
};

class OpenAirAirspaceFormat : public AirspaceFormat {
public:
    virtual AirspaceReader *createReader(std::istream *stream);
    virtual AirspaceWriter *createWriter(std::ostream *stream);
};

AirspaceFormat *getAirspaceFormat(const char *ext);

#endif
