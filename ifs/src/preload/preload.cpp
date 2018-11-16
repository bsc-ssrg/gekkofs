#include <global/log_util.hpp>
#include <global/path_util.hpp>
#include <global/global_defs.hpp>
#include <global/configure.hpp>
#include <preload/preload.hpp>
#include <preload/resolve.hpp>
#include <global/rpc/distributor.hpp>
#include "global/rpc/rpc_types.hpp"
#include <preload/rpc/ld_rpc_management.hpp>
#include <preload/passthrough.hpp>
#include <preload/preload_util.hpp>
#include "preload/rpc/engine.hpp"

#include <fstream>

// thread to initialize the whole margo shazaam only once per process
static pthread_once_t init_env_thread = PTHREAD_ONCE_INIT;

/**
 * This function is only called in the preload constructor and initializes Argobots and Margo clients
 */
void init_ld_environment_() {
    auto rpc_engine = std::make_shared<RPCEngine>(RPC_PROTOCOL, CTX->daemon_addr_str());
    CTX->rpc(rpc_engine);

    if (!rpc_send::get_fs_config()) {
        CTX->log()->error("{}() Unable to fetch file system configurations from daemon process through RPC.", __func__);
        exit(EXIT_FAILURE);
    }

    /* Setup distributor */
    auto simple_hash_dist = std::make_shared<SimpleHashDistributor>(CTX->fs_conf()->host_id, CTX->fs_conf()->host_size);
    CTX->distributor(simple_hash_dist);

    if (!read_system_hostfile()) {
        CTX->log()->error("{}() Unable to read system hostfile /etc/hosts for address mapping.", __func__);
        exit(EXIT_FAILURE);
    }

    lookup_all_hosts();

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
        throw std::runtime_error("Unable to open cmdline file");
    }
    if(!getline(cmdline, line)) {
        throw std::runtime_error("Unable to read cmdline file");
    }
    CTX->log()->info("Command to itercept: '{}'", line);
    cmdline.close();
}

/**
 * Called initially ONCE when preload library is used with the LD_PRELOAD environment variable
 */
void init_preload() {
    init_passthrough_if_needed();
    init_logging();
    CTX->log()->debug("Initialized logging subsystem");
    log_prog_name();
    init_cwd();
    CTX->log()->debug("Current working directory: '{}'", CTX->cwd());
    if (get_daemon_pid() == -1 || CTX->mountdir().empty()) {
        std::cerr << "ADA-FS daemon not running or mountdir could not be loaded. Check adafs_preload.log" << std::endl;
        CTX->log()->error("{}() Daemon not running or mountdir not set", __func__);
        exit(EXIT_FAILURE);
    } else {
        CTX->log()->info("{}() mountdir '{}' loaded", __func__, CTX->mountdir());
    }
    CTX->initialized(true);
    CTX->log()->debug("{}() exit", __func__);
}

/**
 * Called last when preload library is used with the LD_PRELOAD environment variable
 */
void destroy_preload() {
    CTX->log()->info("Client shutdown completed");
}
