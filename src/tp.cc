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

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>

#include "tp.hh"

Runway::Runway()
    :type(TYPE_UNKNOWN), direction(UINT_MAX), length(0) {
}

Runway::Runway(type_t _type, unsigned _direction, unsigned _length)
    :type(_type), direction(_direction), length(_length) {
}

bool Runway::defined() const {
    return direction != UINT_MAX;
}

TurnPoint::TurnPoint(void)
    :type(TYPE_UNKNOWN),
     frequency(0) {
}

TurnPoint::TurnPoint(const std::string &_title,
                     const std::string &_code,
                     const std::string &_country,
                     const Position &_position,
                     type_t _type,
                     const Runway &_runway,
                     unsigned _frequency,
                     const std::string &_description)
    :title(_title), code(_code), country(_country),
     position(_position),
     type(_type),
     runway(_runway),
     frequency(_frequency),
     description(_description) {
}

TurnPoint::~TurnPoint(void) {
}

void TurnPoint::setTitle(const std::string &_title) {
    title = _title;
}

void TurnPoint::setCode(const std::string &_code) {
    code = _code;
}

void TurnPoint::setCountry(const std::string &_country) {
    country = _country;
}

void TurnPoint::setPosition(const Position &_position) {
    position = _position;
}

void TurnPoint::setType(type_t _type) {
    type = _type;
}

void TurnPoint::setRunway(const Runway &_runway) {
    runway = _runway;
}

void TurnPoint::setFrequency(unsigned _frequency) {
    frequency = _frequency;
}

void TurnPoint::setDescription(const std::string &_description) {
    description = _description;
}

TurnPointReaderException::TurnPointReaderException(const char *fmt, ...) {
    va_list ap;
    char buffer[4096];

    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, ap);

    msg = strdup(buffer);
}

TurnPointReaderException::TurnPointReaderException(const TurnPointReaderException &ex)
    :msg(strdup(ex.getMessage())) {
}

TurnPointReaderException::~TurnPointReaderException(void) {
    if (msg != NULL)
        free(msg);
}

TurnPointWriterException::TurnPointWriterException(const char *fmt, ...) {
    va_list ap;
    char buffer[4096];

    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, ap);

    msg = strdup(buffer);
}

TurnPointWriterException::TurnPointWriterException(const TurnPointWriterException &ex)
    :msg(strdup(ex.getMessage())) {
}

TurnPointWriterException::~TurnPointWriterException(void) {
    if (msg != NULL)
        free(msg);
}

TurnPointReader::~TurnPointReader(void) {
}

TurnPointWriter::~TurnPointWriter(void) {
}

TurnPointFormat::~TurnPointFormat(void) {
}

static SeeYouTurnPointFormat seeYouFormat;
static CenfisTurnPointFormat cenfisFormat;
static CenfisDatabaseFormat cenfisDatabaseFormat;
static FilserTurnPointFormat filserFormat;
static ZanderTurnPointFormat zanderFormat;

TurnPointFormat *getTurnPointFormat(const char *ext) {
    if (strcasecmp(ext, "cup") == 0)
        return &seeYouFormat;
    else if (strcasecmp(ext, "cdb") == 0 ||
             strcasecmp(ext, "idb") == 0)
        return &cenfisFormat;
    else if (strcasecmp(ext, "dab") == 0)
        return &cenfisDatabaseFormat;
    else if (strcasecmp(ext, "da4") == 0)
        return &filserFormat;
    else if (strcasecmp(ext, "wz") == 0)
        return &zanderFormat;
    else
        return NULL;
}

