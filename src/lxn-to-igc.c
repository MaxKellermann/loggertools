/*
 * loggertools
 * Copyright (C) 2004-2007 Max Kellermann <max@duempel.org>
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

#include "lxn-to-igc.h"
#include "lxn-reader.h"

struct lxn_to_igc {
    struct lxn_reader reader;
    FILE *igc;
    unsigned char flight_no;
    char date[7];
    struct lxn_flight_info flight_info;
    unsigned time, origin_time;
    int origin_latitude, origin_longitude;
    int is_event;
    struct lxn_event event;
    char fix_stat;
    const char *error;
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

int lxn_to_igc_open(FILE *igc, lxn_to_igc_t *fti_r) {
    lxn_to_igc_t fti;

    assert(igc != NULL);

    fti = (lxn_to_igc_t)calloc(1, sizeof(*fti));
    if (fti == NULL)
        return errno;

    fti->igc = igc;

    *fti_r = fti;
    return 0;
}

int lxn_to_igc_close(lxn_to_igc_t *fti_r) {
    lxn_to_igc_t fti;

    assert(fti_r != NULL);
    assert(*fti_r != NULL);

    fti = *fti_r;
    *fti_r = NULL;

    if (!fti->reader.is_end) {
        free(fti);
        return -1;
    }

    free(fti);

    return 0;
}

static int set_error(lxn_to_igc_t fti, const char *error) {
    fti->error = error;
    return -1;
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

static int handle_position(lxn_to_igc_t fti,
                           const struct lxn_position *position) {
    int latitude, longitude;

    fti->fix_stat = position->cmd == LXN_POSITION_OK ? 'A' : 'V';
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

    if (fti->reader.b_ext.num == 0)
        fprintf(fti->igc, "\r\n");

    return 0;
}

int lxn_to_igc_process(lxn_to_igc_t fti,
                       const unsigned char *fil,
                       size_t length, size_t *consumed_r) {
    unsigned i, l;
    int ret;
    char ch;
    union lxn_packet p;

    if (length <= 0)
        return EINVAL;

    if (fti->reader.is_end) {
        *consumed_r = length;
        return 0;
    }

    fti->reader.input = fil;
    fti->reader.input_length = length;
    fti->reader.input_consumed = 0;

    while (fti->reader.input_consumed < fti->reader.input_length) {
        ret = lxn_read(&fti->reader);
        *consumed_r = fti->reader.input_consumed;
        if (ret != 0)
            return ret;

        p = fti->reader.packet;

        switch (*p.cmd) {
        case 0x00:
            fprintf(fti->igc, "LFILEMPTY%u\r\n",
                    (unsigned)fti->reader.packet_length);
            break;

        case LXN_END:
            assert(fti->reader.is_end);
            *consumed_r = length;
            return 0;

        case LXN_VERSION:
            fprintf(fti->igc,
                    "HFRFWFIRMWAREVERSION:%3.1f\r\n"
                    "HFRHWHARDWAREVERSION:%3.1f\r\n",
                    p.version->software / 10.,
                    p.version->hardware / 10.);
            break;

        case LXN_START:
            if (memcmp(p.start->streraz, "STReRAZ\0", 8) != 0)
                return set_error(fti, "Invalid signature LXN_START packet");

            fti->flight_no = p.start->flight_no;
            break;

        case LXN_ORIGIN:
            fti->origin_time = ntohl(p.origin->time);
            fti->origin_latitude = (int32_t)ntohl(p.origin->latitude);
            fti->origin_longitude = (int32_t)ntohl(p.origin->longitude);

            fprintf(fti->igc, "LLXNORIGIN%02d%02d%02d" "%02d%05d%c" "%03d%05d%c\r\n",
                    fti->origin_time / 3600, fti->origin_time % 3600 / 60, fti->origin_time % 60,
                    abs(fti->origin_latitude) / 60000, abs(fti->origin_latitude) % 60000,
                    fti->origin_latitude >= 0 ? 'N' : 'S',
                    abs(fti->origin_longitude) / 60000, abs(fti->origin_longitude) % 60000,
                    fti->origin_longitude >= 0 ? 'E' : 'W');
            break;

        case LXN_SECURITY_OLD:
            fprintf(fti->igc, "G%22.22s\r\n", p.security_old->foo);
            break;

        case LXN_SERIAL:
            if (!valid_string(p.serial->serial, sizeof(p.serial->serial)))
                return set_error(fti, "Invalid serial number in LXN_SERIAL packet");

            fprintf(fti->igc, "A%sFLIGHT:%u\r\nHFDTE%s\r\n",
                    p.serial->serial, fti->flight_no, fti->date);
            break;

        case LXN_POSITION_OK:
        case LXN_POSITION_BAD:
            ret = handle_position(fti, p.position);
            if (ret != 0)
                return ret;
            break;

        case LXN_SECURITY:
            if (p.security->type == LXN_SECURITY_HIGH)
                ch = '2';
            else if (p.security->type == LXN_SECURITY_MED)
                ch = '1';
            else if (p.security->type == LXN_SECURITY_LOW)
                ch = '0';
            else
                return set_error(fti, "Invalid security type");

            fprintf(fti->igc, "G%c", ch);

            for (i = 0; i < p.security->length; ++i)
                fprintf(fti->igc, "%02X", p.security->foo[i]);

            fprintf(fti->igc, "\r\n");
            break;

        case LXN_COMPETITION_CLASS:
            if (!valid_string(p.competition_class->class_id,
                              sizeof(p.competition_class->class_id)))
                return set_error(fti, "Invalid competition class");

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
            break;

        case LXN_TASK:
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
                        return set_error(fti, "Invalid task name");

                    fprintf(fti->igc, "C%02d%05d%c" "%03d%05d%c" "%s\r\n",
                            abs(latitude) / 60000, abs(latitude) % 60000, latitude >= 0 ?  'N' : 'S',
                            abs(longitude) / 60000, abs(longitude) % 60000, longitude >= 0 ? 'E' : 'W',
                            p.task->name[i]);
                }
            }
            break;

        case LXN_EVENT:
            if (!valid_string(p.event->foo, sizeof(p.event->foo)))
                return set_error(fti, "Invalid event name");

            fti->event = *p.event;
            fti->is_event = 1;
            break;

        case LXN_B_EXT:
            for (i = 0; i < fti->reader.b_ext.num; ++i)
                fprintf(fti->igc, "%0*u", fti->reader.b_ext.widths[i],
                        ntohs(p.b_ext->data[i]));

            fprintf(fti->igc, "\r\n");
            break;

        case LXN_K_EXT:
            l = fti->time + p.k_ext->foo;
            fprintf(fti->igc, "K%02d%02d%02d",
                    l / 3600, l % 3600 / 60, l % 60);

            for (i = 0; i < fti->reader.k_ext.num; ++i)
                fprintf(fti->igc, "%0*u", fti->reader.k_ext.widths[i],
                        ntohs(p.k_ext->data[i]));

            fprintf(fti->igc, "\r\n");
            break;

        case LXN_DATE:
            if (p.date->day > 31 || p.date->month > 12)
                return set_error(fti, "Invalid date");

            snprintf(fti->date, sizeof(fti->date),
                     "%02d%02d%02d",
                     p.date->day % 100, p.date->month % 100,
                     ntohs(p.date->year));
            break;

        case LXN_FLIGHT_INFO:
            if (!valid_string(p.flight_info->pilot,
                              sizeof(p.flight_info->pilot)) ||
                !valid_string(p.flight_info->glider,
                              sizeof(p.flight_info->glider)) ||
                !valid_string(p.flight_info->registration,
                              sizeof(p.flight_info->registration)) ||
                !valid_string(p.flight_info->competition_class,
                              sizeof(p.flight_info->competition_class)) ||
                !valid_string(p.flight_info->gps, sizeof(p.flight_info->gps)))
                return set_error(fti, "Invalid LXN_FLIGHT_INFO packet");

            if (p.flight_info->competition_class_id > 7)
                return set_error(fti, "Invalid competition class id in LXN_FLIGHT_INFO packet");

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
            break;

        case LXN_K_EXT_CONFIG:
        case LXN_B_EXT_CONFIG:
            break;

        default:
            if (*p.cmd < 0x40) {
                fprintf(fti->igc, "%.*s\r\n",
                        p.string->length, p.string->value);
            } else {
                return set_error(fti, "Unknown packet");
            }
        }
    }

    return EAGAIN;
}

const char *lxn_to_igc_error(lxn_to_igc_t fti) {
    return fti->error;
}
