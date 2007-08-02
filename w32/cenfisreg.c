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

#include <windows.h>

#include "cenfistool.h"

static const char className[] = "CenfisHexfile";

/** extract the program path from the command line */
static void get_program_path(char *path, size_t path_max_len,
                             size_t *path_len) {
    const char *cmdline, *p;
    size_t len;

    cmdline = GetCommandLine();

    if (cmdline[0] == '"') {
        p = strchr(cmdline + 1, '"');
        if (p != NULL)
            p++;
    } else {
        p = strchr(cmdline, ' ');
    }

    len = p == NULL ? strlen(cmdline) : p - cmdline;

    if (len + 16 > path_max_len)
        len = path_max_len - 16;

    memcpy(path, cmdline, len);

    *path_len = len;
}

/** register file types etc. */
void register_cenfistool(void) {
    LONG ret;
    HKEY key, key2;
    char path[MAX_PATH];
    size_t path_len;

    get_program_path(path, sizeof(path), &path_len);

    /* register the .bhf extension */
    ret = RegCreateKeyEx(HKEY_CLASSES_ROOT, ".bhf", 0, NULL,
                         REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL);
    if (ret == ERROR_SUCCESS) {
        RegSetValueEx(key, NULL, 0, REG_SZ, className, sizeof(className));
        RegCloseKey(key);
    }

    /* .. and the file type */
    ret = RegCreateKeyEx(HKEY_CLASSES_ROOT, className, 0, NULL,
                         REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL);
    if (ret == ERROR_SUCCESS) {
        ret = RegCreateKeyEx(key, "shell\\open\\command", 0, NULL,
                             REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key2, NULL);
        if (ret == ERROR_SUCCESS) {
            strcpy(path + path_len, " \"%1\"");
            RegSetValueEx(key2, NULL, 0, REG_SZ, path, strlen(path));
            RegCloseKey(key2);
        }

        ret = RegCreateKeyEx(key, "DefaultIcon", 0, NULL,
                             REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key2, NULL);
        if (ret == ERROR_SUCCESS) {
            strcpy(path + path_len, ",0");
            RegSetValueEx(key2, NULL, 0, REG_SZ, path, strlen(path));
            RegCloseKey(key2);
        }

        RegCloseKey(key);
    }
}
