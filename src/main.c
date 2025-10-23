#include <stdio.h>
#include <string.h>
#include "pkgdiff.h"

static void usage(const char *prog) {
    printf("Usage: %s [--arch <name>]\n", prog);
    printf("Examples:\n");
    printf("  %s --arch x86_64\n", prog);
    printf("  %s\n", prog);
}

int main(int argc, char **argv) {
    const char *arch = NULL;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--arch") == 0) {
            if (i + 1 < argc) {
                arch = argv[++i];
            } else {
                fprintf(stderr, "Missing value for --arch\n");
                return 2;
            }
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            usage(argv[0]);
            return 2;
        }
    }
    hello_pkgdiff();
    compare_branches_interactive_min("p11", "sisyphus", arch);
    return 0;
}
