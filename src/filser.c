/*
 * loggertools
 * Copyright (C) 2004 Max Kellermann (max@duempel.org)
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

static void alarm_handler(int dummy) {
    (void)dummy;
}

static int syn_ack(int fd) {
    const char syn = 0x16, ack = 0x06;
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
    time_t timeout = time(NULL) + 10;

    alarm(10);

    for (;;) {
        nbytes = read(fd, buffer + pos, len - pos);
        if (nbytes < 0) {
            alarm(0);
            return (int)nbytes;
        }

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

static unsigned char calc_crc_char(unsigned char d, unsigned char crc) {
    unsigned char tmp;
    const unsigned char crcpoly = 0x69;
    int count;

    for (count = 8; --count >= 0; d <<= 1) {
        tmp = crc ^ d;
        crc <<= 1;
        if (tmp & 0x80)
            crc ^= crcpoly;
    }
    return crc;
}

static unsigned char calc_crc(const unsigned char *buffer, size_t len) {
    unsigned z;
    unsigned char crc = 0xff;

    for(z = 0; z < len; z++)
        crc = calc_crc_char(buffer[z], crc);

    return crc;
}


static ssize_t read_full_crc(int fd, unsigned char *buffer, size_t len) {
    ssize_t nbytes;
    unsigned char crc;

    nbytes = read_full(fd, buffer, len);
    if (nbytes < 0)
        return nbytes;

    crc = calc_crc(buffer, len - 1);
    if (crc != buffer[len - 1]) {
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

    crc = calc_crc(buffer, nbytes - 1);
    if (crc != buffer[nbytes - 1]) {
        fprintf(stderr, "CRC error\n");
        _exit(1);
    }

    return nbytes;
}

static int communicate(int fd, unsigned char cmd,
                       unsigned char *buffer, size_t buffer_len) {
    const unsigned char prefix = 0x02;
    ssize_t nbytes;
    int ret;

    syn_ack_wait(fd);

    tcflush(fd, TCIOFLUSH);

    nbytes = write(fd, &prefix, sizeof(prefix));
    if (nbytes <= 0)
        return (int)nbytes;

    nbytes = write(fd, &cmd, sizeof(cmd));
    if (nbytes <= 0)
        return (int)nbytes;

    ret = read_full_crc(fd, buffer, buffer_len);
    if (ret < 0)
        return -1;

    return 1;
}

static int check_mem_settings(int fd) {
    int ret;
    unsigned char buffer[7];

    ret = communicate(fd, 'Q' | 0x80,
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

static int flight_list(int argpos, int argc, char **argv) {
    const char *device = "/dev/ttyS0";
    unsigned char cmd[] = { 0x02, 'M' | 0x80 };
    int fd;
    unsigned char record[0x60];
    int ret;

    (void)argpos;
    (void)argc;
    (void)argv;

    fd = connect(device);

    check_mem_settings(fd);

    tcflush(fd, TCIOFLUSH);
    write(fd, cmd, sizeof(cmd));

    for (;;) {
        ret = read_full_crc(fd, record, sizeof(record));
        if (ret < 0) {
            fprintf(stderr, "failed to read from '%s': %s\n",
                    device, strerror(errno));
            _exit(1);
        }

        if (record[0] != 1)
            break;

        printf("%s\t%s-%s\t%s\n", record + 9, record + 18, record + 0x1b, record + 40);
    }

    return 0;
}

static int get_basic_data(int argpos, int argc, char **argv) {
    const char *device = "/dev/ttyS0";
    unsigned char cmd[] = { 0x02, 0xc4 };
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
    unsigned char cmd[] = { 0x02, 0xc9 };
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

int main(int argc, char **argv) {
    signal(SIGALRM, alarm_handler);

    if (argc < 2) {
        fprintf(stderr, "usage: filser command [arg ...]\n");
        _exit(1);
    }

    if (strcmp(argv[1], "raw") == 0) {
        return raw(2, argc, argv);
    } else if (strcmp(argv[1], "list") == 0) {
        return flight_list(2, argc, argv);
    } else if (strcmp(argv[1], "basic") == 0) {
        return get_basic_data(2, argc, argv);
    } else if (strcmp(argv[1], "flight") == 0) {
        return get_flight_info(2, argc, argv);
    } else {
        fprintf(stderr, "usage: filser command [arg ...]\n");
        _exit(1);
    }
}
