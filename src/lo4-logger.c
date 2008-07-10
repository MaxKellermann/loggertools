/*
 * loggertools
 * Copyright (C) 2004-2008 Max Kellermann <max@duempel.org>
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
#include <ctype.h>
#include <arpa/inet.h>
#ifdef __GLIBC__
#include <getopt.h>
#endif

#include "version.h"
#include "filser.h"

#define MAX_FLIGHTS 256

struct config {
    int verbose;
    const char *tty;
};

static void usage(void) {
    puts("usage: lo4-logger [OPTIONS] FILE\n"
         "valid options:\n"
#ifdef __GLIBC__
         " --help\n"
#endif
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
         " --tty DEVICE\n"
#endif
         " -t DEVICE      open this tty device (default /dev/ttyS0)\n"
         );
}

static void arg_error(const char *argv0, const char *msg)
    __attribute__((noreturn));
static void
arg_error(const char *argv0, const char *msg)
{
    if (msg != NULL)
        fprintf(stderr, "%s: %s\n", argv0, msg);
    fprintf(stderr, "Try '%s -h' for more information.\n",
            argv0);
    exit(1);
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
    config->verbose = 1;
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
            puts("loggertools v" VERSION " (C) 2004-2007 Max Kellermann <max@duempel.org>\n"
                 "http://max.kellermann.name/projects/loggertools/\n");
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

        case '?':
            arg_error(argv[0], NULL);

        default:
            exit(1);
        }
    }
}

static void alarm_handler(int dummy) {
    (void)dummy;
}

static void syn_ack_wait(filser_t device) {
    int ret;
    unsigned tries = 10;

    do {
        alarm(10);
        ret = filser_syn_ack(device);
        alarm(0);
        if (ret < 0) {
            fprintf(stderr, "failed to connect: %s\n",
                    strerror(errno));
            exit(1);
        }

        if (ret == 0) {
            fprintf(stderr, "no filser found, trying again\n");
            sleep(1);
        }
    } while (ret == 0 && tries-- > 0);

    if (ret == 0) {
        fprintf(stderr, "no filser\n");
        exit(1);
    }
}

static int read_full_crc(filser_t device, void *buffer, size_t len) {
    int ret;

    ret = filser_read_crc(device, buffer, len, 40);
    if (ret == -2) {
        fprintf(stderr, "CRC error\n");
        exit(1);
    }

    return ret;
}

static int communicate(filser_t device, unsigned char cmd,
                       unsigned char *buffer, size_t buffer_len) {
    int ret;

    syn_ack_wait(device);

    tcflush(device->fd, TCIOFLUSH);

    ret = filser_write_cmd(device, cmd);
    if (ret <= 0)
        return -1;

    ret = read_full_crc(device, buffer, buffer_len);
    if (ret <= 0)
        return -1;

    return 1;
}

static int check_mem_settings(filser_t device) {
    int ret;
    unsigned char buffer[6];

    ret = communicate(device, FILSER_CHECK_MEM_SETTINGS,
                      buffer, sizeof(buffer));
    if (ret < 0) {
        fprintf(stderr, "failed to communicate: %s\n",
                strerror(errno));
        exit(1);
    }

    if (ret == 0) {
        fprintf(stderr, "no valid response\n");
        exit(1);
    }

    return 0;
}

static int
download_lo4_section(filser_t device, unsigned char cmd, int fd)
{
    static unsigned char buffer[0x8000];
    int ret;
    ssize_t nbytes;

    ret = filser_send_command(device, cmd);
    if (ret <= 0)
        return -1;

    ret = read_full_crc(device, buffer, sizeof(buffer));
    if (ret <= 0)
        return -1;

    nbytes = write(fd, buffer, sizeof(buffer));
    if (nbytes < 0) {
        perror("failed to write to file");
        exit(2);
    }

    if (nbytes != sizeof(buffer)) {
        fprintf(stderr, "short write to file\n");
        exit(2);
    }

    return 0;
}

int main(int argc, char **argv) {
    int ret;
    filser_t device;
    struct config config;
    int fd;
    unsigned i;

    signal(SIGALRM, alarm_handler);

    parse_cmdline(&config, argc, argv);

    if (optind < argc - 1)
        arg_error(argv[0], "too many arguments");

    if (optind >= argc)
        arg_error(argv[0], "not enough arguments");

    fputs("loggertools v" VERSION " (C) 2004-2008 Max Kellermann <max@duempel.org>\n"
          "http://max.kellermann.name/projects/loggertools/\n\n", stderr);

    ret = filser_open(config.tty, &device);
    if (ret != 0) {
        fprintf(stderr, "filser_open failed: %s\n",
                strerror(errno));
        exit(1);
    }

    fd = open(argv[optind], O_CREAT|O_WRONLY, 0777);
    if (fd < 0) {
        fprintf(stderr, "failed to create file %s: %s\n",
                argv[optind], strerror(errno));
        exit(1);
    }

    check_mem_settings(device);

    syn_ack_wait(device);

    for (i = 0; i < 3; ++i) {
        printf("downloading section %u\n", i);
        download_lo4_section(device, 0x30 + i, fd);
    }

    filser_close(&device);
}
