#include <preload/preload_util.hpp>
#include <global/rpc/rpc_utils.hpp>
#include <global/rpc/distributor.hpp>
#include <global/global_func.hpp>

#include <fstream>
#include <iterator>
#include <memory>
#include <unordered_map>
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
    attr.st_blksize = BLOCKSIZE;
    attr.st_blocks = 0;

    memset(&attr.st_atim, 0, sizeof(timespec));
    memset(&attr.st_mtim, 0, sizeof(timespec));
    memset(&attr.st_ctim, 0, sizeof(timespec));

    attr.st_mode = md.mode();
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
    if (CTX->fs_conf()->uid_state) {
        attr.st_uid = md.uid();
    }
    if (CTX->fs_conf()->gid_state) {
        attr.st_gid = md.gid();
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

/**
 * Read /etc/hosts and put hostname - ip association into a map in fs config.
 * We are working with hostnames but some network layers (such as Omnipath) does not look into /etc/hosts.
 * Hence, we have to store the mapping ourselves.
 * @return success
 */
bool read_system_hostfile() {
    ifstream hostfile("/etc/hosts");
    if (!hostfile.is_open())
        return false;
    string line;
    map<string, string> sys_hostfile;
    while (getline(hostfile, line)) {
        if (line.empty() || line == "\n" || line.at(0) == '#')
            continue;
        std::istringstream iss(line);
        std::vector<string> tmp_list((istream_iterator<string>(iss)), istream_iterator<string>());
        for (unsigned int i = 1; i < tmp_list.size(); i++) {
            if (tmp_list[i].find(HOSTNAME_SUFFIX) != string::npos)
                sys_hostfile.insert(make_pair(tmp_list[i], tmp_list[0]));
        }
    }
    CTX->fs_conf()->sys_hostfile = sys_hostfile;
    CTX->log()->info("{}() /etc/hosts successfully mapped into ADA-FS", __func__);
    return true;
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
    auto daemon_addr = CTX->daemon_addr_str();
    auto last_separator = daemon_addr.find_last_of(';');
    if (last_separator != std::string::npos) {
        daemon_addr.erase(0, ++last_separator);
    }
    return margo_addr_lookup_retry(daemon_addr);
}

/*
 * Get the URI of a given hostname.
 *
 * This URI is to be used for margo lookup function.
 */
std::string get_uri_from_hostname(const std::string& hostname) {
    auto host = hostname + HOSTNAME_SUFFIX;
    // get the ip address from /etc/hosts which is mapped to the sys_hostfile map
    if (CTX->fs_conf()->sys_hostfile.count(host) == 1) {
        host = CTX->fs_conf()->sys_hostfile.at(host);
    }
    auto internal_protocol = ""s;
    if(RPC_PROTOCOL == "ofi+tcp"s) {
        internal_protocol = "fi_sockaddr_in://";
    }
    return fmt::format("{}://{}{}:{}", RPC_PROTOCOL, internal_protocol, host, CTX->fs_conf()->rpc_port);
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
        auto hostname = CTX->fs_conf()->hosts.at(host);
        auto uri = get_uri_from_hostname(hostname);
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
