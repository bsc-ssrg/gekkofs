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

// thread to initialize the whole margo shazaam only once per process
static pthread_once_t init_env_thread = PTHREAD_ONCE_INIT;

// RPC IDs
hg_id_t rpc_config_id;
hg_id_t rpc_minimal_id;
hg_id_t rpc_mk_node_id;
hg_id_t rpc_insert_node_id;
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
margo_instance_id ld_margo_rpc_id;


/**
 * Registers a margo instance with all used RPC, differentiating between IPC and RPC client
 * Note that the rpc tags are redundant for rpc and ipc ids
 * @param mid
 * @param mode
 */
void register_client_rpcs(margo_instance_id mid) {
    rpc_config_id = MARGO_REGISTER(mid, hg_tag::fs_config, ipc_config_in_t, ipc_config_out_t,
                                   NULL);
    rpc_minimal_id = MARGO_REGISTER(mid, hg_tag::minimal, rpc_minimal_in_t, rpc_minimal_out_t, NULL);
    rpc_mk_node_id = MARGO_REGISTER(mid, hg_tag::create, rpc_mk_node_in_t, rpc_err_out_t, NULL);
    rpc_insert_node_id = MARGO_REGISTER(mid, hg_tag::insert, rpc_insert_node_in_t, rpc_err_out_t, NULL);
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

/**
 * Initializes the Margo client for a given na_plugin
 * @param mode
 * @param na_plugin
 * @return
 */
bool init_margo_client(const std::string& na_plugin) {
    // IMPORTANT: this struct needs to be zeroed before use
    struct hg_init_info hg_options = {};
    hg_options.auto_sm = HG_TRUE;
    hg_options.stats = HG_FALSE;
    hg_options.na_class = nullptr;

    // Note: localhost should not be required and actually doesn't do anything. But it is required for OFI for Mercury to start
    ld_margo_rpc_id = margo_init_info(na_plugin.c_str(), HG_FALSE, &hg_options, HG_FALSE, 1);
    if (ld_margo_rpc_id == MARGO_INSTANCE_NULL) {
        CTX->log()->error("{}() margo_init_pool failed to initialize the Margo client", __func__);
        return false;
    }
    register_client_rpcs(ld_margo_rpc_id);
#ifdef MARGODIAG
    margo_diag_start(ld_margo_rpc_id);
#endif
    return true;
}

/**
 * This function is only called in the preload constructor and initializes Argobots and Margo clients
 */
void init_ld_environment_() {
    //use rpc_addresses here to avoid "static initialization order problem"
    if (!init_margo_client(RPC_PROTOCOL)) {
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
    if (ld_margo_rpc_id == nullptr) {
        CTX->log()->debug("{}() No services in preload library used. Nothing to shut down.", __func__);
        return;
    }
#ifdef MARGODIAG
    std::cout << "\n####################\n\nMargo RPC client stats: " << std::endl;
    margo_diag_dump(ld_margo_rpc_id, "-", 0);
#endif
    cleanup_addresses();
    CTX->log()->debug("{}() About to finalize the margo RPC client", __func__);
    // XXX Sometimes this hangs on the cluster. Investigate.
    // Might been solved in margo 0.3. It is not an issue with Omnipath for sure. Maybe CCI only issue.
    margo_finalize(ld_margo_rpc_id);
    CTX->log()->debug("{}() Shut down Margo RPC client successful", __func__);
    CTX->log()->info("All services shut down. Client shutdown complete.");
}
