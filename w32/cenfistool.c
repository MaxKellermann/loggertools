/*
 * loggertools
 * Copyright (C) 2004 Max Kellermann (max@duempel.org)
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

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#include <serialio.h>
#include <cenfis.h>

#include "resource.h"
#include "cenfistool.h"

static HINSTANCE hInstance;
static char data_filename[4096];
static char port_filename[32];
static HANDLE comm_thread = NULL;
static DWORD comm_thread_id = 0;

static BOOL setup_comm(const char *filename) {
    int ret;
    struct serialio *serio;

    ret = serialio_open(filename, &serio);
    if (ret < 0)
        return FALSE;

    serialio_close(serio);

    return TRUE;
}

static BOOL detect_cenfis(const char *filename) {
    if (!setup_comm(filename))
        return FALSE;

    /* XXX: do magic here */

    return TRUE;
}

static BOOL find_cenfis(char *filename, size_t filename_max_len) {
    unsigned port;

    for (port = 1; port <= 4; port++) {
        _snprintf(filename, filename_max_len, "COM%u", port);

        if (detect_cenfis(filename))
            return TRUE;
    }

    return FALSE;
}

static void make_comm_list(HWND hCombo) {
    unsigned port;
    char filename[32];

    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);

    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)"Auto");

    for (port = 1; port <= 4; port++) {
        _snprintf(filename, sizeof(filename), "COM%u", port);

        if (setup_comm(filename))
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)filename);
    }

    SendMessage(hCombo, CB_SETCURSEL, 0, 0);
}

static BOOL drop_files(HWND hDlg, HDROP hDrop) {
    char filename[4096];
    UINT ret;

    ret = DragQueryFile(hDrop, 0, filename, sizeof(filename));
    if (ret == 0)
        return TRUE;

    SendMessage(GetDlgItem(hDlg, IDC_FILE), WM_SETTEXT, 0, (LPARAM)filename);

    return TRUE;
}

static BOOL autodetect_port(HWND hWnd, char *filename, size_t filename_max_len) {
    if (find_cenfis(filename, filename_max_len)) {
        return TRUE;
    } else {
        MessageBox(hWnd, "Es wurde kein Cenfis gefunden.",
                   "Kein Cenfis gefunden", MB_OK|MB_ICONERROR);
        return FALSE;
    }
}

static BOOL get_selected_port(HWND hWnd, HWND hCombo, char *filename, size_t filename_max_len) {
    LRESULT sel, ret;

    sel = SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    if (sel == CB_ERR || sel == 0)
        return autodetect_port(hWnd, filename, filename_max_len);

    ret = SendMessage(hCombo, CB_GETLBTEXTLEN, sel, 0);
    if (ret == CB_ERR || (size_t)ret >= filename_max_len)
        return autodetect_port(hWnd, filename, filename_max_len);

    ret = SendMessage(hCombo, CB_GETLBTEXT, sel, (LPARAM)(LPCSTR)filename);
    if (ret == CB_ERR)
        return FALSE;

    if (!detect_cenfis(filename)) {
        MessageBox(hWnd, "Es wurde kein Cenfis am angegebenen Port gefunden.",
                   "Kein Cenfis gefunden", MB_OK|MB_ICONERROR);
        return FALSE;
    }

    return TRUE;
}

static BOOL send_button_pressed(HWND hDlg) {
    LRESULT ret;

    ret = SendMessage(GetDlgItem(hDlg, IDC_FILE), WM_GETTEXT,
                      sizeof(data_filename), (LPARAM)data_filename);
    if (ret == 0 && 0) {
        MessageBox(hDlg, "Bitte waehlen Sie eine Datei aus, die in das Cenfis hochgeladen werden soll.",
                   "Keine Datei angegeben", MB_OK|MB_ICONWARNING);
        return FALSE;
    }

    if (!get_selected_port(hDlg, GetDlgItem(hDlg, IDC_PORT),
                           port_filename, sizeof(port_filename)))
        return FALSE;

    EndDialog(hDlg, TRUE);

    return TRUE;
}

static BOOL CALLBACK MainWinDlgProc(HWND hDlg, UINT uMsg,
                                    WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
        SendMessage(hDlg, WM_SETICON, (WPARAM)ICON_BIG,
                    (LPARAM)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CENFIS)));
        make_comm_list(GetDlgItem(hDlg, IDC_PORT));
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            return send_button_pressed(hDlg);
        case IDCANCEL:
        case IDABORT:
            EndDialog(hDlg, FALSE);
            return TRUE;
        }

    case WM_DROPFILES:
        return drop_files(hDlg, (HDROP)wParam);

    default:
        return FALSE;
    }
}

static DWORD WINAPI comm_thread_func(LPVOID ctx) {
    HWND hDlg = (HWND)ctx;
    HWND hText = GetDlgItem(hDlg, IDC_TEXT);
    HWND hProgress = GetDlgItem(hDlg, IDC_PROGRESS);
    cenfis_status_t status;
    struct cenfis *cenfis;
    struct timeval tv;
    FILE *file;
    char line[1024];
    size_t length;
    unsigned long file_size, file_done;

    SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"Opening Cenfis device...");

    status = cenfis_open(port_filename, &cenfis);
    if (cenfis_is_error(status)) {
        SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"Failed");
        return 1;
    }

    SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"Waiting for Cenfis device...");
    while (1) {
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        status = cenfis_select(cenfis, &tv);
        if (cenfis_is_error(status)) {
            SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"cenfis_select failed");
            cenfis_close(cenfis);
            return 1;
        }

        if (status == CENFIS_STATUS_DIALOG_CONFIRM) {
            SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"Sending 'Y' to confirm");

            status = cenfis_confirm(cenfis);
            if (cenfis_is_error(status)) {
                SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"cenfis_confirm failed");
                cenfis_close(cenfis);
                return 1;
            }
        } else if (status == CENFIS_STATUS_DIALOG_SELECT) {
            SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"Sending 'P' response");

            status = cenfis_dialog_respond(cenfis, 'P');
            if (cenfis_is_error(status)) {
                SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"cenfis_dialog_respond failed");
                cenfis_close(cenfis);
                return 1;
            }
        } else if (status == CENFIS_STATUS_DATA) {
            break;
        }
    }

    strcpy(data_filename, "c:\\CFP_V35B.BHF");

    file = fopen(data_filename, "r");
    if (file == NULL) {
        SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"Failed to open data file");
        cenfis_close(cenfis);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    file_done = 0;

    SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, file_size / 1024));

    while (fgets(line, sizeof(line) - 2, file) != NULL) {
        /* format the line */
        length = strlen(line);
        file_done += length;
        while (length > 0 && line[length - 1] >= 0 &&
               line[length - 1] <= ' ')
            length--;

        if (length == 0)
            continue;

        line[length++] = '\r';
        line[length++] = '\n';

        /* write line to device */
        SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"write");
        status = cenfis_write_data(cenfis, line, length);
        SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"write done");
        if (cenfis_is_error(status)) {
            SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"cenfis_write_data failed");
            cenfis_close(cenfis);
            return 1;
        }

        /* wait for ACK from device */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"select");
        status = cenfis_select(cenfis, &tv);
        SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"select done");
        if (cenfis_is_error(status)) {
            SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"cenfis_select failed");
            cenfis_close(cenfis);
            return 1;
        }

        if (status == CENFIS_STATUS_WAIT_ACK) {
            SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"no ack");
            cenfis_close(cenfis);
            return 1;
        } else if (status == CENFIS_STATUS_DATA) {
            SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"sending");
        } else {
            SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"wrong status");
            cenfis_close(cenfis);
            return 1;
        }

        SendMessage(hProgress, PBM_SETPOS, file_done / 1024, 0);
    }

    cenfis_close(cenfis);

    SendMessage(hProgress, PBM_SETPOS, file_size / 1024, 0);
    SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"Done");

    return 0;
}

static BOOL CALLBACK ProgressDlgProc(HWND hDlg, UINT uMsg,
                                     WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
        comm_thread = CreateThread(NULL, 0, comm_thread_func, (LPVOID)hDlg, 0, &comm_thread_id);
        if (comm_thread == NULL) {
            MessageBox(hDlg, "Failed to create thread",
                       "Error", MB_OK|MB_ICONERROR);
            EndDialog(hDlg, FALSE);
        }

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
        case IDABORT:
            EndDialog(hDlg, FALSE);
            return TRUE;
        }

    default:
        return FALSE;
    }
}

int WINAPI WinMain(HINSTANCE _hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    int ret;

    hInstance = _hInstance;

    register_cenfistool();

    ret = DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, MainWinDlgProc);
    if (!ret)
        return 0;

    DialogBox(hInstance, MAKEINTRESOURCE(IDD_PROGRESS), NULL, ProgressDlgProc);

    return 0;
}
