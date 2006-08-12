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

/*
 * Convert an LXN file to the IGC/GNSS format.
 *
 * Information about the LXN file format as well as some code taken
 * from the KFlog sources (http://www.kflog.org/).  The GNSS
 * specification (http://www.fai.org/gliding/gnss/tech_spec_gnss.asp)
 * also helped implementing this filter.
 */

#include <assert.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>

#include "filser-to-igc.h"

enum fil_commands {
    FIL_END = 0x40,
    FIL_VERSION = 0x7f,
    FIL_START = 0x80,
    FIL_ORIGIN = 0xa0,
    FIL_SECURITY_OLD = 0xf5,
    FIL_SERIAL = 0xf6,
    FIL_POSITION_OK = 0xbf,
    FIL_POSITION_BAD = 0xc3,
    FIL_SECURITY = 0xf0,
    FIL_COMPETITION_CLASS = 0xf1,
    FIL_EVENT = 0xf4,
    FIL_TASK = 0xf7,
    FIL_B_EXT = 0xf9,
    FIL_K_EXT = 0xfa,
    FIL_DATE = 0xfb,
    FIL_FLIGHT_INFO = 0xfc,
    FIL_K_EXT_CONFIG = 0xfe, /* 'J': extensions in the 'K' record */
    FIL_B_EXT_CONFIG = 0xff, /* 'I': extensions to the 'B' record */
};

enum fil_security_type {
    FIL_SECURITY_LOW = 0x0d,
    FIL_SECURITY_MED = 0x0e,
    FIL_SECURITY_HIGH = 0x0f,
};

struct fil_string {
    unsigned char length;
    char value[];
} __attribute__((packed));

struct fil_end {
    unsigned char cmd;
} __attribute__((packed));

struct fil_version {
    unsigned char cmd;
    unsigned char hardware, software;
} __attribute__((packed));

struct fil_start {
    unsigned char cmd;
    char streraz[8];
    unsigned char flight_no;
} __attribute__((packed));

struct fil_origin {
    unsigned char cmd;
    u_int32_t time, latitude, longitude;
} __attribute__((packed));

struct fil_security_old {
    unsigned char cmd;
    char foo[22];
} __attribute__((packed));

struct fil_serial {
    unsigned char cmd;
    char serial[9];
} __attribute__((packed));

struct fil_position {
    unsigned char cmd;
    u_int16_t time, latitude, longitude, aalt, galt;
} __attribute__((packed));

struct fil_security {
    unsigned char cmd;
    unsigned char length, type;
    unsigned char foo[64];
} __attribute__((packed));

struct fil_competition_class {
    unsigned char cmd;
    char class_id[9];
} __attribute__((packed));

struct fil_event {
    unsigned char cmd;
    char foo[9];
} __attribute__((packed));

struct fil_task {
    unsigned char cmd;
    u_int32_t time;
    unsigned char day, month, year;
    unsigned char day2, month2, year2;
    u_int16_t task_id;
    unsigned char num_tps;
    unsigned char usage[12];
    u_int32_t longitude[12], latitude[12];
    char name[12][9];
} __attribute__((packed));

struct fil_b_ext {
    unsigned char cmd;
    u_int16_t data[];
} __attribute__((packed));

struct fil_k_ext {
    unsigned char cmd;
    unsigned char foo;
    u_int16_t data[];
} __attribute__((packed));

struct fil_date {
    unsigned char cmd;
    unsigned char day, month;
    u_int16_t year;
} __attribute__((packed));

struct fil_flight_info {
    unsigned char cmd;
    u_int16_t id;
    char pilot[19];
    char glider[12];
    char registration[8];
    char competition_class[4];
    unsigned char competition_class_id;
    char observer[10];
    unsigned char gps_date;
    unsigned char fix_accuracy;
    char gps[60];
} __attribute__((packed));

struct fil_ext_config {
    unsigned char cmd;
    u_int16_t time, dat;
} __attribute__((packed));

struct extension_config {
    unsigned num, widths[16];
};

struct filser_to_igc {
    FILE *igc;
    unsigned char flight_no;
    char date[7];
    struct fil_flight_info flight_info;
    struct extension_config k_ext, b_ext;
    unsigned time, origin_time;
    int origin_latitude, origin_longitude;
    int is_event;
    struct fil_event event;
    char fix_stat;
    int is_end;
};

struct extension_definition {
    char name[4];
    unsigned width;
};

static const struct extension_definition extension_defs[16] = {
    { "FXA", 3 },
    { "VXA", 3 },
    { "RPM", 5 },
    { "GSP", 5 },
    { "IAS", 5 },
    { "TAS", 5 },
    { "HDM", 3 },
    { "HDT", 3 },
    { "TRM", 3 },
    { "TRT", 3 },
    { "TEN", 5 },
    { "WDI", 3 },
    { "WVE", 5 },
    { "ENL", 3 },
    { "VAR", 3 },
    { "XX3", 3 }
};

int filser_to_igc_open(FILE *igc, struct filser_to_igc **fti_r) {
    struct filser_to_igc *fti;

    assert(igc != NULL);

    fti = (struct filser_to_igc*)calloc(1, sizeof(*fti));
    if (fti == NULL)
        return errno;

    fti->igc = igc;

    *fti_r = fti;
    return 0;
}

int filser_to_igc_close(struct filser_to_igc **fti_r) {
    struct filser_to_igc *fti;

    assert(fti_r != NULL);
    assert(*fti_r != NULL);

    fti = *fti_r;
    *fti_r = NULL;

    if (!fti->is_end) {
        free(fti);
        return -1;
    }

    free(fti);

    return 0;
}

static int valid_string(const char *p, size_t size) {
    return memchr(p, 0, size) != NULL;
}

static const char *format_gps_date(unsigned char gps_date) {
    static const char *const gps_date_tab[] = {
        "ADINDAN        ",
        "AFGOOYE        ",
        "AIN EL ABD 1970",
        "COCOS ISLAND   ",
        "ARC 1950       ",
        "ARC 1960       ",
        "ASCENSION 1958 ",
        "ASTRO BEACON E ",
        "AUSTRALIAN 1966",
        "AUSTRALIAN 1984",
        "ASTRO DOS 7/14 ",
        "MARCUS ISLAND  ",
        "TERN ISLAND    ",
        "BELLEVUE (IGN) ",
        "BERMUDA 1957   ",
        "COLOMBIA       ",
        "CAMPO INCHAUSPE",
        "CANTON ASTRO   ",
        "CAPE CANAVERAL ",
        "CAPE (AFRICA)  ",
        "CARTHAGE       ",
        "CHATHAM 1971   ",
        "CHUA ASTRO     ",
        "CORREGO ALEGRE ",
        "DJAKARTA       ",
        "DOS 1968       ",
        "EASTER ISLAND  ",
        "EUROPEAN 1950  ",
        "EUROPEAN 1979  ",
        "FINLAND 1910   ",
        "GANDAJIKA BASE ",
        "NEW ZEALAND '49",
        "OSGB 1936      ",
        "GUAM 1963      ",
        "GUX 1 ASTRO    ",
        "HJOESEY 1955   ",
        "HONG KONG 1962 ",
        "INDIAN/NEPAL   ",
        "INDIAN/VIETNAM ",
        "IRELAND 1965   ",
        "DIEGO GARCIA   ",
        "JOHNSTON 1961  ",
        "KANDAWALA      ",
        "KERGUELEN ISL. ",
        "KERTAU 1948    ",
        "CAYMAN BRAC    ",
        "LIBERIA 1964   ",
        "LUZON/MINDANAO ",
        "LUZON PHILIPPI.",
        "MAHE 1971      ",
        "MARCO ASTRO    ",
        "MASSAWA        ",
        "MERCHICH       ",
        "MIDWAY ASTRO'61",
        "MINNA (NIGERIA)",
        "NAD-1927 ALASKA",
        "NAD-1927 BAHAM.",
        "NAD-1927 CENTR.",
        "NAD-1927 CANAL ",
        "NAD-1927 CANADA",
        "NAD-1927 CARIB.",
        "NAD-1927 CONUS ",
        "NAD-1927 CUBA  ",
        "NAD-1927 GREEN.",
        "NAD-1927 MEXICO",
        "NAD-1927 SALVA.",
        "NAD-1983       ",
        "NAPARIMA       ",
        "MASIRAH ISLAND ",
        "SAUDI ARABIA   ",
        "ARAB EMIRATES  ",
        "OBSERVATORIO'66",
        "OLD EGYIPTIAN  ",
        "OLD HAWAIIAN   ",
        "OMAN           ",
        "CANARY ISLAND  ",
        "PICAIRN 1967   ",
        "PUERTO RICO    ",
        "QATAR NATIONAL ",
        "QORNOQ         ",
        "REUNION        ",
        "ROME 1940      ",
        "RT-90 SWEDEN   ",
        "S.AMERICA  1956",
        "S.AMERICA  1956",
        "SOUTH ASIA     ",
        "CHILEAN 1963   ",
        "SANTO(DOS)     ",
        "SAO BRAZ       ",
        "SAPPER HILL    ",
        "SCHWARZECK     ",
        "SOUTHEAST BASE ",
        "FAIAL          ",
        "TIMBALI 1948   ",
        "TOKYO          ",
        "TRISTAN ASTRO  ",
        "RESERVED       ",
        "VITI LEVU 1916 ",
        "WAKE-ENIWETOK  ",
        "WGS-1972       ",
        "WGS-1984       ",
        "ZANDERIJ       ",
        "CH-1903        "
    };

    if (gps_date < 103)
        return gps_date_tab[gps_date];
    else
        return "";
}

static const char *format_competition_class(unsigned char class_id) {
    static const char *const names[] = {
        "STANDARD",
        "15-METER",
        "OPEN",
        "18-METER",
        "WORLD",
        "DOUBLE",
        "MOTOR_GL",
    };

    assert(class_id < 7);
    return names[class_id];
}

static int handle_position(struct filser_to_igc *fti,
                           const struct fil_position *position) {
    int latitude, longitude;

    fti->fix_stat = position->cmd == FIL_POSITION_OK ? 'A' : 'V';
    fti->time = fti->origin_time + (int16_t)ntohs(position->time);
    latitude = fti->origin_latitude + (int16_t)ntohs(position->latitude);
    longitude = fti->origin_longitude + (int16_t)ntohs(position->longitude);

    if (fti->is_event) {
        fprintf(fti->igc,"E%02d%02d%02d%s\r\n",
                fti->time / 3600, fti->time % 3600 / 60, fti->time % 60,
                fti->event.foo);
        fti->is_event = 0;
    }

    fprintf(fti->igc, "B%02d%02d%02d",
            fti->time / 3600, fti->time % 3600 / 60, fti->time % 60);
    fprintf(fti->igc, "%02d%05d%c" "%03d%05d%c",
            abs(latitude) / 60000, abs(latitude) % 60000,
            latitude >= 0 ? 'N' : 'S',
            abs(longitude) / 60000, abs(longitude) % 60000,
            longitude >= 0 ? 'E' : 'W');
    fprintf(fti->igc, "%c", fti->fix_stat);
    fprintf(fti->igc, "%05d%05d",
            ntohs(position->aalt), ntohs(position->galt));

    if (fti->b_ext.num == 0)
        fprintf(fti->igc, "\r\n");

    return 0;
}

static int handle_ext_config(struct filser_to_igc *fti,
                             struct extension_config *config,
                             const struct fil_ext_config *packet,
                             char record, unsigned column) {
    unsigned ext_dat, i, bit;

    /* count bits in extension mask */
    ext_dat = ntohs(packet->dat);
    for (bit = 0, config->num = 0; bit < 16; ++bit)
        if (ext_dat & (1 << bit))
            ++config->num;

    if (config->num == 0)
        return 0;

    /* begin record */
    fprintf(fti->igc, "%c%02d", record, config->num);

    /* write information about each extension */
    for (i = 0, bit = 0; bit < 16; ++bit) {
        if (ext_dat & (1 << bit)) {
            fprintf(fti->igc, "%02d%02d%s", column,
                    column + extension_defs[bit].width - 1,
                    extension_defs[bit].name);
            column += extension_defs[bit].width;
            config->widths[i] = extension_defs[bit].width;
            i++;
        }
    }

    fprintf(fti->igc, "\r\n");

    return 0;
}

int filser_to_igc_process(struct filser_to_igc *fti,
                          const unsigned char *fil,
                          size_t length, size_t *consumed_r) {
    unsigned i, l;
    int ret;
    char ch;
    size_t position = 0;
    union {
        const unsigned char *cmd;
        const struct fil_string *string;
        const struct fil_end *end;
        const struct fil_version *version;
        const struct fil_start *start;
        const struct fil_origin *origin;
        const struct fil_security_old *security_old;
        const struct fil_serial *serial;
        const struct fil_position *position;
        const struct fil_security *security;
        const struct fil_competition_class *competition_class;
        const struct fil_event *event;
        const struct fil_task *task;
        const struct fil_b_ext *b_ext;
        const struct fil_k_ext *k_ext;
        const struct fil_date *date;
        const struct fil_flight_info *flight_info;
        const struct fil_ext_config *ext_config;
    } p;

    if (length <= 0)
        return EINVAL;

    if (fti->is_end) {
        *consumed_r = length;
        return 0;
    }

    while (position < length) {
        p.cmd = fil + position;

        switch (fil[position]) {
        case 0x00:
            for (i = 1, ++position; position < length && fil[position] == 0; ++i)
                ++position;
            fprintf(fti->igc, "LFILEMPTY%u\r\n", i);
            break;

        case FIL_END:
            if (length - position < sizeof(*p.version)) {
                *consumed_r = position;
                return EAGAIN;
            }

            fti->is_end = 1;

            *consumed_r = length;
            return 0;

        case FIL_VERSION:
            if (length - position < sizeof(*p.version)) {
                *consumed_r = position;
                return EAGAIN;
            }

            fprintf(fti->igc,
                    "HFRFWFIRMWAREVERSION:%3.1f\r\n"
                    "HFRHWHARDWAREVERSION:%3.1f\r\n",
                    p.version->software / 10.,
                    p.version->hardware / 10.);

            position += sizeof(*p.version);
            break;

        case FIL_START:
            if (length - position < sizeof(*p.start)) {
                *consumed_r = position;
                return EAGAIN;
            }

            if (memcmp(p.start->streraz, "STReRAZ\0", 8) != 0)
                return -1;

            fti->flight_no = p.start->flight_no;

            position += sizeof(*p.start);
            break;

        case FIL_ORIGIN:
            if (length - position < sizeof(*p.origin)) {
                *consumed_r = position;
                return EAGAIN;
            }

            fti->origin_time = ntohl(p.origin->time);
            fti->origin_latitude = (int32_t)ntohl(p.origin->latitude);
            fti->origin_longitude = (int32_t)ntohl(p.origin->longitude);

            fprintf(fti->igc, "LLXNORIGIN%02d%02d%02d" "%02d%05d%c" "%03d%05d%c\r\n",
                    fti->origin_time / 3600, fti->origin_time % 3600 / 60, fti->origin_time % 60,
                    abs(fti->origin_latitude) / 60000, abs(fti->origin_latitude) % 60000,
                    fti->origin_latitude >= 0 ? 'N' : 'S',
                    abs(fti->origin_longitude) / 60000, abs(fti->origin_longitude) % 60000,
                    fti->origin_longitude >= 0 ? 'E' : 'W');

            position += sizeof(*p.origin);
            break;

        case FIL_SECURITY_OLD:
            if (length - position < sizeof(*p.security_old)) {
                *consumed_r = position;
                return EAGAIN;
            }

            fprintf(fti->igc, "G%22.22s\r\n", p.security_old->foo);

            position += sizeof(*p.security_old);
            break;

        case FIL_SERIAL:
            if (length - position < sizeof(*p.serial)) {
                *consumed_r = position;
                return EAGAIN;
            }

            if (!valid_string(p.serial->serial, sizeof(p.serial->serial)))
                return -1;

            fprintf(fti->igc, "A%sFLIGHT:%u\r\nHFDTE%s\r\n",
                    p.serial->serial, fti->flight_no, fti->date);

            position += sizeof(*p.serial);
            break;

        case FIL_POSITION_OK:
        case FIL_POSITION_BAD:
            if (length - position < sizeof(*p.position)) {
                *consumed_r = position;
                return EAGAIN;
            }

            ret = handle_position(fti, p.position);
            if (ret != 0)
                return ret;

            position += sizeof(*p.position);
            break;

        case FIL_SECURITY:
            if (length - position < sizeof(*p.security)) {
                *consumed_r = position;
                return EAGAIN;
            }

            if (p.security->type == FIL_SECURITY_HIGH)
                ch = '2';
            else if (p.security->type == FIL_SECURITY_MED)
                ch = '1';
            else if (p.security->type == FIL_SECURITY_LOW)
                ch = '0';
            else
                return -1;

            fprintf(fti->igc, "G%c", ch);

            for (i = 0; i < p.security->length; ++i)
                fprintf(fti->igc, "%02X", p.security->foo[i]);

            fprintf(fti->igc, "\r\n");

            position += sizeof(*p.security);
            break;

        case FIL_COMPETITION_CLASS:
            if (length - position < sizeof(*p.competition_class)) {
                *consumed_r = position;
                return EAGAIN;
            }

            if (!valid_string(p.competition_class->class_id,
                              sizeof(p.competition_class->class_id)))
                return -1;

            if (fti->flight_info.competition_class_id == 7)
                fprintf(fti->igc,
                        "HFFXA%03d\r\n"
                        "HFPLTPILOT:%s\r\n"
                        "HFGTYGLIDERTYPE:%s\r\n"
                        "HFGIDGLIDERID:%s\r\n"
                        "HFDTM%03dGPSDATUM:%s\r\n"
                        "HFCIDCOMPETITIONID:%s\r\n"
                        "HFCCLCOMPETITIONCLASS:%s\r\n"
                        "HFGPSGPS:%s\r\n",
                        fti->flight_info.fix_accuracy,
                        fti->flight_info.pilot,
                        fti->flight_info.glider,
                        fti->flight_info.registration,
                        fti->flight_info.gps_date,
                        format_gps_date(fti->flight_info.gps_date),
                        fti->flight_info.competition_class,
                        p.competition_class->class_id,
                        fti->flight_info.gps);

            position += sizeof(*p.competition_class);
            break;

        case FIL_TASK:
            if (length - position < sizeof(*p.task)) {
                *consumed_r = position;
                return EAGAIN;
            }

            fti->time = ntohl(p.task->time);

            fprintf(fti->igc, "C%02d%02d%02d%02d%02d%02d"
                    "%02d%02d%02d" "%04d%02d\r\n",
                    p.task->day, p.task->month, p.task->year,
                    fti->time / 3600, fti->time % 3600 / 60, fti->time % 60,
                    p.task->day2, p.task->month2, p.task->year2,
                    ntohs(p.task->task_id), p.task->num_tps);

            for (i = 0; i < 12; ++i) {
                if (p.task->usage[i]) {
                    int latitude = (int32_t)ntohl(p.task->latitude[i]);
                    int longitude = (int32_t)ntohl(p.task->longitude[i]);

                    if (!valid_string(p.task->name[i], sizeof(p.task->name[i])))
                        return -1;

                    fprintf(fti->igc, "C%02d%05d%c" "%03d%05d%c" "%s\r\n",
                            abs(latitude) / 60000, abs(latitude) % 60000, latitude >= 0 ?  'N' : 'S',
                            abs(longitude) / 60000, abs(longitude) % 60000, longitude >= 0 ? 'E' : 'W',
                            p.task->name[i]);
                }
            }

            position += sizeof(*p.task);
            break;

        case FIL_EVENT:
            if (length - position < sizeof(*p.event)) {
                *consumed_r = position;
                return EAGAIN;
            }

            if (!valid_string(p.event->foo, sizeof(p.event->foo)))
                return -1;

            fti->event = *p.event;
            fti->is_event = 1;

            position += sizeof(*p.event);
            break;

        case FIL_B_EXT:
            if (length - position < sizeof(*p.b_ext)) {
                *consumed_r = position;
                return EAGAIN;
            }

            for (i = 0; i < fti->b_ext.num; ++i)
                fprintf(fti->igc, "%0*u", fti->b_ext.widths[i],
                        ntohs(p.b_ext->data[i]));

            fprintf(fti->igc, "\r\n");

            position += sizeof(*p.b_ext) + fti->b_ext.num * sizeof(p.b_ext->data[0]);
            break;

        case FIL_K_EXT:
            if (length - position < sizeof(*p.k_ext)) {
                *consumed_r = position;
                return EAGAIN;
            }

            l = fti->time + p.k_ext->foo;
            fprintf(fti->igc, "K%02d%02d%02d",
                    l / 3600, l % 3600 / 60, l % 60);

            for (i = 0; i < fti->k_ext.num; ++i)
                fprintf(fti->igc, "%0*u", fti->k_ext.widths[i],
                        ntohs(p.k_ext->data[i]));

            fprintf(fti->igc, "\r\n");

            position += sizeof(*p.k_ext) + fti->k_ext.num * sizeof(p.k_ext->data[0]);
            break;

        case FIL_DATE:
            if (length - position < sizeof(*p.date)) {
                *consumed_r = position;
                return EAGAIN;
            }

            if (p.date->day > 31 || p.date->month > 12)
                return -1;

            snprintf(fti->date, sizeof(fti->date),
                     "%02d%02d%02d",
                     p.date->day % 100, p.date->month % 100,
                     ntohs(p.date->year));

            position += sizeof(*p.date);
            break;

        case FIL_FLIGHT_INFO:
            if (length - position < sizeof(*p.flight_info)) {
                *consumed_r = position;
                return EAGAIN;
            }

            if (!valid_string(p.flight_info->pilot,
                              sizeof(p.flight_info->pilot)) ||
                !valid_string(p.flight_info->glider,
                              sizeof(p.flight_info->glider)) ||
                !valid_string(p.flight_info->registration,
                              sizeof(p.flight_info->registration)) ||
                !valid_string(p.flight_info->competition_class,
                              sizeof(p.flight_info->competition_class)) ||
                !valid_string(p.flight_info->gps, sizeof(p.flight_info->gps)))
                return -1;

            if (p.flight_info->competition_class_id > 7)
                return -1;

            if (p.flight_info->competition_class_id < 7)
                fprintf(fti->igc,
                        "HFFXA%03d\r\n"
                        "HFPLTPILOT:%s\r\n"
                        "HFGTYGLIDERTYPE:%s\r\n"
                        "HFGIDGLIDERID:%s\r\n"
                        "HFDTM%03dGPSDATUM:%s\r\n"
                        "HFCIDCOMPETITIONID:%s\r\n"
                        "HFCCLCOMPETITIONCLASS:%s\r\n"
                        "HFGPSGPS:%s\r\n",
                        p.flight_info->fix_accuracy,
                        p.flight_info->pilot,
                        p.flight_info->glider,
                        p.flight_info->registration,
                        p.flight_info->gps_date,
                        format_gps_date(p.flight_info->gps_date),
                        p.flight_info->competition_class,
                        format_competition_class(p.flight_info->competition_class_id),
                        p.flight_info->gps);

            fti->flight_info = *p.flight_info;

            position += sizeof(*p.flight_info);
            break;

        case FIL_K_EXT_CONFIG:
            if (length - position < sizeof(*p.ext_config)) {
                *consumed_r = position;
                return EAGAIN;
            }

            ret = handle_ext_config(fti, &fti->k_ext, p.ext_config, 'J', 8);
            if (ret != 0)
                return ret;

            position += sizeof(*p.ext_config);
            break;


        case FIL_B_EXT_CONFIG:
            if (length - position < sizeof(*p.ext_config)) {
                *consumed_r = position;
                return EAGAIN;
            }

            ret = handle_ext_config(fti, &fti->b_ext, p.ext_config, 'I', 36);
            if (ret != 0)
                return ret;

            position += sizeof(*p.ext_config);
            break;

        default:
            if (*p.cmd < 0x40) {
                if (length - position < sizeof(*p.string) + p.string->length) {
                    *consumed_r = position;
                    return EAGAIN;
                }

                fprintf(fti->igc, "%.*s\r\n",
                        p.string->length, p.string->value);

                position += sizeof(*p.string) + p.string->length;
            } else {
                return -1;
            }
        }
    }

    *consumed_r = position;
    return EAGAIN;
}
