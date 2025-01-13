/*
 * This file is part of AdaptiveCpp, an implementation of SYCL and C++ standard
 * parallelism for CPUs and GPUs.
 *
 * Copyright The AdaptiveCpp Contributors
 *
 * AdaptiveCpp is released under the BSD 2-Clause "Simplified" License.
 * See file LICENSE in the project root for full license details.
 */
// SPDX-License-Identifier: BSD-2-Clause
#ifndef COMPLETECALLSET_H
#define COMPLETECALLSET_H

#include "clang/AST/RecursiveASTVisitor.h"

#include <unordered_set>

namespace hipsycl::compiler::detail {
///
/// Utility type to generate the set of all function declarations
/// implicitly or explicitly reachable from some initial declaration.
///
/// NOTE: Must only be used when the full translation unit is present,
/// e.g. in HandleTranslationUnitDecl, otherwise the callset
/// might not be complete.
///
class CompleteCallSet : public clang::RecursiveASTVisitor<CompleteCallSet> {
public:
  using FunctionSet = std::unordered_set<clang::FunctionDecl *>;

  explicit CompleteCallSet(clang::Decl *D);

  bool VisitFunctionDecl(clang::FunctionDecl *FD);
  bool VisitCallExpr(clang::CallExpr *CE);
  bool VisitCXXConstructExpr(clang::CXXConstructExpr *CE);
  bool TraverseDecl(clang::Decl *D);

  bool shouldWalkTypesOfTypeLocs() const { return false; }
  bool shouldVisitTemplateInstantiations() const { return true; }
  bool shouldVisitImplicitCode() const { return true; }

  const FunctionSet &getReachableDecls() const { return visitedDecls; }

private:
  FunctionSet visitedDecls;
};

}

#endif // COMPLETECALLSET_H
