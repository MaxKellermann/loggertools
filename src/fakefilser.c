/*
 * loggertools
 * Copyright (C) 2004-2005 Max Kellermann <max@duempel.org>
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

#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <sys/select.h>

#include "filser.h"

struct filser_wtf37 {
    char space[36];
    char null;
};

struct filser {
    int fd;
    struct filser_flight_info flight_info;
    struct filser_contest_class contest_class;
    struct filser_setup setup;
    struct filser_tp_tsk tp_tsk;
    unsigned start_address, end_address;
};


static void alarm_handler(int dummy) {
    (void)dummy;
}

static void usage(void) {
    printf("usage: fakefilser\n");
}

static void arg_error(const char *msg) __attribute__ ((noreturn));
static void arg_error(const char *msg) {
    fprintf(stderr, "fakefilser: %s\n", msg);
    fprintf(stderr, "Try 'fakefilser --help' for more information.\n");
    _exit(1);
}

static void default_filser(struct filser *filser) {
    /*unsigned i;*/

    memset(filser, 0, sizeof(*filser));
    strcpy(filser->flight_info.pilot, "Mustermann");
    strcpy(filser->flight_info.model, "G103");
    strcpy(filser->flight_info.registration, "D-3322");
    strcpy(filser->flight_info.number, "2H");

    strcpy(filser->contest_class.contest_class, "FOOBAR");

    filser->setup.sampling_rate_normal = 5;
    filser->setup.sampling_rate_near_tp = 1;
    filser->setup.sampling_rate_button = 1;
    filser->setup.sampling_rate_k = 20;
    filser->setup.button_fixes = 15;
    filser->setup.tp_radius = htons(1000);

    /*
    for (i = 0; i < 100; i++)
        memset(filser->tp_tsk.wtf37[i].space,
               ' ',
               sizeof(filser->tp_tsk.wtf37[i].space));
    */
}

static void write_crc(int fd, const unsigned char *buffer, size_t length) {
    ssize_t nbytes;
    unsigned char crc;
    size_t pos, sublen;

    for (pos = 0; pos < length; pos += sublen) {
        if (pos > 0) {
            struct timeval tv;

            /* we have to throttle here ... */
            tv.tv_sec = 0;
            tv.tv_usec = 50000;
            select(0, NULL, NULL, NULL, &tv);
        }

        sublen = length - pos;
        if (sublen > 1024)
            sublen = 1024;

        nbytes = write(fd, buffer, sublen);
        if (nbytes < 0) {
            fprintf(stderr, "write failed: %s\n", strerror(errno));
            _exit(1);
        }

        if ((size_t)nbytes != sublen) {
            fprintf(stderr, "short write\n");
            _exit(1);
        }
    }

    crc = filser_calc_crc(buffer, length);

    nbytes = write(fd, &crc, sizeof(crc));
    if (nbytes < 0) {
        fprintf(stderr, "write failed: %s\n", strerror(errno));
        _exit(1);
    }

    if ((size_t)nbytes != sizeof(crc)) {
        fprintf(stderr, "short write\n");
        _exit(1);
    }
}

static ssize_t read_full(int fd, unsigned char *buffer, size_t len) {
    ssize_t nbytes;
    size_t pos = 0;
    time_t timeout = time(NULL) + 40;

    alarm(40);

    for (;;) {
        nbytes = read(fd, buffer + pos, len - pos);
        alarm(0);
        if (nbytes < 0)
            return nbytes;

        pos += (size_t)nbytes;
        if (pos >= len)
            return (ssize_t)pos;

        if (time(NULL) > timeout) {
            alarm(0);
            errno = EINTR;
            return -1;
        }
    }
}

static void dump_line(size_t offset,
                      const unsigned char *line,
                      size_t length) {
    size_t i;

    assert(length > 0);
    assert(length <= 0x10);

    if (length == 0)
        return;

    printf("%04x ", (unsigned)offset);

    for (i = 0; i < 0x10; i++) {
        if (i % 8 == 0)
            printf(" ");

        if (i >= length)
            printf("   ");
        else
            printf(" %02x", line[i]);
    }

    printf(" | ");
    for (i = 0; i < length; i++)
        putchar(line[i] >= 0x20 && line[i] < 0x80 ? line[i] : '.');

    printf("\n");
}

static void dump_buffer(const void *q,
                        size_t length) {
    const unsigned char *p = q;
    size_t offset = 0;

    while (length >= 0x10) {
        dump_line(offset, p, 0x10);

        p += 0x10;
        length -= 0x10;
        offset += 0x10;
    }

    if (length > 0)
        dump_line(offset, p, length);
}

static void dump_buffer_crc(const void *q,
                            size_t length) {
    dump_buffer(q, length);
    printf("crc = %02x\n",
           filser_calc_crc((const unsigned char*)q, length));
}

static ssize_t read_full_crc(int fd, void *buffer, size_t len) {
    ssize_t nbytes, nbytes2;
    unsigned char crc1, crc2;

    nbytes = read_full(fd, (unsigned char*)buffer, len);
    if (nbytes < 0)
        return nbytes;
    if ((size_t)nbytes != len) {
        fprintf(stderr, "short read %lu, wanted %lu\n",
                (unsigned long)nbytes, (unsigned long)len);
        _exit(1);
    }

    nbytes2 = read_full(fd, &crc1, sizeof(crc1));
    if (nbytes2 < 0)
        return nbytes2;

    crc2 = filser_calc_crc((unsigned char*)buffer, len);
    if (crc2 != crc1) {
        dump_buffer_crc(buffer, len);
        fprintf(stderr, "CRC error, %02x vs %02x\n", crc1, crc2);
        _exit(1);
    }

    return nbytes;
}

static void dump_timeout(struct filser *filser) {
    unsigned char data;
    ssize_t nbytes;
    time_t timeout = time(NULL) + 2;
    unsigned offset = 0, i;
    unsigned char row[0x10];

    while (1) {
        alarm(1);
        nbytes = read(filser->fd, &data, sizeof(data));
        alarm(0);
        if (nbytes < 0)
            break;
        if ((size_t)nbytes >= sizeof(data)) {
            if (offset % 16 == 0)
                printf("%04x ", offset);
            else if (offset % 8 == 0)
                printf(" ");

            printf(" %02x", data);
            row[offset%0x10] = data;
            ++offset;

            if (offset % 16 == 0) {
                printf(" | ");
                for (i = 0; i < 0x10; i++)
                    putchar(row[i] >= 0x20 && row[i] < 0x80 ? row[i] : '.');
                printf("\n");
            }

            timeout = time(NULL) + 2;
        } else if (time(NULL) >= timeout)
            break;
    }

    if (offset % 16 > 0) {
        printf(" | ");
        for (i = 0; i < offset % 16; i++)
            putchar(row[i] >= 0x20 && row[i] < 0x80 ? row[i] : '.');
        printf("\n");
    }

    printf("\n");
}

static void send_ack(struct filser *filser) {
    static const unsigned char ack = 0x06;
    ssize_t nbytes;

    nbytes = write(filser->fd, &ack, sizeof(ack));
    if (nbytes < 0) {
        fprintf(stderr, "write() failed: %s\n", strerror(errno));
        _exit(1);
    }

    if ((size_t)nbytes < sizeof(ack)) {
        fprintf(stderr, "short write()\n");
        _exit(1);
    }
}

static void handle_syn(struct filser *filser) {
    send_ack(filser);
}

static void handle_check_mem_settings(struct filser *filser) {
    static const unsigned char buffer[6] = {
        0x00, 0x00, 0x06, 0x80, 0x00, 0x0f,
    };

    write_crc(filser->fd, buffer, sizeof(buffer));
}

static void handle_open_flight_list(struct filser *filser) {
    struct filser_flight_index flight;

    memset(&flight, 0, sizeof(flight));

    flight.valid = 1;
    flight.start_address2 = 0x06;
    flight.end_address1 = 0x0f;
    flight.end_address0 = 0xd1;
    flight.end_address2 = 0x06;
    strcpy(flight.date, "16.07.05");
    strcpy(flight.start_time, "20:14:06");
    strcpy(flight.stop_time, "21:07:47");
    strcpy(flight.pilot, "Max Kellermann");
    flight.logger_id = htons(15149);
    flight.flight_no = 1;
    dump_buffer_crc(&flight, sizeof(flight));
    write_crc(filser->fd, (const unsigned char*)&flight, sizeof(flight));

    flight.start_address0 = 0x01;
    flight.start_address1 = 0x23;
    flight.start_address2 = 0x45;
    flight.start_address3 = 0x67;
    flight.end_address0 = 0x01;
    flight.end_address1 = 0x23;
    flight.end_address2 = 0x45;
    flight.end_address3 = 0x80;
    strcpy(flight.date, "23.08.05");
    strcpy(flight.start_time, "08:55:11");
    strcpy(flight.stop_time, "16:22:33");
    strcpy(flight.pilot, "Max Kellermann");
    flight.flight_no++;
    write_crc(filser->fd, (const unsigned char*)&flight, sizeof(flight));

    strcpy(flight.date, "24.08.05");
    strcpy(flight.start_time, "09:55:11");
    strcpy(flight.stop_time, "15:22:33");
    strcpy(flight.pilot, "Max Kellermann");
    dump_buffer_crc(&flight, sizeof(flight));
    write_crc(filser->fd, (const unsigned char*)&flight, sizeof(flight));

    memset(&flight, 0, sizeof(flight));
    write_crc(filser->fd, (const unsigned char*)&flight, sizeof(flight));
}

static void handle_get_basic_data(struct filser *filser) {
    unsigned char buffer[0x140];

    memset(&buffer, 0, sizeof(buffer));
    write(filser->fd, buffer, sizeof(buffer));
}

static void handle_get_flight_info(struct filser *filser) {
    write_crc(filser->fd, (const unsigned char*)&filser->flight_info, sizeof(filser->flight_info));
}

static void handle_get_mem_section(struct filser *filser) {
    unsigned char mem_section[0x20];

    memset(mem_section, 0, sizeof(mem_section));
    mem_section[0] = 0x07;
    mem_section[1] = 0x40;
    mem_section[2] = 0x09;
    mem_section[3] = 0xef;

    write_crc(filser->fd, mem_section, sizeof(mem_section));
}

static void handle_def_mem(struct filser *filser) {
    unsigned char address[6];

    read_full_crc(filser->fd, address, sizeof(address));

    filser->start_address = address[2] + (address[1] << 8) + (address[0] << 16);
    filser->end_address = address[5] + (address[4] << 8) + (address[3] << 16);

    printf("def_mem: address = %02x %02x %02x %02x %02x %02x\n",
           address[0], address[1], address[2],
           address[3], address[4], address[5]);

    printf("def_mem: address = %06x %06x\n",
           filser->start_address, filser->end_address);

    send_ack(filser);
}

static void handle_get_logger_data(struct filser *filser, unsigned block) {
    unsigned size;
    unsigned char *foo;

    switch (block) {
    case 0:
        size = 0x740;
        break;
    case 1:
        size = 0x9ef;
        break;
    default:
        size = 0;
    }

    printf("reading block %u size %u\n", block, size);

    foo = malloc(size);
    if (foo == NULL) {
        fprintf(stderr, "malloc(%u) failed\n", size);
        _exit(1);
    }

    memset(foo, 0, sizeof(foo));

    write_crc(filser->fd, foo, size);
}

static void handle_write_flight_info(struct filser *filser) {
    struct filser_flight_info flight_info;
    ssize_t nbytes;

    nbytes = read_full_crc(filser->fd, &flight_info,
                           sizeof(flight_info));
    if (nbytes < 0) {
        fprintf(stderr, "read failed: %s\n", strerror(errno));
        _exit(1);
    }
    if ((size_t)nbytes < sizeof(flight_info)) {
        fprintf(stderr, "short read\n");
        _exit(1);
    }

    printf("FI: Pilot = %s\n", flight_info.pilot);
    printf("FI: Model = %s\n", flight_info.model);
    printf("FI: Registration = %s\n", flight_info.registration);

    filser->flight_info = flight_info;

    send_ack(filser);
}

static void handle_read_tp_tsk(struct filser *filser) {
    write_crc(filser->fd, (const unsigned char*)&filser->tp_tsk,
              sizeof(filser->tp_tsk));
}

static void handle_write_tp_tsk(struct filser *filser) {
    struct filser_tp_tsk tp_tsk;
    ssize_t nbytes;

    nbytes = read_full_crc(filser->fd, &tp_tsk, sizeof(tp_tsk));
    if (nbytes < 0) {
        fprintf(stderr, "read failed: %s\n", strerror(errno));
        _exit(1);
    }
    if ((size_t)nbytes < sizeof(tp_tsk)) {
        fprintf(stderr, "short read\n");
        _exit(1);
    }

    filser->tp_tsk = tp_tsk;

    send_ack(filser);
}

static void handle_read_setup(struct filser *filser) {
    write_crc(filser->fd, (const unsigned char*)&filser->setup,
              sizeof(filser->setup));
}

static void handle_write_setup(struct filser *filser) {
    struct filser_setup setup;
    ssize_t nbytes;

    nbytes = read_full_crc(filser->fd, &setup, sizeof(setup));
    if (nbytes < 0) {
        fprintf(stderr, "read failed: %s\n", strerror(errno));
        _exit(1);
    }
    if ((size_t)nbytes < sizeof(setup)) {
        fprintf(stderr, "short read\n");
        _exit(1);
    }

    filser->setup = setup;

    send_ack(filser);
}

static void handle_read_contest_class(struct filser *filser) {
    write_crc(filser->fd, (const unsigned char*)&filser->contest_class,
              sizeof(filser->contest_class));
    dump_buffer_crc(&filser->contest_class, sizeof(filser->contest_class));
}

static void handle_get_extra_data(struct filser *filser) {
    static const unsigned char extra_data[0x100];

    write_crc(filser->fd, extra_data, sizeof(extra_data));
}

static void handle_write_contest_class(struct filser *filser) {
    struct filser_contest_class contest_class;
    ssize_t nbytes;

    nbytes = read_full_crc(filser->fd, &contest_class, sizeof(contest_class));
    if (nbytes < 0) {
        fprintf(stderr, "read failed: %s\n", strerror(errno));
        _exit(1);
    }
    if ((size_t)nbytes < sizeof(contest_class)) {
        fprintf(stderr, "short read\n");
        _exit(1);
    }

    filser->contest_class = contest_class;

    send_ack(filser);
}

int main(int argc, char **argv) {
    struct filser filser;
    int ret;

    (void)argv;

    signal(SIGALRM, alarm_handler);

    if (argc > 1) {
        if (strcmp(argv[0], "--help") == 0) {
            usage();
            _exit(0);
        } else {
            arg_error("Too many arguments");
        }
    }

    default_filser(&filser);

    filser.fd = open("/dev/ptmx", O_RDWR|O_NOCTTY);
    if (filser.fd < 0) {
        fprintf(stderr, "failed to open /dev/ptmx: %s\n", strerror(errno));
        _exit(1);
    }

    ret = unlockpt(filser.fd);
    if (ret < 0) {
        fprintf(stderr, "failed to unlockpt(): %s\n", strerror(errno));
        _exit(1);
    }

    printf("Slave terminal is %s\n", ptsname(filser.fd));

    unlink("/tmp/fakefilser");
    ret = symlink(ptsname(filser.fd), "/tmp/fakefilser");
    if (ret < 0) {
        fprintf(stderr, "symlink() failed: %s\n", strerror(errno));
        _exit(1);
    }

    printf("Symlinked to /tmp/fakefilser\n");

    while (1) {
        unsigned char cmd;
        ssize_t nbytes;

        nbytes = read(filser.fd, &cmd, sizeof(cmd));
        if (nbytes < 0) {
            fprintf(stderr, "read failed: %s\n", strerror(errno));
            _exit(1);
        }

        if ((size_t)nbytes < sizeof(cmd))
            continue;

        switch (cmd) {
        case FILSER_SYN: /* SYN (expects ACK) */
            printf("received SYN\n");
            /* send ACK */
            handle_syn(&filser);
            break;

        case FILSER_PREFIX:
            nbytes = read(filser.fd, &cmd, sizeof(cmd));
            if (nbytes < 0) {
                fprintf(stderr, "read failed: %s\n", strerror(errno));
                _exit(1);
            }
            if ((size_t)nbytes < sizeof(cmd)) {
                fprintf(stderr, "short read\n");
                _exit(1);
            }

            if (cmd >= FILSER_READ_LOGGER_DATA) {
                printf("received get_logger_data\n");
                handle_get_logger_data(&filser, cmd - FILSER_READ_LOGGER_DATA);
                break;
            }

            switch (cmd) {
            case FILSER_READ_TP_TSK:
                printf("received read_tp_tsk\n");
                handle_read_tp_tsk(&filser);
                break;

            case FILSER_WRITE_TP_TSK:
                printf("received write_tp_tsk\n");
                handle_write_tp_tsk(&filser);
                break;

            case FILSER_READ_FLIGHT_LIST:
                printf("received open_flight_list\n");
                handle_open_flight_list(&filser);
                break;

            case FILSER_READ_BASIC_DATA:
                printf("received get_basic_data\n");
                handle_get_basic_data(&filser);
                break;

            case FILSER_READ_SETUP:
                printf("received read_setup\n");
                handle_read_setup(&filser);
                break;

            case FILSER_WRITE_SETUP:
                printf("received write_setup\n");
                handle_write_setup(&filser);
                break;

            case FILSER_READ_FLIGHT_INFO:
                printf("received get_flight_info\n");
                handle_get_flight_info(&filser);
                break;

            case FILSER_WRITE_FLIGHT_INFO:
                printf("received write_flight_info\n");
                handle_write_flight_info(&filser);
                break;

            case FILSER_READ_EXTRA_DATA:
                printf("received get_extra_data\n");
                handle_get_extra_data(&filser);
                break;

            case FILSER_GET_MEM_SECTION:
                printf("received get_mem_section\n");
                handle_get_mem_section(&filser);
                break;

            case FILSER_DEF_MEM:
                printf("received def_mem\n");
                handle_def_mem(&filser);
                break;

            case FILSER_READ_CONTEST_CLASS:
                printf("received read_contest_class\n");
                handle_read_contest_class(&filser);
                break;

            case FILSER_WRITE_CONTEST_CLASS:
                printf("received write_contest_class\n");
                handle_write_contest_class(&filser);
                break;

            case FILSER_CHECK_MEM_SETTINGS:
                printf("received check_mem_settings\n");
                handle_check_mem_settings(&filser);
                break;

            default:
                fprintf(stderr, "unknown command 0x02 0x%02x received, dumping input\n", cmd);
                dump_timeout(&filser);
            }
            break;

        default:
            fprintf(stderr, "unknown command 0x%02x received\n", cmd);
            _exit(1);
        }
    }

    return 0;
}
