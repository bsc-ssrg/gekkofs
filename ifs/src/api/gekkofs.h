#ifndef __GEKKOFS_API_H__
#define __GEKKOFS_API_H__ 1

#include <sys/types.h>
#include <stdint.h>

#ifndef GEKKOFS_API_VERSION
#define GEKKOFS_API_VERSION 10
#endif

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/**  Common types                                                             */
/******************************************************************************/

/** Server ID type */
typedef uint32_t gfs_server_id_t;

/** Server name type */
typedef const char* gfs_server_name_t;

/** File version ID type */
typedef const char* gfs_vid_t;

/** Error codes */
typedef enum {

    /* Generic */
    GEKKOFS_SUCCESS = 0

    //TODO add more error codes
} gfs_error_t;



/******************************************************************************/
/** Definitions for file distribution policies                                */
/******************************************************************************/

/** 
 * Pseudo-randomized file chunking:
 *
 * This policy splits a file imported into GekkoFS into fixed-sized chunks which
 * are then pseudo-randomly distributed among all available data servers.
 * */
typedef struct {

    /** The algorithm to use for chunk distribution */
    enum {
        /** Use C++ std::hash() function */
        FA_CHUNKING_ALGO_STDHASH

        //TODO add more algorithms
    } fa_chunk_algo;

    /** The chunk size in bytes used for splitting files */
    size_t fa_chunk_size;

} gfs_falloc_chunking_args_t;


/** 
 * File striping:
 *
 * This policy splits a file imported into GekkoFS into fixed-sized stripes 
 * which are then distributed in a round-robin fashion among all available data 
 * servers.
 * */
typedef struct {

    /** The stripe size in bytes used for splitting files */
    size_t fa_stripe_size;

} gfs_falloc_striping_args_t;

/**
 * File allocation policy:
 *
 * This structure defines the file allocation policy to use when importing a 
 * file into GekkoFS.
 * */
typedef struct {

    /** Desired file allocation policy */
    enum {
        /** The file is considered as an indivisible unit by GekkoFS,
         *  and hence will be managed by only one data server */
        GEKKOFS_FILE_UNIT,

        /** Pseudo-random file chunking (requires initializing the
         * .fa_chunking_args field ) */
        GEKKOFS_FILE_CHUNKING,

        /** Round robin file striping (requires initializing the
         * .fa_striping field ) */
        GEKKOFS_FILE_STRIPING
    } fa_type;

    /** Specific arguments for the file allocation policy */
    union {
        /** Arguments for GEKKOFS_FILE_CHUNKING */
        gfs_falloc_chunking_args_t fa_chunking_args;

        /** Arguments for GEKKOFS_FILE_STRIPING */
        gfs_falloc_striping_args_t fa_striping_args;
    };
} gfs_falloc_t;



/******************************************************************************/
/** Basic GekkoFS API                                                         */
/******************************************************************************/

/**
 * Import a file into GekkoFS 
 *
 * @param: in_pathname the path to the file to import 
 * @param: gekkofs_pathname the path used in GekkoFS to refer to the file
 * @param: alloc_policy the data allocation policy for the file 
 *
 * @return: GEKKOFS_SUCCESS on success, 
 *          TODO, otherwise
 * */
gfs_error_t
gfs_import(const char* in_pathname,
           const char* gekkofs_pathname,
           gfs_falloc_t alloc_policy);


/**
 * Export a file out of GekkoFS into another POSIX filesystem, and remove it 
 * from GekkoFS when finished
 *
 * @param: gekkofs_pathname the path used in GekkoFS to refer to the file
 * @param: out_pathname the path where the file should be exported
 * @param: alloc_policy the data allocation policy for the file 
 *
 * @return: GEKKOFS_SUCCESS on success, 
 *          TODO, otherwise
 * */
gfs_error_t
gfs_export(const char* gekkofs_pathname,
           const char* out_pathname);


/**
 * Commands supported by gfs_fcntl()
 * */
typedef enum {

    /** Retrieve list of data server IDs where the file is located.
     *  Expected arguments: 
     *
     *     gfs_server_name_t* list[]: an input/output argument that can be 
     *       initialized by the API with a NULL terminated array containing the 
     *       names of the servers involved with this file. The array returned
     *       is malloc()'ed by the API and it is the callers responsibility to
     *       free() it when done.
     **/
    F_GET_LOCATIONS,

    /** Create a new version of the file.
     *  Expected arguments: 
     *
     *     gfs_vid_t* new_vid: an input/output argument to be filled by the API
     *       with the ID generated for the new version. The argument returned
     *       is malloc()'ed by the API and it is the callers responsibility to
     *       free() it when done.
     * */
    F_NEW_VERSION,

    /** Get the current version of the file.
     *  Expected arguments: 
     *
     *     gfs_vid_t* vid: an input/output argument to be filled by the API
     *       with the ID generated for the new version. The argument returned
     *       is malloc()'ed by the API and it is the callers responsibility to
     *       free() it when done.
     * */
    F_GET_CURRENT_VERSION,

    /** Set the current version of the file.
     *  Expected arguments: 
     *
     *     const gfs_vid_t* vid: an input argument specifying the VID desired 
     *       by the caller.
     * */
    F_SET_CURRENT_VERSION
} gfs_cmd_t;


/**
 * Manipulate a GekkoFS file
 *
 * @param: gekkofs_pathname the path used in GekkoFS to refer to the file
 * @param: cmd the command to perform on the file (see the definition of 
 *         gfs_cmd_t for details)
 *
 * @return: GEKKOFS_SUCCESS on success, 
 *          TODO, otherwise
 * */
gfs_error_t
gfs_fcntl(const char* gekkofs_pathname,
          gfs_cmd_t cmd,
          gfs_server_name_t* foo[],
          ... /* arg */);

#ifdef __cplusplus
}
#endif

#endif // __GEKKOFS_API_H__
