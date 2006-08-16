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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <netinet/in.h>

static int send_seek_mem(int fd, const struct filser_flight_index *flight) {
    struct filser_packet_def_mem packet;
    int ret;

    /* ignore highest byte here, the same as in kflog */

    /* start address */
    packet.start_address[0] = flight->start_address0;
    packet.start_address[1] = flight->start_address1;
    packet.start_address[2] = flight->start_address2;

    /* end address */
    packet.end_address[0] = flight->end_address0;
    packet.end_address[1] = flight->end_address1;
    packet.end_address[2] = flight->end_address2;

    tcflush(fd, TCIOFLUSH);

    ret = filser_write_packet(fd, FILSER_DEF_MEM,
                              &packet, sizeof(packet));
    if (ret <= 0)
        return -1;

    return 0;
}

static void allocate_logger_data(struct lxui *lxui,
                                 size_t length) {
    assert(length > 0);

    if (lxui->logger_data != NULL) {
        if (lxui->logger_data_length >= length) {
            lxui->logger_data_length = length;
            return;
        }

        free(lxui->logger_data);
    }

    lxui->logger_data_length = length;
    lxui->logger_data = (unsigned char*)malloc(length);
    if (lxui->logger_data == NULL)
        abort();
}

static int download_igc_flight(struct lxui *lxui,
                               const struct filser_flight_index *flight) {
    newtComponent form;
    struct newtExitStruct es;
    enum lxui_status prev_status;
    int should_exit = 0, cancel = 0;
    int ret, i;
    size_t section_lengths[0x10], overall_length = 0;
    FILE *lxn;

    if (lxui_device_wait(lxui) <= 0)
        return -1;

    lxn = fopen("/tmp/foo.lxn", "w");
    if (lxn == NULL)
        return -1;

    lxui->mem_section_ok = 0;

    send_seek_mem(lxui->fd, flight);
    prev_status = lxui->status = LXUI_STATUS_DEF_MEM;

    newtCenteredWindow(35, 2, "Downloading");
    form = newtForm(NULL, NULL, 0);
    newtFormSetTimer(form, 10);
    newtFormAddComponents(form,
                          newtLabel(1, 0, "Downloading flight..."),
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
            if (lxui->status == LXUI_STATUS_IDLE) {
                switch (prev_status) {
                case LXUI_STATUS_DEF_MEM:
                    ret = filser_send_command(lxui->fd, FILSER_GET_MEM_SECTION);
                    if (ret > 0)
                        prev_status = lxui->status = LXUI_STATUS_GET_MEM_SECTION;
                    break;

                case LXUI_STATUS_GET_MEM_SECTION:
                    if (lxui->mem_section_ok) {
                        overall_length = 0;
                        for (i = 0; i < 0x10; ++i) {
                            section_lengths[i] = ntohs(lxui->mem_section.section_lengths[i]);
                            overall_length += section_lengths[i];
                        }

                        lxui->current_mem_section = 0;
                        if (section_lengths[lxui->current_mem_section] > 0) {
                            allocate_logger_data(lxui, section_lengths[lxui->current_mem_section]);
                            filser_send_command(lxui->fd, FILSER_READ_LOGGER_DATA + lxui->current_mem_section);
                            prev_status = lxui->status = LXUI_STATUS_READ_LOGGER_DATA;
                        } else {
                            should_exit = 1;
                        }
                    } else {
                        filser_send_command(lxui->fd, FILSER_GET_MEM_SECTION);
                        lxui->status = LXUI_STATUS_GET_MEM_SECTION;
                    }
                    break;

                case LXUI_STATUS_READ_LOGGER_DATA:
                    if (lxui->logger_data_ok) {
                        fwrite(lxui->logger_data, lxui->logger_data_length, 1, lxn);
                        fflush(lxn);
                        ++lxui->current_mem_section;
                        if (lxui->current_mem_section < 0x10 &&
                            section_lengths[lxui->current_mem_section] > 0) {
                            allocate_logger_data(lxui, section_lengths[lxui->current_mem_section]);
                            filser_send_command(lxui->fd, FILSER_READ_LOGGER_DATA + lxui->current_mem_section);
                            prev_status = lxui->status = LXUI_STATUS_READ_LOGGER_DATA;
                        } else {
                            should_exit = 1;
                        }
                    } else {
                        filser_send_command(lxui->fd, FILSER_READ_LOGGER_DATA + lxui->current_mem_section);
                        lxui->status = LXUI_STATUS_READ_LOGGER_DATA;
                    }
                    break;

                default:
                    should_exit = 1;
                }
            }
            break;

        default:
            break;
        }
    } while (!should_exit);

    if (lxui->logger_data != NULL) {
        free(lxui->logger_data);
        lxui->logger_data = NULL;
    }

    fclose(lxn);

    newtFormDestroy(form);
    newtPopWindow();
    return cancel
        ? 0 : 1;
}

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
            if (es.u.co == download_button) {
                const struct filser_flight_index *flight
                    = (const struct filser_flight_index*)newtListboxGetCurrent(list_box);
                if (flight != NULL)
                    download_igc_flight(lxui, flight);
                should_exit = 1;
            } else if (es.u.co == close_button)
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
