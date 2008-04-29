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

#ifndef __LXN_READER_H
#define __LXN_READER_H

#include <stdint.h>
#include <stddef.h>

enum lxn_commands {
    LXN_END = 0x40,
    LXN_VERSION = 0x7f,
    LXN_START = 0x80,
    LXN_ORIGIN = 0xa0,
    LXN_SECURITY_OLD = 0xf5,
    LXN_SERIAL = 0xf6,
    LXN_POSITION_OK = 0xbf,
    LXN_POSITION_BAD = 0xc3,
    LXN_SECURITY = 0xf0,
    LXN_COMPETITION_CLASS = 0xf1,
    LXN_EVENT = 0xf4,
    LXN_TASK = 0xf7,
    LXN_B_EXT = 0xf9,
    LXN_K_EXT = 0xfa,
    LXN_DATE = 0xfb,
    LXN_FLIGHT_INFO = 0xfc,
    LXN_K_EXT_CONFIG = 0xfe, /* 'J': extensions in the 'K' record */
    LXN_B_EXT_CONFIG = 0xff, /* 'I': extensions to the 'B' record */
};

enum lxn_security_type {
    LXN_SECURITY_LOW = 0x0d,
    LXN_SECURITY_MED = 0x0e,
    LXN_SECURITY_HIGH = 0x0f,
};

struct lxn_string {
    unsigned char length;
    char value[];
} __attribute__((packed));

struct lxn_end {
    unsigned char cmd;
} __attribute__((packed));

struct lxn_version {
    unsigned char cmd;
    unsigned char hardware, software;
} __attribute__((packed));

struct lxn_start {
    unsigned char cmd;
    char streraz[8];
    unsigned char flight_no;
} __attribute__((packed));

struct lxn_origin {
    unsigned char cmd;
    uint32_t time, latitude, longitude;
} __attribute__((packed));

struct lxn_security_old {
    unsigned char cmd;
    char foo[22];
} __attribute__((packed));

struct lxn_serial {
    unsigned char cmd;
    char serial[9];
} __attribute__((packed));

struct lxn_position {
    unsigned char cmd;
    uint16_t time, latitude, longitude, aalt, galt;
} __attribute__((packed));

struct lxn_security {
    unsigned char cmd;
    unsigned char length, type;
    unsigned char foo[64];
} __attribute__((packed));

struct lxn_competition_class {
    unsigned char cmd;
    char class_id[9];
} __attribute__((packed));

struct lxn_event {
    unsigned char cmd;
    char foo[9];
} __attribute__((packed));

struct lxn_task {
    unsigned char cmd;
    uint32_t time;
    unsigned char day, month, year;
    unsigned char day2, month2, year2;
    uint16_t task_id;
    unsigned char num_tps;
    unsigned char usage[12];
    uint32_t longitude[12], latitude[12];
    char name[12][9];
} __attribute__((packed));

struct lxn_b_ext {
    unsigned char cmd;
    uint16_t data[];
} __attribute__((packed));

struct lxn_k_ext {
    unsigned char cmd;
    unsigned char foo;
    uint16_t data[];
} __attribute__((packed));

struct lxn_date {
    unsigned char cmd;
    unsigned char day, month;
    uint16_t year;
} __attribute__((packed));

struct lxn_flight_info {
    unsigned char cmd;
    uint16_t id;
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

struct lxn_ext_config {
    unsigned char cmd;
    uint16_t time, dat;
} __attribute__((packed));

struct extension_definition {
    char name[4];
    unsigned width;
};

struct extension_config {
    unsigned num;
    struct extension_definition extensions[16];
};

union lxn_packet {
    const unsigned char *cmd;
    const struct lxn_string *string;
    const struct lxn_end *end;
    const struct lxn_version *version;
    const struct lxn_start *start;
    const struct lxn_origin *origin;
    const struct lxn_security_old *security_old;
    const struct lxn_serial *serial;
    const struct lxn_position *position;
    const struct lxn_security *security;
    const struct lxn_competition_class *competition_class;
    const struct lxn_event *event;
    const struct lxn_task *task;
    const struct lxn_b_ext *b_ext;
    const struct lxn_k_ext *k_ext;
    const struct lxn_date *date;
    const struct lxn_flight_info *flight_info;
    const struct lxn_ext_config *ext_config;
};

struct lxn_reader {
    const unsigned char *input;
    size_t input_length, input_consumed, packet_length;
    union lxn_packet packet;

    int is_end;
    struct extension_config k_ext, b_ext;

    const char *error;
};

int lxn_read(struct lxn_reader *lxn);

#endif
