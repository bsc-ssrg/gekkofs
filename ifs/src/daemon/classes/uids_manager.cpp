#include "daemon/classes/uids_manager.hpp"

#include <stdexcept>
#include <limits>


UidsManager::UidsManager(const UID slots_num, const UID my_slot) :
    slots_num_(slots_num),
    my_slot_(my_slot),
    uids_max_(std::numeric_limits<UID>::max() / slots_num_),
    uids_offset_(uids_max_ * my_slot_),
    uids_count_(0)
{
    if (slots_num <= 0) {
        throw std::runtime_error("Number of slots needs to be more then 0");
    }
    UID global_max_uids = std::numeric_limits<UID>::max();
    if (slots_num_ > global_max_uids) {
        throw std::runtime_error("Too many slots requested: " +
                                 std::to_string(slots_num_) +
                                 ". Max is: " + std::to_string(global_max_uids));
    }
}

UID UidsManager::generate_uid() {
    UID new_uid;
    UID curr_uid;

    /* Since we want to check if the increment of `uids_max_` led to an overflow
     * we need to use a CAS operation. In fact, if we rely on the simpler std::atomic increment
     * we would have one thread causing the overflow to raise an exception, but since the
     * unsinged integer have been reset due to the overflow, all the subsequent
     * increment would not raise any exception at all.
     */
     do {
        curr_uid = uids_count_;
        if (uids_count_ == uids_max_) {
            throw std::overflow_error("No more UIDs available");
        }
        new_uid = curr_uid + 1;
    } while(!std::atomic_compare_exchange_weak(&uids_count_, &curr_uid, new_uid));

    return new_uid + uids_offset_;
}
