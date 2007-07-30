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

#include "flarm-internal.h"

#include <assert.h>
#include <unistd.h>
#include <errno.h>

flarm_result_t
flarm_send_ping(flarm_t flarm)
{
    return flarm_send_frame(flarm, FLARM_MESSAGE_PING, NULL, 0);
}

flarm_result_t
flarm_send_set_baud_rate(flarm_t flarm, int speed)
{
    uint8_t payload = (uint8_t)speed;

    return flarm_send_frame(flarm, FLARM_MESSAGE_SETBAUDRATE,
                            &payload, sizeof(payload));
}

flarm_result_t
flarm_send_exit(flarm_t flarm)
{
    return flarm_send_frame(flarm, FLARM_MESSAGE_EXIT, NULL, 0);
}

flarm_result_t
flarm_send_select_record(flarm_t flarm, unsigned record_no)
{
    uint8_t payload = (uint8_t)record_no;

    return flarm_send_frame(flarm, FLARM_MESSAGE_SELECTRECORD,
                            &payload, sizeof(payload));
}

flarm_result_t
flarm_send_get_record_info(flarm_t flarm)
{
    return flarm_send_frame(flarm, FLARM_MESSAGE_GETRECORDINFO, NULL, 0);
}

flarm_result_t
flarm_send_get_igc_data(flarm_t flarm)
{
    return flarm_send_frame(flarm, FLARM_MESSAGE_GETIGCDATA, NULL, 0);
}
