#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "command.h"

void cmd_floppy_load(int drive, char *filename)
{
    printf("LOADING drive %d filename %s\n", drive, filename);
}

void arc_reset() {
    printf("RESET\n");
}

char machine_config_file[256];

int main()
{
    char *lines[] = {
        "floppy load 1 test1.img",
        "reset",
        "config xyz",
        "*Testing star commands",
        NULL
    };
    for (int i = 0 ; lines[i]; i++) {
        command_parse_t pctx = {.in = fmemopen(lines[i], strlen(lines[i]), "r")};
        if (!pctx.in)
            abort();
        command_context_t *ctx = command_create(&pctx);
        int r;
        printf("result for '%s' is %d (%d)\n", lines[i], command_parse(ctx, &r), r);
        command_destroy(ctx);
    }
    return 0;
}
