/*
 * loggertools
 * Copyright (C) 2004-2005 Max Kellermann (max@duempel.org)
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

#ifndef __FILSER_H
#define __FILSER_H

struct filser_turn_point {
    char valid;
    char code[8];
    char zero1[1];
    char reserved1[3];
    char fortytwo;
    char reserved2[10];
    char zero2[3];
    char n_47_49[2];
    char zero3[2];
};

struct filser_task {
    char valid;
    unsigned char dummy0[31];
};

struct filser_tp_tsk {
    struct filser_turn_point tp[600];
    struct filser_task tasks[100];
};

struct filser_flight_info {
    unsigned char dummy0[3];
    char pilot[19];
    char model[12];
    /* offset 34 */
    char registration[8];
    /* offset 42 */
    char number[4];
    /* offset 46 */
    unsigned char dummy1[1];
    /* offset 47 */
    char observer[10];
    /* offset 57 */
    unsigned char dummy2[5];
    /* offset 62 */
    unsigned char dummy3[287];
} __attribute__((packed));

struct filser_contest_class {
    char contest_class[9];
} __attribute__((packed));

struct filser_setup {
    unsigned char sampling_rate_normal;
    unsigned char sampling_rate_near_tp;
    unsigned char sampling_rate_button;
    unsigned char sampling_rate_k;
    unsigned char button_fixes;
    unsigned char dummy1;
    unsigned char timezone;
    unsigned char extension_fxa;
    unsigned char extension_vxa;
    unsigned char extension_rpm;
    unsigned char extension_gsp;
    unsigned char extension_ias;
    unsigned char extension_tas;
    unsigned char extension_hdm;
    unsigned char extension_hdt;
    unsigned char extension_trm;
    unsigned char extension_trt;
    unsigned char extension_ten;
    unsigned char extension_wdi;
    unsigned char extension_wve;
    unsigned char extension_enl;
    unsigned char extension_var;
    unsigned char dummy2;
    unsigned char k_extension_fxa;
    unsigned char k_extension_vxa;
    unsigned char k_extension_rpm;
    unsigned char k_extension_gsp;
    unsigned char k_extension_ias;
    unsigned char k_extension_tas;
    unsigned char k_extension_hdm;
    unsigned char k_extension_hdt;
    unsigned char k_extension_trm;
    unsigned char k_extension_trt;
    unsigned char k_extension_ten;
    unsigned char k_extension_wdi;
    unsigned char k_extension_wve;
    unsigned char k_extension_enl;
    unsigned char k_extension_var;
    unsigned char dummy3;
    unsigned short tp_radius;
    unsigned char tp_zone;
    unsigned char nmea_output_gpgga;
    unsigned char nmea_output_gprmc;
    unsigned char nmea_output_gprmb;
    unsigned char nmea_output_gpgll;
    unsigned char nmea_output_gpr00;
    unsigned char nmea_output_gpwpl;
    unsigned char nmea_output_gplx1;
    unsigned char nmea_output_gpbwc;
} __attribute__((packed));

struct filser_flight_index {
    unsigned char valid;
    unsigned char start_address1;
    unsigned char start_address0;
    unsigned char start_address3;
    unsigned char start_address2;
    unsigned char end_address1;
    unsigned char end_address0;
    unsigned char end_address3;
    unsigned char end_address2;
    char date[9];
    char start_time[9];
    char stop_time[9];
    unsigned char dummy0[4];
    char pilot[52];
    unsigned short logger_id;
    unsigned char flight_no; /* ? */
} __attribute__((packed));

struct filser_packet_def_mem {
    unsigned char start_address[3];
    unsigned char end_address[3];
} __attribute__((packed));

struct filser_packet_mem_section {
    u_int16_t section_lengths[0x10];
} __attribute__((packed));

enum filser_command {
    FILSER_PREFIX = 0x02,
    FILSER_ACK = 0x06,
    FILSER_SYN = 0x16,
    FILSER_READ_TP_TSK = 0x52,
    FILSER_WRITE_TP_TSK = 0x57,
    FILSER_READ_BASIC_DATA = 0xc4,
    FILSER_READ_SETUP = 0xc5,
    FILSER_WRITE_SETUP = 0xc6,
    FILSER_READ_FLIGHT_INFO = 0xc9,
    FILSER_WRITE_FLIGHT_INFO = 0xca,
    FILSER_READ_EXTRA_DATA = 0xcb,
    FILSER_GET_MEM_SECTION = 0xcc,
    FILSER_READ_FLIGHT_LIST = 0xcd,
    FILSER_DEF_MEM = 0xce,
    FILSER_READ_CONTEST_CLASS = 0xcf,
    FILSER_WRITE_CONTEST_CLASS = 0xd0,
    FILSER_CHECK_MEM_SETTINGS = 0xd1,
    FILSER_READ_LOGGER_DATA = 0xe6
};


#ifdef __cplusplus
extern "C" {
#endif

/* filser-crc.c */

unsigned char filser_calc_crc(const void *p0, size_t len);

/* filser-open.c */

int filser_open(const char *device);

/* filser-io.c */

int filser_write_cmd(int fd, unsigned char cmd);

int filser_write_crc(int fd, const void *p, size_t length);

int filser_write_packet(int fd, unsigned char cmd,
                        const void *packet, size_t length);

int filser_read_crc(int fd, void *p, size_t length,
                    time_t timeout);


#ifdef __cplusplus
}
#endif


#endif
