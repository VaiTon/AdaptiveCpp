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
#include "hipSYCL/compiler/utils/CompleteCallSet.hpp"

using namespace hipsycl::compiler::detail;

CompleteCallSet::CompleteCallSet(clang::Decl *D) { TraverseDecl(D); }

bool CompleteCallSet::VisitFunctionDecl(clang::FunctionDecl *FD) {
  visitedDecls.insert(FD);
  return true;
}

bool CompleteCallSet::VisitCallExpr(clang::CallExpr *CE) {
  if (auto Callee = CE->getDirectCallee())
    TraverseDecl(Callee);
  return true;
}

bool CompleteCallSet::VisitCXXConstructExpr(clang::CXXConstructExpr *CE) {
  if (auto Callee = CE->getConstructor()) {
    TraverseDecl(Callee);
    // Since for destructor calls no explicit AST nodes are created, we simply use this
    // opportunity to find the corresponding destructor for all constructed types (since we assume
    // that every type that can be constructed on the GPU also can and will be destructed).
    if (auto Ptr = llvm::dyn_cast_or_null<clang::PointerType>(
            Callee->getThisType()->getCanonicalTypeUnqualified()))
      if (auto Record = llvm::dyn_cast<clang::RecordType>(Ptr->getPointeeType()))
        if (auto RecordDecl = llvm::dyn_cast<clang::CXXRecordDecl>(Record->getDecl()))
          if (auto DtorDecl = RecordDecl->getDestructor())
            TraverseDecl(DtorDecl);
  }
  return true;
}

bool CompleteCallSet::TraverseDecl(clang::Decl *D) {
  // fixme: investigate where the invalid decls come from..
  if (!D)
    return true;

  clang::Decl *DefinitionDecl = D;
  clang::FunctionDecl *FD = clang::dyn_cast<clang::FunctionDecl>(D);

  if (FD) {
    const clang::FunctionDecl *ActualDefinition;
    if (FD->isDefined(ActualDefinition)) {

      DefinitionDecl = const_cast<clang::FunctionDecl *>(ActualDefinition);
    }
  }

  if (visitedDecls.find(llvm::dyn_cast_or_null<clang::FunctionDecl>(DefinitionDecl)) ==
      visitedDecls.end())
    return clang::RecursiveASTVisitor<CompleteCallSet>::TraverseDecl(DefinitionDecl);

  return true;
}