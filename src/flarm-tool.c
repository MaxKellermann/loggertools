/*
 * loggertools
 * Copyright (C) 2004-2007 Max Kellermann (max@duempel.org)
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
#include <arpa/inet.h>
#ifdef __GLIBC__
#include <getopt.h>
#endif

#include "version.h"
#include "flarm.h"
#include "datadir.h"

struct config {
    int verbose;
    const char *tty;
};

static void usage(void) {
    puts("usage: flarmtool [OPTIONS] COMMAND [ARGUMENTS]\n"
         "valid options:\n"
#ifdef __GLIBC__
         " --help\n"
#endif
         " -h             help (this text)\n"
#ifdef __GLIBC__
         " --tty DEVICE\n"
#endif
         " -t DEVICE      open this tty device (default /dev/ttyS0)\n"
         "valid commands:\n"
         "  ping\n"
         "        perform a connection test\n"
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

        default:
            exit(1);
        }
    }
}

static void arg_error(const char *msg) __attribute__ ((noreturn));
static void arg_error(const char *msg) {
    fprintf(stderr, "flarmtool: %s\n", msg);
    fprintf(stderr, "Try 'flarmtool -h' for more information.\n");
    _exit(1);
}

static void alarm_handler(int dummy) {
    (void)dummy;
}

static void
cmd_ping(const struct config *config, flarm_t flarm,
         int argc, char **argv)
{
    int ret;
    uint8_t version, type;
    uint16_t seq_no;
    const void *payload;
    size_t length;

    (void)config;
    (void)argv;

    if (optind < argc)
        arg_error("Too many arguments");

    ret = flarm_send_frame(flarm, FLARM_MESSAGE_PING, NULL, 0);
    if (ret != 0) {
        fprintf(stderr, "failed to send ping: %s\n",
                strerror(ret));
        exit(2);
    }

    ret = flarm_recv_frame(flarm, &version, &type, &seq_no,
                           &payload, &length);
    if (ret != 0) {
        fprintf(stderr, "failed to receive ack: %s\n",
                strerror(ret));
        exit(2);
    }

    if (type != FLARM_MESSAGE_ACK) {
        fprintf(stderr, "expected ACK, flarm response was %d\n",
                type);
        exit(2);
    }
}

int main(int argc, char **argv) {
    struct config config;
    const char *cmd;
    int ret;
    flarm_t flarm;

    signal(SIGALRM, alarm_handler);

    parse_cmdline(&config, argc, argv);

    if (optind >= argc)
        arg_error("no command specified");

    cmd = argv[optind++];

    ret = flarm_open(config.tty, &flarm);
    if (ret < 0) {
        perror("failed to open flarm");
        exit(2);
    }

    if (strcmp(cmd, "ping") == 0) {
        cmd_ping(&config, flarm, argc, argv);
    } else {
        arg_error("unknown command");
    }

    flarm_close(&flarm);
}
