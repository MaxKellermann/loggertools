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
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static void usage() {
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
    fprintf(file, "%02X\n", checksum & 0xff);
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
        if (address > 0xffff) {
            /* switch to next bank */
            bank++;
            address &= 0xffff;

            write_record(out, 0, 0, 0x10 + bank, NULL);
        }
    }

    /* write end-of-file record */
    write_record(out, 0, 0, 1, NULL);

    return 0;
}

static unsigned decode_hex_digit(char ch) {
    if (ch >= '0' && ch <= '9')
        return ch - '0';

    if (ch >= 'a' && ch <= 'f')
        return 0x10 + ch - 'a';

    if (ch >= 'A' && ch <= 'F')
        return 10 + ch - 'A';

    return 0;
}

static int decode_record(char *line, size_t line_length,
                         size_t *lengthp, unsigned *addressp,
                         unsigned *typep,
                         unsigned char *data, size_t data_max_len) {
    unsigned char header[4];
    unsigned char checksum = 0, checksum2;
    unsigned z;

    if (line[0] != ':' || line_length < 11)
        return 0;

    for (z = 0; z < sizeof(header) / sizeof(header[0]); z++) {
        header[z] = decode_hex_digit(line[z * 2 + 1]) * 0x10 +
            decode_hex_digit(line[z * 2 + 2]);
        checksum -= header[z];
    }

    *lengthp = header[0];
    if (line_length != *lengthp * 2 + 11 ||
        *lengthp > data_max_len)
        return 0;

    *addressp = header[1] * 0x100 + header[2];
    *typep = header[3];

    for (z = 0; z < *lengthp; z++) {
        data[z] = decode_hex_digit(line[z * 2 + 9]) * 0x10 +
            decode_hex_digit(line[z * 2 + 10]);
        checksum -= data[z];
    }

    checksum2 = decode_hex_digit(line[line_length - 2]) * 0x10 +
        decode_hex_digit(line[line_length - 1]);

    if (checksum != checksum2)
        return 0;

    return 1;
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

static int decode(FILE *in, FILE *out) {
    char line[4096];
    unsigned bank = 0;
    long position = 0, new_position;
    int ret;
    size_t line_length, record_length, nbytes;
    unsigned record_address, record_type;
    unsigned char data[256];

    while ((fgets(line, sizeof(line), in)) != NULL) {
        line_length = strlen(line);

        /* trim */
        while (line_length > 0 &&
               line[line_length - 1] >= 0 &&
               line[line_length - 1] <= ' ')
            line_length--;

        /* decode this record */
        ret = decode_record(line, line_length,
                            &record_length, &record_address,
                            &record_type, data, sizeof(data));
        if (ret <= 0) {
            fprintf(stderr, "invalid record\n");
            _exit(1);
        }

        if (record_type == 0x00) {
            /* data record */
            new_position = (long)bank * 0x8000 + record_address;
            if (new_position != position) {
                force_seek_cur(out, new_position - position);
                position = new_position;
            }

            nbytes = fwrite(data, 1, record_length, out);
            if (nbytes < record_length) {
                fprintf(stderr, "short write\n");
                _exit(1);
            }

            position += nbytes;
        } else if (record_type == 0x01) {
            /* EOF record */
            return 0;
        } else if (record_type >= 0x10) {
            /* switch memory bank */
            bank = record_type - 0x10;
        } else {
            fprintf(stderr, "unknown record type %u\n", record_type);
            _exit(1);
        }
    }

    fprintf(stderr, "no EOF record\n");
    _exit(1);
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
