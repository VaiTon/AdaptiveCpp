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
#ifndef HIPSYCL_GLUE_BACKEND_INTEROP_HPP
#define HIPSYCL_GLUE_BACKEND_INTEROP_HPP

#include "hipSYCL/sycl/libkernel/backend.hpp"

#include "hipSYCL/runtime/device_id.hpp"
#include "hipSYCL/runtime/executor.hpp"
#include "hipSYCL/runtime/multi_queue_executor.hpp"

#include "hipSYCL/sycl/backend.hpp"

namespace hipsycl::glue {

/// Specializations should define for interop with a sycl type T:
/// \code
/// using native_T_type = <native-backend-type>
/// static native_T_type get_native_T(const T&)
/// T make_T(const native_T_type&, <potentially additional args>)
/// \endcode
/// For interop_handle, the following is required:
/// \code
/// native_queue_type get_native_queue(rt::backend_kernel_launcher*)
/// native_queue_type get_native_queue(rt::device_id, rt::backend_executor*)
/// \endcode
/// In any case, the following should be defined:
/// \code
/// static constexpr bool can_make_T = // whether make_T exists
/// static constexpr bool can_extract_native_T = // whether get_native_T exists
/// \endcode
template <sycl::backend> struct backend_interop {};

} // namespace hipsycl::glue

#include "cuda/cuda_interop.hpp"
#include "hip/hip_interop.hpp"
#include "omp/omp_interop.hpp"
#include "ze/ze_interop.hpp"

#endif
