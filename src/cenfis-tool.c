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

#include "version.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef __GLIBC__
#include <getopt.h>
#endif

#include "cenfis.h"

struct config {
    int verbose;
    const char *tty;
};

static void usage(void) {
    puts("usage: cenfistool COMMAND [ARGUMENTS]\n"
         "commands:\n"
         "  send <filename.bhf>\n"
         "      directly send the specified file to the Cenfis device\n"
         "  upload <filename.bhf>\n"
         "      synchronize with the Cenfis device, then upload the specified hexfile.\n"
         "  help\n"
         "      print this help page\n");
}

static void arg_error(const char *msg) __attribute__ ((noreturn));
static void arg_error(const char *msg) {
    fprintf(stderr, "cenfistool: %s\n", msg);
    fprintf(stderr, "Try 'cenfistool -h' for more information.\n");
    _exit(1);
}

/** read configuration options from the command line */
static void
parse_cmdline(struct config *config, int argc, char **argv)
{
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
            puts("loggertools v" VERSION " (C) 2004-2008 Max Kellermann <max@duempel.org>\n"
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

static void
command_send(const struct config *config, int argc, char **argv, int pos)
{
    const char *filename = NULL;
    FILE *file;
    char line[256];
    size_t length;
    cenfis_status_t status;
    cenfis_t cenfis;
    struct timeval tv;

    if (pos >= argc)
        arg_error("Please provide a file name");

    if (pos < argc - 1)
        arg_error("Too many arguments after command");

    filename = argv[pos];

    file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "failed to open '%s': %s\n",
                filename, strerror(errno));
        _exit(1);
    }

    status = cenfis_open(config->tty, &cenfis);
    if (cenfis_is_error(status)) {
        if (status == CENFIS_STATUS_ERRNO) {
            fprintf(stderr, "failed to open '%s': %s\n",
                    config->tty, strerror(errno));
        } else {
            fprintf(stderr, "cenfis_open('%s') failed with status %d\n",
                    config->tty, status);
        }
        _exit(1);
    }

    printf("Sending data to device\n");

    /* it's data time. read the input file */
    while (fgets(line, sizeof(line) - 2, file) != NULL) {
        /* format the line */
        length = strlen(line);
        while (length > 0 && line[length - 1] >= 0 &&
               line[length - 1] <= ' ')
            length--;

        if (length == 0)
            continue;

        line[length++] = '\r';
        line[length++] = '\n';

        /* write line to device */
        status = cenfis_write_data(cenfis, line, length);
        if (cenfis_is_error(status)) {
            fprintf(stderr, "cenfis_write_data failed with status %d\n",
                    status);
            _exit(1);
        }

        /* wait for ACK from device */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        status = cenfis_select(cenfis, &tv);
        if (cenfis_is_error(status)) {
            fprintf(stderr, "cenfis_select failed with status %d\n", status);
            _exit(1);
        }

        if (status == CENFIS_STATUS_WAIT_ACK) {
            fprintf(stderr, "no ack from device\n");
            _exit(1);
        } else if (status == CENFIS_STATUS_DATA) {
            printf(".");
            fflush(stdout);
        } else {
            fprintf(stderr, "wrong status %d\n", status);
            _exit(1);
        }
    }
}

static void
command_upload(const struct config *config, int argc, char **argv, int pos)
{
    const char *filename = NULL;
    FILE *file;
    char line[256];
    size_t length;
    cenfis_status_t status;
    cenfis_t cenfis;
    struct timeval tv;

    if (pos >= argc)
        arg_error("Please provide a file name");

    if (pos < argc - 1)
        arg_error("Too many arguments after command");

    filename = argv[pos];

    file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "failed to open '%s': %s\n",
                filename, strerror(errno));
        _exit(1);
    }

    status = cenfis_open(config->tty, &cenfis);
    if (cenfis_is_error(status)) {
        if (status == CENFIS_STATUS_ERRNO) {
            fprintf(stderr, "failed to open '%s': %s\n",
                    config->tty, strerror(errno));
        } else {
            fprintf(stderr, "cenfis_open('%s') failed with status %d\n",
                    config->tty, status);
        }
        _exit(1);
    }

    /*cenfis_dump(cenfis, 1);*/

    /* wait for the device to become ready and automatically respond
       to questions */
    while (1) {
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        printf("Waiting for device\n");

        status = cenfis_select(cenfis, &tv);
        if (cenfis_is_error(status)) {
            fprintf(stderr, "cenfis_select failed with status %d\n", status);
            _exit(1);
        }

        if (status == CENFIS_STATUS_DIALOG_CONFIRM) {
            printf("Sending 'Y' to confirm\n");

            status = cenfis_confirm(cenfis);
            if (cenfis_is_error(status)) {
                fprintf(stderr, "cenfis_confirm failed with status %d\n",
                        status);
                _exit(1);
            }
        } else if (status == CENFIS_STATUS_DIALOG_SELECT) {
            printf("Sending 'P' response\n");

            status = cenfis_dialog_respond(cenfis, 'P');
            if (cenfis_is_error(status)) {
                fprintf(stderr, "cenfis_dialog_respond failed with status %d\n",
                        status);
                _exit(1);
            }
        } else if (status == CENFIS_STATUS_DATA) {
            break;
        }
    }

    printf("Sending data to device\n");

    /* it's data time. read the input file */
    while (fgets(line, sizeof(line) - 2, file) != NULL) {
        /* format the line */
        length = strlen(line);
        while (length > 0 && line[length - 1] >= 0 &&
               line[length - 1] <= ' ')
            length--;

        if (length == 0)
            continue;

        line[length++] = '\r';
        line[length++] = '\n';

        /* write line to device */
        status = cenfis_write_data(cenfis, line, length);
        if (cenfis_is_error(status)) {
            fprintf(stderr, "cenfis_write_data failed with status %d\n",
                    status);
            _exit(1);
        }

        /* wait for ACK from device */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        status = cenfis_select(cenfis, &tv);
        if (cenfis_is_error(status)) {
            fprintf(stderr, "cenfis_select failed with status %d\n", status);
            _exit(1);
        }

        if (status == CENFIS_STATUS_WAIT_ACK) {
            fprintf(stderr, "no ack from device\n");
            _exit(1);
        } else if (status == CENFIS_STATUS_DATA) {
            printf(".");
            fflush(stdout);
        } else {
            fprintf(stderr, "wrong status %d\n", status);
            _exit(1);
        }
    }
}

int main(int argc, char **argv) {
    struct config config;
    const char *cmd;

    parse_cmdline(&config, argc, argv);

    if (optind >= argc)
        arg_error("no command specified");

    cmd = argv[optind++];

    if (strcmp(cmd, "send") == 0) {
        command_send(&config, argc, argv, optind);
    } else if (strcmp(cmd, "upload") == 0) {
        command_upload(&config, argc, argv, optind);
    } else {
        arg_error("Unknown command");
    }

    return 0;
}
