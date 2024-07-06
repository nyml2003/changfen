#ifndef SED_IR_PASSES_MEM2REG_H
#define SED_IR_PASSES_MEM2REG_H

#include "index.h"
#include "ir/passes/control_flow_analysis.h"

namespace sed::ir
{

struct Mem2reg
{
  private:
    Builder &builder;
    FunctionPtr curr_function;

    CFA cfa;
    std::set<OperandID> promotable;
    std::map<OperandID, TypePtr> var_type;
    std::map<OperandID, std::set<BasicBlockID>> def_bb;
    std::map<OperandID, std::set<BasicBlockID>> inserted_bb;
    std::map<OperandID, std::queue<BasicBlockID>> worklist_map;
    std::map<InstructionID, OperandID> phi_map;
    std::map<OperandID, std::stack<OperandID>> def_stack;

    bool isAllocaPromotable(OperandID alloca);
    void gen_promotable();

    void insert_phi();
    void rename(BasicBlockPtr curr);

  public:
    Mem2reg(Builder &builder);

    void run();
};
// TODO

} // namespace sed::ir

#endif // SED_IR_PASSES_MEM2REG_H