#ifndef IFS_PRELOAD_CTX_HPP
#define IFS_PRELOAD_CTX_HPP

#include <spdlog/spdlog.h>
#include <map>
#include <memory>
#include <vector>
#include <string>

/* Forward declarations */
class OpenFileMap;
class Distributor;
class RPCEngine;


struct FsConfig {
    // configurable metadata
    bool atime_state;
    bool mtime_state;
    bool ctime_state;
    bool uid_state;
    bool gid_state;
    bool link_cnt_state;
    bool blocks_state;

    uid_t uid;
    gid_t gid;

    std::string rootdir;

    // rpc infos
    std::map<uint64_t, std::string> hosts;
    std::map<std::string, std::string> sys_hostfile;
    uint64_t host_id; // my host number
    size_t host_size;
    std::string rpc_port;
};

enum class RelativizeStatus {
    internal,
    external,
    fd_unknown,
    fd_not_a_dir
};

class PreloadContext {
    private:
    PreloadContext();

    std::shared_ptr<spdlog::logger> log_;
    std::shared_ptr<OpenFileMap> ofm_;
    std::shared_ptr<Distributor> distributor_;
    std::shared_ptr<FsConfig> fs_conf_;
    std::shared_ptr<RPCEngine> rpc_;

    std::string cwd_;
    std::vector<std::string> mountdir_components_;
    std::string mountdir_;
    std::string daemon_addr_str_;
    bool initialized_;

    public:
    static PreloadContext* getInstance() {
        static PreloadContext instance;
        return &instance;
    }

    PreloadContext(PreloadContext const&) = delete;
    void operator=(PreloadContext const&) = delete;

    void log(std::shared_ptr<spdlog::logger> logger);
    std::shared_ptr<spdlog::logger> log() const;

    void mountdir(const std::string& path);
    const std::string& mountdir() const;
    const std::vector<std::string>& mountdir_components() const;

    void daemon_addr_str(const std::string& path);
    const std::string& daemon_addr_str() const;

    void cwd(const std::string& path);
    const std::string& cwd() const;

    RelativizeStatus relativize_fd_path(int dirfd,
                                        const char * raw_path,
                                        std::string& relative_path,
                                        bool resolve_last_link = true) const;

    bool relativize_path(const char * raw_path, std::string& relative_path, bool resolve_last_link = true) const;
    const std::shared_ptr<OpenFileMap>& file_map() const;

    void distributor(std::shared_ptr<Distributor> distributor);
    std::shared_ptr<Distributor> distributor() const;
    void rpc(std::shared_ptr<RPCEngine> rpc);
    std::shared_ptr<RPCEngine> rpc() const;
    const std::shared_ptr<FsConfig>& fs_conf() const;

    void initialized(const bool& flag);
    bool initialized() const;
};


#endif //IFS_PRELOAD_CTX_HPP

