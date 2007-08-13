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

#ifndef __CENFIS_AIRSPACE_H
#define __CENFIS_AIRSPACE_H

#include <stdint.h>

struct cenfis_airspace_pointer {
    uint32_t offset;
    uint16_t total_size;
    uint16_t num_elements;
} __attribute__((packed));

struct cenfis_airspace_file_header {
    uint8_t reserved0[0x40];
    struct cenfis_airspace_pointer asp, config;
    uint8_t reserved1[0xf0];
    struct cenfis_airspace_pointer asp_index;
    uint8_t reserved2[0xb8];
} __attribute__((packed));

struct cenfis_airspace_header {
    uint16_t asp_rec_lengh;
    uint16_t ac_rel_ind;
    uint16_t s_rel_ind;
    uint16_t ap_rel_ind;
    uint16_t c_rel_ind;
    uint16_t an_rel_ind;
    uint16_t an2_rel_ind;
    uint16_t an3_rel_ind;
    uint16_t al_rel_ind;
    uint16_t ah_rel_ind;
    uint16_t l_rel_ind;
    uint16_t fis_rel_ind;
    uint16_t an4_rel_ind;
    uint16_t file_info_ind;
    uint16_t voice_ind;
} __attribute__((packed));

#endif
