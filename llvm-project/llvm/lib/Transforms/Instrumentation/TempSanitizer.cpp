#include "llvm/Pass.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Triple.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Use.h"
#include "llvm/IR/Value.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Instrumentation.h"
#include "llvm/Transforms/Utils/ASanStackFrameLayout.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include "llvm/Transforms/Instrumentation/TempSanitizerPass.h"
#include <algorithm>
#include <cstdint>
#include <string>
#include <tuple>

using namespace llvm;
#define DEBUG_TYPE "tempsan"

namespace {

// class PrepareTempSanitizerStructList **************************************************

class PrepareTempSanitizerStructList {
private:
  std::vector<StringRef>  TempSanitizerStructList;
  void initTempSanitizerStructList();

public:
  bool Initialize() {
    initTempSanitizerStructList();
    return false;
  }

  std::vector<StringRef> *getTempSanitizerStructList() {
    return &TempSanitizerStructList;
  }

}; /* class PrepareTempSanitizerStructList */

/* PrepareTempSanitizerStructList::initTempSanitizerStructList() */
void PrepareTempSanitizerStructList::initTempSanitizerStructList() {
  assert((!isWhite || !isBlack) && "cannot use both whitelist and blacklist");

  TempSanitizerStructList.push_back("task_struct");
  TempSanitizerStructList.push_back("");

  return;
} /* PrepareTempSanitizerStructList::initTempSanitizerStructList() */
/*  class StructAccessInst */
class StructAccessInst {
private:
  std::vector<StringRef> *TempSanitizerStructList;

public:
  static char ID;
  LLVMContext *C;
  Type *VoidTy;
  Type *Int8Ty;
  Type *Int32Ty;
  Type *Int64Ty;
  Type *Int8PtrTy;
  Type *Int32PtrTy;
  Type *Int64PtrTy;

  StructAccessInst() {}

  bool Initialize(Module &M, std::vector<StringRef> *List) {
    C = &(M.getContext());
    VoidTy     = Type::getVoidTy(*C);
    Int8Ty     = Type::getInt8Ty(*C);
    Int32Ty    = Type::getInt32Ty(*C);
    Int64Ty    = Type::getInt64Ty(*C);
    Int8PtrTy  = Type::getInt8PtrTy(*C);
    Int32PtrTy = Type::getInt32PtrTy(*C);
    Int64PtrTy = Type::getInt64PtrTy(*C);
    TempSanitizerStructList = List;
  }
  bool Run(Module &M);
  bool IsStructAccess(Instruction &I);
  bool IsStructPtrAccess(Instruction &I);
  bool IsInterestingStructType(Type *T);
}; /*  class StructAccessInst */

bool StructAccessInst::Run(Module &M){
  for (auto &F : M) {
    for (auto &B : F) {
      for (auto &I : B) {
        if (IsStructAccess(I) | IsStructPtrAccess(I)) {
          errs() << I << "\n";
       }
      }
    }
  }
  return true;
}

bool StructAccessInst::IsStructAccess(Instruction &I) {
  bool hasTempSanitizerStruct = false;
  for (auto &O : I.operands()) {
    if (IsInterestingStructType(O.get()->getType()))
      hasTempSanitizerStruct |= true;
  }
  return hasTempSanitizerStruct;
}

bool StructAccessInst::IsStructPtrAccess(Instruction &I) {
  bool hasTempSanitizerStruct = false;
  for (auto &O : I.operands()) {
    if (auto PTy = dyn_cast<PointerType>(O.get()->getType())){
      if (IsInterestingStructType(PTy->getElementType()))
        hasTempSanitizerStruct |= true;
    }
  }
  return hasTempSanitizerStruct;
}

bool StructAccessInst::IsInterestingStructType(Type *T) {

  if (!T->isStructTy())
    return false;
 
  StructType *STy = dyn_cast<StructType>(T);
  StringRef Name = STy->getName();

  if (Name.startswith("struct."))
    Name.consume_front("struct.");
 
  if (Name.startswith("anon"))
    return false;

  return (find(TempSanitizerStructList->begin(), TempSanitizerStructList->end(), Name)
      != TempSanitizerStructList->end());
}

/*  class TempSanitizerLegacyPass */
class TempSanitizerLegacyPass : public ModulePass {
public:
  static char ID;
  TempSanitizerLegacyPass() : ModulePass(ID) {}

  bool doInitialization(Module &M) {
    LLVMContext &C = M.getContext();
     return true;
  }

  virtual bool runOnModule(Module &M) {
    errs() << "TempSanitizer\n";

    PrepareTempSanitizerStructList Prep;
    Prep.Initialize();
    std::vector<StringRef> *List = Prep.getTempSanitizerStructList();

    StructAccessInst SAI;
    SAI.Initialize(M, List);
    SAI.Run(M);
    return true;
  }
}; // class TempSanitizerLegacyPass

} // namespace

char TempSanitizerLegacyPass::ID = 0;
INITIALIZE_PASS_BEGIN(TempSanitizerLegacyPass, "TempSanitizer", "TempSanitizerPass: Sanitizer Template",
                      false, false);
INITIALIZE_PASS_END(TempSanitizerLegacyPass, "TempSanitizer", "TempSanitizerPass: Sanitizer Template",
                    false, false);
ModulePass *llvm::createTempSanitizerLegacyPassPass() {
  return new TempSanitizerLegacyPass();
}
 
TempSanitizerPass::TempSanitizerPass() {}
PreservedAnalyses TempSanitizerPass::run(Function &F, FunctionAnalysisManager &FAM) {}
