/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  SPDX-License-Identifier: MIT
*/


#include <daemon/main.hpp>
#include "version.hpp"
#include <global/log_util.hpp>
#include <global/rpc/rpc_types.hpp>
#include <global/rpc/rpc_utils.hpp>
#include <global/rpc/distributor.hpp>
#include <daemon/handler/rpc_defs.hpp>
#include <daemon/ops/metadentry.hpp>
#include <daemon/backend/metadata/db.hpp>
#include <daemon/backend/data/chunk_storage.hpp>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>

#include <iostream>
#include <fstream>
#include <csignal>

#include <condition_variable>
#include <global/global_func.hpp>

using namespace std;
namespace po = boost::program_options;
namespace bfs = boost::filesystem;

static condition_variable shutdown_please;
static mutex mtx;

bool init_environment() {
    // Initialize metadata db
    std::string metadata_path = ADAFS_DATA->metadir() + "/rocksdb"s;
    try {
        ADAFS_DATA->mdb(std::make_shared<MetadataDB>(metadata_path));
    } catch (const std::exception & e) {
        ADAFS_DATA->spdlogger()->error("{}() unable to initialize metadata DB: {}", __func__, e.what());
        return false;
    }

    // Initialize data backend
    std::string chunk_storage_path = ADAFS_DATA->rootdir() + "/data/chunks"s;
    ADAFS_DATA->spdlogger()->debug("{}() Creating chunk storage directory: '{}'", __func__, chunk_storage_path);
    bfs::create_directories(chunk_storage_path);
    try {
        ADAFS_DATA->storage(std::make_shared<ChunkStorage>(chunk_storage_path, CHUNKSIZE));
    } catch (const std::exception & e) {
        ADAFS_DATA->spdlogger()->error("{}() unable to initialize storage backend: {}", __func__, e.what());
    }

    // Init margo for RPC
    if (!init_rpc_server()) {
        ADAFS_DATA->spdlogger()->error("{}() unable to initialize margo rpc server.", __func__);
        return false;
    }

    // Init Argobots ESs to drive IO
    if (!init_io_tasklet_pool()) {
        ADAFS_DATA->spdlogger()->error("{}() Unable to initialize Argobots pool for I/O.", __func__);
        return false;
    }

    /* Setup distributor */
    auto simple_hash_dist = std::make_shared<SimpleHashDistributor>(ADAFS_DATA->host_id(), ADAFS_DATA->host_size());
    ADAFS_DATA->distributor(simple_hash_dist);

    // TODO set metadata configurations. these have to go into a user configurable file that is parsed here
    ADAFS_DATA->atime_state(MDATA_USE_ATIME);
    ADAFS_DATA->mtime_state(MDATA_USE_MTIME);
    ADAFS_DATA->ctime_state(MDATA_USE_CTIME);
    ADAFS_DATA->link_cnt_state(MDATA_USE_LINK_CNT);
    ADAFS_DATA->blocks_state(MDATA_USE_BLOCKS);
    // Create metadentry for root directory
    Metadata root_md{S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO};
    try {
        create_metadentry("/", root_md);
    } catch (const std::exception& e ) {
        ADAFS_DATA->spdlogger()->error("{}() Unable to write root metadentry to KV store: {}", __func__, e.what());
        return false;
    }

    try {
        // Register daemon to system
        register_daemon_proc();
    } catch (const std::exception& e ) {
        cout << "ERROR: failed to register daemon process: " << e.what() << endl;
        ADAFS_DATA->spdlogger()->error("{}() Unable to register the daemon process to the system: {}", __func__, e.what());
        return false;
    }

    try {
        if (!ADAFS_DATA->lookup_file().empty()) {
            populate_lookup_file();
        }
    } catch (const std::exception& e ) {
        cout << "ERROR: failed to populate lookup file: " << e.what() << endl;
        ADAFS_DATA->spdlogger()->error("{}() failed to populate lookup file: {}", __func__, e.what());
        return false;
    }

    ADAFS_DATA->spdlogger()->info("Startup successful. Daemon is ready.");
    return true;
}

/**
 * Destroys the margo, argobots, and mercury environments
 */
void destroy_enviroment() {
#ifdef MARGODIAG
    cout << "\n####################\n\nMargo RPC server stats: " << endl;
    margo_diag_dump(RPC_DATA->server_rpc_mid(), "-", 0);
#endif
    bfs::remove_all(ADAFS_DATA->mountdir());
    for (unsigned int i = 0; i < RPC_DATA->io_streams().size(); i++) {
        ABT_xstream_join(RPC_DATA->io_streams().at(i));
        ABT_xstream_free(&RPC_DATA->io_streams().at(i));
    }
    ADAFS_DATA->spdlogger()->info("{}() Freeing I/O executions streams successful", __func__);
    if (!deregister_daemon_proc())
        ADAFS_DATA->spdlogger()->warn("{}() Unable to clean up auxiliary files", __func__);
    else
        ADAFS_DATA->spdlogger()->debug("{}() Cleaning auxiliary files successful", __func__);

    if (!ADAFS_DATA->lookup_file().empty()) {
        destroy_lookup_file();
    }

    if (RPC_DATA->server_rpc_mid() != nullptr) {
        margo_finalize(RPC_DATA->server_rpc_mid());
        ADAFS_DATA->spdlogger()->info("{}() Margo RPC server shut down successful", __func__);
    }

    ADAFS_DATA->spdlogger()->info("{}() Closing DB...", __func__);
    ADAFS_DATA->close_mdb();

    ADAFS_DATA->spdlogger()->info("Shutdown complete");
}

bool init_io_tasklet_pool() {
    assert(DAEMON_IO_XSTREAMS >= 0);
    unsigned int xstreams_num = DAEMON_IO_XSTREAMS;

    //retrieve the pool of the just created scheduler
    ABT_pool pool;
    auto ret = ABT_pool_create_basic(ABT_POOL_FIFO_WAIT, ABT_POOL_ACCESS_MPMC, ABT_TRUE, &pool);
    if (ret != ABT_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to create I/O tasks pool", __func__);
        return false;
    }

    //create all subsequent xstream and the associated scheduler, all tapping into the same pool
    vector<ABT_xstream> xstreams(xstreams_num);
    for (unsigned int i = 0; i < xstreams_num; ++i) {
        ret = ABT_xstream_create_basic(ABT_SCHED_BASIC_WAIT, 1, &pool,
                ABT_SCHED_CONFIG_NULL, &xstreams[i]);
        if (ret != ABT_SUCCESS) {
            ADAFS_DATA->spdlogger()->error("{}() Failed to create task execution streams for I/O operations", __func__);
            return false;
        }
    }

    RPC_DATA->io_streams(xstreams);
    RPC_DATA->io_pool(pool);
    return true;
}

bool init_rpc_server() {
    auto protocol_port = fmt::format("{}://{}:{}", RPC_PROTOCOL, ADAFS_DATA->rpc_addr(), ADAFS_DATA->rpc_port());
    hg_addr_t addr_self;
    hg_size_t addr_self_cstring_sz = 128;
    char addr_self_cstring[128];
    ADAFS_DATA->spdlogger()->debug("{}() Initializing Margo RPC server...", __func__);
    // IMPORTANT: this struct needs to be zeroed before use
    struct hg_init_info hg_options = {};
    hg_options.auto_sm = HG_TRUE;
    hg_options.stats = HG_FALSE;
    hg_options.na_class = nullptr;
    // Start Margo (this will also initialize Argobots and Mercury internally)
    auto mid = margo_init_opt(protocol_port.c_str(),
                              MARGO_SERVER_MODE,
                              &hg_options,
                              HG_TRUE,
                              DAEMON_RPC_HANDLER_XSTREAMS);
    if (mid == MARGO_INSTANCE_NULL) {
        ADAFS_DATA->spdlogger()->error("{}() margo_init failed to initialize the Margo RPC server", __func__);
        return false;
    }
#ifdef MARGODIAG
    margo_diag_start(mid);
#endif
    // Figure out what address this server is listening on (must be freed when finished)
    auto hret = margo_addr_self(mid, &addr_self);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() margo_addr_self() Failed to retrieve server RPC address", __func__);
        margo_finalize(mid);
        return false;
    }
    // Convert the address to a cstring (with \0 terminator).
    hret = margo_addr_to_string(mid, addr_self_cstring, &addr_self_cstring_sz, addr_self);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() margo_addr_to_string Failed to convert address to cstring", __func__);
        margo_addr_free(mid, addr_self);
        margo_finalize(mid);
        return false;
    }
    margo_addr_free(mid, addr_self);

    std::string addr_self_str(addr_self_cstring);
    RPC_DATA->self_addr_str(addr_self_str);

    ADAFS_DATA->spdlogger()->info("{}() Margo RPC server initialized. Accepting RPCs on address {}", __func__,
                                  addr_self_cstring);

    // Put context and class into RPC_data object
    RPC_DATA->server_rpc_mid(mid);

    // register RPCs
    register_server_rpcs(mid);

    return true;
}

/**
 * Registers RPC handlers to Margo instance
 * @param hg_class
 */
void register_server_rpcs(margo_instance_id mid) {
    MARGO_REGISTER(mid, hg_tag::fs_config, rpc_config_in_t, rpc_config_out_t, rpc_srv_fs_config);
    MARGO_REGISTER(mid, hg_tag::create, rpc_mk_node_in_t, rpc_err_out_t, rpc_srv_mk_node);
    MARGO_REGISTER(mid, hg_tag::stat, rpc_path_only_in_t, rpc_stat_out_t, rpc_srv_stat);
    MARGO_REGISTER(mid, hg_tag::decr_size, rpc_trunc_in_t, rpc_err_out_t, rpc_srv_decr_size);
    MARGO_REGISTER(mid, hg_tag::remove, rpc_rm_node_in_t, rpc_err_out_t, rpc_srv_rm_node);
    MARGO_REGISTER(mid, hg_tag::update_metadentry, rpc_update_metadentry_in_t, rpc_err_out_t,
                   rpc_srv_update_metadentry);
    MARGO_REGISTER(mid, hg_tag::get_metadentry_size, rpc_path_only_in_t, rpc_get_metadentry_size_out_t,
                   rpc_srv_get_metadentry_size);
    MARGO_REGISTER(mid, hg_tag::update_metadentry_size, rpc_update_metadentry_size_in_t,
                   rpc_update_metadentry_size_out_t, rpc_srv_update_metadentry_size);
    MARGO_REGISTER(mid, hg_tag::get_dirents, rpc_get_dirents_in_t, rpc_get_dirents_out_t,
                   rpc_srv_get_dirents);
#ifdef HAS_SYMLINKS
    MARGO_REGISTER(mid, hg_tag::mk_symlink, rpc_mk_symlink_in_t, rpc_err_out_t, rpc_srv_mk_symlink);
#endif
    MARGO_REGISTER(mid, hg_tag::write_data, rpc_write_data_in_t, rpc_data_out_t, rpc_srv_write_data);
    MARGO_REGISTER(mid, hg_tag::read_data, rpc_read_data_in_t, rpc_data_out_t, rpc_srv_read_data);
    MARGO_REGISTER(mid, hg_tag::trunc_data, rpc_trunc_in_t, rpc_err_out_t, rpc_srv_trunc_data);
    MARGO_REGISTER(mid, hg_tag::chunk_stat, rpc_chunk_stat_in_t, rpc_chunk_stat_out_t, rpc_srv_chunk_stat);
}

/**
 * Registers the daemon process to the system.
 * This will create a file with additional information for clients started on the same node.
 * @return
 */
void register_daemon_proc() {
    auto daemon_aux_path = DAEMON_AUX_PATH;
    if (!bfs::exists(daemon_aux_path) && !bfs::create_directories(daemon_aux_path)) {
        throw runtime_error(fmt::format("Unable to create adafs auxiliary directory in {}", daemon_aux_path));
    }

    auto pid_file = daemon_pid_path();
    // check if a pid file exists from another daemon
    if (bfs::exists(pid_file)) {
        throw runtime_error(
                fmt::format("Pid file already exists, "
                            "probably another daemon is still running. pid file: '{}'",
                            pid_file));
    }

    auto my_pid = getpid();
    if (my_pid == -1) {
        throw runtime_error("Unable to get process ID");
    }

    ofstream ofs(pid_file, ::ofstream::trunc);
    if (!ofs) {
        throw runtime_error(
                fmt::format("Unable to create daemon pid file: '{}'", pid_file));
    }
    ofs << to_string(my_pid) << std::endl;
    ofs << RPC_DATA->self_addr_str() << std::endl;
    ofs << ADAFS_DATA->mountdir() << std::endl;
    ofs.close();
}

bool deregister_daemon_proc() {
    return bfs::remove(daemon_pid_path());
}

void populate_lookup_file() {
    ofstream lfstream(ADAFS_DATA->lookup_file(), ios::out | ios::app);
    lfstream <<
        get_my_hostname(true) + " "s + RPC_DATA->self_addr_str() << std::endl;
}

void destroy_lookup_file() {
    std::remove(ADAFS_DATA->lookup_file().c_str());
}

void shutdown_handler(int dummy) {
    shutdown_please.notify_all();
}

void initialize_loggers() {
    std::string path = DEFAULT_DAEMON_LOG_PATH;
    // Try to get log path from env variable
    std::string env_path_key = ENV_PREFIX;
    env_path_key += "DAEMON_LOG_PATH";
    char* env_path = getenv(env_path_key.c_str());
    if (env_path != nullptr) {
        path = env_path;
    }

    spdlog::level::level_enum level = get_spdlog_level(DEFAULT_DAEMON_LOG_LEVEL);
    // Try to get log path from env variable
    std::string env_level_key = ENV_PREFIX;
    env_level_key += "LOG_LEVEL";
    char* env_level = getenv(env_level_key.c_str());
    if (env_level != nullptr) {
        level = get_spdlog_level(env_level);
    }

    auto logger_names = std::vector<std::string>{
        "main",
        "MetadataDB",
        "ChunkStorage",
    };

    setup_loggers(logger_names, level, path);
}

int main(int argc, const char* argv[]) {

    initialize_loggers();
    ADAFS_DATA->spdlogger(spdlog::get("main"));

    // Parse input
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help,h", "Help message")
            ("mountdir,m", po::value<string>()->required(), "User Fuse mountdir")
            ("rootdir,r", po::value<string>()->required(), "data directory")
            ("metadir,i", po::value<string>(), "metadata directory, if not set rootdir is used for metadata ")
            ("listen,l", po::value<string>(), "Address or interface to bind the daemon on. Default: local hostname")
            ("port,p", po::value<unsigned int>()->default_value(DEFAULT_RPC_PORT), "Port to bind the server on. Default: 4433")
            ("hostfile", po::value<string>(), "Path to the hosts_file for all fs participants")
            ("hosts,h", po::value<string>(), "Comma separated list of hosts_ for all fs participants")
            ("hostname-suffix,s", po::value<string>()->default_value(""), "Suffix that is added to each given host. Consult /etc/hosts for allowed suffixes")
            ("lookup-file,k", po::value<string>(), "Shared file used by deamons to register their enpoints. (Needs to be on a shared filesystem)")
            ("version,h", "print version and exit");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    if (vm.count("version")) {
        cout << GKFS_VERSION_STRING << endl;
#ifndef NDEBUG
        cout << "Debug: ON" << endl;
#else
        cout << "Debug: OFF" << endl;
#endif
        cout << "RPC protocol: " << RPC_PROTOCOL << endl;
        cout << "Chunk size: " << CHUNKSIZE << " bytes" << endl;
        return 0;
    }

    try {
        po::notify(vm);
    } catch (po::required_option& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    ADAFS_DATA->hostname_suffix(vm["hostname-suffix"].as<string>());

    if (vm.count("listen")) {
        ADAFS_DATA->rpc_addr(vm["listen"].as<string>());
    } else {
        ADAFS_DATA->rpc_addr(get_my_hostname(true) + ADAFS_DATA->hostname_suffix());
    }
    ADAFS_DATA->rpc_port(vm["port"].as<unsigned int>());

    // parse host parameters
    vector<string> hosts{};
    if (vm.count("hostfile")) {
        auto host_path = vm["hostfile"].as<string>();
        fstream host_file(host_path);
        if (host_file.is_open()) {
            for (string line; getline(host_file, line);) {
                if (line.at(0) != '#') {
                    auto subline = line.substr(0, line.find(' '));
                    hosts.push_back(subline);
                }
            }
        } else {
            cerr << "Hostfile path does not exist. Exiting ..." << endl;
            ADAFS_DATA->spdlogger()->error("{}() Hostfile path does not exist. Exiting ...", __func__);
            assert(host_file.is_open());
        }
    } else if (vm.count("hosts")) {
        // split comma separated host string
        boost::char_separator<char> sep(",");
        boost::tokenizer<boost::char_separator<char>> tok(vm["hosts"].as<string>(), sep);
        for (auto&& s : tok) {
            hosts.push_back(s);
        }
    }
    // convert host parameters into datastructures
    std::map<uint64_t, std::string> hostmap;
    auto hosts_raw = ""s;
    if (!hosts.empty()) {
        auto i = static_cast<uint64_t>(0);
        auto found_hostname = false;
        auto hostname = get_my_hostname(true);
        if (hostname.empty()) {
            cerr << "Unable to read the machine's hostname" << endl;
            ADAFS_DATA->spdlogger()->error("{}() Unable to read the machine's hostname", __func__);
            assert(!hostname.empty());
        }
        for (auto&& host : hosts) {
            hostmap[i] = host;
            hosts_raw += host + ","s;
            if (hostname == host) {
                ADAFS_DATA->host_id(i);
                found_hostname = true;
            }
            i++;
        }
        if (!found_hostname) {
            ADAFS_DATA->spdlogger()->error("{}() Hostname was not found in given parameters. Exiting ...", __func__);
            cerr << "Hostname was not found in given parameters. Exiting ..." << endl;
            assert(found_hostname);
        }
        hosts_raw = hosts_raw.substr(0, hosts_raw.size() - 1);
    } else {
        // single node mode
        ADAFS_DATA->spdlogger()->info("{}() Single node mode set to self", __func__);
        auto hostname = get_my_hostname(false);
        hostmap[0] = hostname;
        hosts_raw = hostname;
        ADAFS_DATA->host_id(0);
    }

    if (vm.count("lookup-file")) {
        ADAFS_DATA->lookup_file(vm["lookup-file"].as<string>());
    }

    ADAFS_DATA->hosts(hostmap);
    ADAFS_DATA->host_size(hostmap.size());
    ADAFS_DATA->hosts_raw(hosts_raw);

    ADAFS_DATA->spdlogger()->info("{}() Initializing environment. Hold on ...", __func__);

    assert(vm.count("mountdir"));
    auto mountdir = vm["mountdir"].as<string>();
    // Create mountdir. We use this dir to get some information on the underlying fs with statfs in adafs_statfs
    bfs::create_directories(mountdir);
    ADAFS_DATA->mountdir(bfs::canonical(mountdir).native());

    assert(vm.count("rootdir"));
    auto rootdir = vm["rootdir"].as<string>();
    bfs::create_directories(rootdir);
    ADAFS_DATA->rootdir(bfs::canonical(rootdir).native());

    if (vm.count("metadir")) {
        auto metadir = vm["metadir"].as<string>();
        bfs::create_directories(metadir);
        ADAFS_DATA->metadir(bfs::canonical(metadir).native());
    } else {
        // use rootdir as metadata dir
        ADAFS_DATA->metadir(ADAFS_DATA->rootdir());
    }

    if (init_environment()) {
        signal(SIGINT, shutdown_handler);
        signal(SIGTERM, shutdown_handler);
        signal(SIGKILL, shutdown_handler);

        unique_lock<mutex> lk(mtx);
        // Wait for shutdown signal to initiate shutdown protocols
        shutdown_please.wait(lk);
        ADAFS_DATA->spdlogger()->info("{}() Shutting done signal encountered. Shutting down ...", __func__);
    } else {
        ADAFS_DATA->spdlogger()->info("{}() Starting up daemon environment failed. Shutting down ...", __func__);
    }

    destroy_enviroment();

    return 0;

}
