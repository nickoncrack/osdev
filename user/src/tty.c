#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/exec.h>

__attribute__((section(".text._start")))
#define _cleanup_args for (int i = 0; i < argc; i++) free(argv[i]); free(argv);

void _start() {
    // listdir once to avoid multiple calls
    // to refresh new binaries a terminal restart is needed
    node_t *n;
    uint32_t count = listdir("/bin", &n);

    fprintf(stdout, ">> ");

    while (1) {
        char in[64] = {0};
        int argc = 0;
        char *argv[16] = {0};

        scanf(in, sizeof(in));

        if (strlen(in) == 0) {
            fprintf(stdout, ">> ");
            continue;
        }

        argc = split_string(in, ' ', argv, 16);

        if (strcmp(argv[0], "exit")) {
            break;
        } else if (strcmp(argv[0], "clear")) {
            puts(STDOUT_CLEAR);
        } else {
            int found = 0;

            for (int i = 0; i < count; i++) {
                if (strcmp(argv[0], n[i].name)) {
                    found = 1;
                    break;
                }
            }

            if (found) {
                char path[256] = {0};

                sprintf(path, "/bin/%s", argv[0]);
                int ret = exec(path);
            }

            if (!found) {
                fprintf(stdout, "Command not found: %s\n", argv[0]);
            }
        }

        fprintf(stdout, ">> ");
    }

    exit(0);
}