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

class SeeYouTurnPointReader : public TurnPointReader {
private:
    char *filename;
    FILE *file;
    int is_eof;
    unsigned num_columns;
    char **columns;
public:
    SeeYouTurnPointReader(const char *_filename);
    virtual ~SeeYouTurnPointReader();
public:
    virtual const TurnPoint *read();
};

class SeeYouTurnPointWriter : public TurnPointWriter {
private:
    char *filename;
    FILE *file;
public:
    SeeYouTurnPointWriter(const char *_filename);
    virtual ~SeeYouTurnPointWriter();
public:
    virtual void write(const TurnPoint &tp);
    virtual void flush(void);
};
