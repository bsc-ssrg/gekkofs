#include <global/log_util.hpp>
#include <global/path_util.hpp>
#include <global/global_defs.hpp>
#include <global/configure.hpp>
#include <preload/preload.hpp>
#include <preload/resolve.hpp>
#include <global/rpc/ipc_types.hpp>
#include <global/rpc/distributor.hpp>
#include <preload/margo_ipc.hpp>
#include <preload/rpc/ld_rpc_data_ws.hpp>
#include <preload/preload_util.hpp>
#include <preload/intercept.hpp>

#include <fstream>

enum class Margo_mode {
    RPC, IPC
};

// thread to initialize the whole margo shazaam only once per process
static pthread_once_t init_env_thread = PTHREAD_ONCE_INIT;

// external variables that are initialized here
// IPC IDs
hg_id_t ipc_minimal_id;
hg_id_t ipc_config_id;
hg_id_t ipc_mk_node_id;
hg_id_t ipc_mk_symlink_id;
hg_id_t ipc_access_id;
hg_id_t ipc_stat_id;
hg_id_t ipc_rm_node_id;
hg_id_t ipc_decr_size_id;
hg_id_t ipc_update_metadentry_id;
hg_id_t ipc_get_metadentry_size_id;
hg_id_t ipc_update_metadentry_size_id;
hg_id_t ipc_write_data_id;
hg_id_t ipc_read_data_id;
hg_id_t ipc_trunc_data_id;
hg_id_t ipc_get_dirents_id;
hg_id_t ipc_chunk_stat_id;
// RPC IDs
hg_id_t rpc_minimal_id;
hg_id_t rpc_mk_node_id;
hg_id_t rpc_mk_symlink_id;
hg_id_t rpc_access_id;
hg_id_t rpc_stat_id;
hg_id_t rpc_rm_node_id;
hg_id_t rpc_decr_size_id;
hg_id_t rpc_update_metadentry_id;
hg_id_t rpc_get_metadentry_size_id;
hg_id_t rpc_update_metadentry_size_id;
hg_id_t rpc_write_data_id;
hg_id_t rpc_read_data_id;
hg_id_t rpc_trunc_data_id;
hg_id_t rpc_get_dirents_id;
hg_id_t rpc_chunk_stat_id;
// Margo instances
margo_instance_id ld_margo_ipc_id;
margo_instance_id ld_margo_rpc_id;
// local daemon IPC address
hg_addr_t daemon_svr_addr = HG_ADDR_NULL;

/**
 * Initializes the Argobots environment
 * @return
 */
bool init_ld_argobots() {
    CTX->log()->debug("{}() Initializing Argobots ...", __func__);

    /*
     * Single producer (progress function) and multiple consumers are causing an excess memory consumption
     * in some Argobots version. It does only show if an ES with a pool is created.
     * Although this is probably not an issue on the consumer process, we set reduce the Argobots stack size here,
     * just in case we change the client process in the future.
     * See for reference: https://xgitlab.cels.anl.gov/sds/margo/issues/40
     */
    putenv(const_cast<char*>("ABT_MEM_MAX_NUM_STACKS=8"));

    // We need no arguments to init
    auto err = ABT_init(0, nullptr);
    if (err != ABT_SUCCESS) {
        CTX->log()->error("{}() failed to init Argobots environment", __func__);
        return false;
    }

    ABT_xstream self_xstream;
    err = ABT_xstream_self(&self_xstream);
    if (err != ABT_SUCCESS) {
        CTX->log()->error("{}() failed to get argobots self xstream", __func__);
        return false;
    }

    err = ABT_xstream_set_main_sched_basic(self_xstream, ABT_SCHED_BASIC_WAIT, 1, nullptr);
    if (err != ABT_SUCCESS) {
        CTX->log()->error("{}() failed to set scheduler for main threads pool", __func__);
        return false;
    }

    CTX->log()->debug("{}() Argobots initialization successful.", __func__);
    return true;
}

/**
 * Registers a margo instance with all used RPC, differentiating between IPC and RPC client
 * Note that the rpc tags are redundant for rpc and ipc ids
 * @param mid
 * @param mode
 */
void register_client_rpcs(margo_instance_id mid, Margo_mode mode) {
    if (mode == Margo_mode::IPC) {
        // IPC IDs
        ipc_config_id = MARGO_REGISTER(mid, hg_tag::fs_config, ipc_config_in_t, ipc_config_out_t,
                                       NULL);
        ipc_minimal_id = MARGO_REGISTER(mid, hg_tag::minimal, rpc_minimal_in_t, rpc_minimal_out_t, NULL);
        ipc_mk_node_id = MARGO_REGISTER(mid, hg_tag::create, rpc_mk_node_in_t, rpc_err_out_t, NULL);
#ifdef HAS_SYMLINKS
        ipc_mk_symlink_id = MARGO_REGISTER(mid, hg_tag::mk_symlink, rpc_mk_symlink_in_t, rpc_err_out_t, NULL);
#endif
        ipc_access_id = MARGO_REGISTER(mid, hg_tag::access, rpc_access_in_t, rpc_err_out_t, NULL);
        ipc_stat_id = MARGO_REGISTER(mid, hg_tag::stat, rpc_path_only_in_t, rpc_stat_out_t, NULL);
        ipc_rm_node_id = MARGO_REGISTER(mid, hg_tag::remove, rpc_rm_node_in_t,
                                        rpc_err_out_t, NULL);
        ipc_decr_size_id = MARGO_REGISTER(mid, hg_tag::decr_size, rpc_trunc_in_t, rpc_err_out_t,
                                          NULL);
        ipc_update_metadentry_id = MARGO_REGISTER(mid, hg_tag::update_metadentry, rpc_update_metadentry_in_t,
                                                  rpc_err_out_t, NULL);
        ipc_get_metadentry_size_id = MARGO_REGISTER(mid, hg_tag::get_metadentry_size, rpc_path_only_in_t,
                                                    rpc_get_metadentry_size_out_t, NULL);
        ipc_update_metadentry_size_id = MARGO_REGISTER(mid, hg_tag::update_metadentry_size,
                                                       rpc_update_metadentry_size_in_t,
                                                       rpc_update_metadentry_size_out_t,
                                                       NULL);
        ipc_write_data_id = MARGO_REGISTER(mid, hg_tag::write_data, rpc_write_data_in_t, rpc_data_out_t,
                                           NULL);
        ipc_read_data_id = MARGO_REGISTER(mid, hg_tag::read_data, rpc_read_data_in_t, rpc_data_out_t,
                                          NULL);
        ipc_trunc_data_id = MARGO_REGISTER(mid, hg_tag::trunc_data, rpc_trunc_in_t, rpc_err_out_t,
                                          NULL);
        ipc_get_dirents_id = MARGO_REGISTER(mid, hg_tag::get_dirents, rpc_get_dirents_in_t, rpc_get_dirents_out_t,
                                          NULL);
        ipc_chunk_stat_id = MARGO_REGISTER(mid, hg_tag::chunk_stat, rpc_minimal_in_t, rpc_chunk_stat_out_t,
                                          NULL);
    } else {
        // RPC IDs
        rpc_minimal_id = MARGO_REGISTER(mid, hg_tag::minimal, rpc_minimal_in_t, rpc_minimal_out_t, NULL);
        rpc_mk_node_id = MARGO_REGISTER(mid, hg_tag::create, rpc_mk_node_in_t, rpc_err_out_t, NULL);
#ifdef HAS_SYMLINKS
        rpc_mk_symlink_id = MARGO_REGISTER(mid, hg_tag::mk_symlink, rpc_mk_symlink_in_t, rpc_err_out_t, NULL);
#endif
        rpc_access_id = MARGO_REGISTER(mid, hg_tag::access, rpc_access_in_t, rpc_err_out_t, NULL);
        rpc_stat_id = MARGO_REGISTER(mid, hg_tag::stat, rpc_path_only_in_t, rpc_stat_out_t, NULL);
        rpc_rm_node_id = MARGO_REGISTER(mid, hg_tag::remove, rpc_rm_node_in_t,
                                        rpc_err_out_t, NULL);
        rpc_decr_size_id = MARGO_REGISTER(mid, hg_tag::decr_size, rpc_trunc_in_t, rpc_err_out_t,
                                          NULL);
        rpc_update_metadentry_id = MARGO_REGISTER(mid, hg_tag::update_metadentry, rpc_update_metadentry_in_t,
                                                  rpc_err_out_t, NULL);
        rpc_get_metadentry_size_id = MARGO_REGISTER(mid, hg_tag::get_metadentry_size, rpc_path_only_in_t,
                                                    rpc_get_metadentry_size_out_t, NULL);
        rpc_update_metadentry_size_id = MARGO_REGISTER(mid, hg_tag::update_metadentry_size,
                                                       rpc_update_metadentry_size_in_t,
                                                       rpc_update_metadentry_size_out_t,
                                                       NULL);
        rpc_write_data_id = MARGO_REGISTER(mid, hg_tag::write_data, rpc_write_data_in_t, rpc_data_out_t,
                                           NULL);
        rpc_read_data_id = MARGO_REGISTER(mid, hg_tag::read_data, rpc_read_data_in_t, rpc_data_out_t,
                                          NULL);
        rpc_trunc_data_id = MARGO_REGISTER(mid, hg_tag::trunc_data, rpc_trunc_in_t, rpc_err_out_t,
                                          NULL);
        rpc_get_dirents_id = MARGO_REGISTER(mid, hg_tag::get_dirents, rpc_get_dirents_in_t, rpc_get_dirents_out_t,
                                          NULL);
        rpc_chunk_stat_id = MARGO_REGISTER(mid, hg_tag::chunk_stat, rpc_minimal_in_t, rpc_chunk_stat_out_t,
                                          NULL);
    }
}

/**
 * Initializes the Margo client for a given na_plugin
 * @param mode
 * @param na_plugin
 * @return
 */
bool init_margo_client(Margo_mode mode, const string na_plugin) {

    ABT_xstream xstream = ABT_XSTREAM_NULL;
    ABT_pool pool = ABT_POOL_NULL;

    // get execution stream and its main pools
    auto ret = ABT_xstream_self(&xstream);
    if (ret != ABT_SUCCESS)
        return false;
    ret = ABT_xstream_get_main_pools(xstream, 1, &pool);
    if (ret != ABT_SUCCESS) return false;
    if (mode == Margo_mode::IPC)
        CTX->log()->debug("{}() Initializing Mercury IPC client ...", __func__);
    else
        CTX->log()->debug("{}() Initializing Mercury RPC client ...", __func__);
    /* MERCURY PART */
    // Init Mercury layer (must be finalized when finished)
    hg_class_t* hg_class;
    hg_context_t* hg_context;
    // Note: localhost should not be required and actually doesn't do anything. But it is required for OFI for Mercury to start
    hg_class = HG_Init((na_plugin + "://localhost"s).c_str(), HG_FALSE);
    if (hg_class == nullptr) {
        CTX->log()->error("{}() HG_Init() Failed to init Mercury client layer", __func__);
        return false;
    }
    // Create a new Mercury context (must be destroyed when finished)
    hg_context = HG_Context_create(hg_class);
    if (hg_context == nullptr) {
        CTX->log()->error("{}() HG_Context_create() Failed to create Mercury client context", __func__);
        HG_Finalize(hg_class);
        return false;
    }
    CTX->log()->debug("{}() Mercury initialized.", __func__);

    /* MARGO PART */
    if (mode == Margo_mode::IPC)
        CTX->log()->debug("{}() Initializing Margo IPC client ...", __func__);
    else
        CTX->log()->debug("{}() Initializing Margo RPC client ...", __func__);
    // margo will run in the context of thread
    auto mid = margo_init_pool(pool, pool, hg_context);
    if (mid == MARGO_INSTANCE_NULL) {
        CTX->log()->error("{}() margo_init_pool failed to initialize the Margo client", __func__);
        return false;
    }
    CTX->log()->debug("{}() Margo initialized.", __func__);

    if (mode == Margo_mode::IPC) {
        ld_margo_ipc_id = mid;
        auto adafs_daemon_pid = get_daemon_pid();
        if (adafs_daemon_pid == -1) {
            CTX->log()->error("{}() ADA-FS daemon not running. Exiting ...", __func__);
            return false;
        }
        CTX->log()->debug("{}() ADA-FS daemon with PID {} found.", __func__, adafs_daemon_pid);

        string sm_addr_str = "na+sm://"s + to_string(adafs_daemon_pid) + "/0";
        margo_addr_lookup(ld_margo_ipc_id, sm_addr_str.c_str(), &daemon_svr_addr);
    } else
        ld_margo_rpc_id = mid;

    register_client_rpcs(mid, mode);
#ifdef MARGODIAG
    margo_diag_start(mid);
#endif
    return true;
}

/**
 * This function is only called in the preload constructor and initializes Argobots and Margo clients
 */
void init_ld_environment_() {
    if (!init_ld_argobots()) {
        CTX->log()->error("{}() Unable to initialize Argobots.", __func__);
        exit(EXIT_FAILURE);
    }
    if (!init_margo_client(Margo_mode::IPC, "na+sm"s)) {
        CTX->log()->error("{}() Unable to initialize Margo IPC client.", __func__);
        exit(EXIT_FAILURE);
    }
    if (!ipc_send_get_fs_config()) {
        CTX->log()->error("{}() Unable to fetch file system configurations from daemon process through IPC.", __func__);
        exit(EXIT_FAILURE);
    }

    /* Setup distributor */
    auto simple_hash_dist = std::make_shared<SimpleHashDistributor>(CTX->fs_conf()->host_id, CTX->fs_conf()->host_size);
    CTX->distributor(simple_hash_dist);

    if (!init_margo_client(Margo_mode::RPC, RPC_PROTOCOL)) {
        CTX->log()->error("{}() Unable to initialize Margo RPC client.", __func__);
        exit(EXIT_FAILURE);
    }
    if (!read_system_hostfile()) {
        CTX->log()->error("{}() Unable to read system hostfile /etc/hosts for address mapping.", __func__);
        exit(EXIT_FAILURE);
    }
    if (!lookup_all_hosts()) {
        CTX->log()->error("{}() Unable to lookup all host RPC addresses.", __func__);
        exit(EXIT_FAILURE);
    }
    CTX->log()->info("{}() Environment initialization successful.", __func__);
}

void init_ld_env_if_needed() {
    pthread_once(&init_env_thread, init_ld_environment_);
}

void init_logging() {
    std::string path = DEFAULT_PRELOAD_LOG_PATH;
    // Try to get log path from env variable
    std::string env_key = ENV_PREFIX;
    env_key += "PRELOAD_LOG_PATH";
    char* env_log_path = getenv(env_key.c_str());
    if (env_log_path != nullptr) {
        path = env_log_path;
    }

    spdlog::level::level_enum level = get_spdlog_level(DEFAULT_DAEMON_LOG_LEVEL);
    // Try to get log path from env variable
    std::string env_level_key = ENV_PREFIX;
    env_level_key += "LOG_LEVEL";
    char* env_level = getenv(env_level_key.c_str());
    if (env_level != nullptr) {
        level = get_spdlog_level(env_level);
    }

    auto logger_names = std::vector<std::string> {"main"};

    setup_loggers(logger_names, level, path);

    CTX->log(spdlog::get(logger_names.at(0)));
}

void log_prog_name() {
    std::string line;
    std::ifstream cmdline("/proc/self/cmdline");
    if (!cmdline.is_open()) {
        CTX->log()->error("Unable to open cmdline file");
        throw runtime_error("Unable to open cmdline file");
    }
    if(!getline(cmdline, line)) {
        throw runtime_error("Unable to read cmdline file");
    }
    CTX->log()->info("Command to itercept: '{}'", line);
    cmdline.close();
}

/**
 * Called initially ONCE when preload library is used with the LD_PRELOAD environment variable
 */
void init_preload() {
    init_logging();
    CTX->log()->debug("Initialized logging subsystem");
    log_prog_name();
    init_cwd();
    CTX->log()->debug("Current working directory: '{}'", CTX->cwd());
    if (get_daemon_pid() == -1 || CTX->mountdir().empty()) {
        cerr << "ADA-FS daemon not running or mountdir could not be loaded. Check adafs_preload.log" << endl;
        CTX->log()->error("{}() Daemon not running or mountdir not set", __func__);
        exit(EXIT_FAILURE);
    } else {
        CTX->log()->info("{}() mountdir '{}' loaded", __func__, CTX->mountdir());
    }
    init_ld_env_if_needed();
    CTX->initialized(true);
    CTX->log()->debug("{}() exit", __func__);
    start_interception();
}

/**
 * Called last when preload library is used with the LD_PRELOAD environment variable
 */
void destroy_preload() {
    stop_interception();
    auto services_used = (ld_margo_ipc_id != nullptr || ld_margo_rpc_id != nullptr);
#ifdef MARGODIAG
    if (ld_margo_ipc_id != nullptr) {
        cout << "\n####################\n\nMargo IPC client stats: " << endl;
        margo_diag_dump(ld_margo_ipc_id, "-", 0);
    }
    if (ld_margo_rpc_id != nullptr) {
        cout << "\n####################\n\nMargo RPC client stats: " << endl;
        margo_diag_dump(ld_margo_rpc_id, "-", 0);
    }
#endif
    // Shut down RPC client if used
    if (ld_margo_rpc_id != nullptr) {
        // free all rpc addresses in LRU map and finalize margo rpc
        cleanup_addresses();
        CTX->log()->debug("{}() About to finalize the margo RPC client. Actually not doing it XXX", __func__);
        // XXX Sometimes this hangs on the cluster. Investigate.
        // Might been solved in margo 0.3. It is not an issue with Omnipath for sure. Maybe CCI only issue.
        margo_finalize(ld_margo_rpc_id);
        CTX->log()->debug("{}() Shut down Margo RPC client successful", __func__);
    }
    // Shut down IPC client if used
    if (ld_margo_ipc_id != nullptr) {
        CTX->log()->debug("{}() Freeing Margo IPC daemon svr address ...", __func__);
        if (margo_addr_free(ld_margo_ipc_id, daemon_svr_addr) != HG_SUCCESS)
            CTX->log()->warn("{}() Unable to free IPC client's daemon svr address.", __func__);
        CTX->log()->debug("{}() About to finalize the margo IPC client. Actually not doing it XXX", __func__);
        margo_finalize(ld_margo_ipc_id);
        CTX->log()->debug("{}() Shut down Margo IPC client successful", __func__);
    }
    if (services_used) {
        CTX->log()->info("All services shut down. Client shutdown complete.");
    }
    else
        CTX->log()->debug("{}() No services in preload library used. Nothing to shut down.", __func__);
}
