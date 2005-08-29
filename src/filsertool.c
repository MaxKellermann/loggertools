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

#include "filser.h"

static void usage(void) {
    printf("usage: filsertool <command>\n"
           "valid commands:\n"
           "  list\n"
           "        print a list of flights\n"
           "  mem_section <start_adddress> <end_address>\n"
           "        print memory section info\n"
           "  raw_mem <start_adddress> <end_address>\n"
           "        download raw memory\n"
           );
}

static void arg_error(const char *msg) __attribute__ ((noreturn));
static void arg_error(const char *msg) {
    fprintf(stderr, "fakefilser: %s\n", msg);
    fprintf(stderr, "Try 'fakefilser --help' for more information.\n");
    _exit(1);
}

static void alarm_handler(int dummy) {
    (void)dummy;
}

static int syn_ack(int fd) {
    const char syn = FILSER_SYN, ack = FILSER_ACK;
    char buffer;
    ssize_t nbytes;

    tcflush(fd, TCIOFLUSH);

    nbytes = write(fd, &syn, sizeof(syn));
    if (nbytes <= 0)
        return (int)nbytes;

    nbytes = read(fd, &buffer, sizeof(buffer));
    if (nbytes <= 0)
        return (int)nbytes;

    return buffer == ack
        ? 1 : 0;
}

static void syn_ack_wait(int fd) {
    int ret;
    unsigned tries = 10;

    do {
        alarm(10);
        ret = syn_ack(fd);
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

static int connect(const char *device) {
    int fd, ret;
    struct termios attr;

    fd = open(device, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        fprintf(stderr, "failed to open '%s': %s\n",
                device, strerror(errno));
        _exit(1);
    }

    ret = tcgetattr(fd, &attr);
    if (ret < 0) {
        fprintf(stderr, "tcgetattr failed: %s\n",
                strerror(errno));
        _exit(1);
    }

    attr.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    attr.c_oflag &= ~OPOST;
    attr.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    attr.c_cflag &= ~(CSIZE | PARENB | CRTSCTS | IXON | IXOFF);
    attr.c_cflag |= (CS8 | CLOCAL);
    attr.c_cc[VMIN] = 0;
    attr.c_cc[VTIME] = 1;
    cfsetospeed(&attr, B19200);
    cfsetispeed(&attr, B19200);
    ret = tcsetattr(fd, TCSANOW, &attr);
    if (ret < 0) {
        fprintf(stderr, "tcsetattr failed: %s\n",
                strerror(errno));
        _exit(1);
    }

    return fd;
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
    ssize_t nbytes;
    unsigned char crc1, crc2;

    nbytes = read_full(fd, (unsigned char*)buffer, len);
    if (nbytes < 0)
        return nbytes;

    nbytes = read_full(fd, &crc1, sizeof(crc1));
    if (nbytes < 0)
        return nbytes;

    crc2 = filser_calc_crc((unsigned char*)buffer, len);
    if (crc2 != crc1) {
        fprintf(stderr, "CRC error\n");
        _exit(1);
    }

    return nbytes;
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
    const unsigned char prefix = FILSER_PREFIX;
    ssize_t nbytes;
    int ret;

    syn_ack_wait(fd);

    tcflush(fd, TCIOFLUSH);

    nbytes = write(fd, &prefix, sizeof(prefix));
    if (nbytes <= 0)
        return -1;

    nbytes = write(fd, &cmd, sizeof(cmd));
    if (nbytes <= 0)
        return -1;

    ret = read_full_crc(fd, buffer, buffer_len);
    if (ret < 0)
        return -1;

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

static int raw(int argpos, int argc, char **argv) {
    const char *device = "/dev/ttyS0";
    int fd;
    unsigned char buffer[0x4000];
    ssize_t nbytes1, nbytes2;

    (void)argpos;
    (void)argc;
    (void)argv;

    fd = connect(device);

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
                device, strerror(errno));
        _exit(1);
    }

    nbytes1 = read_timeout(fd, buffer, sizeof(buffer));
    if (nbytes1 < 0) {
        fprintf(stderr, "failed to read from '%s': %s\n",
                device, strerror(errno));
        _exit(1);
    }

    write(1, buffer, (size_t)nbytes1);

    return 0;
}

static int open_flight_list(int fd) {
    unsigned char cmd[] = { FILSER_PREFIX, FILSER_READ_FLIGHT_LIST };
    ssize_t nbytes;

    syn_ack_wait(fd);

    tcflush(fd, TCIOFLUSH);
    nbytes = write(fd, cmd, sizeof(cmd));
    if (nbytes < 0)
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

static int flight_list(int argpos, int argc, char **argv) {
    const char *device = "/dev/ttyS0";
    int fd, ret;
    struct filser_flight_index flight;

    (void)argpos;
    (void)argc;
    (void)argv;

    fd = connect(device);

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

static int get_basic_data(int argpos, int argc, char **argv) {
    const char *device = "/dev/ttyS0";
    unsigned char cmd[] = { FILSER_PREFIX, FILSER_READ_BASIC_DATA };
    int fd;
    unsigned char buffer[0x200];
    ssize_t nbytes;
    unsigned z;

    (void)argpos;
    (void)argc;
    (void)argv;

    fd = connect(device);

    check_mem_settings(fd);

    tcflush(fd, TCIOFLUSH);
    write(fd, cmd, sizeof(cmd));

    nbytes = read_timeout(fd, buffer, sizeof(buffer));
    if (nbytes < 0) {
        fprintf(stderr, "failed to read from '%s': %s\n",
                device, strerror(errno));
        _exit(1);
    }

    for (z = 0; z < (unsigned)nbytes; z++)
        putchar(buffer[z]);

    return 0;
}

static int get_flight_info(int argpos, int argc, char **argv) {
    const char *device = "/dev/ttyS0";
    unsigned char cmd[] = { FILSER_PREFIX, FILSER_READ_FLIGHT_INFO };
    int fd;
    unsigned char buffer[0x200];
    ssize_t nbytes;

    (void)argpos;
    (void)argc;
    (void)argv;

    fd = connect(device);

    check_mem_settings(fd);

    tcflush(fd, TCIOFLUSH);
    write(fd, cmd, sizeof(cmd));

    nbytes = read_timeout_crc(fd, buffer, sizeof(buffer));
    if (nbytes < 0) {
        fprintf(stderr, "failed to read from '%s': %s\n",
                device, strerror(errno));
        _exit(1);
    }

    printf("Pilot: %s\n", buffer + 0x03);
    printf("Model: %s\n", buffer + 0x16);
    printf("ID: %s\n", buffer + 0x22);
    printf("ID2: %s\n", buffer + 0x2a);

    return 0;
}

static int seek_mem_x(int fd, unsigned start_address, unsigned end_address) {
    unsigned char cmd[] = { FILSER_PREFIX, FILSER_DEF_MEM, };
    unsigned char buffer[7];
    ssize_t nbytes;
    unsigned char response;

    /* ignore highest byte here, the same as in kflog */

    /* start address */
    buffer[0] = start_address & 0xff;
    buffer[1] = (start_address >> 8) & 0xff;
    buffer[2] = (start_address >> 16) & 0xff;

    /* end address */
    buffer[3] = end_address & 0xff;
    buffer[4] = (end_address >> 8) & 0xff;
    buffer[5] = (end_address >> 16) & 0xff;

    buffer[6] = filser_calc_crc(buffer, 6);

    tcflush(fd, TCIOFLUSH);

    nbytes = write(fd, cmd, sizeof(cmd));
    if (nbytes <= 0)
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

static int seek_mem(int fd, struct filser_flight_index *flight) {
    unsigned char cmd[] = { FILSER_PREFIX, FILSER_DEF_MEM, };
    unsigned char buffer[7];
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

    tcflush(fd, TCIOFLUSH);

    nbytes = write(fd, cmd, sizeof(cmd));
    if (nbytes <= 0)
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
    unsigned char cmd[] = { FILSER_PREFIX, FILSER_GET_MEM_SECTION, };
    unsigned char mem_section[0x20];
    ssize_t nbytes;
    unsigned z;

    tcflush(fd, TCIOFLUSH);

    nbytes = write(fd, cmd, sizeof(cmd));
    if (nbytes <= 0)
        return -1;

    nbytes = read_full_crc(fd, mem_section, sizeof(mem_section));
    if (nbytes <= 0)
        return -1;

    *overall_lengthp = 0;

    for (z = 0; z < 0x10; z++) {
        section_lengths[z] = (mem_section[z * 2] << 8) +
            mem_section[z * 2 + 1];
        (*overall_lengthp) += section_lengths[z];
    }

    return 0;
}

static int download_section(int fd, unsigned section,
                            unsigned char *buffer, size_t length) {
    unsigned char cmd[] = { FILSER_PREFIX, FILSER_READ_LOGGER_DATA, };
    ssize_t nbytes;

    tcflush(fd, TCIOFLUSH);

    cmd[1] += section;
    nbytes = write(fd, cmd, sizeof(cmd));
    if (nbytes <= 0)
        return -1;

    nbytes = read_full_crc(fd, buffer, length);
    if (nbytes < 0)
        return -1;

    return 0;
}

static int cmd_mem_section(int argpos, int argc, char **argv) {
    const char *device = "/dev/ttyS0";
    int fd, ret;
    unsigned start_address, end_address;
    size_t section_lengths[0x10], overall_length;
    unsigned z;

    if (argc - argpos != 2)
        arg_error("wrong number of arguments after 'mem_section'");

    start_address = (unsigned)strtoul(argv[argpos++], NULL, 0);
    end_address = (unsigned)strtoul(argv[argpos++], NULL, 0);

    fd = connect(device);

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

static int cmd_raw_mem(int argpos, int argc, char **argv) {
    const char *device = "/dev/ttyS0";
    int fd, ret;
    unsigned start_address, end_address;
    size_t section_lengths[0x10], overall_length;
    unsigned z;

    if (argc - argpos != 2)
        arg_error("wrong number of arguments after 'raw_mem'");

    start_address = (unsigned)strtoul(argv[argpos++], NULL, 0);
    end_address = (unsigned)strtoul(argv[argpos++], NULL, 0);

    fd = connect(device);

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

static int download_flight(int argpos, int argc, char **argv) {
    const char *device = "/dev/ttyS0";
    int fd, ret, fd2;
    struct filser_flight_index buffer, flight, flight2;
    size_t section_lengths[0x10], overall_length;
    unsigned char *data, *p;
    unsigned z;

    (void)argpos;
    (void)argc;
    (void)argv;

    fd = connect(device);

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

static int cmd_read_tp_tsk(int argpos, int argc, char **argv) {
    const char *device = "/dev/ttyS0";
    int fd, ret;
    struct filser_tp_tsk tp_tsk;

    (void)argpos;
    (void)argc;
    (void)argv;

    fd = connect(device);

    syn_ack_wait(fd);

    ret = communicate(fd, FILSER_READ_TP_TSK,
                      (unsigned char*)&tp_tsk, sizeof(tp_tsk));
    if (ret < 0) {
        fprintf(stderr, "io error: %s\n", strerror(errno));
        _exit(1);
    }

    write(1, &tp_tsk, sizeof(tp_tsk));

    close(fd);

    return 0;
}

int main(int argc, char **argv) {
    signal(SIGALRM, alarm_handler);

    if (argc < 2)
        arg_error("no command specified");

    if (strcmp(argv[1], "raw") == 0) {
        return raw(2, argc, argv);
    } else if (strcmp(argv[1], "list") == 0) {
        return flight_list(2, argc, argv);
    } else if (strcmp(argv[1], "basic") == 0) {
        return get_basic_data(2, argc, argv);
    } else if (strcmp(argv[1], "flight") == 0) {
        return get_flight_info(2, argc, argv);
    } else if (strcmp(argv[1], "mem_section") == 0) {
        return cmd_mem_section(2, argc, argv);
    } else if (strcmp(argv[1], "raw_mem") == 0) {
        return cmd_raw_mem(2, argc, argv);
    } else if (strcmp(argv[1], "download") == 0) {
        return download_flight(2, argc, argv);
    } else if (strcmp(argv[1], "read_tp_tsk") == 0) {
        return cmd_read_tp_tsk(2, argc, argv);
    } else if (strcmp(argv[1], "help") == 0 ||
               strcmp(argv[1], "--help") == 0) {
        usage();
        return 0;
    } else {
        arg_error("unknown command");
    }
}
