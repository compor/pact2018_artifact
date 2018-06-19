//
// Created by Peng Jiang on 5/8/17.
//

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Debug.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/Support/CommandLine.h"

#include <set>
#include <map>
#include <unordered_map>
#include <memory>
#include <queue>
#include <sys/stat.h>
#include <fcntl.h>

using namespace llvm;




// condition is the branch instruction with going either left (0) or right (1)
using Cond = std::pair<BranchInst*, int>;
using Assign = StoreInst*;
using Ftype = std::bitset<9>;


static const Ftype LINR_P("000000001");
static const Ftype LINR_N("000000010");
static const Ftype RECT_A("000000100");
static const Ftype RECT_D("000001000");
static const Ftype RECT_E("000010000");
static const Ftype RECT_B("000100000");
static const Ftype RECT_C("001000000");
static const Ftype RECT_F("010000000");
static const Ftype FST("100000000");


static std::unordered_map<Ftype, std::string> fname = {{LINR_P, "LINR_P"}, {LINR_N, "LINR_N"}, {RECT_A, "RECT_A"},
    {RECT_B, "RECT_B"}, {RECT_C, "RECT_C"}, {RECT_D, "RECT_D"},
    {RECT_E, "RECT_E"}, {RECT_F, "RECT_F"}, {FST, "FST"}
};

static cl::opt<std::string> OutputFilename("myoutput", cl::desc("Specify input filename for mypass"), cl::value_desc("filename"));

struct Cmp {
    enum Pred {GTE, LTE, EQ, NE, WD};

    static Pred getOp(CmpInst *ci) {
        auto p = ci->getPredicate();
        if (p == CmpInst::FCMP_OLT || p == CmpInst::FCMP_OLE || p == CmpInst::ICMP_ULT || p == CmpInst::ICMP_ULE ||
                p == CmpInst::ICMP_SLT || p == CmpInst::ICMP_SLE) {
            return Pred::LTE;
        } else if (p == CmpInst::FCMP_OGT || p == CmpInst::FCMP_OGE || p == CmpInst::ICMP_UGT ||
                   p == CmpInst::ICMP_UGE || p == CmpInst::ICMP_SGT || p == CmpInst::ICMP_SGE) {
            return Pred::GTE;
        } else if (p == CmpInst::FCMP_OEQ || p == CmpInst::FCMP_UEQ || p == CmpInst::ICMP_EQ) {
            return Pred::EQ;
        } else if (p == CmpInst::FCMP_ONE || p == CmpInst::FCMP_UNE || p == CmpInst::ICMP_NE) {
            return Pred::NE;
        }
        errs() << "unknown comparison operator\n";
        ci->dump();
        exit(-1);
    }

    static Pred flipOp(Pred p) {
        if (p == Pred::GTE) return Pred::LTE;
        if (p == Pred::LTE) return Pred::GTE;
        if (p == Pred::EQ) return Pred::NE;
        if (p == Pred::NE) return Pred::EQ;
    }
};

// the dependent var, the function type, the coefficient, whether the offset is zero
using Relation = std::tuple<Value*, Ftype, double*, bool>;

// the last bool variable in tuple indicates (1) whether it is the left or right piece of a rectified-linear function (true is right peice)
// or (2) whether it is an equivalene or inequivalence of a FST function
using AdjList = std::map<Value*, std::map<Value*, std::tuple<Ftype, double*, bool>>>;

void flipSign(Ftype &t) {
    Ftype res = 0;
    if (t.test(0)) {
        t.reset(0);
        res.set(1);
    }
    if (t.test(1)) {
        t.reset(1);
        res.set(0);
    }
    t |= res;
}

struct FuncTypePass: public FunctionPass {
    static char ID;

    FuncTypePass() : FunctionPass(ID) {}

    // the recurrence variables and their storing instructions
    std::map<Value *, std::set<StoreInst *>> recur_vars;

    // reverse map from store instructions to recur variables
    std::map<StoreInst *, Value *> recur_rmap;

    // the main body of the loop
    BasicBlock *loopbody = nullptr;

    MemorySSA *MA = nullptr;

    DominatorTree *DT = nullptr;
    PostDominatorTree *PDT = nullptr;

    LoopInfo *LI = nullptr;

    BasicBlock* identifyRecurVars(Loop *L) {

        std::map<Value *, std::set<LoadInst *>> loaded;
        std::map<Value *, std::set<StoreInst *>> stored;

        // iterate over all of the blocks in the loop
        for (auto *B : L->blocks()) {

            if (B->getName().compare("for.body") == 0) {
                loopbody = B;
            }

            // ignore the cond and inc block to exclude the induction variable 'e.g., i'
            if (B->getName().compare("for.cond") == 0 || B->getName().compare("for.inc") == 0) continue;
            // iterate over all the instructions in a block
            for (Instruction &I : *B) {
                // record the (relative) line number of that instruction
                // used to check if the first instruction on a variable is a store (assumption: instruction appears earlier will be executed earlier, not accurate but works)
                //
                // store all the variables used by load instructions
                if (auto *op = dyn_cast<LoadInst>(&I)) {
                    if (!op->getPointerOperandType()->getPointerElementType()->isPointerTy()) {
                        auto *V = op->getPointerOperand();

                        if (auto *ttv = dyn_cast<AllocaInst>(
                                            V)) {   // if it is the allocation of that variable, store it
                            if (loaded.find(ttv) == loaded.end())
                                loaded[ttv] = std::set<LoadInst *>();
                            loaded[ttv].insert(op);
                        } else if (auto *getop = dyn_cast<GetElementPtrInst>(
                                                     V)) {  // if it is an array access, trace back to the allocation of the head of the array
                            auto *tv = getop->getPointerOperand();
                            while (auto *tttv = dyn_cast<LoadInst>(tv)) {
                                tv = tttv->getPointerOperand();
                            }
                            if (auto *tttv = dyn_cast<AllocaInst>(tv)) {
                                if (loaded.find(tttv) == loaded.end())
                                    loaded[tttv] = std::set<LoadInst *>();
                                loaded[tttv].insert(op);
                            }
                        }
                    }
                }
                // store all the variables used by store instructions
                if (auto *op = dyn_cast<StoreInst>(&I)) {

                    auto *V = op->getPointerOperand();

                    if (auto *ttv = dyn_cast<AllocaInst>(V)) {

                        if (stored.find(ttv) == stored.end())
                            stored[ttv] = std::set<StoreInst *>();
                        stored[ttv].insert(op);

                    } else if (auto *getop = dyn_cast<GetElementPtrInst>(V)) {
                        auto *tv = getop->getPointerOperand();
                        while (auto *tttv = dyn_cast<LoadInst>(tv)) {
                            tv = tttv->getPointerOperand();
                        }
                        if (auto *tttv = dyn_cast<AllocaInst>(tv)) {
                            if (stored.find(tttv) == stored.end())
                                stored[tttv] = std::set<StoreInst *>();
                            stored[tttv].insert(op);
                        }
                    }
                }
            }

        }


        DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
        PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();


        for (auto &IT : stored) {
            Value *V = IT.first;


            if (loaded.find(V) != loaded.end()) {

                bool isDefFirst = false;

                for (auto *a1 : IT.second) {
                    bool domall = true;
                    for (auto *a2 : loaded[V]) {
                        if (!DT->dominates(a1, a2)) {
                            domall = false;
                            break;
                        }
                    }
                    if (domall) {
                        isDefFirst = true;
                        break;
                    }
                }

                if (!isDefFirst) {
                    recur_vars.insert(IT);
                }
            } else {
                bool isDefPart = false;
                for (auto *a1 : IT.second) {
                    if (loopbody != nullptr && !PDT->dominates(PDT->getNode(a1->getParent()), PDT->getNode(loopbody))) {
                        isDefPart = true;
                    }
                }
                if (isDefPart) {
                    recur_vars.insert(IT);
                }
            }
        }

        for (auto &V : recur_vars) {
            for (StoreInst *sinst : V.second) {
                recur_rmap[sinst] = V.first;
            }
        }

        // print out the recurrence variables
        // prototype of recur: std::map<Value*, std::set<StoreInst*>>
        // Value* is the pointer to the alloca instruction for the recurrence variable
#define DEBUG_TYPE "print_recurvars"
        DEBUG(
        for (auto &V : recur_vars) {
        errs() << "==========recurrence variables =========\n";
            V.first->dump();
            errs() << "-----store instructions: --------------\n";
            for (auto *S : V.second) {
                S->dump();
            }
        });
#undef DEBUG_TYPE

        return loopbody;
    }


    // TODO: MemorySSA is not so good ... need more test in future
    // TODO: May seek other ways to get reaching defintion in future
    void getReachingDefinition(std::vector<Instruction *> &res, MemoryAccess *mp, const std::vector<Cond> &cond) {
        if (MA->isLiveOnEntryDef(mp)) return;
        if (auto *mu = dyn_cast<MemoryUse>(mp)) {
            if (MemoryAccess *ma = mu->getDefiningAccess()) {
                getReachingDefinition(res, ma, cond);
            }
        } else if (auto *md = dyn_cast<MemoryDef>(mp)) {

            if (md->getMemoryInst() && dyn_cast<StoreInst>(md->getMemoryInst())) {
                res.push_back(md->getMemoryInst());
            }
        } else if (auto *mphi = dyn_cast<MemoryPhi>(mp)) {
            for (unsigned i = 0, j = mphi->getNumIncomingValues(); i < j; i++) {
                if (isValidPrefix(mphi->getIncomingBlock(i), cond)) {
                    MemoryAccess *t = mphi->getIncomingValue(i);
                    getReachingDefinition(res, t, cond);
                }
            }
        }
    }

    void unionRelationSets(std::vector<Relation> &res, const std::vector<Relation> &rel) {
        for (auto &r : rel) {
            bool inleft = false;
            for (auto &l : res) {
                if (std::get<0>(l) == std::get<0>(r)) {
                    inleft = true;
                    if (std::get<1>(l) != std::get<1>(r)) {
                        errs() << "inconsistent types\n";
                        exit(-1);
                    } else if ((std::get<2>(l) && std::get<2>(r) == nullptr) ||
                               (std::get<2>(r) && std::get<2>(l) == nullptr) ||
                               (std::get<2>(l) && std::get<2>(r) && *std::get<2>(l) != *std::get<2>(r))) {
                        std::get<2>(l) = nullptr;
                    }
                }
            }
            if (!inleft) res.push_back(r);
        }
    }

    bool isSame(Value *a, Value *b) {
        if (a && b) {
            if (a == b) return true;
            if (auto *lda = dyn_cast<LoadInst>(a)) {
                if (auto *ldb = dyn_cast<LoadInst>(b)) {
                    return isSame(lda->getPointerOperand(), ldb->getPointerOperand());
                }
            } else if (auto *geta = dyn_cast<GetElementPtrInst>(a)) {
                if (auto *getb = dyn_cast<GetElementPtrInst>(b)) {
                    bool is_base_same = isSame(geta->getPointerOperand(), getb->getPointerOperand());
                    // TODO: currently, we do not differentiate array indexing
                    return is_base_same;
                }
            }
        }
        return false;
    }

    bool impliesGT0(Cond cond, Value* v) {
        if (auto *cmp = dyn_cast<CmpInst>(cond.first->getCondition())) {
            if (cmp->getNumOperands() == 2) {
                auto *lhs = cmp->getOperand(0);
                auto *rhs = cmp->getOperand(1);
                if (isSame(lhs, v)) {
                    auto op = Cmp::getOp(cmp);
                    if (cond.second == 1) op = Cmp::flipOp(op);
                    if (op == Cmp::Pred::GTE) {
                        if (auto *r = dyn_cast<ConstantInt>(rhs)) {
                            if(!r->isNegative()) return true;
                        }
                        if (auto *r = dyn_cast<ConstantFP>(rhs)) {
                            if (!r->isNegative()) return true;
                        }
                    }
                } else if (isSame(rhs, v)) {
                    auto op = Cmp::getOp(cmp);
                    if (cond.second == 1) op = Cmp::flipOp(op);
                    if (op == Cmp::Pred::LTE) {
                        if (auto *l = dyn_cast<ConstantInt>(lhs)) {
                            if(!l->isNegative()) return true;
                        }
                        if (auto *l = dyn_cast<ConstantFP>(lhs)) {
                            if (!l->isNegative()) return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    bool impliesLT0(Cond cond, Value* v) {
        if (auto *cmp = dyn_cast<CmpInst>(cond.first->getCondition())) {
            if (cmp->getNumOperands() == 2) {
                auto *lhs = cmp->getOperand(0);
                auto *rhs = cmp->getOperand(1);
                if (isSame(lhs, v)) {
                    auto op = Cmp::getOp(cmp);
                    if (cond.second == 1) op = Cmp::flipOp(op);
                    if (op == Cmp::Pred::LTE) {
                        if (auto *r = dyn_cast<ConstantInt>(rhs)) {
                            if (r->isZero() || r->isNegative()) return true;

                        }

                        if (auto *r = dyn_cast<ConstantFP>(rhs)) {
                            if (r->isZero() || r->isNegative()) return true;

                        }


                    }
                } else if (isSame(rhs, v)) {
                    auto op = Cmp::getOp(cmp);
                    if (cond.second == 1) op = Cmp::flipOp(op);
                    if (op == Cmp::Pred::GTE) {
                        if (auto *l = dyn_cast<ConstantInt>(lhs)) {
                            if(l->isZero() || l->isNegative()) return true;
                        }
                        if (auto *l = dyn_cast<ConstantFP>(lhs)) {
                            if(l->isZero() || l->isNegative()) return true;
                        }
                    }
                }
            }
        }
        return false;
    }



    bool isValidPrefix(BasicBlock *bb, const std::vector<Cond> &cond) {
        for (auto &cc : cond) {
            if (cc.first->getSuccessor(cc.second) == bb) return true;
        }
        return false;
    }

    bool isConstant(Value *exp) {
        if (dyn_cast<ConstantInt>(exp) || dyn_cast<ConstantFP>(exp)) return true;
        return false;
    }


    std::vector<Relation> checkLinear(Value *exp, const std::vector<Cond> &cond) {
        if (dyn_cast<AllocaInst>(exp) || dyn_cast<GetElementPtrInst>(exp) || dyn_cast<ConstantFP>(exp) || dyn_cast<ConstantInt>(exp) ||
                dyn_cast<Argument>(exp)) {
            std::vector<Relation> res;
            if (isRecurVar(exp))
                res.emplace_back(std::make_tuple(exp, LINR_P, new double(1.0), true));
            return res;
        } else if (auto *inst = dyn_cast<PHINode>(exp)) {
            std::vector<Relation> res;
            for (unsigned i = 0, j = inst->getNumIncomingValues(); i < j; i++) {
                if (isValidPrefix(inst->getIncomingBlock(i), cond))
                    return checkLinear(inst->getIncomingValue(i), cond);
            }
            return res;
        } else if (auto *inst = dyn_cast<LoadInst>(exp)) {
            std::vector<Relation> res;
            if (dyn_cast<GetElementPtrInst>(inst->getPointerOperand()) || dyn_cast<ConstantFP>(inst->getPointerOperand()) || dyn_cast<ConstantInt>(inst->getPointerOperand()) ||
                    dyn_cast<Argument>(inst->getPointerOperand())) {
                return res;
            } else if (dyn_cast<AllocaInst>(inst->getPointerOperand()) && isRecurVar(inst->getPointerOperand())) {
                res.emplace_back(std::make_tuple(inst->getPointerOperand(), LINR_P, new double(1.0), true));
                return res;
            } else {
                // get reaching defintion of a loaded value using MemorySSA
                // TODO: due to the use of MemorySSA, may have bug here, need more test in future
                MemoryUseOrDef *mu = MA->getMemoryAccess(inst);
                std::vector<Instruction *> reachdef;
                getReachingDefinition(reachdef, mu, cond);
                for (auto *def : reachdef) {
                    if (auto *si = dyn_cast<StoreInst>(def)) {
                        std::vector<Relation> rel = checkLinear(si->getValueOperand(), cond);
                        unionRelationSets(res, rel);
                    }
                }
                return res;
            }
        } else if (auto *inst = dyn_cast<BinaryOperator>(exp)) {
            std::vector<Relation> lrel;
            std::vector<Relation> rrel;
            Value *lhs = inst->getOperand(0), *rhs = inst->getOperand(1);
            if (lhs) lrel = checkLinear(lhs, cond);
            if (rhs) rrel = checkLinear(rhs, cond);


            if (strstr(inst->getOpcodeName(), "add")) {
                for (auto &r : rrel) {
                    bool inleft = false;
                    for (auto &l : lrel) {
                        if (std::get<0>(l) == std::get<0>(r)) {
                            inleft = true;
                            if(std::get<1>(l) == std::get<1>(r)) {
                                if (std::get<2>(l) && std::get<2>(r))
                                    *std::get<2>(l) = *std::get<2>(l) + *std::get<2>(r);
                            } else {
                                std::get<1>(l) =  std::get<1>(l) | std::get<1>(r);
                            }

                        }
                    }
                    if (!inleft) lrel.push_back(r);
                }
                for (auto &l : lrel) {
                    std::get<3>(l) = false;
                }
                return lrel;
            } else if (strstr(inst->getOpcodeName(), "sub")) {
                // combine the relations in the left and right hand side and change the relations to linear
                // x = a - b, the constant relation in a become linear_pos
                for (auto &r : rrel) {
                    bool inleft = false;
                    flipSign(std::get<1>(r));
                    for (auto &l : lrel) {
                        if (std::get<0>(l) == std::get<0>(r)) {
                            inleft = true;
                            if(std::get<1>(l) == std::get<1>(r)) {
                                if (std::get<2>(l) && std::get<2>(r))
                                    *std::get<2>(l) = *std::get<2>(l) + *std::get<2>(r);
                            } else {
                                std::get<1>(l) =  std::get<1>(l) | std::get<1>(r);
                            }

                        }
                    }
                    if (!inleft) lrel.push_back(r);
                }
                for (auto &l : lrel) {
                    std::get<3>(l) = false;
                }
                return lrel;
            } else if (strstr(inst->getOpcodeName(), "mul")) {
                // case#1: ... = a * x
                if (lrel.empty()) {
                    // if a is a literal constant, and a is negative, flip the sign in x
                    if (auto *c = dyn_cast<ConstantFP>(lhs)) {
                        if (c->isNegative()) {
                            for (auto &ri : rrel) {
                                flipSign(std::get<1>(ri));
                            }
                        }
                        for (auto &ri : rrel) {
                            if (std::get<2>(ri))
                                *std::get<2>(ri) = *std::get<2>(ri) * c->getValueAPF().convertToDouble();
                        }
                    } else if (auto *c = dyn_cast<ConstantInt>(lhs)) {
                        if (c->isNegative()) {
                            for (auto &ri : rrel) {
                                flipSign(std::get<1>(ri));
                            }
                        }
                        for (auto &ri : rrel) {
                            if (std::get<2>(ri)) *std::get<2>(ri) = *std::get<2>(ri) * c->getValue().roundToDouble();
                        }
                    } else {
                        // TODO: check the constraint multplier, for msp.cpp
                        bool sign_determined = false;
                        for (auto &cd : cond) {
                            if (impliesGT0(cd, lhs)) {
                                sign_determined = true;
                                // the coefficient cannot be determined because c is not known in complie time
                                for (auto &ri : rrel) {
                                    std::get<2>(ri) = nullptr;
                                }
                                break;
                            } else if (impliesLT0(cd, lhs)) {
                                sign_determined = true;
                                for (auto &ri : rrel) {
                                    flipSign(std::get<1>(ri));
                                    std::get<2>(ri) = nullptr;
                                }
                                break;
                            }
                        }
                        if (!sign_determined) {
                            for (auto &ri : rrel) {
                                // set the function type to both LINR_P and LINR_N
                                std::get<1>(ri) = LINR_P | LINR_N;
                                std::get<2>(ri) = nullptr;
                            }

                        }
                    }
                    return rrel;
                }
                // case#2: ... = x * a
                if (rrel.empty()) {
                    // if a is a literal constant, and a is negative, flip the sign in x
                    if (auto *c = dyn_cast<ConstantFP>(rhs)) {
                        if (c->isNegative()) {
                            for (auto &li : lrel) {
                                flipSign(std::get<1>(li));
                            }
                        }
                        for (auto &li : lrel) {
                            if (std::get<2>(li))
                                *std::get<2>(li) = *std::get<2>(li) * c->getValueAPF().convertToDouble();
                        }
                    } else if (auto *c = dyn_cast<ConstantInt>(rhs)) {
                        if (c->isNegative()) {
                            for (auto &li : lrel) {
                                flipSign(std::get<1>(li));
                            }
                        }
                        for (auto &li : lrel) {
                            if (std::get<2>(li)) *std::get<2>(li) = *std::get<2>(li) * c->getValue().roundToDouble();
                        }
                    } else {
                        // TODO: check the constraint multplier, for msp.cpp

                        bool sign_determined = false;
                        for (auto &cd : cond) {
                            if (impliesGT0(cd, rhs)) {

                                sign_determined = true;
                                // the coefficient cannot be determined because c is not known in complie time
                                for (auto &li : lrel) {
                                    std::get<2>(li) = nullptr;
                                }
                                break;
                            } else if (impliesLT0(cd, rhs)) {

                                sign_determined = true;
                                for (auto &li : lrel) {
                                    flipSign(std::get<1>(li));
                                    std::get<2>(li) = nullptr;
                                }
                                break;
                            }
                        }
                        if (!sign_determined) {
                            for (auto &li : lrel) {
                                // set the function type to both LINR_P and LINR_N
                                std::get<1>(li) = LINR_P | LINR_N;
                                std::get<2>(li) = nullptr;
                            }
                        }
                    }
                    return lrel;
                } else {
                    // if both lrel and rrel are related to recurrence variable
                    // can consider more complicated cases in future implementation
                    errs() << "error in multiplication \t";
                    inst->dump();
                    exit(-1);
                }
            } else if (strstr(inst->getOpcodeName(), "div")) {
                if (rrel.empty()) {
                    if (auto *c = dyn_cast<ConstantFP>(rhs)) {
                        if (c->isNegative()) {
                            for (auto &li : lrel) {
                                flipSign(std::get<1>(li));
                            }
                        }
                        for (auto &li : lrel) {
                            if (std::get<2>(li)) *std::get<2>(li) /= c->getValueAPF().convertToDouble();
                        }
                    } else if (auto *c = dyn_cast<ConstantInt>(rhs)) {
                        if (c->isNegative()) {
                            for (auto &li : lrel) {
                                flipSign(std::get<1>(li));
                            }
                        }
                        for (auto &li : lrel) {
                            if (std::get<2>(li)) *std::get<2>(li) /= c->getValue().roundToDouble();
                        }
                    } else {
                        for (auto &li : lrel) {
                            // set the function type to both LINR_P and LINR_N
                            std::get<1>(li).set(0);
                            std::get<1>(li).set(1);
                        }
                    }
                    return lrel;
                } else {
                    // if rrel are related to recurrence variable, abort
                    errs() << "error in division \t";
                    inst->dump();
                    exit(-1);
                }
            }
            errs() << "error in binary operator ";
            inst->dump();
            exit(-1);
        } else if (auto *inst = dyn_cast<CallInst>(exp)) {
            // may be extended in future to support inter-procedure analysis
            // currently, a function call is assumed to be unrelated to recurrence variables
            std::vector<Relation> res;
            return res;
        }
        std::vector<Relation> res;
        return res;
    }

    std::vector<std::pair<const std::vector<Cond>, Assign>> getCondAssignPairs() {

        // obtaining all the condition-assignment pairs
        std::vector<std::pair<const std::vector<Cond>, Assign>> S;
        std::vector<Cond> current_path;

        traverseCFG(loopbody, current_path, S);



        // print out the recurrence variables
        // prototype of recur: std::map<Value*, std::set<StoreInst*>>
        // Value* is the pointer to the alloca instruction for the recurrence variable
#define DEBUG_TYPE "print_condassigns"
        DEBUG(
            errs() << "========== condition-assignment pairs  =========\n";
        for (auto &p : S) {
        errs() << "-------conditions----------\n";
            for (auto &c : p.first) {
                errs() << c.second << ": ";
                c.first->dump();
            }
            errs() << "-------assignment----------\n";
            p.second->dump();
        });
#undef DEBUG_TYPE

        return S;
    }


    bool isReachable(BasicBlock *B, Instruction *ii) {
        if (B->getName().compare("for.cond") == 0 || B->getName().compare("for.inc") == 0) return false;
        for (Instruction &I : B->getInstList()) {
            if (&I == ii) return true;
            if (auto *bi = dyn_cast<BranchInst>(&I)) {
                for (unsigned i = 0, num = bi->getNumSuccessors(); i < num; i++) {
                    if(isReachable(bi->getSuccessor(i), ii)) return true;
                }
            }
        }
        return false;
    }

    // traverse the CFG to obtain the condition-assignment pairs
    void traverseCFG(BasicBlock *B, std::vector<Cond> &current_path,
                     std::vector<std::pair<const std::vector<Cond>, Assign>> &S) {
        if (B->getName().compare("for.cond") == 0 || B->getName().compare("for.inc") == 0) return;
        std::vector<Cond> tpath(current_path.begin(), current_path.end());

        for (Instruction &I : B->getInstList()) {
            if (auto *sinst = dyn_cast<StoreInst>(&I)) {
                if (recur_rmap.find(sinst) != recur_rmap.end()) {
                    S.emplace_back(std::make_pair(tpath, sinst));
                }
            }

            if (auto *bi = dyn_cast<BranchInst>(&I)) {
                if (bi->isConditional() && bi->getNumSuccessors() == 2) {
                    if (auto *cmp = dyn_cast<CmpInst>(bi->getCondition())) {

                        // br %cmp label ...

                        current_path.emplace_back(std::make_pair(bi, 0));
                        traverseCFG(bi->getSuccessor(0), current_path, S);
                        current_path.pop_back();

                        current_path.emplace_back(std::make_pair(bi, 1));
                        traverseCFG(bi->getSuccessor(1), current_path, S);
                        current_path.pop_back();

                    } else {
                        // br %call label ...
                        for (unsigned i = 0, num = bi->getNumSuccessors(); i < num; i++) {
                            traverseCFG(bi->getSuccessor(i), current_path, S);
                        }
                    }
                } else {
                    // br label
                    for (unsigned i = 0, num = bi->getNumSuccessors(); i < num; i++) {
                        traverseCFG(bi->getSuccessor(i), current_path, S);
                    }
                }
            } else if (auto *ii = dyn_cast<InvokeInst>(&I)) {
                // invoke .. to .. unwind ...
                for (unsigned i = 0, num = ii->getNumSuccessors(); i < num; i++) {
                    traverseCFG(ii->getSuccessor(i), current_path, S);
                }
            }
        }
    }

    inline bool isRecurVar(Value *v) {
        return recur_vars.find(v) != recur_vars.end();
    }

    inline bool isLinr(Ftype t) {
        return (t & Ftype(0xFC)) == 0 && (t & Ftype(0x03)) != 0;
    }

    inline bool isRectADE(Ftype t) {
        Ftype ade = RECT_A | RECT_D | RECT_E;
        return (t & ~ade) == 0 && (t & ade) != 0;
    }

    inline bool isRectBCF(Ftype t) {
        Ftype bcf = RECT_B | RECT_C | RECT_F;
        return (t & ~bcf) == 0 && (t & bcf) != 0;
    }


    inline bool isRect(Ftype t) {
        Ftype rect =  RECT_A | RECT_D | RECT_E | RECT_B | RECT_C | RECT_F;
        return (t & ~rect) == 0 && (t & rect) != 0;
    }

    std::tuple<Ftype, double*, bool> mergeRelation(const std::tuple<Ftype, double*, bool> &a, const std::tuple<Ftype, double*, bool> &b) {
        std::tuple<Ftype, double*, bool> res;

        if (std::get<0>(a) == std::get<0>(b)) {
            std::get<0>(res) = std::get<0>(a);
        } else if (isLinr(std::get<0>(a)) && isLinr(std::get<0>(b))) {
            std::get<0>(res) = std::get<0>(a) | std::get<0>(b);
        } else if (isRectADE(std::get<0>(a)) && std::get<0>(b) == LINR_P) {
            std::get<0>(res) = std::get<0>(a);
        } else if (isRectADE(std::get<0>(b)) && std::get<0>(a) == LINR_P) {
            std::get<0>(res) = std::get<0>(b);
        } else if (isRectBCF(std::get<0>(a)) && std::get<0>(b) == LINR_N) {
            std::get<0>(res) = std::get<0>(a);
        } else if (isRectBCF(std::get<0>(b)) && std::get<0>(a) == LINR_N) {
            std::get<0>(res) = std::get<0>(b);
        } else if (std::get<0>(a) == RECT_E && isRectADE(std::get<0>(b))) {
            std::get<0>(res) = RECT_E;
        } else if (std::get<0>(b) == RECT_E && isRectADE(std::get<0>(a))) {
            std::get<0>(res) = RECT_E;
        } else if (std::get<0>(a) == RECT_F && isRectBCF(std::get<0>(b))) {
            std::get<0>(res) = RECT_F;
        } else if (std::get<0>(b) == RECT_F && isRectBCF(std::get<0>(a))) {
            std::get<0>(res) = RECT_F;
        } else if (isRect(std::get<0>(a)) && isRect(std::get<0>(b))) {
            std::get<0>(res) = std::get<0>(a) | std::get<0>(b);
        } else if ((std::get<0>(a) == 0) && (std::get<0>(b) != 0)) {
            res = b;
        } else if ((std::get<0>(a) != 0) && (std::get<0>(b) == 0)) {
            res = a;
        } else {
            errs() << std::get<0>(a).to_string() << "   " << std::get<0>(b).to_string() << "\n";
            errs() << "incompatible types in merge\n";
            exit(-1);
        }

        if (std::get<0>(a) != 0 && std::get<0>(b) != 0) {
            if (std::get<1>(a) && std::get<1>(b) && *(std::get<1>(a)) == *(std::get<1>(b))) {
                std::get<1>(res) = std::get<1>(a);
            } else {
                std::get<1>(res) = nullptr;
            }


            std::get<2>(res) = std::get<2>(a) && std::get<2>(b);
        }


        return res;
    }


    std::tuple<Ftype, double*, bool> composeRelation(const std::tuple<Ftype, double*, bool> &a, const std::tuple<Ftype, double*, bool> &b) {

        std::tuple<Ftype, double*, bool> res;

        std::unordered_map<Ftype, std::unordered_map<Ftype, Ftype>> table = {
            {LINR_P, {{LINR_P, LINR_P}, {LINR_N, LINR_N}, {RECT_A, RECT_A}, {RECT_B, RECT_B}, {RECT_C, RECT_C}, {RECT_D, RECT_D}, {RECT_E, RECT_E}, {RECT_F, RECT_F}}},
            {LINR_N, {{LINR_P, LINR_N}, {LINR_N, LINR_P}, {RECT_A, RECT_C}, {RECT_B, RECT_D}, {RECT_C, RECT_A}, {RECT_D, RECT_B}, {RECT_E, RECT_F}, {RECT_F, RECT_E}}},
            {RECT_A, {{LINR_P, RECT_A}, {LINR_N, RECT_B}, {RECT_A, RECT_A}, {RECT_B, RECT_B}, {RECT_C, RECT_F}, {RECT_D, RECT_A}, {RECT_E, RECT_E}, {RECT_F, RECT_F}}},
            {RECT_B, {{LINR_P, RECT_B}, {LINR_N, RECT_A}, {RECT_A, RECT_F}, {RECT_B, RECT_E}, {RECT_C, RECT_A}, {RECT_D, RECT_B}, {RECT_E, RECT_F}, {RECT_F, RECT_E}}},
            {RECT_C, {{LINR_P, RECT_C}, {LINR_N, RECT_D}, {RECT_A, RECT_C}, {RECT_B, RECT_D}, {RECT_C, RECT_E}, {RECT_D, RECT_F}, {RECT_E, RECT_F}, {RECT_F, RECT_E}}},
            {RECT_D, {{LINR_P, RECT_D}, {LINR_N, RECT_C}, {RECT_A, RECT_E}, {RECT_B, RECT_F}, {RECT_C, RECT_C}, {RECT_D, RECT_D}, {RECT_E, RECT_E}, {RECT_F, RECT_F}}},
            {RECT_E, {{LINR_P, RECT_E}, {LINR_N, RECT_F}, {RECT_A, RECT_E}, {RECT_B, RECT_F}, {RECT_C, RECT_F}, {RECT_D, RECT_E}, {RECT_E, RECT_E}, {RECT_F, RECT_F}}},
            {RECT_F, {{LINR_P, RECT_F}, {LINR_N, RECT_E}, {RECT_A, RECT_F}, {RECT_B, RECT_E}, {RECT_C, RECT_E}, {RECT_D, RECT_F}, {RECT_E, RECT_F}, {RECT_F, RECT_E}}},
        };



        if (std::get<0>(a) == FST) {
            return a;

        } else if (std::get<0>(b) == FST) {
            errs() << "error in composition\n";
        } else {
            for (unsigned i = 0; i < std::get<0>(a).size(); i++) {
                Ftype t1;
                if (std::get<0>(a).test(i)) t1.set(i);
                else continue;
                for (unsigned j = 0; j < std::get<0>(b).size(); j++) {
                    Ftype t2;
                    if (std::get<0>(b).test(j)) t2.set(j);
                    else continue;
                    std::get<0>(res) |= table[t1][t2];
                }

            }

        }

        if (std::get<1>(a) && std::get<1>(b)) {
            std::get<1>(res) = new double(*(std::get<1>(a)) * (*(std::get<1>(b))));
        }

        std::get<2>(res) = std::get<2>(a) && std::get<2>(b);

        // errs() << "comp: " << "  " << std::get<0>(a).to_string() << " " << std::get<0>(b).to_string() << " " << std::get<0>(res).to_string() << "\n";

        return res;
    };

    std::vector<std::tuple<Value*, Value*, Cmp::Pred>> separateVars(const std::pair<std::vector<Cond>, std::vector<Cond>> &condpair) {

        // the merged real cond
        const auto &cond = condpair.first;
        // the full cond for determining signs
        const auto &cond_all = condpair.second;

        std::vector<std::tuple<Value *, Value *, Cmp::Pred>> res;
        for (const auto &c : cond) {
            if (auto *cmp = dyn_cast<CmpInst>(c.first->getCondition())) {
                if (cmp->getNumOperands() == 2) {
                    auto lhs = cmp->getOperand(0);
                    auto rhs = cmp->getOperand(1);
                    // for recurrence variable in lhs
                    auto xs = checkLinear(lhs, cond_all);
                    for (auto &x : xs) {
                        std::tuple<Value *, Value *, Cmp::Pred> tmp;
                        std::get<0>(tmp) = std::get<0>(x);
                        std::get<1>(tmp) = rhs;
                        if (std::get<1>(x) == LINR_P) {
                            // x op e
                            if (c.second == 0) {
                                // in the left branch
                                std::get<2>(tmp) = Cmp::getOp(cmp);
                            } else {
                                // in the right branch
                                std::get<2>(tmp) = Cmp::flipOp(Cmp::getOp(cmp));
                            }

                        } else if (std::get<1>(x) == LINR_N) {
                            if (c.second == 0) {
                                // in the left branch
                                std::get<2>(tmp) = Cmp::flipOp(Cmp::getOp(cmp));
                            } else {
                                // in the right branch
                                std::get<2>(tmp) = Cmp::getOp(cmp);
                            }
                        } else {
                            errs() << "sign cannot be determined in conditional\n";
                            exit(-1);
                        }
                        res.push_back(tmp);

                    }

                    // for recurrence variable in rhs
                    auto xss = checkLinear(rhs, cond_all);
                    for (auto &x : xss) {
                        std::tuple<Value *, Value *, Cmp::Pred> tmp;
                        std::get<0>(tmp) = std::get<0>(x);
                        std::get<1>(tmp) = lhs;
                        if (std::get<1>(x) == LINR_P) {
                            // x op e
                            if (c.second == 1) {
                                // in the right branch
                                std::get<2>(tmp) = Cmp::getOp(cmp);
                            } else {
                                // in the left branch
                                std::get<2>(tmp) = Cmp::flipOp(Cmp::getOp(cmp));
                            }

                        } else if (std::get<1>(x) == LINR_N) {
                            if (c.second == 0) {
                                // in the right branch
                                std::get<2>(tmp) = Cmp::flipOp(Cmp::getOp(cmp));
                            } else {
                                // in the left branch
                                std::get<2>(tmp) = Cmp::getOp(cmp);
                            }
                        } else {
                            errs() << "sign cannot be determined in conditional\n";
                            exit(-1);
                        }
                        res.push_back(tmp);
                    }
                }

            }
        }


        return res;
    }

    void addEdge(std::map<Value*, std::map<Value*, std::vector<std::tuple<Ftype, double*, bool, bool>>>> &pdg, Value *src, Value *dest, std::tuple<Ftype, double*, bool, bool> ft) {
        if (pdg[src].find(dest) != pdg[src].end()) {
            pdg[src][dest].push_back(ft);
        } else {
            pdg[src][dest] = std::vector<std::tuple<Ftype, double*, bool, bool>>();
            pdg[src][dest].push_back(ft);
        }
    }


    bool hasConstantAssign(Value *exp) {
        for (auto *si : recur_vars[exp]) {
            if (isConstant(si->getValueOperand())) return true;
        }
        return false;
    }


// TODO: this function need improvement in future, e.g., check connecting point
    AdjList combinePieces(const std::map<Value*, std::map<Value*, std::vector<std::tuple<Ftype, double*, bool, bool>>>> &pdg) {
        AdjList g;

        for (auto &v : recur_vars) {
            g[v.first] = std::map<Value *, std::tuple<Ftype, double *, bool>>();
        }


        for (auto &e : pdg) {
            auto &src = e.first;
            auto &alist = e.second;
            for (auto &ee : alist) {
                auto &dest = ee.first;
                auto &fts = ee.second;

                std::vector<std::tuple<Ftype, double *, bool, bool>> tmp;


                for (auto &ft : fts) {

                    if (std::get<3>(ft) && std::get<0>(ft) == (RECT_C | RECT_D)) {
                        bool has_left_piece = false;
                        for (auto &ftt : fts) {
                            if ((std::get<0>(ftt) == RECT_C || std::get<0>(ftt) == RECT_D || std::get<0>(ftt) == (RECT_C | RECT_D))
                                    && (std::get<1>(ft) == nullptr || std::get<1>(ftt) == nullptr ||
                                        *std::get<1>(ft) == *std::get<1>(ftt))
                                    && (!std::get<3>(ftt))) {
                                has_left_piece = true;
                                tmp.push_back(ftt);
                            }
                        }
                        if (!has_left_piece) {
                            auto new_ft = ft;
                            std::get<0>(new_ft) = RECT_D;
                            if (src == dest) {
                                std::get<1>(new_ft) = new double(1);
                                std::get<2>(new_ft) = true;
                            } else {
                                std::get<1>(new_ft) = new double(0);
                                std::get<2>(new_ft) = false;
                            }
                            tmp.push_back(new_ft);
                        }
                    } else if (!std::get<3>(ft) && std::get<0>(ft) == (RECT_A | RECT_B)) {
                        bool has_right_piece = false;
                        for (auto &ftt : fts) {
                            if ((std::get<0>(ftt) == RECT_A || std::get<0>(ftt) == RECT_B || std::get<0>(ftt) == (RECT_A | RECT_B) )
                                    && (std::get<1>(ft) == nullptr || std::get<1>(ftt) == nullptr ||
                                        *std::get<1>(ft) == *std::get<1>(ftt))
                                    && (std::get<3>(ftt))) {
                                has_right_piece = true;
                                tmp.push_back(ftt);
                            }
                        }
                        if (!has_right_piece) {
                            auto new_ft = ft;
                            std::get<0>(new_ft) = RECT_A;
                            if (src == dest) {
                                std::get<1>(new_ft) = new double(1);
                                std::get<2>(new_ft) = true;
                            } else {
                                std::get<1>(new_ft) = new double(0);
                                std::get<2>(new_ft) = false;
                            }
                            tmp.push_back(new_ft);
                        }
                    } else {
                        tmp.push_back(ft);
                    }
                }
                if (!tmp.empty()) {
                    std::tuple<Ftype, double *, bool> merged_edge = std::make_tuple(std::get<0>(tmp[0]), std::get<1>(tmp[0]), std::get<2>(tmp[0]));
//errs() << std::get<0>(merged_edge).to_string() << "\n";
                    for (auto &tt : tmp) {
                        merged_edge = mergeRelation(merged_edge, std::make_tuple(std::get<0>(tt), std::get<1>(tt), std::get<2>(tt)));
                    }

                    g[src][dest] = merged_edge;
                }
            }
        }


        for (auto &h : g) {
            auto &src = h.first;
            for (auto &hh : h.second) {
                auto &dest = hh.first;
                if (hasConstantAssign(dest) && isLinr(std::get<0>(hh.second))) {
                    std::get<1>(hh.second) = nullptr;
                    std::get<2>(hh.second) = false;
                }

#define DEBUG_TYPE "print_pdg"
                DEBUG(
                    errs() << "--------------\n";
                    src->dump();
                    dest->dump();
                    errs() << std::get<0>(hh.second).to_string() << "  ";

                if (std::get<1>(hh.second)) {
                errs() << *(std::get<1>(hh.second)) << "  ";
                } else {
                    errs() << "NULL" << "  ";
                }

                errs() << std::get<2>(hh.second) << "\n";
                       errs() << "--------------\n";);
#undef DEBUG_TYPE
            }
        }
        return g;
    }



    bool isSameFtype(const std::vector<Relation> &ra, const std::vector<Relation> &rb) {



        if (ra.size() != rb.size()) return false;

        for (unsigned i = 0; i < ra.size(); i++) {
            if (std::get<0>(ra[i]) != std::get<0>(rb[i]) || std::get<1>(ra[i]) != std::get<1>(rb[i])
                    ||  (std::get<2>(ra[i]) && std::get<2>(rb[i]) && *std::get<2>(ra[i]) != *(std::get<2>(rb[i])))
                    || (std::get<2>(ra[i]) && std::get<2>(rb[i]) == nullptr)
                    || (std::get<2>(rb[i]) && std::get<2>(ra[i]) == nullptr)) return false;
        }

        return true;
    }


    bool isValidPath(const std::vector<Cond> &path, StoreInst* si) {

        for (unsigned i = 1; i < path.size(); i++) {
            if (!isReachable(path[i - 1].first->getSuccessor(path[i - 1].second), path[i].first)) return false;
        }
        auto *bb = path.back().first->getSuccessor(path.back().second);
        for (unsigned i = 0; i < path.size(); i++) {
            if (!isReachable(path[i].first->getSuccessor(path[i].second), si)) return false;
        }
        return true;

    }


    std::map<Assign, std::vector<std::pair<std::vector<Cond>, std::vector<Cond>>>> mergeCAPairs(const std::vector<std::pair<const std::vector<Cond>, Assign>> &S) {

        std::map<Assign, std::vector<std::pair<std::vector<Cond>, std::vector<Cond>>>> res;

        for (auto &ca : S) {
            Assign asgn = ca.second;
            if (res.find(asgn) == res.end()) res[asgn] = std::vector<std::pair<std::vector<Cond>, std::vector<Cond>>>();
            res[asgn].push_back(std::make_pair(ca.first, ca.first));
        }

        for (auto &acs : res) {
            Assign asgn = acs.first;
            Value *v = asgn->getPointerOperand();
            Value *e = asgn->getValueOperand();

            if (!isRecurVar(v)) {
                errs() << "non recurrence variable in store instruction, error\n";
                exit(-1);
            }

            // merge the redundant comparison instructions
            for (auto &cps : acs.second) {
                auto &to_merge = cps.first;


                auto flip = cps.first;
                std::set<unsigned> to_delete;
                std::vector<Cond> merged_cond;

                for (unsigned i = 0; i < to_merge.size(); i++) {
                    flip[i].second = 1 - flip[i].second;


                    auto ra = checkLinear(e, to_merge);
                    auto rb = checkLinear(e, flip);
                    if (isValidPath(flip, asgn) && isSameFtype(ra, rb) ) {
                        to_delete.insert(i);
                    }

                    flip[i].second = 1 - flip[i].second;
                }

                for (unsigned i = 0; i < to_merge.size(); i++) {
                    if (to_delete.find(i) == to_delete.end()) {
                        merged_cond.push_back(to_merge[i]);
                    }
                }

                cps.first = merged_cond;

            }


        }

        return res;
    }



    AdjList buildPartialDependenceGraph(const std::vector<std::pair<const std::vector<Cond>, Assign>> &S) {
        std::map<Value*, std::map<Value*, std::vector<std::tuple<Ftype, double*, bool, bool>>>> pdg;
        // add all recurrence variables as nodes
        for (auto &v : recur_vars) {
            pdg[v.first] = std::map<Value*, std::vector<std::tuple<Ftype, double*, bool, bool>>>();
        }

        auto assign2conds = mergeCAPairs(S);
        std::map<Assign, std::vector<std::pair<std::vector<std::tuple<Value *, Value *, Cmp::Pred>>, std::vector<Cond>>>> assign2sbe;


        for (auto &acs : assign2conds) {

            Assign asgn = acs.first;
            for (auto &cc : acs.second) {
                // when separating the recurrence variables in comparison instructions,
                // it is important to pass all the conditional including non-recurrence variable, because we need them to
                // determined the coef if available
                std::vector<std::tuple<Value *, Value *, Cmp::Pred>> sbe = separateVars(cc);

                if (assign2sbe.find(asgn) == assign2sbe.end()) assign2sbe[asgn] = std::vector<std::pair<std::vector<std::tuple<Value *, Value *, Cmp::Pred>>, std::vector<Cond>>>();
                assign2sbe[asgn].emplace_back(make_pair(sbe, cc.second));

            }
        }

        for (auto &as : assign2sbe) {

            Assign asgn = as.first;
            Value *v = asgn->getPointerOperand();
            Value *e = asgn->getValueOperand();

            for (auto &sbe : as.second) {
                auto rels = checkLinear(e, sbe.second);

                for (auto &r : rels) {
                    addEdge(pdg, std::get<0>(r), v, std::make_tuple(std::get<1>(r), std::get<2>(r), std::get<3>(r), false));
                }

                for (auto &be : sbe.first) {
                    if (std::get<2>(be) == Cmp::Pred::EQ) {
                        addEdge(pdg, std::get<0>(be), v, std::make_tuple(FST, (double *) nullptr, false, true));
                    } else if (std::get<2>(be) == Cmp::Pred::NE) {
                        addEdge(pdg, std::get<0>(be), v, std::make_tuple(FST, (double *) nullptr, false, false));
                    } else if (std::get<2>(be) == Cmp::Pred::GTE) {

                        bool x_in_rels = false;
                        for (auto &r : rels) {
                            if (std::get<0>(r) == std::get<0>(be)) {

                                x_in_rels = true;
                                if (std::get<1>(r) == LINR_P) {
                                    addEdge(pdg, std::get<0>(r), v, std::make_tuple(RECT_A, std::get<2>(r), std::get<3>(r), true));
                                } else if (std::get<1>(r) == LINR_N) {
                                    addEdge(pdg, std::get<0>(r), v, std::make_tuple(RECT_B, std::get<2>(r), std::get<3>(r), true));
                                } else if (std::get<1>(r) == (LINR_P | LINR_N)) {
                                    addEdge(pdg, std::get<0>(r), v,
                                            std::make_tuple(RECT_A | RECT_B, std::get<2>(r), std::get<3>(r), true));
                                }
                            }
                        }
                        if (!x_in_rels) {
                            addEdge(pdg, std::get<0>(be), v,
                                    std::make_tuple(RECT_C | RECT_D, (double *) nullptr, true, true));
                        }
                    } else if (std::get<2>(be) == Cmp::Pred::LTE) {

                        bool x_in_rels = false;
                        for (auto &r : rels) {
                            if (std::get<0>(r) == std::get<0>(be)) {

                                x_in_rels = true;
                                if (std::get<1>(r) == LINR_P) {
                                    addEdge(pdg, std::get<0>(r), v, std::make_tuple(RECT_D, std::get<2>(r), std::get<3>(r), false));
                                } else if (std::get<1>(r) == LINR_N) {
                                    addEdge(pdg, std::get<0>(r), v, std::make_tuple(RECT_C, std::get<2>(r), std::get<3>(r), false));
                                } else if (std::get<1>(r) == (LINR_P | LINR_N)) {
                                    addEdge(pdg, std::get<0>(r), v,
                                            std::make_tuple(RECT_C | RECT_D, std::get<2>(r), std::get<3>(r), false));
                                }
                            }
                        }
                        if (!x_in_rels) {

                            addEdge(pdg, std::get<0>(be), v,
                                    std::make_tuple(RECT_A | RECT_B, (double *) nullptr, true, false));
                        }
                    }
                }

            }
        }

        return combinePieces(pdg);


    }

    void outputFRG(const AdjList &res) {
        int fd = open(OutputFilename.c_str(), O_RDWR | O_CREAT |  O_TRUNC, 0644);
        raw_fd_ostream  fout(fd, true);
        for (auto &e : res) {
            auto *src = e.first;

            SmallVector<StringRef, 3> ss;
            src->getName().split(ss, "=");

            auto &alist = e.second;
            for (auto &ee : alist) {
                auto *dest = ee.first;
                auto &edge = ee.second;

                SmallVector<StringRef, 3> dd;
                dest->getName().split(dd, "=");
                fout << ss[0] << "\t";
                fout << dd[0] << "\t";

                bool first = true;
                for (unsigned i = 0; i < std::get<0>(edge).size(); i++) {
                    if (std::get<0>(edge).test(i)) {
                        Ftype t;
                        t.set(i);
                        if (!first) fout << ",";
                        first = false;
                        fout << fname[t];
                    }
                }
                fout << "\t";
                if (std::get<1>(edge)) fout << *(std::get<1>(edge)) << "\t";
                else fout << "NULL\t";
                fout << std::get<2>(edge) << "\n";
            }
        }
    }

    bool isSameGraph(AdjList &a, AdjList &b) {
        for (auto &e : a) {
            if (b.find(e.first) == b.end()) return false;
            for (auto &ee : e.second) {
                if (b[e.first].find(ee.first) == b[e.first].end()) return false;
                auto &x = ee.second;
                auto &y = b[e.first][ee.first];
                if (std::get<0>(x) != std::get<0>(y) || (std::get<1>(x) == nullptr && std::get<1>(y)) || (std::get<1>(x) && std::get<1>(y) == nullptr)  || (std::get<1>(x) && std::get<1>(y) && *std::get<1>(x) != *std::get<1>(y))  || std::get<2>(x) != std::get<2>(y)) return false;
            }
        }
        for (auto &e : b) {
            if (a.find(e.first) == a.end()) return false;
            for (auto &ee : e.second) {
                if (a[e.first].find(ee.first) == a[e.first].end()) return false;
                auto &x = ee.second;
                auto &y = a[e.first][ee.first];
                if (std::get<0>(x) != std::get<0>(y) || (std::get<1>(x) == nullptr && std::get<1>(y)) || (std::get<1>(x) && std::get<1>(y) == nullptr)   ||  (std::get<1>(x) && std::get<1>(y) && *std::get<1>(x) != *std::get<1>(y)) || std::get<2>(x) != std::get<2>(y)) return false;
            }
        }
        return true;
    }


    AdjList inductiveReasoning(AdjList pdg) {
        AdjList res;
        AdjList new_res = pdg;


        while (!isSameGraph(res, new_res)) {
            res = new_res;
            new_res.clear();



            for (auto &e : res) {
                auto *src = e.first;
                auto &alist = e.second;
                for (auto &ee : alist) {
                    auto *dest = ee.first;
                    auto &edge = ee.second;
                    for (auto &k : pdg[dest]) {
                        if (new_res.find(src) == new_res.end()) {
                            new_res[src] = std::map<Value *, std::tuple<Ftype, double *, bool>>();
                            new_res[src][k.first] = std::tuple<Ftype, double *, bool>();
                        }
                        new_res[src][k.first] = mergeRelation(new_res[src][k.first], composeRelation(ee.second, k.second));
                    }
                }
            }
        }

#define DEBUG_TYPE "print_frg"
        DEBUG(
        for (auto &e : res) {
        auto *src = e.first;
        auto &alist = e.second;
        for (auto &ee : alist) {
                auto *dest = ee.first;
                auto &edge = ee.second;
                errs() << "--------------\n";
                src->dump();
                dest->dump();
                errs () << std::get<0>(edge).to_string() << "  ";
                if (std::get<1>(edge)) errs() << *(std::get<1>(edge)) << "  ";
                else errs() << "NULL  ";
                errs() << std::get<2>(edge) << "\n";
            }
        });
#undef DEBUG_TYPE
        return res;
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override {

        AU.addRequired<LoopInfoWrapperPass>();
        AU.addPreserved<LoopInfoWrapperPass>();
        AU.addRequired<DominatorTreeWrapperPass>();
        AU.addPreserved<DominatorTreeWrapperPass>();
        AU.addRequired<PostDominatorTreeWrapperPass>();
        AU.addPreserved<PostDominatorTreeWrapperPass>();
        AU.addRequired<MemorySSAWrapperPass>();
        AU.addPreserved<MemorySSAWrapperPass>();
    }

    bool runOnFunction(Function &F) override {

        LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
        MA = &getAnalysis<MemorySSAWrapperPass>().getMSSA();

        for (auto *L : *LI) {

            identifyRecurVars(L);

            auto S = getCondAssignPairs();

            auto pdg = buildPartialDependenceGraph(S);

            auto frg = inductiveReasoning(pdg);

            outputFRG(frg);

            // only one loop in our analysis
            break;
        }

        return false;
    }

};

char FuncTypePass::ID = 0;

static RegisterPass<FuncTypePass> X("ftype", "Func Type Pass", false, false);
