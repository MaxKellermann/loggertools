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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#include "datadir.h"

struct datadir {
    char *path;
    size_t path_length;
    DIR *dir;
};

struct datadir *datadir_open(const char *path) {
    struct datadir *dir;

    dir = (struct datadir*)calloc(1, sizeof(*dir));
    if (dir == NULL)
        return NULL;

    dir->path = strdup(path);
    if (dir->path == NULL) {
        free(dir);
        return NULL;
    }

    dir->path_length = strlen(dir->path);
    dir->path[dir->path_length++] = '/';

    return dir;
}

void datadir_close(struct datadir *dir) {
    if (dir->path != NULL)
        free(dir->path);

    if (dir->dir != NULL)
        closedir(dir->dir);

    free(dir);
}

static char *make_path(struct datadir *dir,
                       const char *filename) {
    size_t filename_length = strlen(filename) + 1;
    char *p;

    p = (char*)malloc(dir->path_length + filename_length);
    if (p == NULL)
        return NULL;

    memcpy(p, dir->path, dir->path_length);
    memcpy(p + dir->path_length, filename, filename_length);

    return p;
}

void datadir_list_begin(struct datadir *dir) {
    if (dir->dir != NULL)
        closedir(dir->dir);

    dir->dir = opendir(dir->path);
}

const char *datadir_list_next(struct datadir *dir) {
    struct dirent *ent;

    if (dir->dir == NULL)
        return NULL;

    do {
        ent = readdir(dir->dir);
        if (ent == NULL) {
            closedir(dir->dir);
            dir->dir = NULL;
            return NULL;
        }
    } while (ent->d_name[0] == '.');

    return ent->d_name;
}

int datadir_stat(struct datadir *dir,
                 const char *filename,
                 struct stat *st) {
    char *path;
    int ret;

    path = make_path(dir, filename);
    if (path == NULL) {
        errno = ENOMEM;
        return -1;
    }

    ret = stat(path, st);
    free(path);
    return ret;
}

void *datadir_read(struct datadir *dir,
                   const char *filename,
                   size_t length) {
    char *path;
    int fd, ret;
    struct stat st;
    void *buffer;
    ssize_t nbytes;

    path = make_path(dir, filename);
    if (path == NULL)
        return NULL;

    ret = stat(path, &st);
    if (ret < 0 || st.st_size != (off_t)length) {
        free(path);
        return NULL;
    }

    fd = open(path, O_RDONLY);
    free(path);
    if (fd < 0)
        return NULL;

    buffer = malloc(length);
    if (buffer == NULL) {
        close(fd);
        return NULL;
    }

    nbytes = read(fd, buffer, length);
    close(fd);
    if (nbytes != (ssize_t)length) {
        free(buffer);
        return NULL;
    }

    return buffer;
}


int datadir_write(struct datadir *dir,
                  const char *filename,
                  void *data, size_t length) {
    char *path;
    int fd;
    ssize_t nbytes;

    path = make_path(dir, filename);
    if (path == NULL)
        return -1;

    fd = open(path, O_CREAT|O_TRUNC|O_WRONLY, 0666);
    free(path);
    if (fd < 0)
        return -1;

    nbytes = write(fd, data, length);
    if (nbytes < 0)
        return -1;

    if ((size_t)nbytes < length)
        return 0;

    close(fd);

    return 1;
}
