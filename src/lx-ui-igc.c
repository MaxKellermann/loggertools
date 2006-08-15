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

#include "lx-ui.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void insert_flight_index_list_entry(newtComponent list_box,
                                           const struct filser_flight_index *src) {
    struct filser_flight_index *dst;
    char text[256];

    dst = (struct filser_flight_index*)malloc(sizeof(*dst));
    if (dst == NULL)
        return;

    memcpy(dst, src, sizeof(*dst));

    snprintf(text, sizeof(text), "%8s %8s-%8s %s",
             src->date,
             src->start_time,
             src->stop_time,
             src->pilot);
    newtListboxInsertEntry(list_box, text, dst, NULL);
}

static void free_flight_index_list_box(newtComponent list_box) {
    int i;
    char *text;
    void *data;

    for (i = newtListboxItemCount(list_box) - 1; i >= 0; --i) {
        newtListboxGetEntry(list_box, i, &text, &data);
        if (data != NULL)
            free(data);
    }
}

static int download_igc_flight_list(struct lxui *lxui,
                                    newtComponent list_box) {
    newtComponent form;
    struct newtExitStruct es;
    int should_exit = 0, cancel = 0;

    if (lxui_device_wait(lxui) <= 0)
        return -1;

    filser_send_command(lxui->fd, FILSER_READ_FLIGHT_LIST);
    lxui->status = LXUI_STATUS_READ_FLIGHT_LIST;

    newtCenteredWindow(35, 2, "Downloading");
    form = newtForm(NULL, NULL, 0);
    newtFormSetTimer(form, 100);
    newtFormAddComponents(form,
                          newtLabel(1, 0, "Downloading flight list..."),
                          newtCompactButton(1, 1, "Cancel"),
                          NULL);

    do {
        newtFormRun(form, &es);

        switch (es.reason) {
        case NEWT_EXIT_HOTKEY:
        case NEWT_EXIT_COMPONENT:
            should_exit = 1;
            cancel = 1;
            break;

        case NEWT_EXIT_TIMER:
            lxui_device_tick(lxui);
            if (lxui->status != LXUI_STATUS_READ_FLIGHT_LIST)
                should_exit = 1;
            else if (lxui->flight_index_ok)
                insert_flight_index_list_entry(list_box,
                                               &lxui->flight_index);
            break;

        default:
            break;
        }
    } while (!should_exit && lxui->status != LXUI_STATUS_IDLE);

    newtFormDestroy(form);
    newtPopWindow();
    return cancel
        ? 0 : 1;
}

void lxui_download_igc_flights(struct lxui *lxui) {
    newtComponent form, list_box, download_button, close_button;
    struct newtExitStruct es;
    int should_exit = 0;

    list_box = newtListbox(0, 0, 19,
                           NEWT_FLAG_SCROLL|NEWT_FLAG_RETURNEXIT|
                           NEWT_FLAG_MULTIPLE);

    if (download_igc_flight_list(lxui, list_box) <= 0) {
        free_flight_index_list_box(list_box);
        return;
    }

    newtCenteredWindow(60, 20, "Flight list");
    form = newtForm(NULL, NULL, 0);
    newtFormSetTimer(form, 1000);
    newtFormAddComponents(form,
                          list_box,
                          download_button = newtCompactButton(0, 19, "Download"),
                          close_button = newtCompactButton(12, 19, "Close"),
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
            lxui_device_tick(lxui);
            break;

        case NEWT_EXIT_COMPONENT:
            if (es.u.co == download_button)
                should_exit = 1;
            else if (es.u.co == close_button)
                should_exit = 1;
            break;

        default:
            break;
        }
    } while (!should_exit);

    free_flight_index_list_box(list_box);

    newtFormDestroy(form);
    newtPopWindow();
}
