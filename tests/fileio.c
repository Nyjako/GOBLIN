#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#define GOBLIN_FILEIO_IMPLEMENTATION
#include "../include/GOBLIN/fileio.h"

int main(void) {
    FILE *f = tmpfile();
    assert(f != NULL);

    assert(goblin_fseek64(f, 123456789, SEEK_SET) == 0);

    int64_t pos = goblin_ftell64(f);
    assert(pos == 123456789);

    fclose(f);

    puts("fileio tests passed");
    return 0;
}