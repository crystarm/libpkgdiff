#ifndef PKGDIFF_H
#define PKGDIFF_H

#ifdef __cplusplus
extern "C" {
#endif

void hello_pkgdiff();
char *fetch_packages_json(const char *branch);
void json_parse_demo(const char *json_data);

#ifdef __cplusplus
}
#endif

#endif // PKGDIFF_H
