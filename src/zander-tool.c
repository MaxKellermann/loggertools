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
    if (ret != 0) {
        zander_perror("failed to open zander", ret);
        exit(2);
    }

    ret = zander_read_serial(zander, &serial);
    if (ret != 0) {
        zander_perror("failed to connect to zander", ret);
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
    if (ret != 0) {
        zander_perror("failed to open zander", ret);
        exit(2);
    }

    ret = zander_read_personal_data(zander, &pd);
    if (ret != 0) {
        zander_perror("failed to connect to zander", ret);
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

static int
cmd_battery(struct config *config)
{
    int ret;
    zander_t zander;
    struct zander_serial serial;
    struct zander_battery battery;
    float voltage;

    ret = zander_open(config->tty, &zander);
    if (ret != 0) {
        zander_perror("failed to open zander", ret);
        exit(2);
    }

    ret = zander_read_serial(zander, &serial);
    if (ret != 0) {
        zander_perror("failed to connect to zander", ret);
        exit(2);
    }

    ret = zander_read_li_battery(zander, &battery);
    if (ret != 0) {
        zander_perror("failed to read Li battery", ret);
        exit(2);
    }

    voltage = ntohs(battery.voltage) * 0.000686;
    printf("Li battery: %.2f V\n", voltage);

    if (serial.version[1] != '1') {
        /* 9V battery not available in GP940 */

        ret = zander_read_9v_battery(zander, &battery);
        if (ret != 0) {
            zander_perror("failed to read 9V battery", ret);
            exit(2);
        }

        voltage = ntohs(battery.voltage) * 0.000686;
        printf("9V battery: %.2f V\n", voltage);
    }

    zander_close(&zander);
    return 0;
}

static int
cmd_show_task(struct config *config)
{
    int ret;
    zander_t zander;
    struct zander_serial serial;
    struct zander_read_task task;
    unsigned i;

    ret = zander_open(config->tty, &zander);
    if (ret != 0) {
        zander_perror("failed to open zander", ret);
        exit(2);
    }

    ret = zander_read_serial(zander, &serial);
    if (ret != 0) {
        zander_perror("failed to connect to zander", ret);
        exit(2);
    }

    ret = zander_read_task(zander, &task);
    if (ret != 0) {
        zander_perror("failed to read task", ret);
        exit(2);
    }

    printf("uploaded: %02u/%02u/%02u %02u:%02u:%02u\n",
           task.upload_date.year,
           task.upload_date.month,
           task.upload_date.day,
           task.upload_time.hour,
           task.upload_time.minute,
           task.upload_time.second);

    printf("for: %02u/%02u/%02u\n",
           task.waypoints[0].date.year,
           task.waypoints[0].date.month,
           task.waypoints[0].date.day);

    for (i = 0; i < task.waypoints[0].num_wp; ++i) {
        printf("waypoint%u: '%.12s' %02u.%02u.%02u %c, %03u.%02u.%02u %c\n",
               i,
               task.waypoints[i].name,
               task.waypoints[i].latitude.degrees,
               task.waypoints[i].latitude.minutes,
               task.waypoints[i].latitude.seconds,
               task.waypoints[i].latitude.sign == 1 ? 'S' : 'N',
               task.waypoints[i].longitude.degrees,
               task.waypoints[i].longitude.minutes,
               task.waypoints[i].longitude.seconds,
               task.waypoints[i].longitude.sign == 1 ? 'W' : 'E');
    }

    zander_close(&zander);
    return 0;
}

static int
cmd_list(struct config *config)
{
    int ret;
    zander_t zander;
    struct zander_serial serial;
    struct zander_flight flights[ZANDER_MAX_FLIGHTS];
    unsigned i;

    ret = zander_open(config->tty, &zander);
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

    zander_close(&zander);
    return 0;
}

static int
cmd_raw(struct config *config, int argc, char **argv)
{
    int ret;
    zander_t zander;
    struct zander_serial serial;
    unsigned char buffer[1024];
    ssize_t nbytes;
    size_t offset;
    unsigned tries;

    for (ret = 0; ret < argc; ++ret)
        buffer[ret] = strtoul(argv[ret], NULL, 0);

    ret = zander_open(config->tty, &zander);
    if (ret != 0) {
        zander_perror("failed to open zander", ret);
        exit(2);
    }

    ret = zander_read_serial(zander, &serial);
    if (ret != 0) {
        zander_perror("failed to connect to zander", ret);
        exit(2);
    }

    write(zander_fileno(zander), buffer, argc);

    for (tries = 0, offset = 0; tries < 20; ++tries) {
        nbytes = read(zander_fileno(zander), buffer, sizeof(buffer));
        if (nbytes > 0) {
            write(1, buffer, (size_t)nbytes);
            offset += (size_t)nbytes;

            tries = 0;
        } else if (nbytes < 0) {
            perror("failed to read");
            break;
        } else
            usleep(100000);
    }

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
    } if (strcmp(cmd, "battery") == 0) {
        return cmd_battery(&config);
    } if (strcmp(cmd, "show_task") == 0) {
        return cmd_show_task(&config);
    } if (strcmp(cmd, "list") == 0) {
        return cmd_list(&config);
    } if (strcmp(cmd, "raw") == 0) {
        return cmd_raw(&config, argc - 2, argv + 2);
    } else {
        arg_error("unknown command");
    }
}
