
#ifndef IFS_OPEN_DIR_HPP
#define IFS_OPEN_DIR_HPP

#include <string>
#include <vector>

#include <dirent.h>

#include <preload/open_file_map.hpp>


class OpenDir: public OpenFile {
    private:

        class DirEntry {
            public:
                std::string name;
                FileType type;

                DirEntry(const std::string& name, const FileType type);
        };

        std::vector<DirEntry> entries;
        struct dirent dirent_;
        bool is_dirent_valid;

        void update_dirent(unsigned int pos);


    public:
        OpenDir(const fuid_t fuid, const std::string& path);
        void add(const std::string& name, const FileType& type);
        struct dirent * readdir();
        size_t size();
};


#endif //IFS_OPEN_DIR_HPP
