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
#include "lx-ui.h"

static FILE *dbg;

struct config {
    const char *tty;
};

static void usage(void) {
    printf("usage: filsertool [OPTIONS] COMMAND [ARGUMENTS]\n"
           "valid options:\n"
#ifdef __GLIBC__
           " --tty DEVICE\n"
#endif
           " -t DEVICE      open this tty device (default /dev/ttyS0)\n"
           "valid commands:\n"
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
           "  lxn2igc FILENAME.LXN\n"
           "        convert a .lxn file to .igc\n"
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

        ret = getopt_long(argc, argv, "hVt:",
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
            printf("filsertool v" VERSION
                   ", http://max.kellermann.name/projects/loggertools/\n");
            exit(0);

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
    fprintf(stderr, "filsertool: %s\n", msg);
    fprintf(stderr, "Try 'filsertool --help' for more information.\n");
    _exit(1);
}

static void alarm_handler(int dummy) {
    (void)dummy;
}

int main(int argc, char **argv) {
    static struct config config;
    static struct lxui filser;
    newtComponent form, newt_port;
    newtComponent setup_button, info_button, igc_button, tp_button, apt_button,
        reconnect_button, quit_button;
    struct newtExitStruct es;
    static int should_exit;

    dbg = fopen("/tmp/filser-ui.log", "w");

    signal(SIGALRM, alarm_handler);

    parse_cmdline(&config, argc, argv);

    if (optind != argc)
        arg_error("too many arguments");

    filser.fd = filser_open(config.tty);
    if (filser.fd < 0) {
        fprintf(stderr, "failed to open %s: %s\n",
                config.tty, strerror(errno));
        exit(2);
    }

    newtInit();

    newtCls();

    newtDrawRootText(1, -1, "loggertools (C) 2004-2006 Max Kellermann <max@duempel.org>");

    newtCenteredWindow(90, 14, "filserui");

    form = newtForm(NULL, NULL, 0);
    newtFormAddHotKey(form, NEWT_KEY_F10);
    newtFormSetTimer(form, 1000);
    newtFormAddComponents(form,
                          newtLabel(1, 0, "Serial port:"),
                          newt_port = newtLabel(24, 0, config.tty),
                          newtLabel(1, 1, "Status:"),
                          filser.newt_status = newtLabel(24, 1, "n/a"),
                          filser.newt_basic = newtTextbox(1, 2, 40, 10, 0),
                          setup_button = newtCompactButton(0, 13, "Setup"),
                          info_button = newtCompactButton(8, 13, "Flight info"),
                          igc_button = newtCompactButton(22, 13, "IGC flights"),
                          tp_button = newtCompactButton(36, 13, "Turn points"),
                          apt_button = newtCompactButton(50, 13, "Airports"),
                          reconnect_button = newtCompactButton(61, 13, "Reconnect"),
                          quit_button = newtCompactButton(73, 13, "Quit"),
                          NULL);

    do {
        newtFormRun(form, &es);
        switch (es.reason) {
        case NEWT_EXIT_HOTKEY:
            switch (es.u.key) {
            case NEWT_KEY_ESCAPE:
            case NEWT_KEY_F10:
            case NEWT_KEY_F12:
                should_exit = 1;
                break;
            }

        case NEWT_EXIT_TIMER:
            lxui_device_tick(&filser);
            break;

        case NEWT_EXIT_COMPONENT:
            if (es.u.co == setup_button)
                lxui_edit_setup(&filser);
            else if (es.u.co == info_button)
                lxui_edit_flight_info(&filser);
            else if (es.u.co == igc_button)
                lxui_download_igc_flights(&filser);
            else if (es.u.co == reconnect_button) {
                lxui_device_close(&filser);
                filser.fd = filser_open(config.tty);
            }  else if (es.u.co == quit_button)
                should_exit = 1;

        default:
            break;
        }
    } while (!should_exit);

    newtFormDestroy(form);
    newtPopWindow();
    newtFinished();

    return 0;
}
