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

Position::Position(void)
    :latitude(NULL), longitude(NULL), altitude(LONG_MIN) {
}

Position::Position(const char *_lat, const char *_long,
                   long _alt)
    :latitude(copy_string(_lat)),
     longitude(copy_string(_long)),
     altitude(_alt) {
}

Position::Position(const Position &position)
    :latitude(copy_string(position.getLatitude())),
     longitude(copy_string(position.getLongitude())),
     altitude(position.getAltitude()) {
}

Position::~Position(void) {
    free(latitude);
    free(longitude);
}

void Position::operator =(const Position &pos) {
    free(latitude);
    free(longitude);

    latitude = copy_string(pos.getLatitude());
    longitude = copy_string(pos.getLongitude());
    altitude = pos.getAltitude();
}

TurnPoint::TurnPoint(void)
    :title(NULL), code(NULL), country(NULL),
     style(0), direction(NULL), length(0),
     frequency(NULL), description(NULL) {
}

TurnPoint::TurnPoint(const char *_title, const char *_code,
                     const char *_country,
                     const Position &_position,
                     int _style,
                     const char *_direction,
                     unsigned _length,
                     const char *_frequency,
                     const char *_description)
    :title(copy_string(_title)),
     code(copy_string(_code)),
     country(copy_string(_country)),
     position(_position),
     style(_style),
     direction(copy_string(_direction)),
     length(_length),
     frequency(copy_string(_frequency)),
     description(copy_string(_description)) {
}

TurnPoint::~TurnPoint(void) {
    if (title != NULL)
        free(title);
    if (code != NULL)
        free(code);
    if (country != NULL)
        free(country);
    if (direction != NULL)
        free(direction);
    if (frequency != NULL)
        free(frequency);
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

void TurnPoint::setStyle(int _style) {
    style = _style;
}

void TurnPoint::setDirection(const char *_direction) {
    if (direction != NULL)
        free(direction);
    direction = copy_string(_direction);
}

void TurnPoint::setLength(unsigned _length) {
    length = _length;
}

void TurnPoint::setFrequency(const char *_frequency) {
    if (frequency != NULL)
        free(frequency);
    frequency = copy_string(_frequency);
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

TurnPointReaderException::~TurnPointReaderException() {
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

TurnPointWriterException::~TurnPointWriterException() {
    if (msg != NULL)
        free(msg);
}

TurnPointReader::~TurnPointReader() {
}

TurnPointWriter::~TurnPointWriter() {
}
