#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define GOBLIN_CSTR_IMPLEMENTATION
#include "../include/GOBLIN/cstr.h"

int main(void) {
    char *s = goblin_copy_cstr("hello");
    assert(s != NULL);
    assert(strcmp(s, "hello") == 0);
    free(s);

    assert(goblin_copy_cstr(NULL) == NULL);

    puts("cstr tests passed");
    return 0;
}