#ifndef DB_MERGE_HPP
#define DB_MERGE_HPP


#include "rocksdb/merge_operator.h"
#include <daemon/classes/metadata.hpp>

namespace rdb = rocksdb;


class IncreaseSizeOperand {
    public:
        const static char separator;
        const static char true_char;
        const static char false_char;

        size_t size;
        bool append;

        IncreaseSizeOperand(const size_t size, const bool append);
        IncreaseSizeOperand(const std::string& serialized_op);

        std::string serialize() const;
};

class MetadataMergeOperator: public rocksdb::MergeOperator {
    public:
        MetadataMergeOperator(){};
        virtual ~MetadataMergeOperator(){};
        virtual bool FullMergeV2(const MergeOperationInput& merge_in,
                MergeOperationOutput* merge_out) const override;

        virtual bool PartialMergeMulti(const rdb::Slice& key,
                const std::deque<rdb::Slice>& operand_list,
                std::string* new_value, rdb::Logger* logger) const override;

        virtual const char* Name() const override;

        virtual bool AllowSingleOperand() const override;
};


#endif // DB_MERGE_HPP
