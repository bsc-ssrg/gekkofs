#include "gekkofs.h"

gfs_error_t
gfs_import(const char* in_pathname,
           const char* gekkofs_pathname,
           gfs_falloc_t alloc_policy) {

    (void) in_pathname;
    (void) gekkofs_pathname;

    return GEKKOFS_SUCCESS;
}

gfs_error_t
gfs_export(const char* gekkofs_pathname,
           const char* out_pathname) {

    (void) gekkofs_pathname;
    (void) out_pathname;

    return GEKKOFS_SUCCESS;
}


gfs_error_t
gfs_fcntl(const char* gekkofs_pathname,
          gfs_cmd_t cmd,
          gfs_server_name_t* foo[],
          ... /* arg */) {

    (void) gekkofs_pathname;
    (void) cmd;

    return GEKKOFS_SUCCESS;
}
