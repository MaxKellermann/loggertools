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

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>

#include "tp.hh"

static char *copy_string(const char *p) {
    return p == NULL
        ? NULL
        : strdup(p);
}

Altitude::Altitude()
    :value(0), unit(UNIT_UNKNOWN), ref(REF_UNKNOWN) {
}

Altitude::Altitude(long _value, unit_t _unit, ref_t _ref)
    :value(_value), unit(_unit), ref(_ref) {
}

Position::Position(const Angle &_lat, const Angle &_lon,
                   const Altitude &_alt)
    :latitude(_lat),
     longitude(_lon),
     altitude(_alt) {
}

Position::Position(const Position &position)
    :latitude(position.getLatitude()),
     longitude(position.getLongitude()),
     altitude(position.getAltitude()) {
}

void Position::operator =(const Position &pos) {
    latitude = pos.getLatitude();
    longitude = pos.getLongitude();
    altitude = pos.getAltitude();
}

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
    :title(NULL), code(NULL), country(NULL),
     type(TYPE_UNKNOWN),
     frequency(0), description(NULL) {
}

TurnPoint::TurnPoint(const char *_title, const char *_code,
                     const char *_country,
                     const Position &_position,
                     type_t _type,
                     const Runway &_runway,
                     unsigned _frequency,
                     const char *_description)
    :title(copy_string(_title)),
     code(copy_string(_code)),
     country(copy_string(_country)),
     position(_position),
     type(_type),
     runway(_runway),
     frequency(_frequency),
     description(copy_string(_description)) {
}

TurnPoint::~TurnPoint(void) {
    if (title != NULL)
        free(title);
    if (code != NULL)
        free(code);
    if (country != NULL)
        free(country);
    if (description != NULL)
        free(description);
}

void TurnPoint::setTitle(const char *_title) {
    if (title != NULL)
        free(title);
    title = copy_string(_title);
}

void TurnPoint::setCode(const char *_code) {
    if (code != NULL)
        free(code);
    code = copy_string(_code);
}

void TurnPoint::setCountry(const char *_country) {
    if (country != NULL)
        free(country);
    country = copy_string(_country);
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

void TurnPoint::setDescription(const char *_description) {
    if (description != NULL)
        free(description);
    description = copy_string(_description);
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
    else
        return NULL;
}

