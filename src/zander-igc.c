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

#include "zander-igc.h"

#include <string.h>
#include <stdlib.h>

enum zander_command {
    ZAN_CMD_RELATIVE = 0xcf,
    ZAN_CMD_BARO_MINUS_25 = 0xd1,
    ZAN_CMD_BARO_PLUS_25 = 0xd2,
    ZAN_CMD_GPS_MINUS_200 = 0xd3,
    ZAN_CMD_GPS_PLUS_200 = 0xd4,
    ZAN_CMD_POSITION = 0xd5,
    ZAN_CMD_GPS_QUALITY = 0xd7,
    ZAN_CMD_EXTENDED = 0xd8,
    ZAN_CMD_WIND = 0xda,
    ZAN_CMD_UNKNOWN21 = 0xdb,
    ZAN_CMD_EOF = 0xde,
};

enum zander_extended {
    ZAN_EXT_BASIC = 0x08,
    ZAN_EXT_ALTITUDE = 0x0c,
    ZAN_EXT_TASK = 0x13,
    ZAN_EXT_LZAN = 0x14,
    ZAN_EXT_LZAN2 = 0x15,
    ZAN_EXT_LZAN3 = 0x16,
    ZAN_EXT_LZAN4 = 0x17,
    ZAN_EXT_DATETIME = 0x18,
    ZAN_EXT_DATETIME3 = 0x1c,
    ZAN_EXT_UNKNOWN6 = 0x1d,
    ZAN_EXT_DATETIME2 = 0x1e,
    ZAN_EXT_SECURITY2 = 0x24,
    ZAN_EXT_SECURITY = 0x25,
};

struct zander_angle_quarters {
    unsigned char degrees_18;
    unsigned char minutes_17;
    unsigned char seconds_16;
    unsigned char seconds_quarter;
} __attribute__((packed));

struct position {
    int latitude, longitude;
    int baro_altitude, gps_altitude;
    unsigned char gps_quality;
};

/**
 * Read a fixed buffer from the input file, and check for errors and
 * short reads.
 */
static enum zander_to_igc_result
checked_read(FILE *in, void *buffer, size_t length)
{
    ssize_t nbytes = fread(buffer, length, 1, in);
    if (nbytes != 1)
        return ferror(in)
            ? ZANDER_IGC_ERRNO
            : ZANDER_IGC_EOF;

    return ZANDER_IGC_SUCCESS;
}

/**
 * Read a fixed buffer from the input file, and subtracts 0x21 from
 * every byte.  After a short header, all ZAN files are "encrypted"
 * with this method.
 */
static enum zander_to_igc_result
checked_read21(FILE *in, void *buffer, size_t length)
{
    enum zander_to_igc_result ret;
    unsigned char *p, *end;

    ret = checked_read(in, buffer, length);
    if (ret != ZANDER_IGC_SUCCESS)
        return ret;

    p = buffer;
    end = p + length;

    while (p < end) {
        *p = *p - 0x21;
        ++p;
    }

    return ZANDER_IGC_SUCCESS;
}

/**
 * Convert an zander_angle_quarters struct to an integer.
 */
static int
angle_quarters_to_int(const struct zander_angle_quarters *delta)
{
    return ((delta->degrees_18 * 64 +
             delta->minutes_17) * 64 +
            delta->seconds_16) * 64 +
        delta->seconds_quarter;
}

/**
 * Add a specific number of seconds to a zander_datetime struct.
 */
static void
zander_datetime_add(struct zander_datetime *dt, unsigned seconds)
{
    dt->time.second += seconds;
    if (dt->time.second >= 60) {
        dt->time.second -= 60;
        ++dt->time.minute;
        if (dt->time.minute >= 60) {
            dt->time.minute -= 60;
            ++dt->time.hour;
            /* XXX check hour >= 24 */
        }
    }
}

/**
 * Dump the whole input file as "G" records to the IGC file.  This
 * function also seeks the input file to the beginning.
 */
static enum zander_to_igc_result
hexdump_g(FILE *in, FILE *out)
{
    int ret;
    unsigned char buffer[32];
    size_t nbytes, i;

    ret = fseek(in, 0, SEEK_SET);
    if (ret < 0)
        return ZANDER_IGC_ERRNO;

    while ((nbytes = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        fputc('G', out);
        for (i = 0; i < nbytes; ++i)
            fprintf(out, "%02X", buffer[i]);
        fputc('\n', out);
    }

    if (ferror(in))
        return ZANDER_IGC_ERRNO;

    return ZANDER_IGC_SUCCESS;
}

static enum zander_to_igc_result
read_basic(FILE *in, FILE *out, const char *sig,
           const struct zander_date *date)
{
    enum zander_to_igc_result ret;
    struct {
        char unknown1[4];
        unsigned char unknown2[4];
        struct zander_personal_data personal_data;
    } basic;

    ret = checked_read21(in, &basic, sizeof(basic));
    if (ret != ZANDER_IGC_SUCCESS)
        return ret;

    fprintf(out, "AZAN%.4s%.18s\n",
            basic.unknown1, sig + 1);
    fprintf(out, "HFDTE%02u%02u%02u\nHFFXA050\n",
            date->day, date->month, date->year);
    fprintf(out, "HFPLT Pilot             :%.*s\n"
            "HFGTY Glider type       :%.*s\n"
            "HFGID Glider ID         :%.*s\n"
            "HFDTM100 GPS Datum      :WGS84\n"
            "HFRFW Firmware Version  :%.5s\n"
            "HFRHW Hardware version  :A\n"
            "HFFTY FR Type           :%.12s\n"
            "HFGPS                   :Motorola,VP ONCORE,8,30000\n"
            "HFPRS Press Alt Sensor  :Motorola,MPX100A,10000\n"
            "HFCCL Competition Class :%.*s\n"
            "HFCID Competition ID    :%.*s\n",
            (int)sizeof(basic.personal_data.name),
            basic.personal_data.name,
            (int)sizeof(basic.personal_data.aircraft_model),
            basic.personal_data.aircraft_model,
            (int)sizeof(basic.personal_data.registration),
            basic.personal_data.registration,
            sig + 15,
            sig + 1,
            (int)sizeof(basic.personal_data.competition_class),
            basic.personal_data.competition_class,
            (int)sizeof(basic.personal_data.competition_sign),
            basic.personal_data.competition_sign);
    fprintf(out, "I023638IAS3941ENL\n"
            "J020810WDI1113WVE\n");
    return ZANDER_IGC_SUCCESS;
}

static enum zander_to_igc_result
read_task(FILE *in, FILE *out)
{
    enum zander_to_igc_result ret;
    struct zander_task_wp task;

    ret = checked_read21(in, &task, sizeof(task));
    if (ret != ZANDER_IGC_SUCCESS)
        return ret;

    if (task.num_wp == 0x80) {
        /* task header */
        const struct zander_time *time =
            (const struct zander_time *)task.name;
        fprintf(out, "C%02u%02u%02u%02u%02u%02u%02u%02u%02u000101\n",
                task.date.day,
                task.date.month,
                task.date.year,
                time->hour,
                time->minute,
                time->second,
                task.date.day,
                task.date.month,
                task.date.year);
    } else {
        /* turn point */
        fprintf(out, "C%02u%02u%03u%c%03u%02u%03u%c %.12s\n",
                task.latitude.degrees,
                task.latitude.minutes,
                task.latitude.seconds * 1000 / 60,
                task.latitude.sign ? 'S' : 'N',
                task.longitude.degrees,
                task.longitude.minutes,
                task.longitude.seconds * 1000 / 60,
                task.longitude.sign ? 'W' : 'E',
                task.name);
    }

    return ZANDER_IGC_SUCCESS;
}

static enum zander_to_igc_result
read_position(FILE *in, struct position *position)
{
    enum zander_to_igc_result ret;
    struct zander_angle_quarters angle;

    ret = checked_read21(in, &angle, sizeof(angle));
    if (ret != ZANDER_IGC_SUCCESS)
        return ret;

    position->latitude = angle_quarters_to_int(&angle);

    ret = checked_read21(in, &angle, sizeof(angle));
    if (ret != ZANDER_IGC_SUCCESS)
        return ret;

    position->longitude = angle_quarters_to_int(&angle);

    return ZANDER_IGC_SUCCESS;
}

static enum zander_to_igc_result
read_altitude(FILE *in, struct position *position)
{
    enum zander_to_igc_result ret;
    struct {
        uint16_t altitude;
        uint16_t unknown;
    } altitude;

    ret = checked_read21(in, &altitude, sizeof(altitude));
    if (ret != ZANDER_IGC_SUCCESS)
        return ret;

    position->baro_altitude = ntohs(altitude.altitude);
    return ZANDER_IGC_SUCCESS;
}

static enum zander_to_igc_result
read_relative(FILE *in, FILE *out, struct zander_datetime *datetime,
              struct position *position)
{
    enum zander_to_igc_result ret;
    struct {
        unsigned char ias;
        unsigned char baro_altitude;
        unsigned char gps_altitude;
        unsigned char latitude;
        unsigned char longitude;
    } relative;

    ret = checked_read21(in, &relative, sizeof(relative));
    if (ret != ZANDER_IGC_SUCCESS)
        return ret;

    fflush(out);
    fprintf(stderr, "B %02x %02x %02x %02x %02x\n",
            relative.ias, (uint8_t)relative.baro_altitude, (uint8_t)relative.gps_altitude,
            (uint8_t)relative.latitude, (uint8_t)relative.longitude);
    fflush(stderr);

    if (relative.latitude != 0x32) {
        position->latitude += relative.latitude - 0x9a;
        position->longitude += relative.longitude - 0x64;
    }

    position->baro_altitude += relative.baro_altitude % 0x34 - 0x19;
    position->gps_altitude += (relative.gps_altitude % 0x34 - 0x19) * 8;

    zander_datetime_add(datetime, 4);
    fprintf(out, "B%02u%02u%02u%02u%05u%c%03u%05u%c%c%05u%05u%03u%03u\n",
            datetime->time.hour,
            datetime->time.minute,
            datetime->time.second,
            abs(position->latitude) / 3600 / 4,
            abs(position->latitude) % (3600 * 4) * 1000 / 60 / 4,
            position->latitude < 0 ? 'S' : 'N',
            abs(position->longitude) / 3600 / 4,
            abs(position->longitude) % (3600 * 4) * 1000 / 60 / 4,
            position->longitude < 0 ? 'W' : 'E',
            relative.latitude == 0x32 ? 'V' : 'A',
            position->baro_altitude,
            position->gps_quality > 0x80  ? position->gps_altitude : 0,
            relative.ias,
            (relative.baro_altitude / 0x34) * 100 +
            (relative.gps_altitude / 0x34) * 10);

    return ZANDER_IGC_SUCCESS;
}

static enum zander_to_igc_result
read_wind(FILE *in, FILE *out, const struct zander_time *time)
{
    enum zander_to_igc_result ret;
    struct {
        unsigned char unknown;
        unsigned char velocity;
        unsigned char direction;
    } wind;

    ret = checked_read21(in, &wind, sizeof(wind));
    if (ret != ZANDER_IGC_SUCCESS)
        return ret;

    fprintf(out, "K%02u%02u%02u%03u%03u\n",
            time->hour, time->minute, time->second,
            wind.direction * 2, wind.velocity);
    return ZANDER_IGC_SUCCESS;
}

static enum zander_to_igc_result
read_security(FILE *in, FILE *out)
{
    static const char hex_digits[16] = "0123456789ABCDEF";
    enum zander_to_igc_result ret;
    unsigned char security[128];

    ret = checked_read21(in, security, sizeof(security));
    if (ret != ZANDER_IGC_SUCCESS)
        return ret;

    fputc('G', out);
    for (ret = 0; ret < 64; ++ret)
        fputc(hex_digits[security[ret] & 0xf], out);
    fputc('\n', out);
    fputc('G', out);
    for (ret = 0; ret < 64; ++ret)
        fputc(hex_digits[security[64 + ret] & 0xf], out);
    fputc('\n', out);
    return ZANDER_IGC_SUCCESS;
}

enum zander_to_igc_result
zander_to_igc(FILE *in, FILE *out)
{
    enum zander_to_igc_result ret;
    char sig[0x14];
    struct {
        unsigned char unknown1[0xa];
    } header;
    struct zander_datetime datetime, datetime2;
    struct position position;
    unsigned char cmd;
    unsigned char unknown21[21], unknown6[6];

    ret = checked_read(in, sig, sizeof(sig));
    if (ret != ZANDER_IGC_SUCCESS)
        return ret;

    ret = checked_read21(in, &header, sizeof(header));
    if (ret != ZANDER_IGC_SUCCESS)
        return ret;

    memset(&position, 0, sizeof(position));

    while (true) {
        ret = checked_read21(in, &cmd, sizeof(cmd));
        if (ret != ZANDER_IGC_SUCCESS)
            return ret;

        fflush(out);
        fprintf(stderr, "X cmd=0x%02x\n", cmd);
        fflush(stderr);

        switch ((enum zander_command)cmd) {
        case ZAN_CMD_RELATIVE:
            ret = read_relative(in, out, &datetime, &position);
            if (ret != ZANDER_IGC_SUCCESS)
                return ret;
            break;

        case ZAN_CMD_BARO_MINUS_25:
            position.baro_altitude -= 25;
            break;

        case ZAN_CMD_BARO_PLUS_25:
            position.baro_altitude += 25;
            break;

        case ZAN_CMD_GPS_MINUS_200:
            position.gps_altitude -= 200;
            break;

        case ZAN_CMD_GPS_PLUS_200:
            position.gps_altitude += 200;
            break;

        case ZAN_CMD_POSITION:
            ret = read_position(in, &position);
            if (ret != ZANDER_IGC_SUCCESS)
                return ret;
            break;

        case ZAN_CMD_GPS_QUALITY:
            ret = checked_read21(in, &position.gps_quality,
                                 sizeof(position.gps_quality));
            if (ret != ZANDER_IGC_SUCCESS)
                return ret;
            break;

        case ZAN_CMD_WIND:
            ret = read_wind(in, out, &datetime.time);
            if (ret != ZANDER_IGC_SUCCESS)
                return ret;
            break;

        case ZAN_CMD_UNKNOWN21:
            ret = checked_read21(in, unknown21, sizeof(unknown21));
            if (ret != ZANDER_IGC_SUCCESS)
                return ret;
            break;

        case ZAN_CMD_EXTENDED:
            ret = checked_read21(in, &cmd, sizeof(cmd));
            if (ret != ZANDER_IGC_SUCCESS)
                return ret;

            fprintf(stderr, "\text=0x%02x\n", cmd);

            switch ((enum zander_extended)cmd) {
            case ZAN_EXT_BASIC:
                ret = read_basic(in, out, sig, &datetime.date);
                if (ret != ZANDER_IGC_SUCCESS)
                    return ret;
                break;

            case ZAN_EXT_ALTITUDE:
                ret = read_altitude(in, &position);
                if (ret != ZANDER_IGC_SUCCESS)
                    return ret;
               break;

            case ZAN_EXT_TASK:
                /* pre-flight task declaration */
                ret = read_task(in, out);
                if (ret != ZANDER_IGC_SUCCESS)
                    return ret;
                break;

            case ZAN_EXT_LZAN:
                ret = checked_read21(in, &cmd, sizeof(cmd));
                if (ret != ZANDER_IGC_SUCCESS)
                    return ret;

                switch (cmd) {
                case 0x02:
                    fprintf(out, "LZAN %02u%02u%02u PowerOn\n",
                            datetime.time.hour,
                            datetime.time.minute,
                            datetime.time.second);
                    break;

                case 0x0c:
                    fprintf(out, "LZAN %02u%02u%02u PowerOff\n",
                            datetime.time.hour,
                            datetime.time.minute,
                            datetime.time.second);
                    break;

                default:
                    fprintf(stderr, "unknown lzan 0x%02x\n", cmd);
                    return ZANDER_IGC_MALFORMED;
                }
                break;

            case ZAN_EXT_LZAN2:
                ret = checked_read21(in, &cmd, sizeof(cmd));
                if (ret != ZANDER_IGC_SUCCESS)
                    return ret;

                switch (cmd) {
                case 0x16:
                    zander_datetime_add(&datetime, 1);
                    fprintf(out, "LZAN %02u%02u%02u TimeOut\n",
                            datetime.time.hour,
                            datetime.time.minute,
                            datetime.time.second);
                    break;

                default:
                    fprintf(stderr, "unknown lzan2 0x%02x\n", cmd);
                    return ZANDER_IGC_MALFORMED;
                }
                break;

            case ZAN_EXT_LZAN3:
                ret = checked_read21(in, &cmd, sizeof(cmd));
                if (ret != ZANDER_IGC_SUCCESS)
                    return ret;

                switch (cmd) {
                case 0x0c:
                    datetime2 = datetime;
                    zander_datetime_add(&datetime2, 2);
                    fprintf(out, "LZAN %02u%02u%02u PowerOff\n",
                            datetime2.time.hour,
                            datetime2.time.minute,
                            datetime2.time.second);
                    break;

                default:
                    fprintf(stderr, "unknown lzan3 0x%02x\n", cmd);
                    return ZANDER_IGC_MALFORMED;
                }
                break;

            case ZAN_EXT_LZAN4:
                ret = checked_read21(in, &cmd, sizeof(cmd));
                if (ret != ZANDER_IGC_SUCCESS)
                    return ret;

                switch (cmd) {
                case 0x16:
                    zander_datetime_add(&datetime, 3);
                    fprintf(out, "LZAN %02u%02u%02u TimeOut\n",
                            datetime.time.hour,
                            datetime.time.minute,
                            datetime.time.second);
                    break;

                default:
                    fprintf(stderr, "unknown lzan4 0x%02x\n", cmd);
                    return ZANDER_IGC_MALFORMED;
                }
                break;

            case ZAN_EXT_DATETIME:
                ret = checked_read21(in, &datetime, sizeof(datetime));
                if (ret != ZANDER_IGC_SUCCESS)
                    return ret;

                /* round down to 4 seconds */
                datetime.time.second &= ~0x03;
                break;

            case ZAN_EXT_DATETIME2:
            case ZAN_EXT_DATETIME3:
                /* what's this for? */
                ret = checked_read21(in, &datetime2, sizeof(datetime2));
                if (ret != ZANDER_IGC_SUCCESS)
                    return ret;
                break;

            case ZAN_EXT_UNKNOWN6:
                ret = checked_read21(in, unknown6, sizeof(unknown6));
                if (ret != ZANDER_IGC_SUCCESS)
                    return ret;
                break;

            case ZAN_EXT_SECURITY:
            case ZAN_EXT_SECURITY2:
                /* the security "G" record */
                ret = read_security(in, out);
                if (ret != ZANDER_IGC_SUCCESS)
                    return ret;
                break;

            default:
                fprintf(stderr, "unknown ext 0x%02x\n", cmd);
                return ZANDER_IGC_MALFORMED;
            }
            break;

        case ZAN_CMD_EOF:
            return hexdump_g(in, out);

        default:
            fprintf(stderr, "unknown record 0x%02x at 0x%lx\n",
                    cmd, ftell(in));
            return ZANDER_IGC_MALFORMED;
        }
    }
}
