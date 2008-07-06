/*
 * loggertools
 * Copyright (C) 2004-2008 Max Kellermann <max@duempel.org>
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

#ifndef __LOGGERTOOLS_TP_IO_HH
#define __LOGGERTOOLS_TP_IO_HH

#include "io.hh"

typedef Reader<TurnPoint> TurnPointReader;
typedef Writer<TurnPoint> TurnPointWriter;
typedef Format<TurnPoint> TurnPointFormat;
typedef Filter<TurnPoint> TurnPointFilter;

class FancyTurnPointFormat : public TurnPointFormat {
public:
    virtual TurnPointReader *createReader(std::istream *stream) const;
    virtual TurnPointWriter *createWriter(std::ostream *stream) const;
};

class MilomeiTurnPointFormat : public TurnPointFormat {
public:
    virtual TurnPointReader *createReader(std::istream *stream) const;
    virtual TurnPointWriter *createWriter(std::ostream *stream) const;
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


class DistanceTurnPointFilter : public TurnPointFilter {
public:
    virtual TurnPointReader *createFilter(TurnPointReader *reader,
                                          const char *args) const;
};

class AirfieldTurnPointFilter : public TurnPointFilter {
public:
    virtual TurnPointReader *createFilter(TurnPointReader *reader,
                                          const char *args) const;
};

class NameTurnPointFilter : public TurnPointFilter {
public:
    virtual TurnPointReader *createFilter(TurnPointReader *reader,
                                          const char *args) const;
};

const TurnPointFilter *getTurnPointFilter(const char *name);

#endif
