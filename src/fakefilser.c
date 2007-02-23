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
#ifdef __GLIBC__
#include <getopt.h>
#endif

#include "version.h"
#include "filser.h"
#include "datadir.h"

struct config {
    int verbose;
    const char *datadir, *tty;
};

struct filser_wtf37 {
    char space[36];
    char null;
};

struct filser {
    int fd;
    struct datadir *datadir;
    struct filser_flight_info flight_info;
    struct filser_setup setup;
    unsigned start_address, end_address;
};


static void alarm_handler(int dummy) {
    (void)dummy;
}

static void usage(void) {
    puts("usage: fakefilser [options]\n\n"
         "valid options:\n"
         " -h             help (this text)\n"
#ifdef __GLIBC__
         " --version\n"
#endif
         " -V             show fakefilser version\n"
#ifdef __GLIBC__
         " --verbose\n"
#endif
         " -v             be more verbose\n"
#ifdef __GLIBC__
         " --quiet\n"
#endif
         " -q             be quiet\n"
#ifdef __GLIBC__
         " --data DIR\n"
#endif
         " -d DIR         specify the data directory\n"
#ifdef __GLIBC__
         " --tty DEVICE\n"
#endif
         " -t DEVICE      open this tty device (default /dev/ttyS0)\n"
#ifdef __GLIBC__
         " --virtual\n"
#endif
         " -u             create a virtual terminal\n"
         "\n"
         );
}

static void arg_error(const char *msg) __attribute__ ((noreturn));
static void arg_error(const char *msg) {
    fprintf(stderr, "fakefilser: %s\n", msg);
    fprintf(stderr, "Try 'fakefilser --help' for more information.\n");
    _exit(1);
}

/** read configuration options from the command line */
static void parse_cmdline(struct config *config,
                          int argc, char **argv) {
    int ret;
#ifdef __GLIBC__
    static const struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {"version", 0, 0, 'V'},
        {"verbose", 0, 0, 'v'},
        {"quiet", 1, 0, 'q'},
        {"data", 1, 0, 'd'},
        {"tty", 1, 0, 't'},
        {"virtual", 0, 0, 'u'},
        {0,0,0,0}
    };
#endif

    memset(config, 0, sizeof(*config));
    config->tty = "/dev/ttyS0";

    while (1) {
#ifdef __GLIBC__
        int option_index = 0;

        ret = getopt_long(argc, argv, "hVvqd:t:u",
                          long_options, &option_index);
#else
        ret = getopt(argc, argv, "hVvqd:t:u");
#endif
        if (ret == -1)
            break;

        switch (ret) {
        case 'h':
            usage();
            exit(0);

        case 'V':
            printf("fakefilser v" VERSION
                   ", http://max.kellermann.name/projects/loggertools/\n");
            exit(0);

        case 'v':
            ++config->verbose;
            break;

        case 'q':
            config->verbose = 0;
            break;

        case 'd':
            config->datadir = optarg;
            break;

        case 't':
            config->tty = optarg;
            break;

        case 'u':
            config->tty = NULL;
            break;

        default:
            exit(1);
        }
    }

    /* check non-option arguments */

    if (optind < argc) {
        fprintf(stderr, "fakefilser: unrecognized argument: %s\n",
                argv[optind]);
        fprintf(stderr, "Try 'fakefilser -h' for more information\n");
        exit(1);
    }

    if (config->datadir == NULL) {
        fprintf(stderr, "fakefilser: data directory not specified\n");
        fprintf(stderr, "Try 'fakefilser -h' for more information\n");
        exit(1);
    }
}

static void default_filser(struct filser *filser) {
    /*unsigned i;*/

    memset(filser, 0, sizeof(*filser));
    strcpy(filser->flight_info.pilot, "Mustermann");
    strcpy(filser->flight_info.model, "G103");
    strcpy(filser->flight_info.registration, "D-3322");
    strcpy(filser->flight_info.number, "2H");

    filser->setup.sampling_rate_normal = 5;
    filser->setup.sampling_rate_near_tp = 1;
    filser->setup.sampling_rate_button = 1;
    filser->setup.sampling_rate_k = 20;
    filser->setup.button_fixes = 15;
    filser->setup.tp_radius = htons(1000);
}

static void write_crc(int fd, const void *buffer, size_t length) {
    int ret;

    ret = filser_write_crc(fd, buffer, length);
    if (ret < 0) {
        fprintf(stderr, "write failed: %s\n", strerror(errno));
        _exit(1);
    }

    if (ret == 0) {
        fprintf(stderr, "short write\n");
        _exit(1);
    }
}

static ssize_t read_full_crc(int fd, void *buffer, size_t len) {
    int ret;

    ret = filser_read_crc(fd, buffer, len, 40);
    if (ret == -2) {
        fprintf(stderr, "CRC error\n");
        _exit(1);
    }

    if (ret <= 0)
        return -1;

    return (ssize_t)len;
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
        fflush(stdout);
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

static void download_to_file(struct filser *filser,
                             const char *filename,
                             size_t length) {
    void *buffer;
    int ret;

    buffer = malloc(length);
    if (buffer == NULL) {
        fprintf(stderr, "out of memory\n");
        abort();
    }

    ret = filser_read_crc(filser->fd, buffer, length, 10);
    if (ret <= 0) {
        fprintf(stderr, "error during filser_read_crc\n");
        exit(2);
    }

    ret = datadir_write(filser->datadir, filename,
                        buffer, length);
    if (ret < 0) {
        fprintf(stderr, "failed to write %s: %s\n",
                filename, strerror(errno));
        exit(2);
    }

    if (ret == 0) {
        fprintf(stderr, "short write to %s\n",
                filename);
        exit(2);
    }

    free(buffer);
}

static void upload_from_file(struct filser *filser,
                             const char *filename,
                             size_t length) {
    void *buffer;

    buffer = datadir_read(filser->datadir, filename, length);
    if (buffer == NULL) {
        fprintf(stderr, "failed to read %s\n", filename);
        exit(2);
    }

    write_crc(filser->fd, buffer, length);

    free(buffer);
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
    static const unsigned char buffer[0x148] = "\x0d\x0aVersion COLIBRI   V3.01\x0d\x0aSN13123,HW2.0\x0d\x0aID:313-[313]\x0d\x0aChecksum:64\x0d\x0aAPT:APTempty\x0d\x0a 10.6.3\x0d\x0aKey uploaded by\x0d\x0aMihelin Peter\x0d\x0aLX Navigation\x0d\x0a";

    write(filser->fd, buffer, sizeof(buffer));
}

static void handle_get_flight_info(struct filser *filser) {
    write_crc(filser->fd, (const unsigned char*)&filser->flight_info, sizeof(filser->flight_info));
}

static void handle_get_mem_section(struct filser *filser) {
    struct filser_packet_mem_section packet;

    memset(&packet, 0, sizeof(packet));
    packet.section_lengths[0] = htons(0x4000);

    write_crc(filser->fd, &packet, sizeof(packet));
}

static void handle_def_mem(struct filser *filser) {
    struct filser_packet_def_mem packet;

    read_full_crc(filser->fd, &packet, sizeof(packet));

    filser->start_address
        = packet.start_address[2]
        + (packet.start_address[1] << 8)
        + (packet.start_address[0] << 16);
    filser->end_address
        = packet.end_address[2]
        + (packet.end_address[1] << 8)
        + (packet.end_address[0] << 16);

    printf("def_mem: address = %02x %02x %02x %02x %02x %02x\n",
           packet.start_address[0],
           packet.start_address[1],
           packet.start_address[2],
           packet.end_address[0],
           packet.end_address[1],
           packet.end_address[2]);

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
    upload_from_file(filser, "tp_tsk", 0x5528);
}

static void handle_write_tp_tsk(struct filser *filser) {
    download_to_file(filser, "tp_tsk", 0x5528);

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
    upload_from_file(filser, "contest_class",
                     sizeof(struct filser_contest_class));
}

static void handle_get_extra_data(struct filser *filser) {
    upload_from_file(filser, "extra_data", 0x100);
}

static void handle_write_contest_class(struct filser *filser) {
    download_to_file(filser, "contest_class",
                     sizeof(struct filser_contest_class));

    send_ack(filser);
}

static void handle_30(struct filser *filser) {
    char foo[0x8000];
    memset(&foo, 0, sizeof(foo));
    write_crc(filser->fd, foo, sizeof(foo));
}

static void handle_apt_download(struct filser *filser,
                                unsigned idx) {
    char foo[16000];

    (void)idx;

    write_crc(filser->fd, foo, 8164);
}

static void handle_apt_upload(struct filser *filser,
                              unsigned idx) {
    char filename[] = "apt_X";

    filename[4] = '0' + idx;
    download_to_file(filser, filename, 0x8000);

    send_ack(filser);
}

static void handle_apt_state(struct filser *filser) {
    download_to_file(filser, "apt_state", 0xa07);

    send_ack(filser);
}

static int open_virtual(const char *symlink_path) {
    int fd, ret;

    fd = open("/dev/ptmx", O_RDWR|O_NOCTTY);
    if (fd < 0) {
        fprintf(stderr, "failed to open /dev/ptmx: %s\n",
                strerror(errno));
        _exit(1);
    }

    ret = unlockpt(fd);
    if (ret < 0) {
        fprintf(stderr, "failed to unlockpt(): %s\n",
                strerror(errno));
        _exit(1);
    }

    printf("Slave terminal is %s\n", ptsname(fd));

    unlink(symlink_path);
    ret = symlink(ptsname(fd), symlink_path);
    if (ret < 0) {
        fprintf(stderr, "symlink() failed: %s\n",
                strerror(errno));
        _exit(1);
    }

    printf("Symlinked to %s\n", symlink_path);

    return fd;
}

static int open_tty(struct config *config) {
    int fd;

    if (config->tty == NULL)
        fd = open_virtual("/tmp/fakefilser");
    else
        fd = filser_open(config->tty);

    if (fd < 0) {
        fprintf(stderr, "failed to open tty\n");
        exit(2);
    }

    return fd;
}

int main(int argc, char **argv) {
    struct config config;
    struct filser filser;
    int was_70 = 0;

    parse_cmdline(&config, argc, argv);

    signal(SIGALRM, alarm_handler);

    default_filser(&filser);

    filser.fd = open_tty(&config);

    filser.datadir = datadir_open(config.datadir);
    if (filser.datadir == NULL) {
        fprintf(stderr, "failed to open datadir %s: %s\n",
                argv[1], strerror(errno));
        exit(2);
    }

    while (1) {
        unsigned char cmd;
        int ret;

        ret = filser_read(filser.fd, &cmd, sizeof(cmd), 5);
        if (ret < 0) {
            if (errno == EIO) {
                close(filser.fd);
                filser.fd = open_tty(&config);
                continue;
            }

            fprintf(stderr, "read failed: %s\n", strerror(errno));
            _exit(1);
        }

        if (ret == 0)
            continue;

        switch (cmd) {
        case FILSER_SYN: /* SYN (expects ACK) */
            printf("received SYN\n");
            /* send ACK */
            handle_syn(&filser);
            break;

        case FILSER_PREFIX:
            ret = filser_read(filser.fd, &cmd, sizeof(cmd), 5);
            if (ret <= 0) {
                fprintf(stderr, "read failed: %s\n", strerror(errno));
                _exit(1);
            }

            printf("cmd=0x%02x\n", cmd);
            fflush(stdout);

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

            case 0x30: /* read LO4 logger */
            case 0x31:
            case 0x32:
                handle_30(&filser);
                break;

            case 0x4c:
                /* clear LO4 logger? */
                send_ack(&filser);
                break;

            case FILSER_APT_1:
            case FILSER_APT_2:
            case FILSER_APT_3:
            case FILSER_APT_4:
                if (was_70)
                    handle_apt_upload(&filser,
                                      cmd - FILSER_APT_1);
                else
                    handle_apt_download(&filser,
                                        cmd - FILSER_APT_1);
                break;

            case FILSER_APT_STATE:
                handle_apt_state(&filser);
                break;

            case 0x70:
                /* darauf folgt 0x61 mit daten */
                was_70 = 1;
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
