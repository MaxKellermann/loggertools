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

#include <assert.h>
#include <windows.h>

#include <serialio.h>

struct serialio {
    HANDLE hCom;
};

int serialio_open(const char *device,
                  struct serialio **serialiop) {
    DCB dcb;
    HANDLE hCom;
    BOOL fSuccess;
    struct serialio *serio;

    hCom = CreateFile(device, GENERIC_READ | GENERIC_WRITE,
                      0, NULL, OPEN_EXISTING, 0, NULL);
    if (hCom == INVALID_HANDLE_VALUE)
        return -1;

    fSuccess = GetCommState(hCom, &dcb);
    if (!fSuccess) {
        CloseHandle(hCom);
        return -1;
    }

    dcb.BaudRate = CBR_57600;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fErrorChar = FALSE;
    dcb.fNull = FALSE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fAbortOnError = TRUE;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    fSuccess = SetCommState(hCom, &dcb);
    if (!fSuccess) {
        CloseHandle(hCom);
        return -1;
    }

    serio = calloc(1, sizeof(*serio));
    if (serio == NULL)
        return -1;

    serio->hCom = hCom;

    *serialiop = serio;
    return 0;
}

void serialio_close(struct serialio *serio) {
    assert(serio != NULL);

    if (serio->hCom != NULL)
        CloseHandle(serio->hCom);

    free(serio);
}

void serialio_flush(struct serialio *serio, unsigned options) {
    DWORD flags = 0;

    assert(serio != NULL);
    assert(serio->hCom != NULL);

    if (options & SERIALIO_FLUSH_INPUT)
        flags |= PURGE_RXABORT | PURGE_RXCLEAR;
    if (options & SERIALIO_FLUSH_OUTPUT)
        flags |= PURGE_TXABORT | PURGE_TXCLEAR;

    PurgeComm(serio->hCom, flags);
}

int serialio_select(struct serialio *serio, int options,
                    struct timeval *timeout) {
    BOOL ret;
    COMSTAT comstat;
    DWORD dwErrorFlags;
    unsigned long sleeps;
    int result;

    if (1) /* XXX implement */
        return options;

    assert(serio != NULL);
    assert(serio->hCom != NULL);

    sleeps = timeout == NULL
        ? 0xffffffff
        : ( timeout->tv_sec * 100 + timeout->tv_usec / 1000000);

    for (;;) {
        ret = ClearCommError(serio->hCom, &dwErrorFlags, &comstat);
        if (ret < 0)
            return 0;

        result = options;

        if (comstat.cbInQue == 0)
            result &= ~SERIALIO_SELECT_AVAILABLE;
        /*if (comstat.cbOutQue > 0)
          result &= ~SERIALIO_SELECT_READY;*/

        if (result != 0)
            return result;

        if (timeout->tv_usec < 100000 && timeout->tv_sec == 0)
            return 0;

        Sleep(10);

        if (timeout->tv_usec < 1000000) {
            if (timeout->tv_sec == 0)
                return 0;
            timeout->tv_sec--;
            timeout->tv_usec = 9000000;
        }
    }
}

int serialio_read(struct serialio *serio, void *data, size_t *nbytes) {
    BOOL ret;
    DWORD result;

    assert(serio != NULL);
    assert(serio->hCom != NULL);

    ret = ReadFile(serio->hCom, data, *nbytes, &result, NULL);
    if (!ret)
        return -1;

    *nbytes = result;

    return 0;
}

int serialio_write(struct serialio *serio, const void *data, size_t *nbytes) {
    BOOL ret;
    DWORD result;

    assert(serio != NULL);
    assert(serio->hCom != NULL);

    ret = WriteFile(serio->hCom, data, *nbytes, &result, NULL);
    if (!ret)
        return -1;

    *nbytes = result;

    return 0;
}
