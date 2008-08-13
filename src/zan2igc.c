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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#ifdef __GLIBC__
#include <getopt.h>
#endif

#include "zander-igc.h"

struct config {
    const char *input_path, *output_path;
};

static void usage(void) {
    puts("usage: zan2igc [-o FILENAME.igc] FILENAME.zan\n"
         "valid options:\n"
#ifdef __GLIBC__
         " --output FILENAME.igc\n"
#endif
         " -o FILENAME    write output to this file\n"
         );
}

static void arg_error(const char *msg) __attribute__ ((noreturn));
static void arg_error(const char *msg) {
    fprintf(stderr, "zan2igc: %s\n", msg);
    fprintf(stderr, "Try 'zan2igc -h' for more information.\n");
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

        if (length >= 4 && strcasecmp(config->input_path + length - 4, ".zan") == 0)
            length -= 4;

        memcpy(p, config->input_path, length);
        memcpy(p + length, ".igc", 5);

        config->output_path = p;
    } else if (strcmp(config->output_path, "-") == 0) {
        config->output_path = NULL;
    }
}

int main(int argc, char **argv) {
    struct config config;
    FILE *input_file, *output_file;
    enum zander_to_igc_result result;
    int status = 0;

    /* configuration */

    parse_cmdline(&config, argc, argv);

    /* open files */

    input_file = fopen(config.input_path, "r");
    if (input_file == NULL) {
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

    /* convert */

    result = zander_to_igc(input_file, output_file);
    switch (result) {
    case ZANDER_IGC_SUCCESS:
        break;

    case ZANDER_IGC_MALFORMED:
        fprintf(stderr, "malformed input file\n");
        status = 2;
        break;

    case ZANDER_IGC_ERRNO:
        perror("error");
        status = 2;
        break;

    case ZANDER_IGC_EOF:
        fprintf(stderr, "unexpected end of file\n");
        status = 2;
        break;
    }

    /* cleanup */

    if (config.output_path != NULL) {
        fclose(output_file);
        if (status != 0)
            unlink(config.output_path);
    }

    fclose(input_file);

    return status;
}
