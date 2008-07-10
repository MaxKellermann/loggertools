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
#include "filser.h"
#include "lxn-to-igc.h"

#define MAX_FLIGHTS 256

struct config {
    int verbose;
    const char *tty;
};

static void usage(void) {
    puts("usage: lxn-logger [OPTIONS]\n"
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

static void arg_error(const char *argv0, const char *msg)
    __attribute__((noreturn));
static void
arg_error(const char *argv0, const char *msg)
{
    if (msg != NULL)
        fprintf(stderr, "%s: %s\n", argv0, msg);
    fprintf(stderr, "Try '%s -h' for more information.\n",
            argv0);
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
        {"tty", 1, 0, 't'},
        {0,0,0,0}
    };
#endif

    memset(config, 0, sizeof(*config));
    config->verbose = 1;
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

        case '?':
            arg_error(argv[0], NULL);

        default:
            exit(1);
        }
    }
}

static void alarm_handler(int dummy) {
    (void)dummy;
}

static void syn_ack_wait(filser_t device) {
    int ret;
    unsigned tries = 20;

    do {
        alarm(10);
        ret = filser_syn_ack(device);
        alarm(0);
        if (ret < 0) {
            fprintf(stderr, "failed to connect: %s\n",
                    strerror(errno));
            exit(1);
        }

        if (ret == 0) {
            fprintf(stderr, "no filser found, trying again\n");
            sleep(1);
        }
    } while (ret == 0 && tries-- > 0);

    if (ret == 0) {
        fprintf(stderr, "no filser\n");
        exit(1);
    }
}

static int read_full_crc(filser_t device, void *buffer, size_t len) {
    int ret;

    ret = filser_read_crc(device, buffer, len, 40);
    if (ret == -2) {
        fprintf(stderr, "CRC error\n");
        exit(1);
    }

    return ret;
}

static int communicate(filser_t device, unsigned char cmd,
                       unsigned char *buffer, size_t buffer_len) {
    int ret;

    syn_ack_wait(device);

    tcflush(device->fd, TCIOFLUSH);

    ret = filser_write_cmd(device, cmd);
    if (ret <= 0)
        return -1;

    ret = read_full_crc(device, buffer, buffer_len);
    if (ret <= 0)
        return -1;

    return 1;
}

static int check_mem_settings(filser_t device) {
    int ret;
    unsigned char buffer[6];

    ret = communicate(device, FILSER_CHECK_MEM_SETTINGS,
                      buffer, sizeof(buffer));
    if (ret < 0) {
        fprintf(stderr, "failed to communicate: %s\n",
                strerror(errno));
        exit(1);
    }

    if (ret == 0) {
        fprintf(stderr, "no valid response\n");
        exit(1);
    }

    return 0;
}

static int open_flight_list(filser_t device) {
    int ret;

    syn_ack_wait(device);

    ret = filser_send_command(device, FILSER_READ_FLIGHT_LIST);
    if (ret <= 0)
        return -1;

    return 0;
}

static int next_flight(filser_t device, struct filser_flight_index *flight) {
    int ret;

    ret = read_full_crc(device, (unsigned char*)flight, sizeof(*flight));
    if (ret <= 0)
        return -1;

    if (flight->valid != 1)
        return 0;

    return 1;
}

static unsigned flight_list(filser_t device, struct filser_flight_index *flights,
                            unsigned max_flights) {
    int ret;
    unsigned num_flights = 0;
    struct filser_flight_index *flight = flights;

    ret = open_flight_list(device);
    if (ret < 0) {
        fprintf(stderr, "io error: %s\n", strerror(errno));
        exit(1);
    }

    printf("%-2s  %-8s  %-17s  %s\n",
           "No", "Date", "Time", "Pilot");

    for (num_flights = 0; num_flights < max_flights; ++num_flights, ++flight) {
        ret = next_flight(device, flight);
        if (ret < 0) {
            fprintf(stderr, "io error: %s\n", strerror(errno));
            exit(1);
        }

        if (ret == 0)
            break;

        printf("%2u  %8s  %8s-%8s  %s\n",
               num_flights + 1,
               flight->date, flight->start_time,
               flight->stop_time, flight->pilot);
    }

    return num_flights;
}

static int seek_mem(filser_t device, const struct filser_flight_index *flight) {
    struct filser_packet_def_mem packet;
    int ret;
    unsigned char response;

    /* ignore highest byte here, the same as in kflog */

    /* start address */
    packet.start_address[0] = flight->start_address0;
    packet.start_address[1] = flight->start_address1;
    packet.start_address[2] = flight->start_address2;

    /* end address */
    packet.end_address[0] = flight->end_address0;
    packet.end_address[1] = flight->end_address1;
    packet.end_address[2] = flight->end_address2;

    tcflush(device->fd, TCIOFLUSH);

    ret = filser_write_packet(device, FILSER_DEF_MEM,
                              &packet, sizeof(packet));
    if (ret <= 0)
        return -1;

    alarm(40);
    ret = filser_read(device, &response, sizeof(response), 40);
    alarm(0);
    if (ret < 0)
        return -1;

    if (ret == 0) {
        fprintf(stderr, "no response\n");
        exit(2);
    }

    if (response != FILSER_ACK) {
        fprintf(stderr, "no ack in seek_mem\n");
        exit(1);
    }

    return 0;
}

static int get_mem_section(filser_t device, size_t section_lengths[0x10],
                           size_t *overall_lengthp) {
    struct filser_packet_mem_section packet;
    int ret;
    unsigned z;

    ret = filser_send_command(device, FILSER_GET_MEM_SECTION);
    if (ret <= 0)
        return -1;

    ret = read_full_crc(device, &packet, sizeof(packet));
    if (ret <= 0)
        return -1;

    *overall_lengthp = 0;

    for (z = 0; z < 0x10; z++) {
        section_lengths[z] = ntohs(packet.section_lengths[z]);
        (*overall_lengthp) += section_lengths[z];
    }

    return 0;
}

static int download_section(filser_t device, unsigned section,
                            unsigned char *buffer, size_t length) {
    int ret;

    ret = filser_send_command(device, FILSER_READ_LOGGER_DATA + section);
    if (ret <= 0)
        return -1;

    ret = read_full_crc(device, buffer, length);
    if (ret <= 0)
        return -1;

    return 0;
}

static int download_flight(const struct config *config,
                           filser_t device, const struct filser_flight_index *flight,
                           const char *filename) {
    int ret, fd2;
    size_t section_lengths[0x10], overall_length;
    unsigned char *data, *p;
    unsigned i;

    syn_ack_wait(device);

    if (config->verbose >= 3)
        printf("seeking logger memory\n");
    ret = seek_mem(device, flight);
    if (ret < 0) {
        fprintf(stderr, "io error: %s\n", strerror(errno));
        exit(1);
    }

    if (config->verbose >= 3)
        printf("obtaining logger memory section data\n");
    ret = get_mem_section(device, section_lengths, &overall_length);
    if (ret < 0) {
        fprintf(stderr, "io error: %s\n", strerror(errno));
        exit(1);
    }

    data = malloc(overall_length);
    if (data == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

    fd2 = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fd2 < 0) {
        fprintf(stderr, "failed to create %s: %s\n",
                filename, strerror(errno));
        exit(2);
    }

    for (i = 0, p = data; i < 0x10; i++) {
        if (section_lengths[i] == 0)
            break;

        if (config->verbose >= 3)
            printf("downloading memory section %u from logger, %lu bytes\n",
                   i, (unsigned long)section_lengths[i]);

        ret = download_section(device, i,
                               p, section_lengths[i]);
        if (ret < 0) {
            fprintf(stderr, "io error: %s\n", strerror(errno));
            exit(1);
        }

        write(fd2, p, section_lengths[i]);

        p += section_lengths[i];
    }

    close(fd2);

    free(data);

    return 0;
}

static int cmd_lxn2igc(const char *in_path, const char *out_path) {
    int fd, ret, is_eof = 0, done = 0;
    FILE *out;
    unsigned char buffer[4096];
    size_t start = 0, end = 0, consumed;
    ssize_t nbytes;
    lxn_to_igc_t fti;

    fd = open(in_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "failed to open %s: %s\n",
                in_path, strerror(errno));
        return errno;
    }

    out = fopen(out_path, "w");
    if (out == NULL) {
        fprintf(stderr, "failed to create %s: %s\n",
                out_path, strerror(errno));
        return errno;
    }

    ret = lxn_to_igc_open(out, &fti);
    if (ret != 0) {
        fprintf(stderr, "lxn_to_igc_open() failed\n");
        return 2;
    }

    do {
        if (start > 0) {
            if (end > start)
                memmove(buffer, buffer + start, end - start);
            end -= start;
            start = 0;
        }

        if (end < sizeof(buffer)) {
            nbytes = read(fd, buffer + end, sizeof(buffer) - end);
            if (nbytes < 0) {
                fprintf(stderr, "failed to read from %s: %s\n",
                        in_path, strerror(errno));
                return 2;
            } else if (nbytes == 0)
                is_eof = 1;
            else
                end += (size_t)nbytes;
        }

        if (end > start) {
            ret = lxn_to_igc_process(fti, buffer + start, end - start, &consumed);
            if (ret == 0) {
                done = 1;
            } else if (ret != EAGAIN) {
                if (ret == -1 && lxn_to_igc_error(fti) != NULL)
                    fprintf(stderr, "lxn_to_igc_process() failed: %s\n", lxn_to_igc_error(fti));
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

    ret = lxn_to_igc_close(&fti);
    if (ret != 0) {
        fprintf(stderr, "lxn_to_igc_close() failed\n");
        return 2;
    }

    fclose(out);
    close(fd);

    return 0;
}

int main(int argc, char **argv) {
    int ret;
    filser_t device;
    struct config config;
    struct filser_flight_index flights[MAX_FLIGHTS];
    unsigned num_flights;
    char line[256];

    signal(SIGALRM, alarm_handler);

    parse_cmdline(&config, argc, argv);

    if (optind < argc)
        arg_error(argv[0], "too many arguments");

    fputs("loggertools v" VERSION " (C) 2004-2007 Max Kellermann <max@duempel.org>\n"
          "http://max.kellermann.name/projects/loggertools/\n\n", stderr);

    ret = filser_open(config.tty, &device);
    if (ret != 0) {
        fprintf(stderr, "filser_open failed: %s\n",
                strerror(errno));
        exit(1);
    }

    check_mem_settings(device);

    num_flights = flight_list(device, flights, MAX_FLIGHTS);
    if (num_flights == 0) {
        fprintf(stderr, "no flights on the device\n");
        exit(2);
    }

    if (config.verbose >= 1)
        printf("There are %u flights on the device\n", num_flights);

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
            const struct filser_flight_index *flight;
            char lxn_filename[32], igc_filename[32];

            nr = atoi(line);
            if (nr <= 0 || (unsigned)nr > num_flights) {
                printf("invalid flight number\n");
                continue;
            }

            flight = &flights[nr - 1];

            filser_flight_filename(lxn_filename, flight);

            memcpy(igc_filename, lxn_filename, 8);
            strcpy(lxn_filename + 8, ".lxn");
            strcpy(igc_filename + 8, ".igc");

            if (config.verbose >= 1)
                printf("Downloading flight %u to %s\n", nr, lxn_filename);

            download_flight(&config, device, flight, lxn_filename);

            if (config.verbose >= 1)
                printf("Converting to %s\n", igc_filename);

            cmd_lxn2igc(lxn_filename, igc_filename);

            if (config.verbose >= 1)
                printf("Done with flight %u\n", nr);
        } else if (line[0] != 0 && line[0] != '\n') {
            printf("Invalid command.  Type 'help' for more information.\n");
        }
    }

    filser_close(&device);
}
