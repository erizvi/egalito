#include "inferredptrs.h"
#include "chunk/dump.h"
#include "log/log.h"

void InferredPtrsPass::visit(Module *module) {
    this->module = module;
    recurse(module);
}

void InferredPtrsPass::visit(Instruction *instruction) {
    auto semantic = instruction->getSemantic();
    if(auto v = dynamic_cast<DisassembledInstruction *>(semantic)) {
        if(v->getLink()) return;
        auto ins = v->getCapstone();
        if(!ins) return;

#ifdef ARCH_X86_64
        cs_x86 *x = &ins->detail->x86;
        for(size_t i = 0; i < x->op_count; i ++) {
            cs_x86_op *op = &x->operands[i];
            if(op->type == X86_OP_MEM
                && op->mem.base == X86_REG_RIP
                && op->mem.index == X86_REG_INVALID
                && op->mem.scale == 1) {

                address_t target = (instruction->getAddress() + instruction->getSize()) + op->mem.disp;
                auto inferred = new InferredInstruction(instruction, *ins);


                auto functionList = module->getChildren()->getSpatial();
                auto found = functionList->find(target);
                if(found) {
                    inferred->setLink(new NormalLink(found));
                    instruction->setSemantic(inferred);
                    delete v;

#if 0
                    LOG(8, "inferred function pointer at " << instruction->getAddress()
                        << " -> " << target << ":");
                    ChunkDumper d;
                    instruction->accept(&d);
#endif
                }
                else {
                    inferred->setLink(new DataOffsetLink(elf, target));
                    instruction->setSemantic(inferred);
                    delete v;
#if 0
                    LOG(8, "inferred data pointer at " << instruction->getAddress()
                        << " -> " << target << ":");
                    ChunkDumper d;
                    instruction->accept(&d);
#endif
                }
                return;
            }
        }
#endif
    }
}
