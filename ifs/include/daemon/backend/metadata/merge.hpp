#ifndef DB_MERGE_HPP
#define DB_MERGE_HPP


#include "rocksdb/merge_operator.h"
#include <daemon/classes/metadata.hpp>

namespace rdb = rocksdb;

enum class OperandID: char {
    increase_size = 'i',
    decrease_size = 'd',
    create = 'c'
};

class MergeOperand {
    public:
        constexpr static char operand_id_suffix = ':';
        std::string serialize() const;
        static OperandID get_id(const rdb::Slice& serialized_op);
        static rdb::Slice get_params(const rdb::Slice& serialized_op);

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

        const OperandID id() const override;
        std::string serialize_params() const override;
};

class DecreaseSizeOperand: public MergeOperand {
    public:
        size_t size;

        DecreaseSizeOperand(const size_t size);
        DecreaseSizeOperand(const rdb::Slice& serialized_op);

        const OperandID id() const override;
        std::string serialize_params() const override;
};

class CreateOperand: public MergeOperand {
    public:
        std::string metadata;
        CreateOperand(const std::string& metadata);

        const OperandID id() const override;
        std::string serialize_params() const override;
};

class MetadataMergeOperator: public rocksdb::MergeOperator {
    public:
        virtual ~MetadataMergeOperator(){};
        bool FullMergeV2(const MergeOperationInput& merge_in,
                MergeOperationOutput* merge_out) const override;

        bool PartialMergeMulti(const rdb::Slice& key,
                const std::deque<rdb::Slice>& operand_list,
                std::string* new_value, rdb::Logger* logger) const override;

        const char* Name() const override;

        bool AllowSingleOperand() const override;
};


#endif // DB_MERGE_HPP
