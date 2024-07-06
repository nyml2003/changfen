#ifndef SED_BACKEND_OPTIMIZE_REGALLOC_H
#define SED_BACKEND_OPTIMIZE_REGALLOC_H

#include "backend/builder.h"
#include "backend/passes/liveness.h"
#include "backend/register.h"
#include "index.h"

namespace sed
{
namespace backend
{

using AllocNum = std::size_t;

enum class AllocStatus
{
    New = 5,
    Assign = 4,
    Split = 3,
    Spill = 2,
    Memory = 1,
    Done = 0,
};

struct AllocRange
{
    Range range;
    AllocNum alloc_num;

    AllocRange(Range range, AllocNum alloc_num) : range(range), alloc_num(alloc_num){};

    bool operator<(const AllocRange &other) const
    {
        if (range.begin < other.range.begin)
            return true;
        if (range.begin > other.range.begin)
            return false;
        return range.end < other.range.end;
    };
};

using AllocPriority = std::tuple<std::size_t, InstructionNum, AllocStatus, bool>;

struct PrioritizedAlloc
{
    AllocNum alloc_id;
    AllocPriority priority;

    PrioritizedAlloc(AllocNum alloc_id, AllocPriority priority) : alloc_id(alloc_id), priority(priority)
    {
    }

    bool operator<(const PrioritizedAlloc &a) const
    {
        auto [block_cnt, instruction_cnt, status, hinted] = priority;
        auto [a_block_cnt, a_instruction_cnt, a_status, a_hinted] = a.priority;

        // priority:
        // 1. alloc status
        // 2. block_cnt
        // 3. instruction_cnt
        // 4. hinted.
        if (status < a_status)
            return true;
        if (status > a_status)
            return false;
        if (block_cnt < a_block_cnt)
            return true;
        if (block_cnt > a_block_cnt)
            return false;
        if (instruction_cnt < a_instruction_cnt)
            return true;
        if (instruction_cnt > a_instruction_cnt)
            return false;
        return !hinted && a_hinted;
    }
};

// Greedy Linear Scan RegAlloc
class RegAlloc
{
    LivenessAnalysis liveness;
    std::unordered_map<BasicBlockID, double> block_weight_map;

    std::unordered_map<AllocNum, Register> alloc_map;
    // allocnum and operand_id, 一一对应。
    std::unordered_map<AllocNum, OperandID> alloc_operand_map;
    // allocnum(operand_id) and corresponding AllocNum. one-many.
    std::unordered_map<AllocNum, std::vector<Range>> alloc_range_map;

    std::unordered_map<AllocNum, AllocStatus> alloc_status_map;
    std::priority_queue<PrioritizedAlloc> alloc_priority_queue;

    std::unordered_map<Register, std::set<AllocNum>, RegisterHash> occupied_map;
    std::unordered_map<Register, std::set<AllocRange>, RegisterHash> occupied_range_map;

    // offset in memory.
    std::unordered_map<OperandID, size_t> operand_spill_map;

    // used temporary registers.
    std::unordered_map<InstructionID, std::set<Register>> used_temp_map;

    std::unordered_map<OperandID, OperandID> coalesce_map;
    std::unordered_map<OperandID, Register> hint_map;

    AllocNum now_alloc_num = 0;
    AllocNum create_alloc_num()
    {
        return ++now_alloc_num;
    }

    void init(FunctionPtr now_function, Builder &builder);
    void gen_alloc_hint(FunctionPtr now_function, Builder &builder);

    void try_allocate(AllocNum alloc_num, Builder &builder);
    void try_split(AllocNum alloc_num, Builder &builder);
    void spill(AllocNum alloc_num, FunctionPtr now_function, Builder &builder);
    void rewrite_operands(FunctionPtr now_function, Builder &builder);

    void get_block_weight(FunctionPtr now_function, Builder &builder);
    double get_range_weight(Range range);
    double get_spill_weight(AllocNum alloc_num);
    AllocPriority get_alloc_priority(AllocNum alloc_num);

  public:
    RegAlloc() = default;

    void greedy_allocation(FunctionPtr now_function, Builder &builder);
};

} // namespace backend
} // namespace sed

#endif