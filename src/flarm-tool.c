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

static int
flarm_wait_recv_frame(flarm_t flarm,
                      uint8_t *version_r, uint8_t *type_r,
                      uint16_t *seq_no_r,
                      const void **payload_r, size_t *length_r)
{
    int ret, n = 0;
    struct timespec ts = {
        .tv_sec = 0,
        .tv_nsec = 100000,
    };

    for (n = 0; n < 15; ++n) {
        ret = flarm_recv_frame(flarm, version_r, type_r, seq_no_r,
                               payload_r, length_r);
        if (ret != EAGAIN)
            return ret;

        if (n == 5)
            ts.tv_sec = 1;
        else if (n > 5)
            fprintf(stderr, "waiting for flarm\n");

        nanosleep(&ts, NULL);
    }

    if (n > 6)
        fprintf(stderr, "giving up\n");

    return EAGAIN;
}

static int
flarm_expect_ack(flarm_t flarm,
                 uint16_t expected_seq_no,
                 const void **payload_r, size_t *length_r)
{
    int ret;
    uint8_t version, type;
    uint16_t seq_no;

    while (1) {
        ret = flarm_wait_recv_frame(flarm, &version, &type, &seq_no,
                                    payload_r, length_r);
        if (ret != 0)
            return ret;

        if (seq_no != expected_seq_no) {
            fprintf(stderr, "expected seqno %u, got %u\n",
                    expected_seq_no, seq_no);
            continue;
        }

        if (type != FLARM_MESSAGE_ACK) {
            fprintf(stderr, "expected ACK, flarm response was %d\n",
                    type);
            return -1;
        }

        return 0;
    }
}

static void
flarm_ping_wait(flarm_t flarm)
{
    int ret;
    const void *payload;
    size_t length;

    ret = flarm_send_ping(flarm);
    if (ret != 0) {
        fprintf(stderr, "failed to send ping: %s\n",
                strerror(ret));
        exit(2);
    }

    ret = flarm_expect_ack(flarm, flarm_last_seq_no(flarm),
                           &payload, &length);
    if (ret != 0) {
        if (ret > 0)
            fprintf(stderr, "failed to receive ack: %s\n",
                    strerror(ret));
        exit(2);
    }
}

static void
cmd_ping(const struct config *config, flarm_t flarm,
         int argc, char **argv)
{
    (void)config;
    (void)argv;

    if (optind < argc)
        arg_error("Too many arguments");

    flarm_ping_wait(flarm);
}

static void
cmd_list(const struct config *config, flarm_t flarm,
         int argc, char **argv)
{
    unsigned i;
    int ret;
    const char *payload;
    size_t length;

    (void)config;
    (void)argv;

    if (optind < argc)
        arg_error("Too many arguments");

    flarm_ping_wait(flarm);

    for (i = 0;; ++i) {
        ret = flarm_send_select_record(flarm, i);
        if (ret != 0) {
            fprintf(stderr, "failed to send select record: %s\n",
                    strerror(ret));
            exit(2);
        }

        ret = flarm_expect_ack(flarm, flarm_last_seq_no(flarm),
                               (const void**)&payload, &length);
        if (ret < 0)
            break;

        if (ret != 0) {
            fprintf(stderr, "failed to receive ack: %s\n",
                    strerror(ret));
            exit(2);
        }

        ret = flarm_send_get_record_info(flarm);
        if (ret != 0) {
            fprintf(stderr, "failed to send get record info: %s\n",
                    strerror(ret));
            exit(2);
        }

        ret = flarm_expect_ack(flarm, flarm_last_seq_no(flarm),
                               (const void**)&payload, &length);
        if (ret != 0) {
            if (ret > 0)
                fprintf(stderr, "failed to receive ack: %s\n",
                        strerror(ret));
            exit(2);
        }

        if (length < 4 || payload[length - 1] != 0) {
            fprintf(stderr, "invalid record info\n");
            exit(2);
        }

        fprintf(stderr, "%u. %s\n", i, payload + 2);
    }
}

static void
cmd_download(const struct config *config, flarm_t flarm,
         int argc, char **argv)
{
    unsigned record_no;
    int ret;
    const char *payload;
    size_t length;
    int is_eof = 0;

    (void)config;
    (void)argv;

    if (optind == argc)
        arg_error("Record number missing");

    record_no = (unsigned)atoi(argv[optind++]);

    if (optind < argc)
        arg_error("Too many arguments");

    flarm_ping_wait(flarm);

    ret = flarm_send_select_record(flarm, record_no);
    if (ret != 0) {
        fprintf(stderr, "failed to send select record: %s\n",
                strerror(ret));
        exit(2);
    }

    ret = flarm_expect_ack(flarm, flarm_last_seq_no(flarm),
                           (const void**)&payload, &length);
    if (ret != 0) {
        if (ret > 0)
            fprintf(stderr, "failed to receive ack: %s\n",
                    strerror(ret));
        exit(2);
    }

    while (!is_eof) {
        ret = flarm_send_get_igc_data(flarm);
        if (ret != 0) {
            fprintf(stderr, "failed to send get record info: %s\n",
                    strerror(ret));
            exit(2);
        }

        ret = flarm_expect_ack(flarm, flarm_last_seq_no(flarm),
                               (const void**)&payload, &length);
        if (ret != 0) {
            if (ret > 0)
                fprintf(stderr, "failed to receive ack: %s\n",
                        strerror(ret));
            exit(2);
        }

        if (length < 3) {
            fprintf(stderr, "invalid igc chunk\n");
            exit(2);
        }

        /* XXX: check seqno, print progress */

        if (length > 3 && payload[length - 1] == 0x1a) {
            is_eof = 1;
            --length;
        }

        fwrite(payload + 3, length - 3, 1, stdout);
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
    } else if (strcmp(cmd, "list") == 0) {
        cmd_list(&config, flarm, argc, argv);
    } else if (strcmp(cmd, "download") == 0) {
        cmd_download(&config, flarm, argc, argv);
    } else {
        arg_error("unknown command");
    }

    flarm_close(&flarm);
}
