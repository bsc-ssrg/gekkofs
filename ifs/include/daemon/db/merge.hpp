#ifndef DB_MERGE_HPP
#define DB_MERGE_HPP


#include "rocksdb/merge_operator.h"
#include <daemon/classes/metadata.hpp>

namespace rdb = rocksdb;

enum class OperandID: char {
    increase_size = 's',
    create = 'c'
};

class MergeOperand {
    public:
        constexpr static char operand_id_suffix = ':';
        std::string serialize() const;

    protected:
        std::string serialize_id() const;
        virtual std::string serialize_params() const = 0;
        virtual const OperandID id() const = 0;
};

class IncreaseSizeOperand: public MergeOperand {
    public:
        constexpr const static char separator = ',';
        constexpr const static char true_char = 't';
        constexpr const static char false_char = 'f';

        size_t size;
        bool append;

        IncreaseSizeOperand(const size_t size, const bool append);
        IncreaseSizeOperand(const rdb::Slice& serialized_op);

        virtual const OperandID id() const override;
        virtual std::string serialize_params() const override;
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
