#ifndef __GEKKOFS_API_H__
#define __GEKKOFS_API_H__ 1

#ifndef GEKKOFS_API_VERSION
#define GEKKOFS_API_VERSION 10
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum gekkofs_error_t {
    GEKKOFS_SUCCESS = 0
};

// TODO: add interfaces here
//
gekkofs_error_t
gekkofs_import(const char* pathname);

#ifdef __cplusplus
}
#endif

#endif // __GEKKOFS_API_H__
