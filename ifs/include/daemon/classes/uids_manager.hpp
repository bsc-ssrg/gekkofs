#ifndef FS_UIDS_MANAGER_HPP
#define FS_UIDS_MANAGER_HPP
#pragma once

#include <cstdint> // uint64_t def
#include <atomic>


using UID = uint64_t;

class UidsManager {
    private:
       const UID slots_num_; // number of total slots
       const UID my_slot_;   // slot number assigned to this manager
       const UID uids_max_;   // number of uids available for this manager
       const UID uids_offset_; // mask that have the first slot_num_bits_ to 0 and the rest 1
       std::atomic<UID> uids_count_; // number of UIDs generated till now

    public:
       UidsManager(const UID slots_num, const UID my_slot);
       UID generate_uid();
};


#endif //FS_UIDS_MANAGER_HPP
