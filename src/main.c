#include <stdio.h>
#include <string.h>
#include "pkgdiff.h"

#ifndef note
#define note(MSG) printf(C_GREEN "• %s" C_RESET "\n", (MSG))
#endif
#ifndef fail
#define fail(MSG) fprintf(stderr, C_RED "✖ %s" C_RESET "\n", (MSG))
#endif
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
    printf(C_GREEN "• %s" C_RESET "\n", "Fetching package lists...");
    char *__pre_j1 = fetch_packages_json("p11");
    printf("\n");
    char *__pre_j2 = fetch_packages_json("sisyphus");
    if (!__pre_j1 || !__pre_j2) { fprintf(stderr, C_RED "✖ %s" C_RESET "\n", "Failed to download package lists"); return 3; }
    free(__pre_j1); free(__pre_j2);
    printf("\n");
    printf("\n" C_BOLD C_ACCENT "Select operation" C_RESET "\n");
    printf(C_BOLD C_ACCENT "  1) " C_RESET "Unique packages per branch (original diff)\n");
    printf(C_BOLD C_ACCENT "  2) " C_RESET "Common packages with versions (intersection)\n");
    printf(C_BOLD C_ACCENT "[1/2]: " C_RESET);
    fflush(stdout);
    char op[8] = {0};
    if (fgets(op, sizeof(op), stdin) && (op[0] == '2')) {
        common_packages_interactive_min("p11", "sisyphus", arch, NULL);
    } else {
        compare_branches_interactive_min("p11", "sisyphus", arch);
    }
    return 0;
}
