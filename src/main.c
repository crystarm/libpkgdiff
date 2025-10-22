#include <stdio.h>
#include <stdlib.h>
#include "pkgdiff.h"

int main() {
    hello_pkgdiff();

    const char *branch = "p11";
    char *json = fetch_packages_json(branch);
    if (json) {
        printf("\n=== JSON from branch '%s' ===\n", branch);
        json_parse_demo(json);
        free(json);
    } else {
        fprintf(stderr, "Failed to load JSON data from API\n");
    }

    return 0;
}
