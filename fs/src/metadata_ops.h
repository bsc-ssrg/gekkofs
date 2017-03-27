//
// Created by draze on 3/5/17.
//

#ifndef FS_METADATA_OPS_H
#define FS_METADATA_OPS_H

#include "main.h"
#include "metadata.h"

using namespace std;

bool write_all_metadata(const Metadata& md, const unsigned long hash);

template<typename T>
bool write_metadata_field(const T& field, const unsigned long hash, const string& leaf_name);

bool read_all_metadata(Metadata& md, const unsigned long hash);

template<typename T>
unique_ptr<T> read_metadata_field(const uint64_t hash, const string& leaf_name);

int get_metadata(Metadata& md, const std::string& path);

int get_metadata(Metadata& md, const boost::filesystem::path& path);

int read_dentries(std::vector<std::string>& dir, const unsigned long hash);

int create_dentry(const unsigned long parent_dir_hash, const std::string& fname);

#endif //FS_METADATA_OPS_H
