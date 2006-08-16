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

#ifndef __LX_UI_H
#define __LX_UI_H

#include <newt.h>

#include "filser.h"

enum lxui_status {
    LXUI_STATUS_INIT = 0,
    LXUI_STATUS_SYN,
    LXUI_STATUS_IDLE,
    LXUI_STATUS_READ_BASIC,
    LXUI_STATUS_READ_FLIGHT_INFO,
    LXUI_STATUS_READ_FLIGHT_LIST,
    LXUI_STATUS_DEF_MEM,
    LXUI_STATUS_GET_MEM_SECTION,
    LXUI_STATUS_READ_LOGGER_DATA,
};

struct lxui {
    int fd;
    int connected;
    time_t next_syn, ack_timeout;
    enum lxui_status status;
    newtComponent newt_status, newt_basic;
    char basic[0x201];
    size_t basic_length;
    struct filser_flight_info flight_info;
    int flight_info_ok;
    struct filser_flight_index flight_index;
    int flight_index_ok;
    struct filser_packet_mem_section mem_section;
    int mem_section_ok;
    unsigned current_mem_section;
    unsigned char *logger_data;
    size_t logger_data_length;
    int logger_data_ok;
};

void lxui_device_close(struct lxui *lxui);

int lxui_device_tick(struct lxui *lxui);

int lxui_device_wait(struct lxui *lxui);

void lxui_edit_setup(struct lxui *lxui);

void lxui_edit_flight_info(struct lxui *lxui);

void lxui_download_igc_flights(struct lxui *lxui);

#endif
