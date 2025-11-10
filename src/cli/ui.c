#include <stdio.h>
#include "ui.h"
#include "ansi.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>
#include <jansson.h>
#include "pkgdiff.h"
#include "util.h"

#ifndef BUILDING_PKGDIFF
#define BUILDING_PKGDIFF 1
#endif



static FILE *open_art_file(void) {
    const char *env = getenv("PKGDIFF_ART");
    if (env && *env) {
        FILE *f = fopen(env, "rb");
        if (f) return f;
    }
    const char *candidates[] = {
        "picture.txt",
        "/usr/share/libpkgdiff/picture.txt",
        "/usr/local/share/libpkgdiff/picture.txt",
        NULL
    };
    for (int i = 0; candidates[i]; ++i) {
        FILE *f = fopen(candidates[i], "rb");
        if (f) return f;
    }
    return NULL;
}



static void wait_and_show_braille(void) {
    printf(C_BOLD C_ACCENT "\nlibpkgdiff — ALT Package Comparator (interactive mode)" C_RESET "\n");
    printf("\nPlease maximize your terminal window, then press Enter to continue...");
    fflush(stdout);
    char buf[8];
    fgets(buf, sizeof(buf), stdin);
    FILE *pf = open_art_file();
    if (!pf) {
        printf("\n" C_YELLOW "(Art file not found. Set PKGDIFF_ART=/path/to/picture.txt to override.)" C_RESET "\n\n");
        return;
    }
    printf("\n");
    char chunk[4096];
    size_t n;
    while ((n = fread(chunk, 1, sizeof(chunk), pf)) > 0) {
        fwrite(chunk, 1, n, stdout);
    }
    fclose(pf);
    printf("\n\n");
    fflush(stdout);
}



 void hello_pkgdiff(void) { wait_and_show_braille(); }