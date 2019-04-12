#ifndef IFS_RPC_DISTRIBUTOR_HPP
#define IFS_RPC_DISTRIBUTOR_HPP

#include "global/global_defs.hpp"

#include <vector>
#include <string>
#include <numeric>


using ChunkID = unsigned long;
using Host = unsigned long;

class Distributor {
    public:
        virtual Host localhost() const = 0;
        virtual Host locate_data(const fuid_t fuid, const ChunkID chnk_id) const = 0;
        virtual Host locate_file_metadata(const std::string& path) const = 0;
        virtual std::vector<Host> locate_directory_metadata(const std::string& path) const = 0;
};


class SimpleHashDistributor : public Distributor {
    private:
        Host localhost_;
        unsigned int hosts_size_;
        std::vector<Host> all_hosts_;
    public:
        SimpleHashDistributor(Host localhost, unsigned int hosts_size);
        Host localhost() const override;
        Host locate_data(const fuid_t fuid, const ChunkID chnk_id) const override;
        Host locate_file_metadata(const std::string& path) const override;
        std::vector<Host> locate_directory_metadata(const std::string& path) const override;
};

class LocalOnlyDistributor : public Distributor {
    private:
        Host localhost_;
    public:
        LocalOnlyDistributor(Host localhost);
        Host localhost() const override;
        Host locate_data(const fuid_t fuid, const ChunkID chnk_id) const override;
        Host locate_file_metadata(const std::string& path) const override;
        std::vector<Host> locate_directory_metadata(const std::string& path) const override;
};

#endif //IFS_RPC_LOCATOR_HPP
