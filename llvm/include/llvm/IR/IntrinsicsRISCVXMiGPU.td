//===- IntrinsicsRISCVMiGPU.td - MiGPU intrinsics ----------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines all of the MiGPU vendor intrinsics for RISC-V.
//
//===----------------------------------------------------------------------===//

let TargetPrefix = "riscv" in {
    def int_riscv_mi_split :
        Intrinsic<[], [llvm_i32_ty], [IntrNoMem, IntrNoReturn, IntrHasSideEffects]>;
    def int_riscv_mi_join : ClangBuiltin<"__builtin_riscv_mi_join">,
        Intrinsic<[], [], [IntrNoMem, IntrNoReturn, IntrHasSideEffects]>;
    def int_riscv_mi_barrier : ClangBuiltin<"__builtin_riscv_mi_barrier">,
        Intrinsic<[], [], [IntrNoMem, IntrNoReturn, IntrHasSideEffects]>;
}
