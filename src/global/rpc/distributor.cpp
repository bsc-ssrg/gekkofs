#include "global/rpc/distributor.hpp"
#include "global/hash_util.hpp"


SimpleHashDistributor::
SimpleHashDistributor(Host localhost, unsigned int hosts_size) :
    localhost_(localhost),
    hosts_size_(hosts_size),
    all_hosts_(hosts_size)
{
    std::iota(all_hosts_.begin(), all_hosts_.end(), 0);
}

Host SimpleHashDistributor::
localhost() const {
    return localhost_;
}

Host SimpleHashDistributor::
locate_data(const fuid_t fuid, const ChunkID chnk_id) const {
    std::hash<std::pair<fuid_t, ChunkID>> hasher;
    return hasher(std::make_pair(fuid, chnk_id)) % hosts_size_;;
}

Host SimpleHashDistributor::
locate_file_metadata(const std::string& path) const {
    std::hash<std::string> hasher;
    return hasher(path) % hosts_size_;
}


std::vector<Host> SimpleHashDistributor::
locate_directory_metadata(const std::string& path) const {
    return all_hosts_;
}


LocalOnlyDistributor::LocalOnlyDistributor(Host localhost) : localhost_(localhost)
{}

Host LocalOnlyDistributor::
localhost() const {
    return localhost_;
}

Host LocalOnlyDistributor::
locate_data(const fuid_t fuid, const ChunkID chnk_id) const {
    return localhost_;
}

Host LocalOnlyDistributor::
locate_file_metadata(const std::string& path) const {
    return localhost_;
}

std::vector<Host> LocalOnlyDistributor::
locate_directory_metadata(const std::string& path) const {
    return {localhost_};
}
