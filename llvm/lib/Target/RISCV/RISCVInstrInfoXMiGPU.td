let Predicates = [HasVendorXMiGPU, IsRV32], hasSideEffects = 1, mayLoad = 0,
    mayStore = 0 in {
def MI_SPLIT :
    RVInstR<0b0000000, 0b000, OPC_CUSTOM_0, (outs), (ins GPR:$rs1), "mi.split", "$rs1">,
    Sched<[]> {
    let rs2 = 0;
    let rd = 0;
}

def MI_JOIN :
    RVInstR<0b0000000, 0b001, OPC_CUSTOM_0, (outs), (ins), "mi.join", "">,
    Sched<[]> {
    let rs2 = 0;
    let rs1 = 0;
    let rd = 0;
}

def MI_BARRIER :
    RVInstR<0b0000000, 0b010, OPC_CUSTOM_0, (outs), (ins), "mi.barrier", "">,
    Sched<[]> {
    let rs2 = 0;
    let rs1 = 0;
    let rd = 0;
}
} //

// Pattern matching for MiGPU instructions
def : Pat<(int_riscv_mi_split GPR:$in), (MI_SPLIT GPR:$in)>;
def : Pat<(int_riscv_mi_join), (MI_JOIN)>;
def : Pat<(int_riscv_mi_barrier), (MI_BARRIER)>;
