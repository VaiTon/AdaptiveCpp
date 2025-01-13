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
#ifndef HIPSYCL_ATTRIBUTES_HPP
#define HIPSYCL_ATTRIBUTES_HPP

#include <string>

#include "clang/AST/Attr.h"

namespace hipsycl::compiler {

class AddonAttribute {
public:
  explicit AddonAttribute(const std::string &name);

  std::string getString() const;
  bool describedBy(clang::Attr *attrib) const;
  bool isAttachedTo(clang::FunctionDecl *F) const;

private:
  std::string Name;
};

inline AddonAttribute::AddonAttribute(const std::string &name) : Name(name) {}

inline std::string AddonAttribute::getString() const {
  return "__attribute__((diagnose_if(false,\"" + Name + ",\"warning\")))";
}
inline bool AddonAttribute::describedBy(clang::Attr *attrib) const {
  if (clang::isa<clang::DiagnoseIfAttr>(attrib)) {
    clang::DiagnoseIfAttr *attr = clang::cast<clang::DiagnoseIfAttr>(attrib);
    if (attr->getMessage() == Name)
      return true;
  }
  return false;
}
inline bool AddonAttribute::isAttachedTo(clang::FunctionDecl *F) const {
  if (clang::Attr *A = F->getAttr<clang::DiagnoseIfAttr>())
    return describedBy(A);
  return false;
}

class KernelAttribute : public AddonAttribute {
public:
  KernelAttribute() : AddonAttribute{"hipsycl_kernel"} {}
};

class CustomAttributes {
public:
  static const KernelAttribute SyclKernel;
};

const KernelAttribute CustomAttributes::SyclKernel = KernelAttribute{};

} // namespace hipsycl::compiler

#endif
