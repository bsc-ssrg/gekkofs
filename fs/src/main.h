//
// Created by lefthy on 1/24/17.
//

#ifndef MAIN_H
#define MAIN_H

#define FUSE_USE_VERSION 30


#include <fuse3/fuse.h>
#include <string>
#include <iostream>
#include <cstdint>

//boost libraries
#include <boost/filesystem.hpp>

#include "spdlog/spdlog.h"


struct adafs_data {
    std::string                             rootdir;
    std::shared_ptr<spdlog::logger>         logger;
    std::int64_t                            inode_count;
    std::mutex                              inode_mutex;
    int32_t                                 blocksize;
};

#define ADAFS_DATA ((struct adafs_data*) fuse_get_context()->private_data)

namespace util {
    std::string adafs_fullpath(const std::string &path);

    int reset_inode_no();

    ino_t generate_inode_no();
}

#endif //MAIN_H