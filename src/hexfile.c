#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    const int fd = 0;
    FILE *const file = stdout;
    unsigned offset = 0;
    char buffer[16];
    ssize_t nbytes;
    size_t z;
    int checksum;

    (void)argc;
    (void)argv;

    while ((nbytes = read(fd, buffer, sizeof(buffer))) > 0) {
        memset(buffer + nbytes, 0, sizeof(buffer) - nbytes);

        checksum = 0;

        fprintf(file, ":10%04X00", offset & 0xffff);
        checksum -= 0x10 + (offset >> 8) + offset;

        for (z = 0; z < sizeof(buffer); z++) {
            fprintf(file, "%02X", buffer[z] & 0xff);
            checksum -= buffer[z];
        }
        fprintf(file, "%02X", checksum & 0xff);
        fputs("\n", file);

        offset += 0x10;
    }

    fputs(":00000001FF\n", file);

    return 0;
}
