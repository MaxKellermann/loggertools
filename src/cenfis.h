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

#ifndef __LOGGERTOOLS_CENFIS_H
#define __LOGGERTOOLS_CENFIS_H

typedef enum {
    CENFIS_STATUS_SHORT_WRITE = -4,
    CENFIS_STATUS_CONTEXT = -3,
    CENFIS_STATUS_INVALID = -2,
    CENFIS_STATUS_ERRNO = -1,
    CENFIS_STATUS_SUCCESS = 0,
    CENFIS_STATUS_IDLE,
    CENFIS_STATUS_DIALOG_CONFIRM,
    CENFIS_STATUS_DIALOG_SELECT,
    CENFIS_STATUS_WAIT_ACK,
    CENFIS_STATUS_DATA,
} cenfis_status_t;

#define cenfis_is_error(status) ((status) < 0)

struct cenfis;

#define CENFIS_DIALOG_PROGRAM 'P'
#define CENFIS_DIALOG_DATA 'D'

cenfis_status_t cenfis_open(const char *device,
                            struct cenfis **cenfisp);

void cenfis_close(struct cenfis *cenfis);

void cenfis_dump(struct cenfis *cenfis, int fd);

cenfis_status_t cenfis_select(struct cenfis *cenfis,
                              struct timeval *timeout);

cenfis_status_t cenfis_confirm(struct cenfis *cenfis);

cenfis_status_t cenfis_dialog_respond(struct cenfis *cenfis, char ch);

cenfis_status_t cenfis_write_data(struct cenfis *cenfis,
                                  const void *buf, size_t count);

#endif
