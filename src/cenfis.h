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

typedef struct cenfis *cenfis_t;

#define CENFIS_DIALOG_PROGRAM 'P'
#define CENFIS_DIALOG_DATA 'D'

cenfis_status_t
cenfis_open(const char *device,
            cenfis_t *cenfisp);

void
cenfis_close(cenfis_t cenfis);

void
cenfis_dump(cenfis_t cenfis, int fd);

cenfis_status_t
cenfis_select(cenfis_t cenfis,
              struct timeval *timeout);

cenfis_status_t
cenfis_confirm(cenfis_t cenfis);

cenfis_status_t
cenfis_dialog_respond(cenfis_t cenfis, char ch);

cenfis_status_t
cenfis_write_data(cenfis_t cenfis,
                  const void *buf, size_t count);

#endif
