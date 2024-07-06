#ifndef SED_BACKEND_OPTIMIZE_LIVENESS_H
#define SED_BACKEND_OPTIMIZE_LIVENESS_H

#include "backend/builder.h"
#include "index.h"

namespace sed
{
namespace backend
{

using InstructionNum = std::size_t;

struct Range
{
    InstructionNum begin;
    InstructionNum end;

    std::size_t instruction_cnt = 0;
    BasicBlockID block_id;

    bool conflict(const Range &r) const
    {
        return (begin >= r.begin && begin < r.end) || (end > r.begin && end <= r.end) ||
               (begin <= r.begin && end >= r.end);
    }
};

class LivenessAnalysis
{
    std::unordered_map<BasicBlockID, bool> visited_map;

    // 活跃变量分析 主体
    std::unordered_map<BasicBlockID, std::set<OperandID>> def_map;
    std::unordered_map<BasicBlockID, std::set<OperandID>> use_map;
    std::unordered_map<BasicBlockID, std::set<OperandID>> in_map;
    std::unordered_map<BasicBlockID, std::set<OperandID>> out_map;

    InstructionNum now_instruction_num = 0;

    void clear_visit(FunctionPtr function);
    void clear(FunctionPtr function);

  public:
    std::unordered_map<InstructionID, InstructionNum> instr_id2num;
    std::unordered_map<InstructionNum, InstructionID> instr_num2id;
    std::unordered_map<BasicBlockID, std::vector<Range>> live_range_map;

    LivenessAnalysis() = default;

    // 这里需要特地注意，不能为直接1.
    InstructionNum create_instruction_num()
    {
        return now_instruction_num++;
    }

    void depth_first_numbering(BasicBlockPtr block, Builder &builder);
    void analyze(FunctionPtr now_function, Builder &builder);

    std::string to_string(Context &context) const;
};

} // namespace backend
} // namespace sed

#endif