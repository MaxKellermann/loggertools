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

#include "lxn-reader.h"

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

static union lxn_packet hack;

static size_t packet_lengths[0x100] = {
    [LXN_END] = sizeof(*hack.version),
    [LXN_VERSION] = sizeof(*hack.version),
    [LXN_START] = sizeof(*hack.start),
    [LXN_ORIGIN] = sizeof(*hack.origin),
    [LXN_SECURITY_OLD] = sizeof(*hack.security_old),
    [LXN_SERIAL] = sizeof(*hack.serial),
    [LXN_POSITION_OK] = sizeof(*hack.position),
    [LXN_POSITION_BAD] = sizeof(*hack.position),
    [LXN_SECURITY] = sizeof(*hack.security),
    [LXN_COMPETITION_CLASS] = sizeof(*hack.competition_class),
    [LXN_TASK] = sizeof(*hack.task),
    [LXN_EVENT] = sizeof(*hack.event),
    [LXN_DATE] = sizeof(*hack.date),
    [LXN_FLIGHT_INFO] = sizeof(*hack.flight_info),
    [LXN_K_EXT_CONFIG] = sizeof(*hack.ext_config),
    [LXN_B_EXT_CONFIG] = sizeof(*hack.ext_config),
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

static void handle_ext_config(struct extension_config *config,
                              const struct lxn_ext_config *packet,
                              unsigned column) {
    unsigned ext_dat, i, bit;

    /* count bits in extension mask */
    ext_dat = ntohs(packet->dat);
    for (bit = 0, config->num = 0; bit < 16; ++bit)
        if (ext_dat & (1 << bit))
            ++config->num;

    if (config->num == 0)
        return;

    /* write information about each extension */
    for (i = 0, bit = 0; bit < 16; ++bit) {
        if (ext_dat & (1 << bit)) {
            column += extension_defs[bit].width;
            config->widths[i] = extension_defs[bit].width;
            i++;
        }
    }
}

static size_t get_packet_length(struct lxn_reader *lxn) {
    size_t packet_length;

    packet_length = packet_lengths[*lxn->packet.cmd];
    if (packet_length > 0)
        return packet_length;

    switch (*lxn->packet.cmd) {
    case 0x00:
        ++packet_length;
        while (packet_length < lxn->input_length - lxn->input_consumed &&
               lxn->input[lxn->input_consumed + packet_length] == 0)
            ++packet_length;
        return packet_length;

    case LXN_B_EXT:
        return sizeof(*lxn->packet.b_ext) + lxn->b_ext.num * sizeof(lxn->packet.b_ext->data[0]);

    case LXN_K_EXT:
        return sizeof(*lxn->packet.k_ext) + lxn->k_ext.num * sizeof(lxn->packet.k_ext->data[0]);
    }

    if (*lxn->packet.cmd < 0x40) {
        packet_length = sizeof(*lxn->packet.string);
        if (lxn->input_length - lxn->input_consumed >= sizeof(*lxn->packet.string))
            packet_length += lxn->packet.string->length;
        return packet_length;
    }

    return 0;
}

static int set_error(struct lxn_reader *reader, const char *error) {
    reader->error = error;
    return -1;
}

int lxn_read(struct lxn_reader *lxn) {
    size_t packet_length;

    if (lxn->input == NULL || lxn->input_consumed >= lxn->input_length)
        return EINVAL;

    if (lxn->is_end)
        return set_error(lxn, "Read past LXN_END packet");

    lxn->packet.cmd = lxn->input + lxn->input_consumed;
    packet_length = get_packet_length(lxn);
    if (packet_length == 0)
        return set_error(lxn, "Unknown LXN packet");

    if (packet_length > lxn->input_length - lxn->input_consumed)
        return EAGAIN;

    lxn->packet_length = packet_length;
    lxn->input_consumed += packet_length;

    switch (*lxn->packet.cmd) {
    case LXN_END:
        lxn->is_end = 1;
        break;

    case LXN_K_EXT_CONFIG:
        handle_ext_config(&lxn->k_ext, lxn->packet.ext_config, 8);
        break;

    case LXN_B_EXT_CONFIG:
        handle_ext_config(&lxn->b_ext, lxn->packet.ext_config, 36);
        break;
    }

    return 0;
}
