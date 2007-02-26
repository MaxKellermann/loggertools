/*
 * loggertools
 * Copyright (C) 2004-2006 Max Kellermann <max@duempel.org>
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

#include "version.h"
#include "hexfile-decoder.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef __GLIBC__
#include <getopt.h>
#endif

struct config {
    int verbose;
    const char *input_path, *output_path;
    int decode;
};

const unsigned BANK_SIZE = 0x8000;

static void usage(void) {
    puts("usage: hexfile [-o FILENAME.bhf] FILENAME.bin\n"
         "       hexfile -d [-o FILENAME.bin] FILENAME.bhf\n"
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
         " --output FILENAME\n"
#endif
         " -o FILENAME    write output to this file\n"
#ifdef __GLIBC__
         " --decode\n"
#endif
         " -d             decode a hexfile instead of encoding it\n"
         );
}

static void arg_error(const char *msg) __attribute__ ((noreturn));
static void arg_error(const char *msg) {
    fprintf(stderr, "lxn2igc: %s\n", msg);
    fprintf(stderr, "Try 'lxn2igc --help' for more information.\n");
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
        {"output", 1, 0, 'o'},
        {"decode", 0, 0, 'd'},
        {0,0,0,0}
    };
#endif

    memset(config, 0, sizeof(*config));
    config->verbose = 1;

    while (1) {
#ifdef __GLIBC__
        int option_index = 0;

        ret = getopt_long(argc, argv, "hVvqo:d",
                          long_options, &option_index);
#else
        ret = getopt(argc, argv, "hVvqo:d");
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

        case 'o':
            config->output_path = optarg;
            break;

        case 'd':
            config->decode = 1;
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
    if (strcmp(config->input_path, "-") == 0)
        config->input_path = NULL;

    if (strcmp(config->output_path, "-") == 0)
        config->output_path = NULL;
}

static void write_record(FILE *file, size_t length, unsigned address,
                         unsigned type, const unsigned char *data) {
    unsigned z;
    unsigned char checksum = 0;

    /* write header */
    fprintf(file, ":%02X%04X%02X",
            (unsigned)length & 0xff,
            address & 0xffff,
            type & 0xff);

    checksum -= length + (address >> 8) + address + type;

    /* write data */
    for (z = 0; z < length; z++) {
        fprintf(file, "%02X", data[z] & 0xff);
        checksum -= data[z];
    }

    /* write checksum */
    fprintf(file, "%02X\r\n", checksum & 0xff);
}

static int encode(FILE *in, FILE *out) {
    unsigned bank = 0, address = 0;
    unsigned char buffer[0x10];
    size_t nbytes;

    /* write data records, 16 bytes each */
    while ((nbytes = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        write_record(out, (size_t)nbytes, address,
                     0, buffer);

        address += (size_t)nbytes;
        if (address >= BANK_SIZE) {
            /* switch to next bank */
            bank++;
            address %= BANK_SIZE;

            write_record(out, 0, 0, 0x10 + bank, NULL);
        }
    }

    /* write end-of-file record */
    write_record(out, 0, 0, 1, NULL);

    return 0;
}

static void force_seek_cur(FILE *file, long offset) {
    int ret;
    char zero[1024];
    size_t length, nbytes;

    ret = fseek(file, offset, SEEK_CUR);
    if (ret == 0)
        return;

    if (errno != EBADF) {
        fprintf(stderr, "failed to seek: %s\n",
                strerror(errno));
        _exit(1);
    }

    if (offset < 0) {
        fprintf(stderr, "cannot rewind on stream\n");
        _exit(1);
    }

    memset(zero, 0, sizeof(zero));

    while (offset > 0) {
        length = sizeof(zero);
        if (length > (size_t)offset)
            length = (size_t)offset;

        nbytes = fwrite(zero, 1, length, file);
        if (nbytes < length) {
            fprintf(stderr, "short write\n");
            _exit(1);
        }

        offset -= nbytes;
    }
}

struct decoder {
    unsigned base;
    FILE *file;
    int eof;
    long position;
};

static int hfd_callback(void *ctx,
                        unsigned char type, unsigned offset,
                        unsigned char *data, size_t length) {
    struct decoder *decoder = (struct decoder*)ctx;

    if (decoder->eof) {
        errno = EINVAL;
        return -1;
    }

    if (type == 0x00) {
        /* data record */
        long new_position;
        size_t nbytes;

        new_position = (long)decoder->base + offset;
        if (new_position != decoder->position) {
            force_seek_cur(decoder->file, new_position - decoder->position);
            decoder->position = new_position;
        }

        nbytes = fwrite(data, 1, length, decoder->file);
        if (nbytes < length) {
            fprintf(stderr, "short write\n");
            _exit(1);
        }

        decoder->position += nbytes;
        return 0;
    } else if (type == 0x01) {
        /* EOF record */
        decoder->eof = 1;
        return 0;
    } else if (type >= 0x10) {
        /* switch memory bank */
        decoder->base = (type - 0x10) * BANK_SIZE;
        return 0;
    } else {
        errno = ENOSYS;
        return -1;
    }
}

static int decode(FILE *in, FILE *out) {
    struct hexfile_decoder *hfd = NULL;
    struct decoder decoder = {
        .base = 0,
        .file = out,
        .eof = 0,
        .position = 0,
    };
    char buffer[4096];
    int ret;
    size_t nbytes;

    ret = hexfile_decoder_new(hfd_callback, &decoder, &hfd);
    if (ret < 0) {
        fprintf(stderr, "failed to create hexfile decoder\n");
        _exit(2);
    }

    while ((nbytes = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        ret = hexfile_decoder_feed(hfd, buffer, nbytes);
        if (ret < 0) {
            fprintf(stderr, "failed to decode hexfile\n");
            _exit(2);
        }
    }

    ret = hexfile_decoder_close(&hfd);
    if (ret < 0) {
        fprintf(stderr, "failed to close hexfile decoder\n");
        _exit(2);
    }

    if (!decoder.eof) {
        fprintf(stderr, "no EOF record\n");
        _exit(1);
    }

    return 0;
}

int main(int argc, char **argv) {
    struct config config;
    FILE *in, *out;

    parse_cmdline(&config, argc, argv);

    if (config.input_path == NULL) {
        in = stdin;
    } else {
        in = fopen(argv[optind], "r");
        if (in == NULL) {
            fprintf(stderr, "failed to open '%s': %s",
                    argv[optind], strerror(errno));
            _exit(1);
        }
    }

    if (config.output_path == NULL) {
        out = stdout;
    } else {
        out = fopen(optarg, "w");
        if (out == NULL) {
            fprintf(stderr, "failed to create '%s': %s\n",
                    optarg, strerror(errno));
            _exit(1);
        }
    }

    /* do it */
    if (config.decode) {
        return decode(in, out);
    } else {
        return encode(in, out);
    }
}
