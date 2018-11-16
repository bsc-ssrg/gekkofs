
#ifndef IFS_PRELOAD_UTIL_HPP
#define IFS_PRELOAD_UTIL_HPP

#include <preload/preload.hpp>
#include <global/metadata.hpp>
// third party libs
#include <string>
#include <iostream>


struct MetadentryUpdateFlags {
    bool atime = false;
    bool mtime = false;
    bool ctime = false;
    bool uid = false;
    bool gid = false;
    bool mode = false;
    bool link_count = false;
    bool size = false;
    bool blocks = false;
    bool path = false;
};


// function definitions

int metadata_to_stat(const std::string& path, const Metadata& md, struct stat& attr);

int get_daemon_pid();

bool read_system_hostfile();

void lookup_all_hosts();


#endif //IFS_PRELOAD_UTIL_HPP
