
#ifndef IFS_OPEN_DIR_HPP
#define IFS_OPEN_DIR_HPP

#include <string>
#include <vector>

#include <dirent.h>

#include <preload/open_file_map.hpp>


class DirEntry {
    private:
        std::string name_;
        FileType type_;
    public:
        DirEntry(const std::string& name, const FileType type);
        const std::string& name();
        FileType type();
};

class OpenDir: public OpenFile {
    private:
        std::vector<DirEntry> entries;


    public:
        OpenDir(const fuid_t fuid, const std::string& path);
        void add(const std::string& name, const FileType& type);
        const DirEntry& getdent(unsigned int pos);
        size_t size();
};


#endif //IFS_OPEN_DIR_HPP
