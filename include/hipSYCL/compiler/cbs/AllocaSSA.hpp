//===- hipsycl/compiler/cbs/AllocaSSA.h - state monads for allocas --*- C++ -*-===//
//
// Adapted from the RV Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Adaptations: Includes / Namespace, formatting
//
//===----------------------------------------------------------------------===//

#ifndef RV_ANALYSIS_ALLOCASSA_H
#define RV_ANALYSIS_ALLOCASSA_H

#include "hipSYCL/compiler/cbs/VectorizationInfo.hpp"

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

#include <map>

namespace hipsycl::compiler {

using AllocaInstSet = llvm::SmallPtrSet<const llvm::AllocaInst *, 2>;

llvm::raw_ostream &operator<<(llvm::raw_ostream &out, const AllocaInstSet &allocs);

// ptr provenance lattice
enum class ProvType : int32_t {
  Tracked = 0,  // only aliases with @trackedAllocs (bottom, if @trackedAllocs = \emptyset)
  External = 1, // aliases only with @trackedAllocs AND other ptr that do not alias with any allocas
  Wildcard = 2  // alises with everything (top)
};

struct PtrProvenance {
  ProvType provType;    //
  AllocaInstSet allocs; // alias allocaInsts

  explicit PtrProvenance(ProvType Type);
  PtrProvenance();

  // single allocation ctor
  explicit PtrProvenance(const llvm::AllocaInst *Inst);

  // provenance lattice join
  bool merge(const PtrProvenance &Other);

  bool isBottom() const;
  bool isTop() const;

  llvm::raw_ostream &print(llvm::raw_ostream &out) const;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &out, const PtrProvenance &prov);

enum DescType : int32_t { JoinDesc = 0, EffectDesc = 1 };

struct Desc {
  DescType descType;
  const llvm::BasicBlock *place;

  Desc(DescType Type, const llvm::BasicBlock *Place) : descType(Type), place(Place) {}
};

struct Join : Desc {
  PtrProvenance provSet; // affected allocations if this is a join of divergent, disjoint paths

  explicit Join(const llvm::BasicBlock *_place);
};

struct Effect : Desc {
  const llvm::Instruction *inst;

  explicit Effect(const llvm::Instruction *_inst);
};

// constructs SSA form for allocas
// associates every pointer value with the set of allocas it originates from
// the results of this analysis are used by the VectorizationAnalysis to track which allocas may
// remain uniform. this is crucial for stack allocated objects, such as stacks in data structure
// traversal codes.
class AllocaSSA {
public:
  explicit AllocaSSA(Region &_region) : region(_region) {}

  void compute();

  // pointer provenance
  const auto &getProvenance(const llvm::Value &val) const {
    const auto *inst = llvm::dyn_cast<const llvm::Instruction>(&val);
    if (!inst)
      return externalProvSingle;

    auto it = instProvenance.find(inst);
    if (it == instProvenance.end())
      return emptyProvSingle;

    return it->second;
  }

  const Join *getJoinNode(const llvm::BasicBlock &BB) const;

  llvm::raw_ostream &print(llvm::raw_ostream &out) const;

  ~AllocaSSA();

private:
  using DefMap = std::map<const llvm::AllocaInst *, Desc *>;

  struct BlockSummary {
    const llvm::BasicBlock &BB;
    AllocaInstSet liveAllocas; // computed during computeLiveness
    Join allocJoin;
    DefMap lastDef; // live out definitions

    explicit BlockSummary(const llvm::BasicBlock &Block) : BB(Block), allocJoin(&Block) {}

    const PtrProvenance &getJoinSet() const { return allocJoin.provSet; }
  };

  static PtrProvenance emptyProvSingle;    // lattice bottom element
  static PtrProvenance externalProvSingle; // provenance object pointing to external source

  Region &region;
  std::map<const llvm::Instruction *, PtrProvenance> instProvenance;
  std::map<const llvm::Instruction *, Effect *> instEffects; // owns the Effect objects
  std::map<const llvm::BasicBlock *, BlockSummary *> blockSummaries;

  /// Returns the BlockSummary for the given BasicBlock or nullptr if no summary exists.
  const BlockSummary *getBlockSummary(const llvm::BasicBlock &BB) const;

  /// Returns the BlockSummary for the given BasicBlock. If no summary exists, a new one is created.
  BlockSummary &requestBlockSummary(const llvm::BasicBlock &BB);

  /// Associates every (potentially) alloca-derive pointer with its provenance
  void computePointerProvenance();

  // compute liveness per alloca
  void computeLiveness();

  bool isLive(const llvm::AllocaInst &alloca, const llvm::BasicBlock &BB) const;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &out, const AllocaSSA &allocaSSA);

} // namespace hipsycl::compiler

#endif // RV_ANALYSIS_ALLOCASSA_H
