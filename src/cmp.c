#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>
#include <jansson.h>
#include "pkgdiff.h"
#include "u.h"



void pkgdiff_unique_pkgs(const char *branch1, const char *branch2, const char *arch_filter) {
    if (arch_filter && *arch_filter) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Analyzing only architecture: %s", arch_filter);
        note(buf);
    }
    printf("\n");
    note("Fetching package lists...");
    printf("\n");
    char *json1 = fetch_packages_json(branch1);
    printf("\n");
    char *json2 = fetch_packages_json(branch2);
    if (!json1 || !json2) {
        fail("Failed to fetch JSON data for one or both branches");
        free(json1); free(json2);
        return;
    }
    printf("\n");
    note("Parsing JSON...");
    json_error_t err1, err2;
    json_t *root1 = json_loads(json1, 0, &err1);
    json_t *root2 = json_loads(json2, 0, &err2);
    free(json1); free(json2);
    if (!root1 || !root2) {

        fail("JSON parse error");
        if (root1) json_decref(root1);
        if (root2) json_decref(root2);
        return;
    }
    ok("JSON parsed");
    json_t *packages1 = json_object_get(root1, "packages");
    json_t *packages2 = json_object_get(root2, "packages");
    if (!json_is_array(packages1) || !json_is_array(packages2)) {
        fail("Unexpected JSON schema: 'packages' arrays not found");
        json_decref(root1); json_decref(root2);
        return;
    }
    log_indexing_start();
    json_t *map1 = json_object();
    json_t *map2 = json_object();
size_t len1 = json_array_size(packages1), filt1 = 0;for (size_t i = 0; i < len1; ++i) {
        json_t *pkg = json_array_get(packages1, i);
        if (!json_is_object(pkg)) continue;
        const char *name = json_string_value(json_object_get(pkg, "name"));
        const char *arch = json_string_value(json_object_get(pkg, "arch"));
        if (!name || !arch) continue;
        if (!arch_matches(arch_filter, arch)) continue;
        char key[512];
        snprintf(key, sizeof(key), "%s|%s", name, arch);
        json_object_set(map1, key, pkg);
        filt1++;
    }
    size_t len2 = json_array_size(packages2), filt2 = 0;for (size_t i = 0; i < len2; ++i) {
        json_t *pkg = json_array_get(packages2, i);
        if (!json_is_object(pkg)) continue;
        const char *name = json_string_value(json_object_get(pkg, "name"));
        const char *arch = json_string_value(json_object_get(pkg, "arch"));
        if (!name || !arch) continue;
        if (!arch_matches(arch_filter, arch)) continue;
        char key[512];
        snprintf(key, sizeof(key), "%s|%s", name, arch);
        json_object_set(map2, key, pkg);
        filt2++;
    }
    
    
    log_indexing_done();
    printf("\n");
    log_fetched_count("branch1", len1, branch1);
    log_fetched_count("branch2", len2, branch2);
    printf("\n");
    size_t only1_count = 0, only2_count = 0;
    json_t *only1_preview = json_array();
    json_t *only2_preview = json_array();
    const size_t PREVIEW_LIMIT = 10;
    note("Computing differences...");
    void *iter = json_object_iter(map1);
    while (iter) {
        const char *key = json_object_iter_key(iter);
        json_t *pkg1 = json_object_iter_value(iter);
        if (!json_object_get(map2, key)) {
            only1_count++;
            append_preview(only1_preview, pkg1, PREVIEW_LIMIT);
        }
        iter = json_object_iter_next(map1, iter);
    }
    iter = json_object_iter(map2);
    while (iter) {
        const char *key = json_object_iter_key(iter);
        if (!json_object_get(map1, key)) {
            only2_count++;
            append_preview(only2_preview, json_object_iter_value(iter), PREVIEW_LIMIT);
        }
        iter = json_object_iter_next(map2, iter);
    }
    ok("Differences computed");
    printf("\n" C_ACCENT "Show 10-item previews of unique packages?\n" C_RESET);
    printf("  • Only in %s (not in %s): %zu\n", branch1, branch2, only1_count);
    printf("  • Only in %s (not in %s): %zu\n", branch2, branch1, only2_count);
    printf("[y/N]: ");char buf[16] = {0};
    if (fgets(buf, sizeof(buf), stdin) && (buf[0] == 'y' || buf[0] == 'Y')) {
        printf("\n" C_ACCENT "Preview: only in %s (first %d)\n" C_RESET, branch1, (int)PREVIEW_LIMIT);
        int w1N=0,w1A=0,w1V=0; compute_preview_widths(only1_preview,&w1N,&w1A,&w1V);
        for (size_t i = 0; i < json_array_size(only1_preview); ++i) {
            print_pkg_line_aligned(json_array_get(only1_preview, i), w1N, w1A, w1V);
        }
        printf("\n" C_ACCENT "Preview: only in %s (first %d)\n" C_RESET, branch2, (int)PREVIEW_LIMIT);
        int w2N=0,w2A=0,w2V=0; compute_preview_widths(only2_preview,&w2N,&w2A,&w2V);
        for (size_t i = 0; i < json_array_size(only2_preview); ++i) {
            print_pkg_line_aligned(json_array_get(only2_preview, i), w2N, w2A, w2V);
        }
    } else {
        printf("Skipping previews.\n");
    }memset(buf, 0, sizeof(buf));
    printf("\nSave to JSON files? [y/N]: ");if (fgets(buf, sizeof(buf), stdin) && (buf[0] == 'y' || buf[0] == 'Y')) {
        char file1[256], file2[256];
        snprintf(file1, sizeof(file1), "%s_only.json", branch1);
        snprintf(file2, sizeof(file2), "%s_only.json", branch2);

        char line[512] = {0};
        printf("Enter two filenames separated by a space (defaults: %s and %s): ", file1, file2);if (fgets(line, sizeof(line), stdin)) {
            // tokenize by whitespace (simple)
            char *p = line;
            // trim trailing newline
            for (size_t i = 0; line[i]; ++i) { if (line[i]=='\n' || line[i]=='\r') { line[i]='\0'; break; } }
            // skip leading spaces
            while (*p==' ' || *p=='\t') ++p;
            if (*p) {
                // first token
                char *t1 = p;
                while (*p && *p!=' ' && *p!='\t') ++p;
                if (*p) { *p++ = '\0'; }
                if (t1[0]) strncpy(file1, t1, sizeof(file1)-1);

                // skip spaces to second token (optional)
                while (*p==' ' || *p=='\t') ++p;
                if (*p) {
                    char *t2 = p;
                    while (*p && *p!=' ' && *p!='\t') ++p;
                    *p = '\0';
                    if (t2[0]) strncpy(file2, t2, sizeof(file2)-1);
                }
            }
        }

        /* Resolve default save directory for results */
            char results_dir[512]; pkgdiff_get_results_dir(results_dir, sizeof(results_dir));
            ensure_dir_all(results_dir, 0700);
            char f1_final[768]; char f2_final[768];
            if (strchr(file1, '/')) { snprintf(f1_final, sizeof(f1_final), "%s", file1); }
            else { join_path(f1_final, sizeof(f1_final), results_dir, file1); }
            if (strchr(file2, '/')) { snprintf(f2_final, sizeof(f2_final), "%s", file2); }
            else { join_path(f2_final, sizeof(f2_final), results_dir, file2); }

            if (write_full_unique_lists(f1_final, f2_final, map1, map2) == 0) {
            printf(C_GREEN "Saved: %s and %s\n" C_RESET, f1_final, f2_final);
            if (arch_filter && *arch_filter) {
                printf("(Both files contain only '%s' architecture entries)\n", arch_filter);
            }
        } else {
            fail("Failed to write one or both files");
        }
    } else {
        printf("Skipping saving full lists.\n");
    }
json_decref(only1_preview);
    json_decref(only2_preview);
    json_decref(map1);
    json_decref(map2);
    json_decref(root1);
    json_decref(root2);
}



/* ===== New feature: common packages across two branches with version preview & JSON save ===== */



PKGDIFF_API void pkgdiff_common_pkgs_with_versions_ex(const char *branch1, const char *branch2, const char *arch_filter, const char *save_path, int filter) {
    if (arch_filter && *arch_filter) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Analyzing only architecture: %s", arch_filter);
        note(buf);
    }
    printf("\n");
    note("Fetching package lists...");
    printf("\n");
    char *json1 = fetch_packages_json(branch1);
    printf("\n");
    char *json2 = fetch_packages_json(branch2);
    if (!json1 || !json2) {
        fail("Download failed (one of branches)"); 
        free(json1); free(json2); 
        return;
    }
    printf("\n");
    note("Parsing JSON...");
    json_error_t err1 = {0}, err2 = {0};
    json_t *root1 = json_loads(json1, 0, &err1);
    json_t *root2 = json_loads(json2, 0, &err2);
    free(json1); free(json2);
    if (!root1 || !root2) {

        fail("JSON parse error");
        if (root1) json_decref(root1);
        if (root2) json_decref(root2);
        return;
    }
    ok("JSON parsed");
    json_t *packages1 = json_object_get(root1, "packages");
    json_t *packages2 = json_object_get(root2, "packages");
    if (!json_is_array(packages1) || !json_is_array(packages2)) {
        fail("Unexpected JSON: 'packages' is not an array");
        json_decref(root1); json_decref(root2);
        return;
    }

    log_indexing_start();
    json_t *map1 = json_object();
    json_t *map2 = json_object();
size_t len1 = json_array_size(packages1);for (size_t i = 0; i < len1; ++i) {
        json_t *pkg = json_array_get(packages1, i);
        if (!json_is_object(pkg)) continue;
        const char *name = json_string_value(json_object_get(pkg, "name"));
        const char *arch = json_string_value(json_object_get(pkg, "arch"));
        if (!name || !arch) continue;
        if (!arch_matches(arch_filter, arch)) continue;
        char key[512]; snprintf(key, sizeof(key), "%s|%s", name, arch);
        json_object_set(map1, key, pkg);
    }
    size_t len2 = json_array_size(packages2);for (size_t i = 0; i < len2; ++i) {
        json_t *pkg = json_array_get(packages2, i);
        if (!json_is_object(pkg)) continue;
        const char *name = json_string_value(json_object_get(pkg, "name"));
        const char *arch = json_string_value(json_object_get(pkg, "arch"));
        if (!name || !arch) continue;
        if (!arch_matches(arch_filter, arch)) continue;
        char key[512]; snprintf(key, sizeof(key), "%s|%s", name, arch);
        json_object_set(map2, key, pkg);
    }

    
    log_indexing_done();
    printf("\n");
    log_fetched_count("branch1", len1, branch1);
    log_fetched_count("branch2", len2, branch2);
    printf("\n");
note("Computing common set...");
    size_t common_count = 0;
    const size_t PREVIEW_LIMIT = 10;
    json_t *preview = json_array();
    json_t *save_arr = json_array();

    void *iter = json_object_iter(map1);
    while (iter) {
        const char *key = json_object_iter_key(iter);
        json_t *pkg1 = json_object_iter_value(iter);
        json_t *pkg2 = json_object_get(map2, key);
        if (pkg2) {
            const char *name = json_string_value(json_object_get(pkg1, "name"));
            const char *arch = json_string_value(json_object_get(pkg1, "arch"));
            long e1 = 0, e2 = 0;
            json_t *e = json_object_get(pkg1, "epoch"); if (json_is_integer(e)) e1 = json_integer_value(e);
            e = json_object_get(pkg2, "epoch"); if (json_is_integer(e)) e2 = json_integer_value(e);
            const char *v1 = json_string_value(json_object_get(pkg1, "version")); if (!v1) v1 = "?";
            const char *r1 = json_string_value(json_object_get(pkg1, "release")); if (!r1) r1 = "?";
            const char *v2 = json_string_value(json_object_get(pkg2, "version")); if (!v2) v2 = "?";
            const char *r2 = json_string_value(json_object_get(pkg2, "release")); if (!r2) r2 = "?";

            char s1[256], s2[256];
            if (e1 > 0) snprintf(s1, sizeof(s1), "%ld:%s-%s", e1, v1, r1);
            else        snprintf(s1, sizeof(s1), "%s-%s", v1, r1);
            if (e2 > 0) snprintf(s2, sizeof(s2), "%ld:%s-%s", e2, v2, r2);
            else        snprintf(s2, sizeof(s2), "%s-%s", v2, r2);

            json_t *entry = json_object();
            json_object_set_new(entry, "name", json_string(name ? name : ""));
            json_object_set_new(entry, "arch", json_string(arch ? arch : "noarch"));
            json_object_set_new(entry, branch1, json_string(s1));
            json_object_set_new(entry, branch2, json_string(s2));
            json_object_set_new(entry, "different", json_boolean(strcmp(s1, s2) != 0));
            json_array_append_new(save_arr, entry);

            if (json_array_size(preview) < PREVIEW_LIMIT) {
                json_t *pv = json_object();
                json_object_set_new(pv, "name", json_string(name ? name : ""));
                json_object_set_new(pv, "arch", json_string(arch ? arch : "noarch"));
                json_object_set_new(pv, branch1, json_string(s1));
                json_object_set_new(pv, branch2, json_string(s2));
                json_array_append_new(preview, pv);
            }
            common_count++;
        }
        iter = json_object_iter_next(map1, iter);
    }

    
    /* Build filtered set over the FULL common set, not just the preview */
    json_t *matches = json_array();
    json_t *preview_filtered = json_array();
    size_t match_count = 0;
    for (size_t i = 0; i < json_array_size(save_arr); ++i) {
        json_t *entry = json_array_get(save_arr, i);
        int differ = json_is_true(json_object_get(entry, "different"));
        int match = (filter == PKGDIFF_FILTER_ALL) ||
                    (filter == PKGDIFF_FILTER_DIFFER && differ) ||
                    (filter == PKGDIFF_FILTER_EQUAL && !differ);
        if (match) {
            json_array_append(matches, entry);
            match_count++;
            if (json_array_size(preview_filtered) < PREVIEW_LIMIT) {
                json_array_append(preview_filtered, entry);
            }
        }
    }
ok("Common set computed");
        if (filter == PKGDIFF_FILTER_ALL) {
            printf("\n" C_BOLD C_ACCENT "Packages present in BOTH %s and %s: %zu\n" C_RESET, branch1, branch2, json_array_size(matches));
        } else if (filter == PKGDIFF_FILTER_DIFFER) {
            printf("\n" C_BOLD C_ACCENT "Packages with DIFFERENT versions in both branches: %zu\n" C_RESET, json_array_size(matches));
        } else {
            printf("\n" C_BOLD C_ACCENT "Packages with the SAME versions in both branches: %zu\n" C_RESET, json_array_size(matches));
        }
        size_t preview_shown = json_array_size(preview_filtered) < PREVIEW_LIMIT ? json_array_size(preview_filtered) : PREVIEW_LIMIT;
        printf(C_DIM "(Preview: first %zu)\n" C_RESET, preview_shown);
    
int wN=0,wA=0,wC1=0,wC2=0;
compute_versions_widths(preview_filtered, branch1, branch2, &wN, &wA, &wC1, &wC2);
if (json_array_size(preview_filtered) == 0) {
        printf(C_DIM "No matches found for this selection.\n" C_RESET);
    } else {
    for (size_t i = 0; i < json_array_size(preview_filtered); ++i) {
        json_t *pv = json_array_get(preview_filtered, i);
        const char *name = json_string_value(json_object_get(pv, "name"));
        const char *arch = json_string_value(json_object_get(pv, "arch"));
        const char *s1 = json_string_value(json_object_get(pv, branch1));
        const char *s2 = json_string_value(json_object_get(pv, branch2));
        int differ = (s1 && s2) ? strcmp(s1, s2) != 0 : 1;
        print_version_pair_aligned(name ? name : "(unknown)",
                                   arch ? arch : "noarch",
                                   branch1, s1 ? s1 : "?",
                                   branch2, s2 ? s2 : "?",
                                   differ, wN, wA, wC1, wC2);
    }
}
int do_save = 0;
    char pathbuf[512] = {0};
    if (save_path && *save_path) {
        strncpy(pathbuf, save_path, sizeof(pathbuf)-1);
        do_save = 1;
    } else {
        printf("\nSave to JSON file? [y/N]: ");char ans[8] = {0};
        if (fgets(ans, sizeof(ans), stdin) && (ans[0]=='y' || ans[0]=='Y')) {
            

/* Build default file name depending on filter */
char default_name[256];
const char *prefix = (filter==PKGDIFF_FILTER_ALL) ? "" : (filter==PKGDIFF_FILTER_DIFFER ? "diff-" : "same-");
if (prefix[0] == 0) {
    snprintf(default_name, sizeof(default_name), "common-%s-%s.json", branch1, branch2);
} else {
    snprintf(default_name, sizeof(default_name), "common-%s%s-%s.json", prefix, branch1, branch2);
}
printf("Enter filename (default: %s): ", default_name);
if (!fgets(pathbuf, sizeof(pathbuf), stdin)) { pathbuf[0] = '\0'; }
size_t L = strlen(pathbuf); if (L > 0 && pathbuf[L-1] == '\n') pathbuf[L-1] = '\0';
if (pathbuf[0] == '\0') {
    strncpy(pathbuf, default_name, sizeof(pathbuf)-1);
    pathbuf[sizeof(pathbuf)-1] = '\0';
}
do_save = 1;
        }
    }

    /* Normalize save path: if it's just a filename (no '/'), place into results dir */
    if (do_save) {
        if (!strchr(pathbuf, '/')) {
            char results_dir[512]; pkgdiff_get_results_dir(results_dir, sizeof(results_dir));
            ensure_dir_all(results_dir, 0700);
            char tmpbuf[1024];
            join_path(tmpbuf, sizeof(tmpbuf), results_dir, pathbuf);
            strncpy(pathbuf, tmpbuf, sizeof(pathbuf)-1);
            pathbuf[sizeof(pathbuf)-1] = '\0';
        }
    }

    if (do_save) {
        char *dump = json_dumps(matches, JSON_INDENT(2));
        if (!dump) {
            fail("Failed to serialize JSON");
        } else {
            FILE *f = fopen(pathbuf, "wb");
            if (!f) {
                fail("Failed to open output file for writing");
            } else {
                fwrite(dump, 1, strlen(dump), f);
                fclose(f);
                printf(C_GREEN "Saved: %s\n" C_RESET, pathbuf);

}
            free(dump);
        }
    }

    json_decref(preview); json_decref(preview_filtered); json_decref(matches);
    json_decref(save_arr);
    json_decref(map1);
    json_decref(map2);
    json_decref(root1);
    json_decref(root2);
}

PKGDIFF_API void pkgdiff_common_pkgs_with_versions(const char *branch1, const char *branch2, const char *arch_filter, const char *save_path) {
    pkgdiff_common_pkgs_with_versions_ex(branch1, branch2, arch_filter, save_path, PKGDIFF_FILTER_ALL);
}
