#ifndef PKGDIFF_H
#define PKGDIFF_H

#ifdef __cplusplus
extern "C" {
#endif


#ifndef PKGDIFF_API
# if defined(_WIN32) || defined(__CYGWIN__)
#  ifdef BUILDING_PKGDIFF
#   define PKGDIFF_API __declspec(dllexport)
#  else
#   define PKGDIFF_API __declspec(dllimport)
#  endif
# else
#  ifdef BUILDING_PKGDIFF
#   define PKGDIFF_API __attribute__((visibility("default")))
#  else
#   define PKGDIFF_API
#  endif
# endif
#endif


#define PKGDIFF_FILTER_ALL     0
#define PKGDIFF_FILTER_EQUAL   1
#define PKGDIFF_FILTER_DIFFER  2


PKGDIFF_API void pkgdiff_unique_pkgs(const char *branch1, const char *branch2,
                                     const char *arch_filter);

PKGDIFF_API void pkgdiff_common_pkgs_with_versions(const char *branch1, const char *branch2,
                                                   const char *arch_filter,
                                                   const char *save_path);

PKGDIFF_API void pkgdiff_common_pkgs_with_versions_ex(const char *branch1, const char *branch2,
                                                      const char *arch_filter,
                                                      const char *save_path,
                                                      int filter);

#ifdef __cplusplus
}
#endif
#endif 
