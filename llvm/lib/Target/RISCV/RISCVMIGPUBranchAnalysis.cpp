#include "RISCVInstrInfo.h"
#include "RISCVRegisterInfo.h"
#include "RISCVSubtarget.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachinePostDominators.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/InitializePasses.h"

using namespace llvm;
#define DEBUG_TYPE "MiGPU"

namespace {
class RISCVMIGPUBranchAnalysis : public MachineFunctionPass {
public:
  static char ID;
  RISCVMIGPUBranchAnalysis() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override { return "RISCVMIGPUBranchAnalysis"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachinePostDominatorTreeWrapperPass>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }
};
} // namespace

char RISCVMIGPUBranchAnalysis::ID = 0;
char &llvm::RISCVMIGPUBranchAnalysisID = RISCVMIGPUBranchAnalysis::ID;

INITIALIZE_PASS_BEGIN(RISCVMIGPUBranchAnalysis, DEBUG_TYPE,
                      "MiGPU automatically insert split/join pairs", false,
                      false)
INITIALIZE_PASS_DEPENDENCY(MachinePostDominatorTreeWrapperPass)
INITIALIZE_PASS_END(RISCVMIGPUBranchAnalysis, DEBUG_TYPE,
                    "MiGPU automatically insert split/join pairs", false, false)

bool RISCVMIGPUBranchAnalysis::runOnMachineFunction(MachineFunction &MF) {
  // Init
  const auto &ST = MF.getSubtarget<RISCVSubtarget>();
  const RISCVInstrInfo *TII = ST.getInstrInfo();
  MachinePostDominatorTree *MPDT =
      &getAnalysis<MachinePostDominatorTreeWrapperPass>().getPostDomTree();
  bool HasChanged = false;
  LLVM_DEBUG(MF.viewCFG());

  // Analysis
  for (auto &MBB : MF) {
    LLVM_DEBUG(dbgs() << "Analyzing " << MBB.getName() << "...\n");
    LLVM_DEBUG(dbgs() << "Before analysis:\n");
    LLVM_DEBUG(for (auto &MI : MBB) { dbgs() << '\t' << MI; });

    // Split insertion point
    auto BRI = MBB.getFirstInstrTerminator();
    if (BRI == MBB.end() || !BRI->isConditionalBranch()) {
      LLVM_DEBUG(dbgs() << "Fall through, block ignored\n");
      continue;
    }
    auto &CurrInst = *BRI;
    HasChanged = true;

    // split
    auto Ops = BRI->getFirst2Regs();
    DebugLoc DL;
    MachineRegisterInfo *MRI = &MF.getRegInfo();
    Register Temp1, Temp2, Op1 = std::get<0>(Ops), Op2 = std::get<1>(Ops);
    Temp1 = MRI->createVirtualRegister(&RISCV::GPRRegClass);
    switch (CurrInst.getOpcode()) {
    case RISCV::BNE:
    case RISCV::BEQ:
      LLVM_DEBUG(dbgs() << "Branch Type: EQ\n");
      Temp2 = MRI->createVirtualRegister(&RISCV::GPRRegClass);
      BuildMI(MBB, BRI, DL, TII->get(RISCV::SUB), Temp1)
          .addReg(Op1)
          .addReg(Op2);
      BuildMI(MBB, BRI, DL, TII->get(RISCV::SLTIU), Temp2)
          .addReg(Temp1, RegState::Kill)
          .addImm(1);
      BuildMI(MBB, BRI, DL, TII->get(RISCV::MI_SPLIT))
          .addReg(Temp2, RegState::Kill);
      break;
    case RISCV::BGE:
    case RISCV::BGEU:
    case RISCV::BLT:
    case RISCV::BLTU:
      LLVM_DEBUG(dbgs() << "Branch Type: LT\n");
      BuildMI(MBB, BRI, DL, TII->get(RISCV::SLT), Temp1)
          .addReg(Op1)
          .addReg(Op2);
      BuildMI(MBB, BRI, DL, TII->get(RISCV::MI_SPLIT))
          .addReg(Temp1, RegState::Kill);
      break;
    default:
      llvm_unreachable("Unknown RISCV conditional branch!");
    }

    // join
    // get successors
    std::vector<MachineBasicBlock *> SUCCS;
    for (auto &S : MBB.successors()) {
      SUCCS.push_back(S);
    }
    if(SUCCS.size() != 2) {
      llvm_unreachable("Successor size expected to be 2!");
    }
    MachineBasicBlock *IPDOMBB = MPDT->findNearestCommonDominator(SUCCS);
    if(!IPDOMBB) {
      llvm_unreachable("Expects IPDOM to exist!");
    }
    BuildMI(*IPDOMBB, IPDOMBB->instr_begin(), DL, TII->get(RISCV::MI_JOIN));
    LLVM_DEBUG(dbgs() << "Insert JOIN at " << IPDOMBB->getName() << "\n");
    LLVM_DEBUG(dbgs() << "After analysis:\n");
    LLVM_DEBUG(for (auto &MI : MBB) { dbgs() << '\t' << MI; });
  }
  LLVM_DEBUG(MF.viewCFG(););
  return HasChanged;
}

FunctionPass *llvm::createRISCVMIGPUBranchAnalysisPass() {
  return new RISCVMIGPUBranchAnalysis();
}

#undef DEBUG_TYPE