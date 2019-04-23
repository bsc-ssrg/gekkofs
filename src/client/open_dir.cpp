/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  SPDX-License-Identifier: MIT
*/

#include <client/open_dir.hpp>
#include <stdexcept>
#include <cstring>
#include <cassert>


OpenDir::DirEntry::DirEntry(const std::string& name, const FileType type):
    name(name), type(type) {
}


OpenDir::OpenDir(const std::string& path) :
    OpenFile(path, 0, FileType::directory) {
    is_dirent_valid = false;
}

void OpenDir::add(const std::string& name, const FileType& type){
    entries.push_back(DirEntry(name, type));
}

void OpenDir::update_dirent(unsigned int pos){
    DirEntry entry = entries.at(pos);
    std::string entry_absolute_path = path_ + "/" + entry.name;
    dirent_.d_ino = std::hash<std::string>()(entry_absolute_path);
    dirent_.d_off = pos_;
    dirent_.d_type = ((entry.type == FileType::regular)? DT_REG : DT_DIR);
    assert(sizeof(dirent_.d_name) >= strlen(entry.name.c_str()));
    strcpy(dirent_.d_name, entry.name.c_str());
    dirent_.d_reclen = sizeof(dirent_);
}

struct dirent * OpenDir::readdir(){
    unsigned int next_pos;

    if(!is_dirent_valid){
        next_pos = 0;
    }else{
        next_pos = pos_ + 1;
    }

    //We reached the end of entries list
    if( next_pos >= entries.size()){
        is_dirent_valid = false;
        return NULL;
    }

    update_dirent(next_pos);
    pos_ = next_pos;
    is_dirent_valid = true;
    return &dirent_;
}

size_t OpenDir::size(){
    return entries.size();
}
