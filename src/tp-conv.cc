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
#include "tp-io.hh"

#include <fstream>
#include <iostream>
#include <list>

using std::cout;
using std::cerr;
using std::endl;

static void usage() {
    cout << "usage: loggerconv [options] FILE1 ...\n"
        "options:\n"
        " -o outfile   write output to this file\n"
        " -f outformat write output to stdout with this format\n"
        " -F filter    use a filter\n"
        " -h           help (this text)\n";
}

const TurnPointFormat *getFormatFromFilename(const char *filename) {
    const char *dot;
    const TurnPointFormat *format;

    dot = strchr(filename, '.');
    if (dot == NULL || dot[1] == 0) {
        cerr << "No filename extension in " << filename << endl;
        exit(1);
    }

    format = getTurnPointFormat(dot + 1);
    if (format == NULL) {
        cerr << "Format '" << (dot + 1) << "' is not supported" << endl;
        exit(1);
    }

    return format;
}

int main(int argc, char **argv) {
    const char *out_filename = NULL, *stdout_format = NULL;
    std::list<const char*> filters;
    const TurnPointFormat *out_format;
    TurnPointWriter *writer;
    const TurnPoint *tp;

    /* parse command line arguments */
    while (1) {
        int c;

        c = getopt(argc, argv, "ho:f:F:");
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

        case 'F':
            filters.push_back(optarg);
            break;

        default:
            cerr << "Invalid getopt code" << endl;
            exit(1);
        }
    }

    if (out_filename == NULL && stdout_format == NULL) {
        cerr << "No output filename specified" << endl;
        exit(1);
    }

    if (optind >= argc) {
        cerr << "No input filename specified" << endl;
        exit(1);
    }

    /* open output file */

    std::ostream *out;

    if (out_filename == NULL) {
        out_format = getTurnPointFormat(stdout_format);
        if (out_format == NULL) {
            cerr << "Format '" << stdout_format << "' is not supported"
                 << endl;
            exit(1);
        }
    } else {
        out_format = getFormatFromFilename(out_filename);
    }

    if (out_filename == NULL) {
        out = &cout;
    } else {
        out = new std::ofstream(out_filename);
        if (out->fail()) {
            cerr << "Failed to create " << out_filename
                 << ": " << strerror(errno) << endl;
            exit(2);
        }
    }

    out->exceptions(std::ios_base::badbit | std::ios_base::failbit);

    writer = out_format->createWriter(out);
    if (writer == NULL) {
        unlink(out_filename);
        cerr << "Writing this type is not supported" << endl;
        exit(1);
    }

    /* read all input files */

    while (optind < argc) {
        const char *in_filename = argv[optind++];

        const TurnPointFormat *in_format = getFormatFromFilename(in_filename);
        std::ifstream in(in_filename);
        if (in.fail()) {
            cerr << "Failed to open " << in_filename
                 << ": " << strerror(errno) << endl;
            exit(2);
        }

        in.exceptions(std::ios_base::badbit | std::ios_base::failbit);

        TurnPointReader *reader = in_format->createReader(&in);
        if (reader == NULL) {
            cerr << "Reading this type is not supported" << endl;
            exit(1);
        }

        for (std::list<const char*>::const_iterator it = filters.begin();
             it != filters.end(); ++it) {
            std::string s = *it;
            int colon = s.find(':');
            std::string filter_name = colon >= 0
                ? std::string(s, 0, colon)
                : s;
            const char *args = colon >= 0
                ? std::string(s, colon + 1).c_str()
                : NULL;
            const TurnPointFilter *filter
                = getTurnPointFilter(filter_name.c_str());
            reader = filter->createFilter(reader, args);
        }

        /* transfer data */
        try {
            while ((tp = reader->read()) != NULL) {
                writer->write(*tp);
                delete tp;
            }
        } catch (const std::exception &e) {
            delete writer;
            delete reader;
            unlink(out_filename);
            cerr << e.what() << endl;
            exit(2);
        }

        delete reader;
    }

    writer->flush();

    delete writer;

    if (out == &cout)
        out->flush();
    else
        delete out;

    return 0;
}
