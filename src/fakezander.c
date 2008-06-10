/*
 * loggertools
 * Copyright (C) 2004-2008 Max Kellermann <max@duempel.org>
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

#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <termios.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <sys/select.h>
#ifdef __GLIBC__
#include <getopt.h>
#endif

#include "version.h"
#include "zander-internal.h"
#include "datadir.h"
#include "dump.h"

struct config {
    int verbose;
    const char *datadir, *tty;
};

struct fake_zander {
    zander_t device;
    struct datadir *datadir;
};


static int should_exit = 0;

static void exit_handler(int dummy) {
    (void)dummy;
    should_exit = 1;
}

static void alarm_handler(int dummy) {
    (void)dummy;
}

static void usage(void) {
    puts("usage: fakezander [options]\n\n"
         "valid options:\n"
#ifdef __GLIBC__
         " --help\n"
#endif
         " -h             help (this text)\n"
#ifdef __GLIBC__
         " --version\n"
#endif
         " -V             show fakezander version\n"
#ifdef __GLIBC__
         " --verbose\n"
#endif
         " -v             be more verbose\n"
#ifdef __GLIBC__
         " --quiet\n"
#endif
         " -q             be quiet\n"
#ifdef __GLIBC__
         " --data DIR\n"
#endif
         " -d DIR         specify the data directory\n"
#ifdef __GLIBC__
         " --tty DEVICE\n"
#endif
         " -t DEVICE      open this tty device (default /dev/ttyS0)\n"
#ifdef __GLIBC__
         " --virtual\n"
#endif
         " -u             create a virtual terminal\n"
         "\n"
         );
}

static void arg_error(const char *msg) __attribute__ ((noreturn));
static void arg_error(const char *msg) {
    fprintf(stderr, "fakezander: %s\n", msg);
    fprintf(stderr, "Try 'fakezander -h' for more information.\n");
    _exit(1);
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
        {"data", 1, 0, 'd'},
        {"tty", 1, 0, 't'},
        {"virtual", 0, 0, 'u'},
        {0,0,0,0}
    };
#endif

    memset(config, 0, sizeof(*config));
    config->tty = "/dev/ttyS0";

    while (1) {
#ifdef __GLIBC__
        int option_index = 0;

        ret = getopt_long(argc, argv, "hVvqd:t:u",
                          long_options, &option_index);
#else
        ret = getopt(argc, argv, "hVvqd:t:u");
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

        case 'd':
            config->datadir = optarg;
            break;

        case 't':
            config->tty = optarg;
            break;

        case 'u':
            config->tty = NULL;
            break;

        default:
            exit(1);
        }
    }

    /* check non-option arguments */

    if (optind < argc)
        arg_error("Too many arguments");

    if (config->datadir == NULL)
        arg_error("Data directory not specified");
}

static void download_to_file(struct fake_zander *zander,
                             const char *filename,
                             size_t length, bool pad) {
    void *buffer;
    char *p, *end;
    int ret;
    ssize_t nbytes;

    p = buffer = malloc(length);
    if (buffer == NULL) {
        fprintf(stderr, "out of memory\n");
        abort();
    }

    end = p + length;

    do {
        nbytes = read(zander->device->fd, p, end - p);
        if (nbytes < 0) {
            perror("failed to read");
            exit(2);
        }

        p += nbytes;

        if (pad) {
            memset(p, 0, (const char*)buffer + length - p);
            break;
        }
    } while (p < end);

    ret = datadir_write(zander->datadir, filename,
                        buffer, length);
    if (ret < 0) {
        fprintf(stderr, "failed to write %s: %s\n",
                filename, strerror(errno));
        exit(2);
    }

    if (ret == 0) {
        fprintf(stderr, "short write to %s\n",
                filename);
        exit(2);
    }

    free(buffer);
}

static void upload_from_file(struct fake_zander *zander,
                             const char *filename,
                             size_t length) {
    void *buffer;

    buffer = datadir_read(zander->datadir, filename, length);
    if (buffer == NULL) {
        fprintf(stderr, "failed to read %s\n", filename);
        exit(2);
    }

    write(zander->device->fd, buffer, length);

    free(buffer);
}

static void handle_read_serial(struct fake_zander *zander) {
    static const struct zander_serial serial = {
        .serial = "543",
        .foo = "GP",
        .seal = '1',
        .version = "V1.05",
        .ram_type = '0',
    };

    write(zander->device->fd, &serial, sizeof(serial));
}

static void handle_read_li_battery(struct fake_zander *zander) {
    const struct zander_battery battery = {
        .voltage = htons(7000),
    };

    write(zander->device->fd, &battery, sizeof(battery));
}

static void handle_write_personal_data(struct fake_zander *zander) {
    download_to_file(zander, "personal_data",
                     sizeof(struct zander_personal_data), false);
}

static void handle_read_personal_data(struct fake_zander *zander) {
    upload_from_file(zander, "personal_data",
                     sizeof(struct zander_personal_data));
}

static void handle_write_task(struct fake_zander *zander) {
    download_to_file(zander, "task",
                     sizeof(struct zander_write_task), true);
}

static void handle_read_task(struct fake_zander *zander) {
    static const struct zander_read_task task = {
        .upload_date = {
            .day = 10,
            .month = 6,
            .year = 8,
        },
        .upload_time = {
            .hour = 18,
            .minute = 33,
            .second = 11,
        },
    };

    write(zander->device->fd, &task, sizeof(task) - sizeof(task.waypoints));

    upload_from_file(zander, "task",
                     sizeof(struct zander_write_task));
}

static int open_virtual(const char *symlink_path) {
    int fd, ret;

    fd = open("/dev/ptmx", O_RDWR|O_NOCTTY);
    if (fd < 0) {
        fprintf(stderr, "failed to open /dev/ptmx: %s\n",
                strerror(errno));
        _exit(1);
    }

    ret = unlockpt(fd);
    if (ret < 0) {
        fprintf(stderr, "failed to unlockpt(): %s\n",
                strerror(errno));
        _exit(1);
    }

    printf("Slave terminal is %s\n", ptsname(fd));

    unlink(symlink_path);
    ret = symlink(ptsname(fd), symlink_path);
    if (ret < 0) {
        fprintf(stderr, "symlink() failed: %s\n",
                strerror(errno));
        _exit(1);
    }

    printf("Symlinked to %s\n", symlink_path);

    return fd;
}

static void open_tty(struct config *config, zander_t *device_r) {
    int fd, ret;

    if (config->tty == NULL) {
        fd = open_virtual("/tmp/fakezander");
        if (fd < 0) {
            fprintf(stderr, "failed to open tty\n");
            exit(2);
        }

        ret = zander_fdopen(fd, device_r);
        if (ret != 0) {
            fprintf(stderr, "zander_fdopen() failed\n");
            exit(2);
        }
    } else {
        ret = zander_open(config->tty, device_r);
        if (ret != 0) {
            fprintf(stderr, "failed to open tty\n");
            exit(2);
        }
    }
}

int main(int argc, char **argv) {
    struct config config;
    struct fake_zander zander;

    parse_cmdline(&config, argc, argv);

    fputs("loggertools v" VERSION " (C) 2004-2008 Max Kellermann <max@duempel.org>\n"
          "http://max.kellermann.name/projects/loggertools/\n\n", stderr);

    signal(SIGINT, exit_handler);
    signal(SIGTERM, exit_handler);
    signal(SIGQUIT, exit_handler);
    signal(SIGALRM, alarm_handler);

    open_tty(&config, &zander.device);

    zander.datadir = datadir_open(config.datadir);
    if (zander.datadir == NULL) {
        fprintf(stderr, "failed to open datadir %s: %s\n",
                argv[1], strerror(errno));
        exit(2);
    }

    while (!should_exit) {
        unsigned char cmd;
        int ret;

        ret = read(zander.device->fd, &cmd, sizeof(cmd));
        if (ret < 0) {
            if (errno == EIO) {
                zander_close(&zander.device);
                open_tty(&config, &zander.device);
                continue;
            }

            if (errno == EINTR)
                continue;

            fprintf(stderr, "read failed: %s\n", strerror(errno));
            _exit(1);
        }

        if (ret == 0)
            continue;

        switch (cmd) {
        case ZANDER_CMD_READ_SERIAL:
            printf("received READ_SERIAL\n");
            handle_read_serial(&zander);
            break;

        case ZANDER_CMD_READ_LI_BATTERY:
            printf("received READ_LI_BATTERY\n");
            handle_read_li_battery(&zander);
            break;

        case ZANDER_CMD_WRITE_PERSONAL_DATA:
            printf("received WRITE_PERSONAL_DATA\n");
            handle_write_personal_data(&zander);
            break;

        case ZANDER_CMD_READ_PERSONAL_DATA:
            printf("received READ_PERSONAL_DATA\n");
            handle_read_personal_data(&zander);
            break;

        case ZANDER_CMD_WRITE_TASK:
            printf("received WRITE_TASK\n");
            handle_write_task(&zander);
            break;

        case ZANDER_CMD_READ_TASK:
            printf("received READ_TASK\n");
            handle_read_task(&zander);
            break;

        default:
            fprintf(stderr, "unknown command 0x%02x received\n", cmd);
            _exit(1);
        }
    }

    datadir_close(zander.datadir);
    zander_close(&zander.device);

    return 0;
}
