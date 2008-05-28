/*
 * loggertools
 * Copyright (C) 2004-2007 Max Kellermann (max@duempel.org)
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
    puts("usage: zandertool [OPTIONS] COMMAND [ARGUMENTS]\n"
         "valid options:\n"
#ifdef __GLIBC__
         " --help\n"
#endif
         " -h             help (this text)\n"
#ifdef __GLIBC__
         " --tty DEVICE\n"
#endif
         " -t DEVICE      open this tty device (default /dev/ttyS0)\n"
         "valid commands:\n"
         "  test\n"
         "        test connection to the GP940\n"
         "  list\n"
         "        print a list of flights\n"
         "  read_tp_tsk <out_filename.da4>\n"
         "        read the TP and TSK database from the device\n"
         "  write_tp_tsk <in_filename.da4>\n"
         "        write the TP and TSK database to the device\n"
         "  write_apt <data_dir>\n"
         "        write the APT database to the device\n"
         "  mem_section <start_adddress> <end_address>\n"
         "        print memory section info\n"
         "  raw_mem <start_adddress> <end_address>\n"
         "        download raw memory\n"
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
    fprintf(stderr, "zandertool: %s\n", msg);
    fprintf(stderr, "Try 'zandertool -h' for more information.\n");
    _exit(1);
}

static void alarm_handler(int dummy) {
    (void)dummy;
}

static int
cmd_info(struct config *config, int argc, char **argv)
{
    int ret;
    zander_t zander;
    struct zander_serial serial;

    (void)argc;
    (void)argv;

    ret = zander_open(config->tty, &zander);
    if (ret < 0) {
        perror("failed to open zander");
        exit(2);
    }

    ret = zander_read_serial(zander, &serial);
    if (ret < 0) {
        perror("failed to connect to zander");
        exit(2);
    }

    printf("serial: %.3s\n"
           "seal: %s\n"
           "type: %s\n"
           "version: %.5s\n"
           "memory: %s\n",
           serial.serial,
           serial.seal == '1' ? "yes" : "no",
           serial.version[1] == '1' ? "GP940" : "GP941",
           serial.version,
           serial.ram_type == '1'
           ? "1 MB"
           : (serial.ram_type == '0'
              ? "512 kB"
              : "unknown"));

    zander_close(&zander);
    return 0;
}

static int
cmd_read_personal_data(struct config *config)
{
    int ret;
    zander_t zander;
    struct zander_personal_data pd;

    ret = zander_open(config->tty, &zander);
    if (ret < 0) {
        perror("failed to open zander");
        exit(2);
    }

    ret = zander_read_personal_data(zander, &pd);
    if (ret < 0) {
        perror("failed to connect to zander");
        exit(2);
    }

    printf("pilot: %.40s\n"
           "model: %.40s\n"
           "competition class: %.40s\n"
           "registration: %.10s\n"
           "competition sign: %.10s\n",
           pd.name,
           pd.aircraft_model,
           pd.competition_class,
           pd.registration,
           pd.competition_sign);

    zander_close(&zander);
    return 0;
}

int main(int argc, char **argv) {
    struct config config;
    const char *cmd;

    signal(SIGALRM, alarm_handler);

    parse_cmdline(&config, argc, argv);

    if (optind >= argc)
        arg_error("no command specified");

    cmd = argv[optind++];

    if (strcmp(cmd, "info") == 0) {
        return cmd_info(&config, argc, argv);
    } if (strcmp(cmd, "personal_data") == 0) {
        return cmd_read_personal_data(&config);
    } else {
        arg_error("unknown command");
    }
}
