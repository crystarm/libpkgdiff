#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>
#include <jansson.h>
#include "pkgdiff.h"
#include "u.h"

/* ------------------------------------------------------------------------- */
/* Simple in-process cache so we don't download the same JSON twice          */
/* ------------------------------------------------------------------------- */
struct CachedJson {
    char  branch[64];
    char *data; /* NUL-terminated JSON text (heap owned by cache) */
};
static struct CachedJson s_cache[8];
static int s_cache_count = 0;

static char *cache_get_copy(const char *branch) {
    if (!branch || !*branch) return NULL;
    for (int i = 0; i < s_cache_count; ++i) {
        if (strcmp(s_cache[i].branch, branch) == 0 && s_cache[i].data) {
            char msg[192];
            snprintf(msg, sizeof(msg),
                     "Using cached JSON for branch '%s' (%.2f MB)",
                     branch, (double)strlen(s_cache[i].data) / 1048576.0);
            note(msg);
            size_t L = strlen(s_cache[i].data);
            char *copy = (char*)malloc(L + 1);
            if (copy) memcpy(copy, s_cache[i].data, L + 1);
            return copy;
        }
    }
    return NULL;
}

static void cache_put(const char *branch, const char *json_text) {
    if (!branch || !*branch || !json_text) return;
    /* try to update existing */
    for (int i = 0; i < s_cache_count; ++i) {
        if (strcmp(s_cache[i].branch, branch) == 0) {
            free(s_cache[i].data);
            s_cache[i].data = strdup(json_text);
            return;
        }
    }
    /* append or evict oldest if full */
    if (s_cache_count < (int)(sizeof(s_cache)/sizeof(s_cache[0]))) {
        strncpy(s_cache[s_cache_count].branch, branch,
                sizeof(s_cache[s_cache_count].branch) - 1);
        s_cache[s_cache_count].branch[sizeof(s_cache[s_cache_count].branch) - 1] = '\0';
        s_cache[s_cache_count].data = strdup(json_text);
        ++s_cache_count;
    } else {
        /* simple FIFO eviction */
        free(s_cache[0].data);
        for (int j = 1; j < s_cache_count; ++j) s_cache[j-1] = s_cache[j];
        strncpy(s_cache[s_cache_count-1].branch, branch,
                sizeof(s_cache[s_cache_count-1].branch) - 1);
        s_cache[s_cache_count-1].branch[sizeof(s_cache[s_cache_count-1].branch) - 1] = '\0';
        s_cache[s_cache_count-1].data = strdup(json_text);
    }
}




static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    struct MemoryBuffer *mem = (struct MemoryBuffer *)userp;
    char *ptr = (char *)realloc(mem->data, mem->size + real_size + 1);
    if (!ptr) {
        fprintf(stderr, C_RED "realloc failed" C_RESET "\n");
        return 0;
    }
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->data[mem->size] = '\0';
    return real_size;
}



char *fetch_packages_json(const char *branch) {
    /* First try in-process cache */
    {
        char *cached = cache_get_copy(branch);
        if (cached) {
            return cached;
        }
    }
    if (!branch) {
        fail("fetch_packages_json: branch is NULL");
        return NULL;
    }
    char headline[256];
    snprintf(headline, sizeof(headline), "Connecting to " C_GREEN "rdb.altlinux.org" C_RESET);
    print_tight_box(headline);
    char endpoint[512];
    snprintf(endpoint, sizeof(endpoint), "Endpoint: " C_RED "GET /api/export/branch_binary_packages/%s" C_RESET, branch);
    note(endpoint);
    CURL *curl = NULL;
    CURLcode res;
    struct MemoryBuffer chunk = (struct MemoryBuffer){0};
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) {
        fail("curl initialization failed");
        curl_global_cleanup();
        return NULL;
    }
    char url[256];
    snprintf(url, sizeof(url), "https://rdb.altlinux.org/api/export/branch_binary_packages/%s", branch);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "altpkgdiff/1.9");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    note("Requesting branch list...");
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Request failed for branch '%s': %s", branch, curl_easy_strerror(res));
        fail(msg);
        free(chunk.data);
        chunk.data = NULL;
    } else {
        ok("Download complete");
    }
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    if (chunk.data) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Received %.2f MB for branch '%s'", (double)chunk.size/1048576.0, branch);
        note(msg);
    }
    note("Verifying JSON structure...");
    if (!chunk.data || chunk.size == 0 || chunk.data[0] != '{') {
        warn("Unexpected response format (not a JSON object)");
    } else {
        ok("Looks like valid JSON");
    }
    cache_put(branch, chunk.data);
    return chunk.data;
}


