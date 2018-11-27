
#include <daemon/adafs_ops/metadentry.hpp>
#include <daemon/backend/metadata/db.hpp>
#include <daemon/backend/data/chunk_storage.hpp>

using namespace std;

/**
 * Creates metadata (if required) and dentry at the same time
 * @param path
 * @param mode
 */
void create_metadentry(const std::string& path, Metadata& md) {

    // update metadata object based on what metadata is needed
    md.fuid(ADAFS_DATA->fuids_manager()->generate_uid());
#ifndef NDEBUG
    ADAFS_DATA->spdlogger()->debug("Generated fuid: '{}", md.fuid());
#endif
    if (ADAFS_DATA->atime_state() || ADAFS_DATA->mtime_state() || ADAFS_DATA->ctime_state()) {
        std::time_t time;
        std::time(&time);
        auto time_s = fmt::format_int(time).str();
        if (ADAFS_DATA->atime_state())
            md.atime(time);
        if (ADAFS_DATA->mtime_state())
            md.mtime(time);
        if (ADAFS_DATA->ctime_state())
            md.ctime(time);
    }
    if (ADAFS_DATA->uid_state())
        md.uid(getuid());
    if (ADAFS_DATA->gid_state())
        md.gid(getgid());

    ADAFS_DATA->mdb()->put(path, md.serialize());
}

void insert_metadentry_str(const std::string& path, const std::string& serialized_metadata) {
#ifndef NDEBUG
    Metadata md(serialized_metadata);
    assert(md.fuid() != FUID_NULL);
#endif
    ADAFS_DATA->mdb()->put(path, serialized_metadata);
}

std::string get_metadentry_str(const std::string& path) {
        return ADAFS_DATA->mdb()->get(path);
}

/**
 * Returns the metadata of an object at a specific path. The metadata can be of dummy values if configured
 * @param path
 * @param attr
 * @return
 */
Metadata get_metadentry(const std::string& path) {
    return Metadata(get_metadentry_str(path));
}

/**
 * Remove metadentry if exists and try to remove all chunks for path
 * @param path
 * @return
 */
void remove_node(const string& path) {
    ADAFS_DATA->mdb()->remove(path); // remove metadentry
}

/**
 * Gets the size of a metadentry
 * @param path
 * @param ret_size (return val)
 * @return err
 */
size_t get_metadentry_size(const string& path) {
    return get_metadentry(path).size();
}

/**
 * Updates a metadentry's size atomically and returns the corresponding size after update
 * @param path
 * @param io_size
 * @return the updated size
 */
void update_metadentry_size(const string& path, size_t io_size, off64_t offset, bool append) {
#ifdef LOG_TRACE
    ADAFS_DATA->mdb()->iterate_all();
#endif
    ADAFS_DATA->mdb()->increase_size(path, io_size + offset, append);
#ifdef LOG_TRACE
    ADAFS_DATA->mdb()->iterate_all();
#endif
}

void update_metadentry(const string& path, Metadata& md) {
    ADAFS_DATA->mdb()->update(path, path, md.serialize());
}

/**
 * @param path of the object whose permissions are checked
 * @param mask single bit mask to check against
 * @return errno
 */
int check_access_mask(const string& path, const int mask) {
    try {
        Metadata md = get_metadentry(path);
        /*
         * if only check if file exists is wanted, return success.
         * According to POSIX (access(2)) the mask is either the value F_OK,
         * or a mask consisting of the bitwise OR of one or more of R_OK, W_OK, and X_OK.
         */
        if (mask == F_OK)
            return 0;

        // root user is a god
        if (ADAFS_DATA->uid_state() && md.uid() == 0)
            return 0;

        // We do not check for the actual user here, because the cluster batchsystem should take care of it
        // check user leftmost 3 bits for rwx in md->mode
        if (ADAFS_DATA->uid_state()) {
            // Because mode comes only with the first 3 bits used, the user bits have to be shifted to the right to compare
            if ((mask & md.mode() >> 6) == static_cast<unsigned int>(mask))
                return 0;
            else
                return EACCES;
        }

        // check group middle 3 bits for rwx in md->mode
        if (ADAFS_DATA->gid_state()) {
            if ((mask & md.mode() >> 3) == static_cast<unsigned int>(mask))
                return 0;
            else
                return EACCES;
        }

        // check other rightmost 3 bits for rwx in md->mode.
        // Because they are the rightmost bits they don't need to be shifted
        if ((mask & md.mode()) == static_cast<unsigned int>(mask)) {
            return 0;
        }

    } catch (const NotFoundException& e) {
        return ENOENT;
    }
    return EACCES;
}

std::vector<std::pair<std::string, bool>> get_dirents(const std::string& dir){
    return ADAFS_DATA->mdb()->get_dirents(dir);
}