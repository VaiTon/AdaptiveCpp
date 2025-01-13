//===- src/compiler/cbs/Region.cpp - abstract CFG region --*- C++ -*-===//
//
// Adapted from the RV Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Adaptiations: Merged Region definitions in a single file.
//
//===----------------------------------------------------------------------===//
//

#include "hipSYCL/compiler/cbs/Region.hpp"

#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/IR/Function.h>

using namespace llvm;
using namespace hipsycl::compiler;

void RegionImpl::forEachBlock(const BlockPredicate &UserFunc) const {
  auto *Function = getRegionEntry().getParent();
  for (const auto &BB : *Function) {
    if (!contains(&BB)) {
      continue;
    }

    bool carryOn = UserFunc(BB);
    if (!carryOn) {
      break;
    }
  }
}

void RegionImpl::getEndingBlocks(SmallPtrSet<BasicBlock *, 2> &endingBlocks) const {
  assert(endingBlocks.empty());

  std::stack<BasicBlock *> blockStack;
  blockStack.push(&this->getRegionEntry());

  std::set<BasicBlock *> visitedBlocks;

  while (!blockStack.empty()) {
    // Pop the next block
    BasicBlock *block = blockStack.top();
    blockStack.pop();

    // Make sure we haven't seen it already
    if (visitedBlocks.count(block))
      continue;
    visitedBlocks.insert(block);

    // If a successor is outside the region, the region ends here.
    // Successors inside the region need to be processed recursively
    for (BasicBlock *successor : successors(block)) {
      if (this->contains(successor)) {
        blockStack.push(successor);
      } else {
        endingBlocks.insert(successor);
      }
    }
  }
}

Region::Region(RegionImpl &Impl) : mImpl(Impl) {}
std::string Region::str() const { return mImpl.str(); }
void Region::add(const BasicBlock &extra) { extraBlocks.insert(&extra); }

bool Region::contains(const BasicBlock *BB) const {
  if (extraBlocks.count(BB))
    return true;
  else
    return mImpl.contains(BB);
}

void Region::getEndingBlocks(SmallPtrSet<BasicBlock *, 2> &endingBlocks) const {
  mImpl.getEndingBlocks(endingBlocks);
}
BasicBlock &Region::getRegionEntry() const { return mImpl.getRegionEntry(); }
Function &Region::getFunction() { return *getRegionEntry().getParent(); }
const Function &Region::getFunction() const { return *getRegionEntry().getParent(); }

bool Region::isVectorLoop() const { return mImpl.isVectorLoop(); }

void Region::forEachBlock(const BlockPredicate &userFunc) const {
  mImpl.forEachBlock(userFunc);
  for (auto *block : extraBlocks)
    userFunc(*block);
}

void Region::forBlocksRpo(const BlockPredicate &userFunc) const {
  const Function &F = *getRegionEntry().getParent();
  ReversePostOrderTraversal RPOT{&F};

  for (auto *BB : RPOT) {
    if (mImpl.contains(BB) || extraBlocks.count(BB))
      userFunc(*BB);
  }
}

LoopRegion::LoopRegion(Loop &_loop) : loop(_loop) {}

LoopRegion::~LoopRegion() {}

bool LoopRegion::contains(const BasicBlock *BB) const { return loop.contains(BB); }

BasicBlock &LoopRegion::getRegionEntry() const { return *loop.getHeader(); }

void LoopRegion::getEndingBlocks(SmallPtrSet<BasicBlock *, 2> &endingBlocks) const {
  SmallVector<BasicBlock *, 2> endingBlocksVector;
  loop.getExitBlocks(endingBlocksVector);

  for (const auto &endingBB : endingBlocksVector) {
    endingBlocks.insert(endingBB);
  }
}

std::string LoopRegion::str() const {
  auto loopHeaderName = loop.getHeader()->getName();
  return ("LoopRegion (header " + loopHeaderName + ")").str();
}

FunctionRegion::FunctionRegion(Function &F, const ArrayRef<BasicBlock *> Blocks)
    : F{F}, Blocks{Blocks.begin(), Blocks.end()} {}

FunctionRegion::~FunctionRegion() {}
bool FunctionRegion::contains(const BasicBlock *BB) const { return Blocks.contains(BB); }
BasicBlock &FunctionRegion::getRegionEntry() const { return F.getEntryBlock(); }
void FunctionRegion::getEndingBlocks(SmallPtrSet<BasicBlock *, 2> &endingBlocks) const {
  for (auto *BB : Blocks) {
    if (BB->getTerminator()->getNumSuccessors() == 0)
      endingBlocks.insert(BB);
  }
}

std::string FunctionRegion::str() const { return ("FunctionRegion (" + F.getName() + ")").str(); }
bool FunctionRegion::isVectorLoop() const { return false; }
