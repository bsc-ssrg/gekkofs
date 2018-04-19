#include <daemon/db/merge.hpp>


const char IncreaseSizeOperand::separator = ',';
const char IncreaseSizeOperand::true_char = 't';
const char IncreaseSizeOperand::false_char = 'f';

IncreaseSizeOperand::IncreaseSizeOperand(const size_t size, const bool append):
    size(size), append(append) {}

IncreaseSizeOperand::IncreaseSizeOperand(const std::string& serialized_op){
    size_t chrs_parsed = 0;
    size_t read = 0;

    //Parse size
    size = std::stoul(&serialized_op.at(chrs_parsed), &read);
    chrs_parsed += read + 1;
    assert(serialized_op.at(chrs_parsed - 1) == separator);

    //Parse append flag
    assert(serialized_op.at(chrs_parsed) == false_char ||
           serialized_op.at(chrs_parsed) == true_char);
    append = (serialized_op.at(chrs_parsed) == false_char) ? false : true;
    //check that we consumed all the input string
    assert(chrs_parsed + 1 == serialized_op.size());
}

std::string IncreaseSizeOperand::serialize() const {
    std::string s;
    s += std::to_string(size);
    s += this->separator;
    s += (append == false)? false_char : true_char;
    return s;
}

bool MetadataMergeOperator::FullMergeV2(
        const MergeOperationInput& merge_in,
        MergeOperationOutput* merge_out) const {

    if(merge_in.existing_value == nullptr){
        //The key to operate on doesn't exists in DB 

        // TODO use logger to print err info;
        //Log(logger, "Key %s do not exists", existing_value->ToString().c_str());

        return false;
    }

    Metadata md{merge_in.key.ToString(), merge_in.existing_value->ToString()};
    size_t fsize = md.size();

    for(const auto& operand: merge_in.operand_list){
        auto op = IncreaseSizeOperand(operand.ToString());
        if(op.append){
            //append mode, just increment file
            fsize += op.size;
        } else {
            fsize = std::max(op.size, fsize);
        }
    }

    md.size(fsize);
    md.serialize(merge_out->new_value);
    return true;
}

bool MetadataMergeOperator::PartialMergeMulti(const rdb::Slice& key,
                const std::deque<rdb::Slice>& operand_list,
                std::string* new_value, rdb::Logger* logger) const {
    return false;
}

const char* MetadataMergeOperator::Name() const {
    return "MetadataMergeOperator";
}

bool MetadataMergeOperator::AllowSingleOperand() const {
   return true;
}
