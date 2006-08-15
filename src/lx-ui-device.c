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

#include "lx-ui.h"

#include <unistd.h>
#include <time.h>
#include <errno.h>

void lxui_device_close(struct lxui *lxui) {
    if (lxui->fd >= 0)
        close(lxui->fd);
    lxui->status = LXUI_STATUS_INIT;
}

static void lxui_device_send_syn(struct lxui *lxui, time_t now) {
    filser_send_syn(lxui->fd);
    lxui->status = LXUI_STATUS_SYN;
    lxui->ack_timeout = now + 2;
}

static ssize_t read_timeout(int fd, unsigned char *buffer,
                            size_t len) {
    ssize_t nbytes;
    size_t pos = 0;
    time_t timeout = time(NULL) + 10, timeout2 = 0;

    for (;;) {
        alarm(1);
        nbytes = read(fd, buffer + pos, len - pos);
        alarm(0);
        if (nbytes < 0)
            return errno == EINTR
                ? (ssize_t)pos
                : nbytes;

        if (timeout2 == 0) {
            if (nbytes == 0)
                timeout2 = time(NULL) + 1;
        } else {
            if (nbytes > 0)
                timeout2 = 0;
        }

        pos += (size_t)nbytes;
        if (pos >= len)
            return (ssize_t)pos;

        if (timeout2 > 0 && time(NULL) >= timeout2)
            return pos;

        if (time(NULL) >= timeout) {
            errno = EINTR;
            return -1;
        }
    }
}

static void format_basic(char *p) {
    const char *src = p;

    while (*src > 0 && *src <= ' ')
        ++src;

    while (*src) {
        if (*src == '\r')
            ++src;
        else
            *p++ = *src++;
    }

    *p++ = 0;
}

int lxui_device_tick(struct lxui *lxui) {
    const time_t now = time(NULL);
    int ret;
    ssize_t nbytes;

    if (lxui->fd < 0) {
        if (lxui->fd < 0)
            return -1;
        lxui->status = LXUI_STATUS_INIT;
    }

    /*newtTextboxSetText(lxui->newt_basic, "foo\nbar");*/
    switch (lxui->status) {
    case LXUI_STATUS_INIT:
        newtLabelSetText(lxui->newt_status, "connecting...");
        lxui_device_send_syn(lxui, now);
        break;

    case LXUI_STATUS_SYN:
        ret = filser_recv_ack(lxui->fd);
        if (ret > 0) {
            if (lxui->connected) {
                lxui->status = LXUI_STATUS_IDLE;
                lxui->next_syn = now + 4;
                newtLabelSetText(lxui->newt_status, "connected");
            } else {
                filser_send_command(lxui->fd, FILSER_READ_BASIC_DATA);
                lxui->status = LXUI_STATUS_READ_BASIC;
                newtLabelSetText(lxui->newt_status, "reading info...");
            }
        } else if (ret == 0) {
            lxui_device_send_syn(lxui, now);
            if (now >= lxui->ack_timeout)
                newtLabelSetText(lxui->newt_status, "timeout");
        } else {
            lxui_device_send_syn(lxui, now);
            newtLabelSetText(lxui->newt_status, "error");
        }
        break;

    case LXUI_STATUS_IDLE:
        if (now > lxui->next_syn) {
            lxui_device_send_syn(lxui, now);
            newtLabelSetText(lxui->newt_status, "checking...");
        }
        break;

    case LXUI_STATUS_READ_BASIC:
        lxui->connected = 1;
        nbytes = read_timeout(lxui->fd, (unsigned char*)lxui->basic,
                              sizeof(lxui->basic) - 1);
        if (nbytes >= 0) {
            lxui->basic_length = (size_t)nbytes;
            lxui->basic[lxui->basic_length] = 0;
            format_basic(lxui->basic);
            newtTextboxSetText(lxui->newt_basic, lxui->basic);
        } else {
            lxui->basic_length = 0;
            newtLabelSetText(lxui->newt_status, "error");
        }
        lxui_device_send_syn(lxui, now);
        break;

    case LXUI_STATUS_READ_FLIGHT_INFO:
        lxui->flight_info_ok = 0;
        ret = filser_read_crc(lxui->fd, &lxui->flight_info,
                              sizeof(lxui->flight_info), 5);
        if (ret == -2) {
            //fprintf(stderr, "CRC error\n");
            lxui_device_send_syn(lxui, now);
        } else if (ret <= 0) {
            lxui_device_send_syn(lxui, now);
        } else {
            lxui->flight_info_ok = 1;
            lxui->status = LXUI_STATUS_IDLE;
        }
        break;

    case LXUI_STATUS_READ_FLIGHT_LIST:
        lxui->flight_info_ok = 0;
        ret = filser_read_crc(lxui->fd, &lxui->flight_index,
                              sizeof(lxui->flight_index), 5);
        if (ret == -2) {
            //fprintf(stderr, "CRC error\n");
            lxui_device_send_syn(lxui, now);
        } else if (ret <= 0) {
            lxui_device_send_syn(lxui, now);
        } else {
            if (lxui->flight_index.valid == 1)
                lxui->flight_index_ok = 1;
            else
                lxui->status = LXUI_STATUS_IDLE;
        }
        break;
    }

    return 0;
}

int lxui_device_wait(struct lxui *lxui) {
    newtComponent form;
    struct newtExitStruct es;
    int should_exit = 0;

    if (lxui->status == LXUI_STATUS_IDLE)
        return 1;

    lxui_device_tick(lxui);

    if (lxui->status == LXUI_STATUS_IDLE)
        return 1;

    newtCenteredWindow(35, 2, "Waiting");
    form = newtForm(NULL, NULL, 0);
    newtFormSetTimer(form, 100);
    newtFormAddComponents(form,
                          newtLabel(1, 0, "Waiting for device..."),
                          newtCompactButton(1, 1, "Cancel"),
                          NULL);

    do {
        newtFormRun(form, &es);

        switch (es.reason) {
        case NEWT_EXIT_HOTKEY:
        case NEWT_EXIT_COMPONENT:
            should_exit = 1;
            break;

        case NEWT_EXIT_TIMER:
            lxui_device_tick(lxui);
            break;

        default:
            break;
        }
    } while (!should_exit && lxui->status != LXUI_STATUS_IDLE);

    newtFormDestroy(form);
    newtPopWindow();
    return lxui->status == LXUI_STATUS_IDLE
        ? 1 : 0;
}
