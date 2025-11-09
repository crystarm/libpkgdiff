#include <stdio.h>
#include <string.h>
#include "pkgdiff.h"
#include "u.h"

#include <stdlib.h>

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
printf("\n" C_BOLD C_ACCENT "Select operation" C_RESET "\n");
    printf(C_BOLD C_ACCENT "  1) " C_RESET "Unique packages per branch (original diff)\n");
    printf(C_BOLD C_ACCENT "  2) " C_RESET "Common packages with versions (intersection)\n");
    printf(C_BOLD C_ACCENT "[1/2]: " C_RESET);
    fflush(stdout);
    char op[8] = {0};
    if (fgets(op, sizeof(op), stdin) && (op[0] == '2')) {
        printf("\n" C_BOLD C_ACCENT "Select operation" C_RESET "\n");
        printf(C_BOLD C_ACCENT "  1) " C_RESET "Show all common packages\n");
        printf(C_BOLD C_ACCENT "  2) " C_RESET "Show all packages with the same versions in both branches\n");
        printf(C_BOLD C_ACCENT "  3) " C_RESET "Show all packages with the different versions in branches\n");
        printf(C_BOLD C_ACCENT "[1/2/3]: " C_RESET);
        char sub[8] = {0}; fflush(stdout);
        int filter = PKGDIFF_FILTER_ALL;
        if (fgets(sub, sizeof(sub), stdin)) {
            if (sub[0] == '2') filter = PKGDIFF_FILTER_EQUAL;
            else if (sub[0] == '3') filter = PKGDIFF_FILTER_DIFFER;
        }
        pkgdiff_common_pkgs_with_versions_ex("p11", "sisyphus", arch, NULL, filter);
    } else {
        pkgdiff_unique_pkgs("p11", "sisyphus", arch);
    }
    return 0;
}
