/*
 * loggertools
 * Copyright (C) 2004-2008 Max Kellermann (max@duempel.org)
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
#include <ctype.h>
#include <arpa/inet.h>
#ifdef __GLIBC__
#include <getopt.h>
#endif

#include "version.h"
#include "zander.h"
#include "datadir.h"

struct config {
    int verbose;
    const char *tty;
};

static void usage(void) {
    puts("usage: zander-logger [OPTIONS]\n"
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
         " --tty DEVICE\n"
#endif
         " -t DEVICE      open this tty device (default /dev/ttyS0)\n"
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
    fprintf(stderr, "zander-logger: %s\n", msg);
    fprintf(stderr, "Try 'zander-logger -h' for more information.\n");
    _exit(1);
}

static void alarm_handler(int dummy) {
    (void)dummy;
}

static int
download_flight(const struct config *config,
                zander_t device, const struct zander_flight *flight,
                const char *filename)
{
    int ret;
    unsigned start = zander_address_to_host(&flight->memory_start);
    unsigned end = zander_address_to_host(&flight->memory_end);
    unsigned length = zander_address_difference(start, end) + 1;
    void *buffer;
    int fd;
    ssize_t nbytes;

    (void)config;

    assert(length > 0);

    if (!zander_address_valid(start) || !zander_address_valid(end)) {
        fprintf(stderr, "Invalid address\n");
        return -1;
    }

    buffer = malloc(length);
    if (buffer == NULL)
        abort();

    ret = zander_read_memory(device, buffer, start, length);
    if (ret != 0) {
        zander_perror("failed to read memory", ret);
        return ret;
    }

    fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fd < 0) {
        fprintf(stderr, "failed to create %s: %s\n",
                filename, strerror(errno));
        free(buffer);
        return -1;
    }

    nbytes = write(fd, buffer, (size_t)length);
    if (nbytes != (ssize_t)length) {
        fprintf(stderr, "failed to write to %s: %s\n",
                filename, strerror(errno));
        close(fd);
        free(buffer);
        return -1;
    }

    close(fd);
    free(buffer);

    return 0;
}

static int
zan_to_igc(const char *in_path, const char *out_path)
{
    (void)in_path;
    (void)out_path;
    return 0;
}

int main(int argc, char **argv) {
    struct config config;
    int ret;
    zander_t zander;
    struct zander_serial serial;
    struct zander_flight flights[ZANDER_MAX_FLIGHTS];
    unsigned i;
    char line[256];

    signal(SIGALRM, alarm_handler);

    parse_cmdline(&config, argc, argv);

    if (optind < argc)
        arg_error("too many arguments");

    fputs("loggertools v" VERSION " (C) 2004-2008 Max Kellermann <max@duempel.org>\n"
          "http://max.kellermann.name/projects/loggertools/\n\n", stderr);

    ret = zander_open(config.tty, &zander);
    if (ret != 0) {
        zander_perror("failed to open zander", ret);
        exit(2);
    }

    ret = zander_read_serial(zander, &serial);
    if (ret != 0) {
        zander_perror("failed to connect to zander", ret);
        exit(2);
    }

    ret = zander_flight_list(zander, flights);
    if (ret != 0) {
        zander_perror("failed to read flight list", ret);
        exit(2);
    }

    for (i = 0; i < ZANDER_MAX_FLIGHTS; ++i) {
        const struct zander_flight *f = &flights[i];
        struct zander_time record_end = f->record_start,
            flight_start = f->record_start, flight_end = f->record_start;

        if (!zander_address_defined(&f->memory_start) ||
            !zander_address_defined(&f->memory_end))
            continue;

        zander_time_add_duration(&record_end, &f->record_end);
        zander_time_add_duration(&flight_start, &f->flight_start);
        zander_time_add_duration(&flight_end, &f->flight_end);

        printf("%3u | %02u.%02u.%02u | %02u:%02u-%02u:%02u | %02u:%02u-%02u:%02u\n",
               ntohs(f->no), f->date.day, f->date.month, f->date.year,
               f->record_start.hour, f->record_start.minute,
               record_end.hour, record_end.minute,
               flight_start.hour, flight_start.minute,
               flight_end.hour, flight_end.minute);
    }

    printf("Which flight shall I download?\n");

    while (1) {
        printf(">> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL ||
            line[0] == 'q')
            break;

        if (line[0] == 'h') {
            printf("Valid commands:\n"
                   "  help      print this help text\n"
                   "  quit      quit this program\n"
                   "  1         download flight number 1\n"
                   "\n");
        } else if (isdigit(line[0])) {
            int nr;
            char zan_filename[32], igc_filename[32];
            const struct zander_flight *flight;

            nr = atoi(line);

            for (i = 0; i < ZANDER_MAX_FLIGHTS; ++i)
                if (ntohs(flights[i].no) == nr &&
                    zander_address_defined(&flights[i].memory_start) &&
                    zander_address_defined(&flights[i].memory_end))
                    break;

            if (i == ZANDER_MAX_FLIGHTS) {
                printf("invalid flight number\n");
                continue;
            }

            flight = &flights[i];

            /*XXX zander_flight_filename(zan_filename, flight);*/
            strcpy(zan_filename, "foobarab");

            memcpy(igc_filename, zan_filename, 8);
            strcpy(zan_filename + 8, ".zan");
            strcpy(igc_filename + 8, ".igc");

            if (config.verbose >= 1)
                printf("Downloading flight %u to %s\n", nr, zan_filename);

            download_flight(&config, zander, flight, zan_filename);

            if (config.verbose >= 1)
                printf("Converting to %s\n", igc_filename);

            zan_to_igc(zan_filename, igc_filename);

            if (config.verbose >= 1)
                printf("Done with flight %u\n", nr);
        } else if (line[0] != 0 && line[0] != '\n') {
            printf("Invalid command.  Type 'help' for more information.\n");
        }
    }

    zander_close(&zander);
}
