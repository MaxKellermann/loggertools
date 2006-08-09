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

#include <errno.h>
#include <stdio.h>
#include <getopt.h>

#include "airspace.hh"
#include "airspace-io.hh"

#include <fstream>
#include <iostream>

static void usage() {
    std::cout << "usage: asconv [options] FILE\n"
        "options:\n"
        " -o outfile   write output to this file\n"
        " -f outformat write output to stdout with this format\n"
        " -h           help (this text)\n";
}

AirspaceFormat *getFormatFromFilename(const char *filename) {
    const char *dot;
    AirspaceFormat *format;

    dot = strchr(filename, '.');
    if (dot == NULL || dot[1] == 0) {
        fprintf(stderr, "No filename extension in '%s'\n",
                filename);
        _exit(1);
    }

    format = getAirspaceFormat(dot + 1);
    if (format == NULL) {
        fprintf(stderr, "Format '%s' is not supported\n",
                dot + 1);
        _exit(1);
    }

    return format;
}

int main(int argc, char **argv) {
    const char *in_filename, *out_filename = NULL, *stdout_format = NULL;
    AirspaceFormat *in_format, *out_format;
    std::ostream *out;
    AirspaceReader *reader;
    AirspaceWriter *writer;
    const Airspace *tp;

    /* parse command line arguments */
    while (1) {
        int c;

        c = getopt(argc, argv, "ho:f:");
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            usage();
            return 0;
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

    in_filename = argv[optind];

    /* open files */

    if (out_filename == NULL) {
        out_format = getAirspaceFormat(stdout_format);
        if (out_format == NULL) {
            fprintf(stderr, "Format '%s' is not supported\n",
                    stdout_format);
            _exit(1);
        }
    } else {
        out_format = getFormatFromFilename(out_filename);
    }
    in_format = getFormatFromFilename(in_filename);

    std::ifstream in(in_filename);
    if (in.fail()) {
        std::cerr << "Failed to open " << in_filename << std::endl;
        exit(2);
    }

    reader = in_format->createReader(&in);
    if (reader == NULL) {
        fprintf(stderr, "reading this type is not supported\n");
        _exit(1);
    }

    if (out_filename == NULL) {
        out = &std::cout;
    } else {
        out = new std::ofstream(out_filename);
        if (out->fail()) {
            fprintf(stderr, "failed to create '%s': %s\n",
                    out_filename, strerror(errno));
            _exit(1);
        }
    }

    out->exceptions(std::ios_base::badbit | std::ios_base::failbit);

    writer = out_format->createWriter(out);
    if (writer == NULL) {
        unlink(out_filename);
        fprintf(stderr, "writing this type is not supported\n");
        _exit(1);
    }

    /* transfer data */
    try {
        while ((tp = reader->read()) != NULL) {
            writer->write(*tp);
            delete tp;
        }

        writer->flush();
    } catch (const std::exception &e) {
        delete writer;
        delete reader;
        unlink(out_filename);
        fprintf(stderr, "%s\n", e.what());
        _exit(1);
    }

    delete writer;
    delete reader;

    return 0;
}
