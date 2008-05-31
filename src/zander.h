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
 */

#ifndef __ZANDER_H
#define __ZANDER_H

#include <stdint.h>
#include <stddef.h>


typedef struct zander *zander_t;


struct zander_battery {
    /** big endian; this number * 0.686 = voltage [mV] */
    uint16_t voltage;
} __attribute__((packed));

struct zander_serial {
    char serial[3];
    unsigned char foo[2];

    /** '1' or '0';  serial invalid if != '1' */
    char seal;

    /** second char: '1'=GP940, otherwise GP941 */
    char version[5];

    /** how much memory? '1'=1MB; '0'=0.5MB */
    char ram_type;
} __attribute__((packed));

struct zander_personal_data {
    char name[40];
    char aircraft_model[40];
    char competition_class[40];
    char registration[10];
    char competition_sign[10];
} __attribute__((packed));

struct zander_date {
    unsigned char day, month, year;
} __attribute__((packed));

struct zander_time {
    unsigned char hour, minute, second;
} __attribute__((packed));

struct zander_angle {
    unsigned char degrees;
    unsigned char minutes;
    unsigned char seconds;
    /** 0=North/East; 1=South/West */
    unsigned char sign;
} __attribute__((packed));

struct zander_task_wp {
    unsigned char num_wp;
    struct zander_date date;
    char name[12];
    struct zander_angle latitude, longitude;
} __attribute__((packed));

struct zander_write_task {
    struct zander_task_wp waypoints[20];
} __attribute__((packed));

struct zander_read_task {
    unsigned char foo;
    struct zander_date upload_date;
    struct zander_time upload_time;
    unsigned char foo2[18];
    struct zander_date for_date;
    struct zander_task_wp waypoints[20];
} __attribute__((packed));


#ifdef __cplusplus
extern "C" {
#endif


/* zander-open.c */

int zander_fdopen(int fd, zander_t *zander_r);

int zander_open(const char *device_path, zander_t *zander_r);

int zander_fileno(zander_t zander);

void zander_close(zander_t *zander_r);

/* zander-protocol.c */

int zander_read_serial(zander_t zander,
                       struct zander_serial *serial);
int
zander_read_personal_data(zander_t zander,
                          struct zander_personal_data *pd);

int
zander_read_9v_battery(zander_t zander, struct zander_battery *battery);

int
zander_read_li_battery(zander_t zander, struct zander_battery *battery);

int zander_write_memory(zander_t zander, unsigned address,
                        const void *data, size_t length);

/* zander-error.c */

const char *
zander_strerror(int error);

void
zander_perror(const char *msg, int error);

#ifdef __cplusplus
}
#endif

#endif
