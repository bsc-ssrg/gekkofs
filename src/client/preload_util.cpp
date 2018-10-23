#include <client/preload_util.hpp>
#include <global/rpc/distributor.hpp>
#include <global/rpc/rpc_utils.hpp>
#include <global/global_func.hpp>

#include <fstream>
#include <sstream>
#include <csignal>
#include <random>
#include <sys/sysmacros.h>

using namespace std;

// rpc address cache
std::unique_ptr<std::unordered_map<uint64_t, hg_addr_t>> rpc_addresses;

/**
 * Converts the Metadata object into a stat struct, which is needed by Linux
 * @param path
 * @param md
 * @param attr
 * @return
 */
int metadata_to_stat(const std::string& path, const Metadata& md, struct stat& attr) {

    /* Populate default values */
    attr.st_dev = makedev(0, 0);
    attr.st_ino = md.fuid();
    attr.st_nlink = 1;
    attr.st_uid = CTX->fs_conf()->uid;
    attr.st_gid = CTX->fs_conf()->gid;
    attr.st_rdev = 0;
    attr.st_blksize = CHUNKSIZE;
    attr.st_blocks = 0;

    memset(&attr.st_atim, 0, sizeof(timespec));
    memset(&attr.st_mtim, 0, sizeof(timespec));
    memset(&attr.st_ctim, 0, sizeof(timespec));

    attr.st_mode = md.mode();

#ifdef HAS_SYMLINKS
    if (md.is_link())
        attr.st_size = md.target_path().size() + CTX->mountdir().size();
    else
#endif
    attr.st_size = md.size();

    if (CTX->fs_conf()->atime_state) {
        attr.st_atim.tv_sec = md.atime();
    }
    if (CTX->fs_conf()->mtime_state) {
        attr.st_mtim.tv_sec = md.mtime();
    }
    if (CTX->fs_conf()->ctime_state) {
        attr.st_ctim.tv_sec = md.ctime();
    }
    if (CTX->fs_conf()->link_cnt_state) {
        attr.st_nlink = md.link_count();
    }
    if (CTX->fs_conf()->blocks_state) { // last one will not encounter a delimiter anymore
        attr.st_blocks = md.blocks();
    }
    return 0;
}

/**
 * @return daemon pid. If not running @return -1.
 * Loads set deamon mountdir set in daemon.pid file
 */
int get_daemon_pid() {
    ifstream ifs(daemon_pid_path(), ::ifstream::in);
    int adafs_daemon_pid = -1;
    string mountdir;
    if (ifs) {
        string adafs_daemon_pid_s;
        // first line is pid
        if (getline(ifs, adafs_daemon_pid_s) && !adafs_daemon_pid_s.empty())
            adafs_daemon_pid = ::stoi(adafs_daemon_pid_s);
        else {
            cerr << "ADA-FS daemon pid not found. Daemon not running?" << endl;
            CTX->log()->error("{}() Unable to read daemon pid from pid file", __func__);
            ifs.close();
            return -1;
        }
        // check that daemon is running
        if (kill(adafs_daemon_pid, 0) != 0) {
            cerr << "ADA-FS daemon process with pid " << adafs_daemon_pid << " not found. Daemon not running?" << endl;
            CTX->log()->error("{}() ADA-FS daemon pid {} not found. Daemon not running?", __func__, adafs_daemon_pid);
            ifs.close();
            return -1;
        }
        // second line is mountdir
        std::string daemon_addr;
        if (getline(ifs, daemon_addr) && !daemon_addr.empty()) {
            CTX->daemon_addr_str(daemon_addr);
        } else {
            CTX->log()->error("{}() ADA-FS daemon pid file contains no daemon address. Exiting ...", __func__);
            ifs.close();
            return -1;
        }
        // second line is mountdir
        if (getline(ifs, mountdir) && !mountdir.empty()) {
            CTX->mountdir(mountdir);
        } else {
            CTX->log()->error("{}() ADA-FS daemon pid file contains no mountdir path. Exiting ...", __func__);
            ifs.close();
            return -1;
        }
    } else {
        cerr << "No permission to open pid file at " << daemon_pid_path()
             << " or ADA-FS daemon pid file not found. Daemon not running?" << endl;
        CTX->log()->error(
                "{}() Failed to open pid file '{}'. Error: {}",
                __func__, daemon_pid_path(), std::strerror(errno));
    }
    ifs.close();

    return adafs_daemon_pid;
}

unordered_map<string, string> load_lookup_file(const std::string& lfpath) {
    CTX->log()->debug("{}() Loading lookup file: '{}'",
                      __func__, lfpath);
    ifstream lf(lfpath);
    lf.exceptions(ifstream::badbit);

    unordered_map<string, string> endpoints_map;
    string line;
    string hostname;
    string endpoint;
    string::size_type delim_pos;
    while (getline(lf, line)) {
        delim_pos = line.find(" ", delim_pos = 0);
        if(delim_pos == string::npos) {
            throw runtime_error(fmt::format("Failed to parse line in lookup file: '{}'", line));
        }
        hostname = line.substr(0, delim_pos);
        endpoint = line.substr(delim_pos + 1);
        CTX->log()->trace("{}() endpoint loaded: '{}' '{}'", __func__, hostname, endpoint);
        endpoints_map.insert(make_pair(hostname, endpoint));
    }
    return endpoints_map;
}

hg_addr_t margo_addr_lookup_retry(const std::string& uri) {
    // try to look up 3 times before erroring out
    hg_return_t ret;
    hg_addr_t remote_addr = HG_ADDR_NULL;
    ::random_device rd; // obtain a random number from hardware
    unsigned int attempts = 0;
    do {
        ret = margo_addr_lookup(ld_margo_rpc_id, uri.c_str(), &remote_addr);
        if (ret == HG_SUCCESS) {
            break;
        }
        CTX->log()->warn("{}() Failed to lookup address {}. Attempts [{}/3]", __func__, uri, attempts + 1);
        // Wait a random amount of time and try again
        ::mt19937 g(rd()); // seed the random generator
        ::uniform_int_distribution<> distr(50, 50 * (attempts + 2)); // define the range
        ::this_thread::sleep_for(std::chrono::milliseconds(distr(g)));
    } while (++attempts < 3);
    return remote_addr;
}

hg_addr_t get_local_addr() {
    return margo_addr_lookup_retry(CTX->daemon_addr_str());
}

/*
 * Get the URI of a given hostname.
 *
 * This URI is to be used for margo lookup function.
 */
std::string get_uri_from_hostname(const std::string& hostname) {
    if (!CTX->fs_conf()->endpoints.empty()) {
        return CTX->fs_conf()->endpoints.at(hostname);
    }

    auto host = get_host_by_name(hostname + CTX->fs_conf()->hostname_suffix);
    return fmt::format("{}://{}:{}", RPC_PROTOCOL, host, CTX->fs_conf()->rpc_port);
}

bool lookup_all_hosts() {
    rpc_addresses = std::make_unique<std::unordered_map<uint64_t, hg_addr_t>>();
    vector<uint64_t> hosts(CTX->fs_conf()->host_size);
    // populate vector with [0, ..., host_size - 1]
    ::iota(::begin(hosts), ::end(hosts), 0);
    /*
     * Shuffle hosts to balance addr lookups to all hosts
     * Too many concurrent lookups send to same host could overwhelm the server, returning error when addr lookup
     */
    ::random_device rd; // obtain a random number from hardware
    ::mt19937 g(rd()); // seed the random generator
    ::shuffle(hosts.begin(), hosts.end(), g); // Shuffle hosts vector
    // lookup addresses and put abstract server addresses into rpc_addresses
    for (auto& host : hosts) {
        string uri{};
        // If local use address provided by daemon in order to use automatic shared memory routing
        if (host == CTX->fs_conf()->host_id) {
            uri = CTX->daemon_addr_str();
        } else {
            auto hostname = CTX->fs_conf()->hosts.at(host);
            uri = get_uri_from_hostname(hostname);
        }
        auto remote_addr = margo_addr_lookup_retry(uri);
        if (remote_addr == HG_ADDR_NULL) {
            CTX->log()->error("{}() Failed to lookup address {}", __func__, uri);
            return false;
        }
        CTX->log()->trace("{}() Successful address lookup for '{}'", __func__, uri);
        rpc_addresses->insert(make_pair(host, remote_addr));
    }
    return true;
}

void cleanup_addresses() {
    CTX->log()->debug("{}() Freeing Margo RPC svr addresses ...", __func__);
    for (const auto& e : *rpc_addresses) {
        CTX->log()->info("{}() Trying to free hostid {}", __func__, e.first);
        if (margo_addr_free(ld_margo_rpc_id, e.second) != HG_SUCCESS) {
            CTX->log()->warn("{}() Unable to free RPC client's svr address: {}.", __func__, e.first);
        }
    }
}

/**
 * Retrieve abstract svr address handle for hostid
 * @param hostid
 * @param svr_addr
 * @return
 */
bool get_addr_by_hostid(const uint64_t hostid, hg_addr_t& svr_addr) {
    auto address_lookup = rpc_addresses->find(hostid);
    auto found = address_lookup != rpc_addresses->end();
    if (found) {
        svr_addr = address_lookup->second;
        CTX->log()->trace("{}() RPC address lookup success with hostid {}", __func__, address_lookup->first);
        return true;
    } else {
        // not found, unexpected host.
        // This should not happen because all addresses are looked when the environment is initialized.
        CTX->log()->error("{}() Unexpected host id {}. Not found in RPC address cache", __func__, hostid);
        assert(found && "Unexpected host id for rpc address lookup. ID was not found in RPC address cache.");
    }
    return false;
}

hg_return
margo_create_wrap_helper(const hg_id_t rpc_id, uint64_t recipient, hg_handle_t& handle) {
    hg_return_t ret;
    hg_addr_t svr_addr = HG_ADDR_NULL;
    if (!get_addr_by_hostid(recipient, svr_addr)) {
        CTX->log()->error("{}() server address not resolvable for host id {}", __func__, recipient);
        return HG_OTHER_ERROR;
    }
    ret = margo_create(ld_margo_rpc_id, svr_addr, rpc_id, &handle);
    if (ret != HG_SUCCESS) {
        CTX->log()->error("{}() creating handle FAILED", __func__);
        return HG_OTHER_ERROR;
    }
    return ret;
}

/**
 * Wraps certain margo functions to create a Mercury handle
 * @param path
 * @param handle
 * @return
 */
hg_return margo_create_wrap(const hg_id_t rpc_id, const std::string& path, hg_handle_t& handle) {
    auto recipient = CTX->distributor()->locate_file_metadata(path);
    return margo_create_wrap_helper(rpc_id, recipient, handle);
}
