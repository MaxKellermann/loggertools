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

#include "hexfile-decoder.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

const unsigned BANK_SIZE = 0x8000;

static void usage(void)
     __attribute__((noreturn));

static void usage(void) {
    fprintf(stderr, "usage: hexfile [options] [filename]\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, " -d           decode\n");
    fprintf(stderr, " -o outfile   write output to this file\n");
    fprintf(stderr, " -h           help (this text)\n");
    _exit(1);
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
    FILE *in, *out = NULL;
    int decode_flag = 0;

    /* parse command line arguments */
    while (1) {
        int c;

        c = getopt(argc, argv, "dho:");
        if (c == -1)
            break;

        switch (c) {
        case 'd':
            decode_flag = 1;
            break;

        case 'h':
            usage();
            break;

        case 'o':
            if (out != NULL)
                fclose(out);
            out = fopen(optarg, "w");
            if (out == NULL) {
                fprintf(stderr, "failed to create '%s': %s\n",
                        optarg, strerror(errno));
                _exit(1);
            }
            break;

        default:
            fprintf(stderr, "invalid getopt code\n");
            _exit(1);
        }
    }

    /* open input stream */
    if (optind < argc) {
        if (optind + 1 < argc) {
            fprintf(stderr, "invalid argument: %s",
                    argv[optind + 1]);
            _exit(1);
        }

        in = fopen(argv[optind], "r");
        if (in == NULL) {
            fprintf(stderr, "failed to open '%s': %s",
                    argv[optind], strerror(errno));
            _exit(1);
        }
    } else {
        in = stdin;
    }

    /* open output stream */
    if (out == NULL)
        out = stdout;

    /* do it */
    if (decode_flag) {
        return decode(in, out);
    } else {
        return encode(in, out);
    }
}
