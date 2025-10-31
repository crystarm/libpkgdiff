#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>
#include <jansson.h>
#include "pkgdiff.h"
#include "u.h"



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
    return chunk.data;
}

