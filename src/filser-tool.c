/*
 * loggertools
 * Copyright (C) 2004-2006 Max Kellermann (max@duempel.org)
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

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <arpa/inet.h>
#ifdef __GLIBC__
#include <getopt.h>
#endif

#include "filser.h"
#include "datadir.h"
#include "filser-to-igc.h"

#define VERSION "0.0.1"

struct config {
    int verbose;
    const char *tty;
};

static void usage(void) {
    printf("usage: filsertool [OPTIONS] COMMAND [ARGUMENTS]\n"
           "valid options:\n"
#ifdef __GLIBC__
           " --tty DEVICE\n"
#endif
           " -t DEVICE      open this tty device (default /dev/ttyS0)\n"
           "valid commands:\n"
           "  list\n"
           "        print a list of flights\n"
           "  read_tp_tsk <out_filename.da4>\n"
           "        read the TP and TSK database from the device\n"
           "  write_tp_tsk <in_filename.da4>\n"
           "        write the TP and TSK database to the device\n"
           "  write_apt <data_dir>\n"
           "        write the APT database to the device\n"
           "  mem_section <start_adddress> <end_address>\n"
           "        print memory section info\n"
           "  raw_mem <start_adddress> <end_address>\n"
           "        download raw memory\n"
           "  lxn2igc FILENAME.LXN\n"
           "        convert a .lxn file to .igc\n"
           );
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
        {"tty", 1, 0, 't'},
        {0,0,0,0}
    };
#endif

    memset(config, 0, sizeof(*config));
    config->tty = "/dev/ttyS0";

    while (1) {
#ifdef __GLIBC__
        int option_index = 0;

        ret = getopt_long(argc, argv, "hVvqt:",
                          long_options, &option_index);
#else
        ret = getopt(argc, argv, "hVvqt:");
#endif
        if (ret == -1)
            break;

        switch (ret) {
        case 'h':
            usage();
            exit(0);

        case 'V':
            printf("filsertool v" VERSION
                   ", http://max.kellermann.name/projects/loggertools/\n");
            exit(0);

        case 'v':
            ++config->verbose;
            break;

        case 'q':
            config->verbose = 0;
            break;

        case 't':
            config->tty = optarg;
            break;

        default:
            exit(1);
        }
    }
}

static void arg_error(const char *msg) __attribute__ ((noreturn));
static void arg_error(const char *msg) {
    fprintf(stderr, "filsertool: %s\n", msg);
    fprintf(stderr, "Try 'filsertool --help' for more information.\n");
    _exit(1);
}

static void alarm_handler(int dummy) {
    (void)dummy;
}

static void syn_ack_wait(int fd) {
    int ret;
    unsigned tries = 10;

    do {
        alarm(10);
        ret = filser_syn_ack(fd);
        alarm(0);
        if (ret < 0) {
            fprintf(stderr, "failed to connect: %s\n",
                    strerror(errno));
            _exit(1);
        }

        if (ret == 0) {
            fprintf(stderr, "no filser found, trying again\n");
            sleep(1);
        }
    } while (ret == 0 && tries-- > 0);

    if (ret == 0) {
        fprintf(stderr, "no filser\n");
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
            return (int)nbytes;

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

static ssize_t read_timeout_crc(int fd, unsigned char *buffer, size_t len) {
    ssize_t nbytes;
    unsigned char crc;

    nbytes = read_timeout(fd, buffer, len);
    if (nbytes < 0)
        return nbytes;

    crc = filser_calc_crc(buffer, nbytes - 1);
    if (crc != buffer[nbytes - 1]) {
        fprintf(stderr, "CRC error\n");
        _exit(1);
    }

    return nbytes;
}

static int communicate(int fd, unsigned char cmd,
                       unsigned char *buffer, size_t buffer_len) {
    int ret;

    syn_ack_wait(fd);

    tcflush(fd, TCIOFLUSH);

    ret = filser_write_cmd(fd, cmd);
    if (ret <= 0)
        return -1;

    ret = read_full_crc(fd, buffer, buffer_len);
    if (ret < 0)
        return -1;

    return 1;
}

static int filser_send(int fd, unsigned char cmd,
                       const void *buffer, size_t buffer_len,
                       int timeout) {
    ssize_t nbytes;
    unsigned char response;
    int ret;

    syn_ack_wait(fd);

    tcflush(fd, TCIOFLUSH);

    ret = filser_write_packet(fd, cmd,
                              buffer, buffer_len);
    if (ret <= 0)
        return -1;

    nbytes = filser_read(fd, &response, sizeof(response), timeout);
    if (nbytes < 0)
        return -1;

    if ((size_t)nbytes < sizeof(response)) {
        fprintf(stderr, "no response\n");
        _exit(1);
    }

    if (response != FILSER_ACK) {
        fprintf(stderr, "no ACK\n");
        _exit(1);
    }

    return 1;
}

static int check_mem_settings(int fd) {
    int ret;
    unsigned char buffer[6];

    ret = communicate(fd, FILSER_CHECK_MEM_SETTINGS,
                      buffer, sizeof(buffer));
    if (ret < 0) {
        fprintf(stderr, "failed to communicate: %s\n",
                strerror(errno));
        _exit(1);
    }

    if (ret == 0) {
        fprintf(stderr, "no valid response\n");
        _exit(1);
    }

    return 0;
}

static int raw(struct config *config, int argc, char **argv) {
    int fd;
    unsigned char buffer[0x4000];
    ssize_t nbytes1, nbytes2;

    (void)argc;
    (void)argv;

    fd = filser_open(config->tty);
    if (fd < 0) {
        fprintf(stderr, "filser_open failed: %s\n",
                strerror(errno));
        _exit(1);
    }

    check_mem_settings(fd);

    nbytes1 = read_timeout(0, buffer, sizeof(buffer));
    if (nbytes1 < 0) {
        fprintf(stderr, "failed to read from stdin: %s\n",
                strerror(errno));
        _exit(1);
    }

    nbytes2 = write(fd, buffer, (size_t)nbytes1);
    if (nbytes2 < 0) {
        fprintf(stderr, "failed to write to '%s': %s\n",
                config->tty, strerror(errno));
        _exit(1);
    }

    nbytes1 = read_timeout(fd, buffer, sizeof(buffer));
    if (nbytes1 < 0) {
        fprintf(stderr, "failed to read from '%s': %s\n",
                config->tty, strerror(errno));
        _exit(1);
    }

    write(1, buffer, (size_t)nbytes1);

    return 0;
}

static int open_flight_list(int fd) {
    int ret;

    syn_ack_wait(fd);

    ret = filser_send_command(fd, FILSER_READ_FLIGHT_LIST);
    if (ret <= 0)
        return -1;

    return 0;
}

static int next_flight(int fd, struct filser_flight_index *flight) {
    int ret;

    ret = read_full_crc(fd, (unsigned char*)flight, sizeof(*flight));
    if (ret < 0)
        return -1;

    if (flight->valid != 1)
        return 0;

    return 1;
}

static unsigned filser_get_start_address(const struct filser_flight_index *flight) {
    return flight->start_address3 << 24
        | flight->start_address2 << 16
        | flight->start_address1 << 8
        | flight->start_address0;
}

static unsigned filser_get_end_address(const struct filser_flight_index *flight) {
    return flight->end_address3 << 24
        | flight->end_address2 << 16
        | flight->end_address1 << 8
        | flight->end_address0;
}

static int flight_list(struct config *config, int argc, char **argv) {
    int fd, ret;
    struct filser_flight_index flight;

    (void)argc;
    (void)argv;

    fd = filser_open(config->tty);
    if (fd < 0) {
        fprintf(stderr, "filser_open failed: %s\n",
                strerror(errno));
        _exit(1);
    }

    check_mem_settings(fd);

    ret = open_flight_list(fd);
    if (ret < 0) {
        fprintf(stderr, "io error: %s\n", strerror(errno));
        _exit(1);
    }

    for (;;) {
        ret = next_flight(fd, &flight);
        if (ret < 0) {
            fprintf(stderr, "io error: %s\n", strerror(errno));
            _exit(1);
        }

        if (ret == 0)
            break;

        printf("%s\t%s-%s\t%s\t0x%lx-0x%lx\n",
               flight.date, flight.start_time,
               flight.stop_time, flight.pilot,
               (unsigned long)filser_get_start_address(&flight),
               (unsigned long)filser_get_end_address(&flight));
    }

    return 0;
}

static int get_basic_data(struct config *config, int argc, char **argv) {
    int fd;
    unsigned char buffer[0x200];
    ssize_t nbytes;
    unsigned z;

    (void)argc;
    (void)argv;

    fd = filser_open(config->tty);
    if (fd < 0) {
        fprintf(stderr, "filser_open failed: %s\n",
                strerror(errno));
        _exit(1);
    }

    check_mem_settings(fd);

    filser_send_command(fd, FILSER_READ_BASIC_DATA);

    nbytes = read_timeout(fd, buffer, sizeof(buffer));
    if (nbytes < 0) {
        fprintf(stderr, "failed to read from '%s': %s\n",
                config->tty, strerror(errno));
        _exit(1);
    }

    for (z = 0; z < (unsigned)nbytes; z++)
        putchar(buffer[z]);

    return 0;
}

static int get_flight_info(struct config *config, int argc, char **argv) {
    int fd;
    unsigned char buffer[0x200];
    ssize_t nbytes;

    (void)argc;
    (void)argv;

    fd = filser_open(config->tty);
    if (fd < 0) {
        fprintf(stderr, "filser_open failed: %s\n",
                strerror(errno));
        _exit(1);
    }

    check_mem_settings(fd);

    filser_send_command(fd, FILSER_READ_FLIGHT_INFO);

    nbytes = read_timeout_crc(fd, buffer, sizeof(buffer));
    if (nbytes < 0) {
        fprintf(stderr, "failed to read from '%s': %s\n",
                config->tty, strerror(errno));
        _exit(1);
    }

    printf("Pilot: %s\n", buffer + 0x03);
    printf("Model: %s\n", buffer + 0x16);
    printf("ID: %s\n", buffer + 0x22);
    printf("ID2: %s\n", buffer + 0x2a);

    return 0;
}

static int seek_mem_x(int fd, unsigned start_address, unsigned end_address) {
    struct filser_packet_def_mem packet;
    ssize_t nbytes;
    int ret;
    unsigned char response;

    /* ignore highest byte here, the same as in kflog */

    /* start address */
    packet.start_address[0] = start_address & 0xff;
    packet.start_address[1] = (start_address >> 8) & 0xff;
    packet.start_address[2] = (start_address >> 16) & 0xff;

    /* end address */
    packet.end_address[0] = end_address & 0xff;
    packet.end_address[1] = (end_address >> 8) & 0xff;
    packet.end_address[2] = (end_address >> 16) & 0xff;

    tcflush(fd, TCIOFLUSH);

    ret = filser_send_command(fd, FILSER_DEF_MEM);
    if (ret <= 0)
        return -1;

    ret = filser_write_crc(fd, &packet, sizeof(packet));
    if (ret <= 0)
        return -1;

    nbytes = read_full(fd, &response, sizeof(response));
    if (nbytes <= 0)
        return -1;

    if (response != FILSER_ACK) {
        fprintf(stderr, "no ack in seek_mem\n");
        _exit(1);
    }

    return 0;
}

static int seek_mem(int fd, const struct filser_flight_index *flight) {
    unsigned char buffer[7];
    int ret;
    ssize_t nbytes;
    unsigned char response;

    /* ignore highest byte here, the same as in kflog */

    /* start address */
    buffer[0] = flight->start_address0;
    buffer[1] = flight->start_address1;
    buffer[2] = flight->start_address2;

    /* end address */
    buffer[3] = flight->end_address0;
    buffer[4] = flight->end_address1;
    buffer[5] = flight->end_address2;

    buffer[6] = filser_calc_crc((unsigned char*)buffer, 6);

    ret = filser_send_command(fd, FILSER_DEF_MEM);
    if (ret <= 0)
        return -1;

    nbytes = write(fd, buffer, sizeof(buffer));
    if (nbytes <= 0)
        return -1;

    nbytes = read_full(fd, &response, sizeof(response));
    if (nbytes <= 0)
        return -1;

    if (response != FILSER_ACK) {
        fprintf(stderr, "no ack in seek_mem\n");
        _exit(1);
    }

    return 0;
}

static int get_mem_section(int fd, size_t section_lengths[0x10],
                           size_t *overall_lengthp) {
    struct filser_packet_mem_section packet;
    int ret;
    ssize_t nbytes;
    unsigned z;

    ret = filser_send_command(fd, FILSER_GET_MEM_SECTION);
    if (ret <= 0)
        return -1;

    nbytes = read_full_crc(fd, &packet, sizeof(packet));
    if (nbytes <= 0)
        return -1;

    *overall_lengthp = 0;

    for (z = 0; z < 0x10; z++) {
        section_lengths[z] = ntohs(packet.section_lengths[z]);
        (*overall_lengthp) += section_lengths[z];
    }

    return 0;
}

static int download_section(int fd, unsigned section,
                            unsigned char *buffer, size_t length) {
    int ret;
    ssize_t nbytes;

    ret = filser_send_command(fd, FILSER_READ_LOGGER_DATA + section);
    if (ret <= 0)
        return -1;

    nbytes = read_full_crc(fd, buffer, length);
    if (nbytes < 0)
        return -1;

    return 0;
}

static int cmd_mem_section(struct config *config, int argc, char **argv) {
    int fd, ret;
    unsigned start_address, end_address;
    size_t section_lengths[0x10], overall_length;
    unsigned z;

    if (argc - optind != 2)
        arg_error("wrong number of arguments after 'mem_section'");

    start_address = (unsigned)strtoul(argv[optind++], NULL, 0);
    end_address = (unsigned)strtoul(argv[optind++], NULL, 0);

    fd = filser_open(config->tty);
    if (fd < 0) {
        fprintf(stderr, "filser_open failed: %s\n",
                strerror(errno));
        _exit(1);
    }

    check_mem_settings(fd);

    syn_ack_wait(fd);

    seek_mem_x(fd, start_address, end_address);

    printf("seeking = 0x%lx\n", (unsigned long)(end_address - start_address));

    ret = get_mem_section(fd, section_lengths, &overall_length);
    if (ret < 0) {
        fprintf(stderr, "io error: %s\n", strerror(errno));
        _exit(1);
    }

    for (z = 0; z < 0x10 && section_lengths[z] > 0; z++) {
        printf("section %u = 0x%lx\n", z, (unsigned long)section_lengths[z]);
    }

    printf("overall = 0x%lx\n", (unsigned long)overall_length);

    return 0;
}

static int cmd_raw_mem(struct config *config, int argc, char **argv) {
    int fd, ret;
    unsigned start_address, end_address;
    size_t section_lengths[0x10], overall_length;
    unsigned z;

    if (argc - optind != 2)
        arg_error("wrong number of arguments after 'raw_mem'");

    start_address = (unsigned)strtoul(argv[optind++], NULL, 0);
    end_address = (unsigned)strtoul(argv[optind++], NULL, 0);

    fd = filser_open(config->tty);
    if (fd < 0) {
        fprintf(stderr, "filser_open failed: %s\n",
                strerror(errno));
        _exit(1);
    }

    check_mem_settings(fd);

    syn_ack_wait(fd);

    seek_mem_x(fd, start_address, end_address);

    ret = get_mem_section(fd, section_lengths, &overall_length);
    if (ret < 0) {
        fprintf(stderr, "io error: %s\n", strerror(errno));
        _exit(1);
    }

    for (z = 0; z < 0x10 && section_lengths[z] > 0; z++) {
        void *p;

        p = malloc(section_lengths[z]);
        if (p == NULL) {
            fprintf(stderr, "out of memory\n");
            _exit(1);
        }

        ret = download_section(fd, z, p, section_lengths[z]);
        if (ret < 0) {
            fprintf(stderr, "io error: %s\n", strerror(errno));
            _exit(1);
        }

        write(1, p, section_lengths[z]);

        free(p);
    }

    return 0;
}

static int download_flight(struct config *config, int argc, char **argv) {
    int fd, ret, fd2;
    struct filser_flight_index buffer, flight, flight2;
    size_t section_lengths[0x10], overall_length;
    unsigned char *data, *p;
    unsigned z;

    (void)argc;
    (void)argv;

    fd = filser_open(config->tty);
    if (fd < 0) {
        fprintf(stderr, "filser_open failed: %s\n",
                strerror(errno));
        _exit(1);
    }

    check_mem_settings(fd);

    syn_ack_wait(fd);

    ret = open_flight_list(fd);
    if (ret < 0) {
        fprintf(stderr, "io error: %s\n", strerror(errno));
        _exit(1);
    }

    for (;;) {
        ret = next_flight(fd, &buffer);
        if (ret < 0) {
            fprintf(stderr, "io error: %s\n", strerror(errno));
            _exit(1);
        }

        if (ret == 0)
            break;

        flight = flight2;
        flight2 = buffer;

        printf("%s\t%s-%s\t%s\n",
               buffer.date, buffer.start_time,
               buffer.stop_time, buffer.pilot);
    }

    printf("seek_mem\n");
    ret = seek_mem(fd, &flight);
    if (ret < 0) {
        fprintf(stderr, "io error: %s\n", strerror(errno));
        _exit(1);
    }

    printf("get_mem_section\n");
    ret = get_mem_section(fd, section_lengths, &overall_length);
    if (ret < 0) {
        fprintf(stderr, "io error: %s\n", strerror(errno));
        _exit(1);
    }

    data = malloc(overall_length);
    if (data == NULL) {
        fprintf(stderr, "out of memory\n");
        _exit(1);
    }

    fd2 = open("/tmp/foo", O_WRONLY|O_CREAT|O_TRUNC, 0666);
    printf("fd2=%d\n", fd2);

    for (z = 0, p = data; z < 0x10; z++) {
        if (section_lengths[z] == 0)
            break;

        printf("downloading section %u, %lu bytes\n",
               z, (unsigned long)section_lengths[z]);

        ret = download_section(fd, z,
                               p, section_lengths[z]);
        if (ret < 0) {
            fprintf(stderr, "io error: %s\n", strerror(errno));
            _exit(1);
        }

        write(fd2, p, section_lengths[z]);

        p += section_lengths[z];
    }

    close(fd2);

    return 0;
}

static int cmd_read_tp_tsk(struct config *config, int argc, char **argv) {
    const char *filename;
    int fd, ret;
    struct filser_tp_tsk tp_tsk;

    if (argc - optind < 1)
        arg_error("No file name specified");
    if (argc - optind > 1)
        arg_error("Too many arguments");

    filename = argv[optind];

    fd = filser_open(config->tty);
    if (fd < 0) {
        fprintf(stderr, "filser_open failed: %s\n",
                strerror(errno));
        _exit(1);
    }

    ret = communicate(fd, FILSER_READ_TP_TSK,
                      (unsigned char*)&tp_tsk, sizeof(tp_tsk));
    if (ret < 0) {
        fprintf(stderr, "io error: %s\n", strerror(errno));
        _exit(1);
    }

    close(fd);

    if (strcmp(filename, "-") == 0) {
        fd = 1;
    } else {
        fd = open(filename, O_WRONLY | O_CREAT, 0666);
        if (fd < 0) {
            fprintf(stderr, "failed to create %s: %s",
                    filename, strerror(errno));
            _exit(1);
        }
    }

    write(fd, &tp_tsk, sizeof(tp_tsk));

    if (fd != 1)
        close(fd);

    return 0;
}

static int cmd_write_tp_tsk(struct config *config, int argc, char **argv) {
    const char *filename;
    int fd, ret;
    ssize_t nbytes;
    struct filser_tp_tsk tp_tsk;

    if (argc - optind < 1)
        arg_error("No file name specified");
    if (argc - optind > 1)
        arg_error("Too many arguments");

    filename = argv[optind];

    if (strcmp(filename, "-") == 0) {
        fd = 0;
    } else {
        fd = open(filename, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "failed to open %s: %s",
                    filename, strerror(errno));
            _exit(1);
        }
    }

    nbytes = read(fd, &tp_tsk, sizeof(tp_tsk));
    if (nbytes < 0) {
        fprintf(stderr, "read() failed: %s\n", strerror(errno));
        _exit(1);
    }

    if ((size_t)nbytes < sizeof(tp_tsk)) {
        fprintf(stderr, "short read()\n");
        _exit(1);
    }

    if (fd != 0)
        close(fd);

    fd = filser_open(config->tty);
    if (fd < 0) {
        fprintf(stderr, "filser_open failed: %s\n",
                strerror(errno));
        _exit(1);
    }

    ret = filser_send(fd, FILSER_WRITE_TP_TSK,
                      (const unsigned char*)&tp_tsk, sizeof(tp_tsk),
                      15);
    if (ret <= 0) {
        fprintf(stderr, "io error: %s\n", strerror(errno));
        return 1;
    }

    close(fd);

    return 0;
}

static int cmd_write_apt(struct config *config, int argc, char **argv) {
    const char *data_path;
    int fd, ret;
    struct datadir *dir;
    char *data;
    unsigned i;
    char filename[] = "apt_X";

    if (argc - optind < 1)
        arg_error("No data directory specified");
    if (argc - optind > 1)
        arg_error("Too many arguments");

    fd = filser_open(config->tty);
    if (fd < 0) {
        fprintf(stderr, "filser_open failed: %s\n",
                strerror(errno));
        exit(2);
    }

    data_path = argv[optind];
    dir = datadir_open(data_path);
    if (dir == NULL) {
        fprintf(stderr, "failed to datadir %s", data_path);
        exit(2);
    }

    syn_ack_wait(fd);

    ret = filser_write_cmd(fd, 0x70);

    for (i = 0; i < 4; i++) {
        printf("sending APT block %u\n", i + 1);

        filename[4] = '0' + i;
        data = datadir_read(dir, filename, 0x8000);
        ret = filser_send(fd, FILSER_APT_1 + i,
                          data, 0x8000, 10);
        if (ret <= 0) {
            fprintf(stderr, "io error: %s\n", strerror(errno));
            return 1;
        }

        free(data);
    }

    printf("sending APT state\n");

    data = datadir_read(dir, "apt_state", 0xa07);
    ret = filser_send(fd, FILSER_APT_STATE,
                      data, 0xa07, 10);
    if (ret <= 0) {
        fprintf(stderr, "io error: %s\n", strerror(errno));
        return 1;
    }

    free(data);

    datadir_close(dir);
    close(fd);

    return 0;
}

static int cmd_lxn2igc(struct config *config, int argc, char **argv) {
    int fd, ret, is_eof = 0, done = 0;
    unsigned char buffer[4096];
    size_t start = 0, end = 0, consumed;
    ssize_t nbytes;
    struct filser_to_igc *fti;

    (void)config;

    if (argc - optind < 1)
        arg_error("No .lxn file specified");
    if (argc - optind > 1)
        arg_error("Too many arguments");

    fd = open(argv[optind], O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "failed to open %s: %s\n",
                argv[optind], strerror(errno));
        return 2;
    }

    ret = filser_to_igc_open(stdout, &fti);
    if (ret != 0) {
        fprintf(stderr, "filser_to_igc_open() failed\n");
        return 2;
    }

    do {
        if (start > 0) {
            if (end > start)
                memmove(buffer, buffer + start, end - start);
            end -= start;
            start = 0;
        }

        if (end < sizeof(buffer)) {
            nbytes = read(fd, buffer + end, sizeof(buffer) - end);
            if (nbytes < 0) {
                fprintf(stderr, "failed to read from %s: %s\n",
                        argv[optind], strerror(errno));
                return 2;
            } else if (nbytes == 0)
                is_eof = 1;
            else
                end += (size_t)nbytes;
        }

        if (end > start) {
            ret = filser_to_igc_process(fti, buffer + start, end - start, &consumed);
            if (ret == 0) {
                done = 1;
            } else if (ret != EAGAIN) {
                fprintf(stderr, "filser_to_igc_process() failed: %d\n", ret);
                return 2;
            }

            assert(consumed > 0);
            assert(start + consumed <= end);

            start += consumed;
        }
    } while (!is_eof && !done);

    if (end > start) {
        fprintf(stderr, "unexpected eof\n");
        return 2;
    }

    ret = filser_to_igc_close(&fti);
    if (ret != 0) {
        fprintf(stderr, "filser_to_igc_close() failed\n");
        return 2;
    }

    close(fd);

    return 0;
}

int main(int argc, char **argv) {
    struct config config;
    const char *cmd;

    signal(SIGALRM, alarm_handler);

    parse_cmdline(&config, argc, argv);

    if (optind >= argc)
        arg_error("no command specified");

    cmd = argv[optind++];

    if (strcmp(cmd, "raw") == 0) {
        return raw(&config, argc, argv);
    } else if (strcmp(cmd, "list") == 0) {
        return flight_list(&config, argc, argv);
    } else if (strcmp(cmd, "basic") == 0) {
        return get_basic_data(&config, argc, argv);
    } else if (strcmp(cmd, "flight") == 0) {
        return get_flight_info(&config, argc, argv);
    } else if (strcmp(cmd, "mem_section") == 0) {
        return cmd_mem_section(&config, argc, argv);
    } else if (strcmp(cmd, "raw_mem") == 0) {
        return cmd_raw_mem(&config, argc, argv);
    } else if (strcmp(cmd, "download") == 0) {
        return download_flight(&config, argc, argv);
    } else if (strcmp(cmd, "read_tp_tsk") == 0) {
        return cmd_read_tp_tsk(&config, argc, argv);
    } else if (strcmp(cmd, "write_tp_tsk") == 0) {
        return cmd_write_tp_tsk(&config, argc, argv);
    } else if (strcmp(cmd, "write_apt") == 0) {
        return cmd_write_apt(&config, argc, argv);
    } else if (strcmp(cmd, "lxn2igc") == 0) {
        return cmd_lxn2igc(&config, argc, argv);
    } else {
        arg_error("unknown command");
    }
}
