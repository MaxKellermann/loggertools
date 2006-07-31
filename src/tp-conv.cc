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

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>

#include "tp.hh"

static void usage()
    __attribute__((noreturn));

static void usage() {
    fprintf(stderr, "usage: loggerconf [options] FILE1 ...\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, " -o outfile   write output to this file\n");
    fprintf(stderr, " -f outformat write output to stdout with this format\n");
    fprintf(stderr, " -h           help (this text)\n");
    _exit(1);
}

TurnPointFormat *getFormatFromFilename(const char *filename) {
    const char *dot;
    TurnPointFormat *format;

    dot = strchr(filename, '.');
    if (dot == NULL || dot[1] == 0) {
        fprintf(stderr, "No filename extension in '%s'\n",
                filename);
        _exit(1);
    }

    format = getTurnPointFormat(dot + 1);
    if (format == NULL) {
        fprintf(stderr, "Format '%s' is not supported\n",
                dot + 1);
        _exit(1);
    }

    return format;
}

int main(int argc, char **argv) {
    const char *out_filename = NULL, *stdout_format = NULL;
    TurnPointFormat *out_format;
    FILE *out;
    TurnPointWriter *writer;
    const TurnPoint *tp;

    /* parse command line arguments */
    while (1) {
        int c;

        c = getopt(argc, argv, "ho:f:");
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            usage();
            break;

        case 'o':
            out_filename = optarg;
            stdout_format = NULL;
            break;

        case 'f':
            stdout_format = optarg;
            out_filename = NULL;
            break;

        default:
            fprintf(stderr, "invalid getopt code\n");
            _exit(1);
        }
    }

    if (out_filename == NULL && stdout_format == NULL) {
        fprintf(stderr, "no output filename specified\n");
        _exit(1);
    }

    if (optind >= argc) {
        fprintf(stderr, "no input filename specified\n");
        _exit(1);
    }

    /* open output file */

    if (out_filename == NULL) {
        out_format = getTurnPointFormat(stdout_format);
        if (out_format == NULL) {
            fprintf(stderr, "Format '%s' is not supported\n",
                    stdout_format);
            _exit(1);
        }
    } else {
        out_format = getFormatFromFilename(out_filename);
    }

    if (out_filename == NULL) {
        out = stdout;
    } else {
        out = fopen(out_filename, "w");
        if (out == NULL) {
            fprintf(stderr, "failed to create '%s': %s\n",
                    out_filename, strerror(errno));
            _exit(1);
        }
    }

    writer = out_format->createWriter(out);
    if (writer == NULL) {
        unlink(out_filename);
        fprintf(stderr, "writing this type is not supported\n");
        _exit(1);
    }

    /* read all input files */

    while (optind < argc) {
        const char *in_filename = argv[optind++];

        TurnPointFormat *in_format = getFormatFromFilename(in_filename);

        FILE *in = fopen(in_filename, "r");
        if (in == NULL) {
            fprintf(stderr, "failed to open '%s': %s\n",
                    in_filename, strerror(errno));
            _exit(1);
        }

        TurnPointReader *reader = in_format->createReader(in);
        if (reader == NULL) {
            fprintf(stderr, "reading this type is not supported\n");
            _exit(1);
        }

        /* transfer data */
        try {
            while ((tp = reader->read()) != NULL) {
                writer->write(*tp);
                delete tp;
            }

            writer->flush();
        } catch (TurnPointReaderException *e) {
            delete writer;
            delete reader;
            unlink(out_filename);
            fprintf(stderr, "%s\n", e->getMessage());
            _exit(1);
        } catch (TurnPointWriterException *e) {
            delete writer;
            delete reader;
            unlink(out_filename);
            fprintf(stderr, "%s\n", e->getMessage());
            _exit(1);
        }

        delete reader;
    }

    delete writer;

    return 0;
}
