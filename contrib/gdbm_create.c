#include <stdio.h>
#include <gdbm.h>

int main(int argc, char *argv[]) {
    if (argc == 2) {
        gdbm_open (argv[1], 0, GDBM_NEWDB, 0666, 0);
    } else {
        fprintf(stderr, "%s << filename >>", argv[0]);
    }
}
