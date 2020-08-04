#ifndef LLVM_TRANSFORMS_INSTRUMENTATION_TEMPSANPASS_H
#define LLVM_TRANSFORMS_INSTRUMENTATION_TEMPSANPASS_H

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"

namespace llvm {
class TempSanitizerPass : public PassInfoMixin<TempSanitizerPass> {
public:
  explicit TempSanitizerPass();
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};

ModulePass *createTempSanitizerLegacyPassPass();


} // namespace llvm


#endif // LLVM_TRANSFORMS_INSTRUMENTATION_TEMPSANPASS_H
