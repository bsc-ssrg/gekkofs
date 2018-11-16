#include <preload/preload_util.hpp>
#include <global/rpc/distributor.hpp>
#include <global/global_func.hpp>
#include "preload/rpc/engine.hpp"

#include <fstream>
#include <sstream>
#include <csignal>
#include <random>
#include <sys/sysmacros.h>

using namespace std;

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
    attr.st_ino = std::hash<std::string>{}(path);
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
    return fmt::format("{}://{}:{}", RPC_PROTOCOL, host, CTX->fs_conf()->rpc_port);
}

void lookup_all_hosts() {
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
    string remote_uri;
    for (auto& host : hosts) {
        // If local use address provided by daemon in order to use automatic shared memory routing
        if (host == CTX->fs_conf()->host_id) {
            remote_uri = CTX->daemon_addr_str();
        } else {
            auto hostname = CTX->fs_conf()->hosts.at(host);
            remote_uri = get_uri_from_hostname(hostname);
        }
        CTX->rpc()->insert_endpoint(host, remote_uri);
    }
}
