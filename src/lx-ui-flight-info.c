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

void lxui_edit_flight_info(struct lxui *lxui) {
    newtComponent form;
    newtComponent pilot_entry, model_entry, registration_entry;
    newtComponent ok_button, cancel_button;
    struct newtExitStruct es;
    int should_exit = 0, should_refresh = 1;

    if (lxui_device_wait(lxui) <= 0)
        return;

    newtCenteredWindow(34, 10, "Flight info");
    form = newtForm(NULL, NULL, 0);
    newtFormSetTimer(form, 1000);
    newtFormAddComponents(form,
                          newtLabel(0, 0, "Pilot:"),
                          pilot_entry = newtEntry(15, 0, NULL, 18, NULL, 0),
                          newtLabel(0, 1, "Model:"),
                          model_entry = newtEntry(15, 1, NULL, 11, NULL, 0),
                          newtLabel(0, 2, "Registration:"),
                          registration_entry = newtEntry(15, 2, NULL, 7, NULL, 0),
                          newtLabel(0, 3, "Number:"),
                          newtLabel(0, 4, "Observer:"),
                          ok_button = newtCompactButton(0, 9, "OK"),
                          cancel_button = newtCompactButton(8, 9, "Cancel"),
                          NULL);

    filser_send_command(lxui->device, FILSER_READ_FLIGHT_INFO);
    lxui->status = LXUI_STATUS_READ_FLIGHT_INFO;

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
            if (should_refresh && lxui->flight_info_ok) {
                should_refresh = 0;
                newtEntrySet(pilot_entry, lxui->flight_info.pilot, 0);
                newtEntrySet(model_entry, lxui->flight_info.model, 0);
                newtEntrySet(registration_entry, lxui->flight_info.registration, 0);
            }
            break;

        case NEWT_EXIT_COMPONENT:
            if (es.u.co == ok_button)
                should_exit = 1;
            else if (es.u.co == cancel_button)
                should_exit = 1;
            break;

        default:
            break;
        }
    } while (!should_exit);

    newtFormDestroy(form);
    newtPopWindow();
}

