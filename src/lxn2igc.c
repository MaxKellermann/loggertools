/*
 * loggertools
 * Copyright (C) 2004-2012 Max Kellermann <max@duempel.org>
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

#include "open.h"

#include <assert.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#ifdef __GLIBC__
#include <getopt.h>
#endif

#include "lxn-to-igc.h"

struct config {
    const char *input_path, *output_path;
};

static void usage(void) {
    puts("loggertools (C) 2004-2012 Max Kellermann <max@duempel.org>\n"
         "http://max.kellermann.name/projects/loggertools/\n"
         "\n"
         "usage: lxn2igc [-o FILENAME.igc] FILENAME.lxn\n"
         "valid options:\n"
#ifdef __GLIBC__
         " --output FILENAME.igc\n"
#endif
         " -o FILENAME    write output to this file\n"
         );
}

static void arg_error(const char *msg) __attribute__ ((noreturn));
static void arg_error(const char *msg) {
    fprintf(stderr, "lxn2igc: %s\n", msg);
    fprintf(stderr, "Try 'lxn2igc -h' for more information.\n");
    exit(1);
}

/** read configuration options from the command line */
static void parse_cmdline(struct config *config,
                          int argc, char **argv) {
    int ret;
#ifdef __GLIBC__
    static const struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {"output", 0, 0, 'o'},
        {0,0,0,0}
    };
#endif

    memset(config, 0, sizeof(*config));

    while (1) {
#ifdef __GLIBC__
        int option_index = 0;

        ret = getopt_long(argc, argv, "ho:",
                          long_options, &option_index);
#else
        ret = getopt(argc, argv, "ho:");
#endif
        if (ret == -1)
            break;

        switch (ret) {
        case 'h':
            usage();
            exit(0);

        case 'o':
            config->output_path = optarg;
            break;

        default:
            exit(1);
        }
    }

    if (optind == argc)
        arg_error("No input file specified");

    if (optind < argc - 1)
        arg_error("Too many arguments");

    config->input_path = argv[optind];

    if (config->output_path == NULL) {
        size_t length = strlen(config->input_path);
        char *p = malloc(length + 5);

        if (p == NULL)
            abort();

        if (length >= 4 &&
            (strcasecmp(config->input_path + length - 4, ".lxn") == 0 ||
             strcasecmp(config->input_path + length - 4, ".fil") == 0))
            length -= 4;

        memcpy(p, config->input_path, length);
        memcpy(p + length, ".igc", 5);

        config->output_path = p;
    } else if (strcmp(config->output_path, "-") == 0) {
        config->output_path = NULL;
    }
}

static int run(int fd, lxn_to_igc_t filter) {
    size_t start = 0, end = 0, consumed;
    ssize_t nbytes;
    unsigned char buffer[4096];
    int ret, is_eof = 0, done = 0;

    do {
        /* realign buffer */

        if (start > 0) {
            if (end > start)
                memmove(buffer, buffer + start, end - start);
            end -= start;
            start = 0;
        }

        /* read from input file */

        if (end < sizeof(buffer)) {
            nbytes = read(fd, buffer + end, sizeof(buffer) - end);
            if (nbytes < 0) {
                perror("failed to read from input file");
                return 2;
            } else if (nbytes == 0)
                is_eof = 1;
            else
                end += (size_t)nbytes;
        }

        /* convert a block */

        if (end > start) {
            ret = lxn_to_igc_process(filter, buffer + start, end - start, &consumed);
            if (ret == 0) {
                done = 1;
            } else if (ret != EAGAIN) {
                if (ret == -1 && lxn_to_igc_error(filter) != NULL)
                    fprintf(stderr, "lxn_to_igc_process() failed: %s\n", lxn_to_igc_error(filter));
                else
                    fprintf(stderr, "lxn_to_igc_process() failed: %d\n", ret);
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

    return 0;
}

int main(int argc, char **argv) {
    struct config config;
    int fd, ret, status;
    FILE *output_file;
    lxn_to_igc_t filter;

    /* configuration */

    parse_cmdline(&config, argc, argv);

    /* open files */

    fd = open(config.input_path, O_RDONLY|O_BINARY);
    if (fd < 0) {
        fprintf(stderr, "failed to open %s: %s\n",
                config.input_path, strerror(errno));
        exit(2);
    }

    if (config.output_path == NULL) {
        output_file = stdout;
    } else {
        output_file = fopen(config.output_path, "w");
        if (output_file == NULL) {
            fprintf(stderr, "failed to create %s: %s\n",
                    config.output_path, strerror(errno));
            exit(2);
        }
    }

    ret = lxn_to_igc_open(output_file, &filter);
    if (ret != 0) {
        fprintf(stderr, "lxn_to_igc_open() failed\n");
        exit(2);
    }

    /* convert */

    status = run(fd, filter);

    /* cleanup */

    if (config.output_path != NULL) {
        fclose(output_file);
        if (status != 0)
            unlink(config.output_path);
    }

    ret = lxn_to_igc_close(&filter);
    if (ret != 0) {
        fprintf(stderr, "lxn_to_igc_close() failed\n");
        return 2;
    }

    close(fd);

    return status;
}
