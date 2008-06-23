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

#include "zander-internal.h"

#include <assert.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

int zander_read_serial(zander_t zander,
                       struct zander_serial *serial) {
    static const unsigned char cmd = ZANDER_CMD_READ_SERIAL;
    int ret;

    ret = zander_write(zander, &cmd, sizeof(cmd));
    if (ret != 0)
        return ret;

    return zander_read(zander, serial, sizeof(*serial));
}

int
zander_write_personal_data(zander_t zander,
                           const struct zander_personal_data *pd)
{
    static const unsigned char cmd = ZANDER_CMD_WRITE_PERSONAL_DATA;
    int ret;

    ret = zander_write(zander, &cmd, sizeof(cmd));
    if (ret != 0)
        return ret;

    return zander_write(zander, pd, sizeof(*pd));
}

int
zander_read_personal_data(zander_t zander,
                          struct zander_personal_data *pd)
{
    static const unsigned char cmd = ZANDER_CMD_READ_PERSONAL_DATA;
    int ret;

    ret = zander_write(zander, &cmd, sizeof(cmd));
    if (ret != 0)
        return ret;

    return zander_read(zander, pd, sizeof(*pd));
}

int
zander_read_9v_battery(zander_t zander, struct zander_battery *battery)
{
    static const unsigned char cmd = ZANDER_CMD_READ_9V_BATTERY;
    int ret;

    ret = zander_write(zander, &cmd, sizeof(cmd));
    if (ret != 0)
        return ret;

    return zander_read(zander, battery, sizeof(*battery));
}

int
zander_read_li_battery(zander_t zander, struct zander_battery *battery)
{
    static const unsigned char cmd = ZANDER_CMD_READ_LI_BATTERY;
    int ret;

    ret = zander_write(zander, &cmd, sizeof(cmd));
    if (ret != 0)
        return ret;

    return zander_read(zander, battery, sizeof(*battery));
}

int zander_write_memory(zander_t zander, unsigned address,
                        const void *data, size_t length) {
    struct zander_write_data header;
    int ret;

    assert(data != NULL);

    if (length == 0)
        return EINVAL;

    if (length > 1000 || address + length > 0x1000000)
        return EFBIG;

    memset(&header, 0, sizeof(header));
    header.address.address[0] = address >> 16;
    header.address.address[1] = address >> 8;
    header.address.address[2] = address;

    ret = zander_write(zander, &header, sizeof(header));
    if (ret != 0)
        return ret;

    ret = zander_write(zander, data, length);
    if (ret != 0)
        return ret;

    return 0;
}

int
zander_read_task(zander_t zander, struct zander_read_task *task)
{
    static const unsigned char cmd = ZANDER_CMD_READ_TASK;
    int ret;

    ret = zander_write(zander, &cmd, sizeof(cmd));
    if (ret != 0)
        return ret;

    return zander_read(zander, task, sizeof(*task));
}

int
zander_read_memory(zander_t zander, void *dest,
                   unsigned start, unsigned length)
{
    int ret;
    struct {
        unsigned char cmd;
        struct zander_read_data read_data;
    } request = {
        .cmd = ZANDER_CMD_READ_MEMORY,
    };

    zander_host_to_address(&request.read_data.start, start);
    zander_host_to_address(&request.read_data.length, length);

    ret = zander_write(zander, &request, sizeof(request));
    if (ret != 0)
        return ret;

    return zander_read(zander, dest, length);
}

int
zander_flight_list(zander_t zander,
                   struct zander_flight flights[ZANDER_MAX_FLIGHTS])
{
    static struct {
        unsigned char cmd;
        struct zander_read_data read_data;
    } request = {
        .cmd = ZANDER_CMD_READ_MEMORY,
        .read_data = {
            .start = {
                .address = { [1] = 0x30, },
            },
            .length = {
                .address = {
                    [1] = (sizeof(flights[0]) * ZANDER_MAX_FLIGHTS) >> 8,
                    [2] = (sizeof(flights[0]) * ZANDER_MAX_FLIGHTS) & 0xff,
                },
            },
        },
    };
    int ret;

    ret = zander_write(zander, &request, sizeof(request));
    if (ret != 0)
        return ret;

    return zander_read(zander, flights,
                       sizeof(flights[0]) * ZANDER_MAX_FLIGHTS);
}
