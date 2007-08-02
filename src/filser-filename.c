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

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "filser.h"

static const char c36[] = "0123456789abcdefghijklmnopqrstuvwxyz";

void filser_flight_filename(char *filename,
                            const struct filser_flight_index *flight) {
    char year;
    unsigned month, day, logger_id;

    assert(flight != NULL);

    year = flight->date[7];
    if (year < '0' || year > '9')
        year = '0';

    month = (flight->date[3] - '0') * 10 + flight->date[4] - '0';
    if (month >= 36)
        month = 0;

    day = (flight->date[0] - '0') * 10 + flight->date[1] - '0';
    if (day >= 36)
        day = 0;

    logger_id = ntohs(flight->logger_id);
    if (logger_id >= 36 * 36 * 36 * 36)
        logger_id = 36 * 36 * 36 - 1;

    filename[0] = year;
    filename[1] = c36[month];
    filename[2] = c36[day];
    filename[3] = 'l'; /* 'l' for "LX Nagivation" */
    filename[4] = c36[logger_id / 36 / 36];
    filename[5] = c36[(logger_id / 36) % 36];
    filename[6] = c36[logger_id % 36];
    filename[7] = c36[flight->flight_no % 36];
}
