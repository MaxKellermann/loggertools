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
