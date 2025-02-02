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
#ifndef HIPSYCL_FRONTEND_HPP
#define HIPSYCL_FRONTEND_HPP

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"

#include <unordered_set>

namespace hipsycl::compiler {

class FrontendASTVisitor : public clang::RecursiveASTVisitor<FrontendASTVisitor> {
  clang::CompilerInstance &Instance;

public:
  explicit FrontendASTVisitor(clang::CompilerInstance &instance);
  ~FrontendASTVisitor();

  bool shouldVisitTemplateInstantiations() const;
  bool shouldVisitImplicitCode() const;

  // We also need to have look at all statements to identify Lambda declarations
  bool VisitStmt(clang::Stmt *S);
  bool VisitDecl(clang::Decl *D);
  bool VisitFunctionDecl(clang::FunctionDecl *f);
  bool VisitCallExpr(clang::CallExpr *Call);

  void applyAttributes();

  std::unordered_set<clang::FunctionDecl *> &getMarkedHostDeviceFunctions();

  std::unordered_set<clang::FunctionDecl *> &getKernels() { return MarkedKernels; }

private:
  std::unordered_set<clang::FunctionDecl *> MarkedHostDeviceFunctions;
  std::unordered_set<clang::FunctionDecl *> MarkedKernels;
  std::unordered_set<clang::FunctionDecl *> HierarchicalKernels;

  std::unordered_set<clang::FunctionDecl *> UserKernels;
  // Maps a Kernel name tag or kernel body type to the mangled name
  // of a kernel stub function
  std::unordered_map<const clang::RecordType *, clang::FunctionDecl *> KernelManglingNameTemplates;
  // Maps the declaration/instantiation of a kernel to the kernel body
  // (kernel lambda or function object)
  std::unordered_map<clang::FunctionDecl *, const clang::RecordType *> KernelBodies;

  std::unordered_set<clang::FunctionDecl *> HostNDKernels;
  std::unordered_set<clang::FunctionDecl *> SSCPOutliningEntrypoints;

  std::unique_ptr<clang::MangleContext> KernelNameMangler;

  // Only used on clang 13+. Name mangler that takes into account
  // the device numbering of kernel lambdas.
  std::unique_ptr<clang::MangleContext> DeviceKernelNameMangler;

  void markAsHostDevice(clang::FunctionDecl *F);
  void markAsKernel(clang::FunctionDecl *F);
  void markAsNDKernel(clang::FunctionDecl *F);
  void markAsSSCPOutliningEntrypoint(clang::FunctionDecl *F);

  void processFunctionDecl(clang::FunctionDecl *f);

  bool isPrivateMemory(const clang::VarDecl *V) const;
  bool isLocalMemory(const clang::VarDecl *V) const;

  ///
  /// Marks all variable declarations within a given block statement as shared memory,
  /// unless they are explicitly declared as a private memory type.
  ///
  /// Recurses into compound statements (i.e., a set of braces {}).
  ///
  /// NOTE TODO: It is unclear how certain other statement types should be handled.
  /// For example, should the loop variable of a for-loop be marked as shared? Probably not.
  ///
  void storeLocalVariablesInLocalMemory(clang::Stmt *BlockStmt, clang::FunctionDecl *F) const;

  void storeVariableInLocalMemory(clang::VarDecl *V) const;

  const clang::RecordType *getTemplateTypeArgument(clang::FunctionDecl *F, int TemplateArg);

  const clang::RecordType *getKernelNameTag(clang::FunctionDecl *F);

  bool isKernelUnnamed(clang::FunctionDecl *F);

  // Returns either kernel name tag or kernel body, depending on whether
  // the kernel is named or unnamed
  const clang::RecordType *getRelevantKernelNamingComponent(clang::FunctionDecl *F);

  // Should be invoked whenever a call to __acpp_hiplike_kernel stub is encountered.
  // These functions are only used to borrow demangleable kernel names in the form
  // __acpp_hiplike_kernel<KernelName>
  //
  // The kernel stubs are only used to generate mangled names
  // that can then be copied to the actual kernels.
  //
  // This is mainly used on clang 13+ where __builtin_get_device_side_mangled_name()
  // is available, but requires an actual __global__ function on which to operate.
  bool handleKernelStub(clang::FunctionDecl *F);
  bool handleKernel(clang::FunctionDecl *F, const clang::RecordType *KernelBody);

  void setKernelName(clang::FunctionDecl *F, const std::string &name);

  void nameKernelUsingTypes(clang::FunctionDecl *F, bool RenameUnnamedKernels);
  void nameKernelUsingUniqueMangler(clang::FunctionDecl *F);
  void nameKernelUsingKernelManglingStub(clang::FunctionDecl *F);
  void nameKernel(clang::FunctionDecl *F);
};

class FrontendASTConsumer final : public clang::ASTConsumer {
public:
  explicit FrontendASTConsumer(clang::CompilerInstance &I);

  bool HandleTopLevelDecl(clang::DeclGroupRef DG) override;
  void HandleTranslationUnit(clang::ASTContext &context) override;

private:
  FrontendASTVisitor Visitor;
  clang::CompilerInstance &Instance;
};

} // namespace hipsycl::compiler

#endif
