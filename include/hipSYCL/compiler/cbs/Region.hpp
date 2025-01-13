//===- hipSYCL/compiler/cbs/Region.hpp - abstract CFG region --*- C++ -*-===//
//
// Adapted from the RV Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Adapatations: Merged regions
//
//===----------------------------------------------------------------------===//

#ifndef RV_REGION_H
#define RV_REGION_H

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/CFG.h"

#include <set>
#include <stack>

namespace llvm {
class BasicBlock;
class raw_ostream;
} // namespace llvm

namespace hipsycl::compiler {

class RegionImpl {
public:
  using BlockPredicate = std::function<bool(const llvm::BasicBlock &block)>;

  virtual ~RegionImpl() {}

  virtual bool contains(const llvm::BasicBlock *BB) const = 0;
  virtual llvm::BasicBlock &getRegionEntry() const = 0;
  virtual std::string str() const = 0;

  /// Iteratively apply @UserFunc to all blocks in the region.
  /// Stop if @UserFunc returns false or all blocks have been processed.
  virtual void forEachBlock(const BlockPredicate &UserFunc) const;
  virtual void getEndingBlocks(llvm::SmallPtrSet<llvm::BasicBlock *, 2> &endingBlocks) const;

  virtual bool isVectorLoop() const = 0;
};

class Region {
public:
  using BlockPredicate = std::function<bool(const llvm::BasicBlock &block)>;

  explicit Region(RegionImpl &mImpl);

  std::string str() const;

  void add(const llvm::BasicBlock &extra);
  bool contains(const llvm::BasicBlock *BB) const;

  /// Whether the region entry is a loop header that may contain reduction phis.
  bool isVectorLoop() const;

  /// Iteratively apply @UserFunc to all blocks in the region.
  /// Stop if @UserFunc returns false or all blocks have been processed.
  void forEachBlock(const BlockPredicate &userFunc) const;

  /// Iteratively apply @userFunc to all blocks in the region in reverse post-order of the CFG.
  /// Stop if @userFunc returns false or all blocks have been prosessed, otw carry on
  void forBlocksRpo(const BlockPredicate &userFunc) const;

  void getEndingBlocks(llvm::SmallPtrSet<llvm::BasicBlock *, 2> &endingBlocks) const;

  llvm::BasicBlock &getRegionEntry() const;
  llvm::Function &getFunction();
  const llvm::Function &getFunction() const;

private:
  RegionImpl &mImpl;
  llvm::SmallPtrSet<const llvm::BasicBlock *, 32> extraBlocks;
};

// this region object captures the entire CFG of a function
class FunctionRegion final : public RegionImpl {
public:
  FunctionRegion(llvm::Function &F, llvm::ArrayRef<llvm::BasicBlock *> Blocks);
  ~FunctionRegion() override;

  bool contains(const llvm::BasicBlock *BB) const override;
  llvm::BasicBlock &getRegionEntry() const override;
  void getEndingBlocks(llvm::SmallPtrSet<llvm::BasicBlock *, 2> &endingBlocks) const override;
  std::string str() const override;
  bool isVectorLoop() const override;

private:
  llvm::Function &F;
  llvm::SmallPtrSet<llvm::BasicBlock *, 16> Blocks;
};

/// This implementation realizes regions with a single point of entry and exit.
/// All block dominated by the entry and postdominated by the exit are contained in this region
/// The region represented this way has control flow lpossibly diverge after the entry but
/// reconverge at the exit
class LoopRegion final : public RegionImpl {
public:
  explicit LoopRegion(llvm::Loop &);
  ~LoopRegion() override;

  bool contains(const llvm::BasicBlock *BB) const override;
  llvm::BasicBlock &getRegionEntry() const override;
  void getEndingBlocks(llvm::SmallPtrSet<llvm::BasicBlock *, 2> &endingBlocks) const override;
  std::string str() const override;
  bool isVectorLoop() const override { return true; }

private:
  llvm::Loop &loop;
};

} // namespace hipsycl::compiler

#endif // RV_REGION_H
