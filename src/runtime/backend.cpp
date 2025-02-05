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
#include "hipSYCL/runtime/backend.hpp"
#include "hipSYCL/runtime/application.hpp"
#include "hipSYCL/runtime/device_id.hpp"
#include "hipSYCL/runtime/error.hpp"
#include "hipSYCL/runtime/hw_model/hw_model.hpp"
#include "hipSYCL/runtime/hardware.hpp"
#include "hipSYCL/runtime/kernel_cache.hpp"

#include <algorithm>

namespace hipsycl {
namespace rt {

backend_manager::backend_manager()
  : _hw_model(std::make_unique<hw_model>(this)),
    _kernel_cache{kernel_cache::get()}
{

  _loader.query_backends();

  for (std::size_t b_idx = 0; b_idx < _loader.get_num_backends(); ++b_idx) {

    HIPSYCL_DEBUG_INFO << "Registering backend: '"
                       << _loader.get_backend_name(b_idx) << "'..."
                       << std::endl;

    backend *backend_ptr = _loader.create(b_idx);

    if (backend_ptr == nullptr) {
      HIPSYCL_DEBUG_ERROR << "backend_manager: Backend creation failed"
                          << std::endl;
      continue;
    }

    _backends.emplace_back(std::shared_ptr<backend>(backend_ptr));
  }

  for (const auto &b : _backends) {
    HIPSYCL_DEBUG_INFO << "Discovered devices from backend '" << b->get_name()
                       << "': " << std::endl;
    backend_hardware_manager* hw_manager = b->get_hardware_manager();
    if(hw_manager->get_num_devices() == 0) {
      HIPSYCL_DEBUG_INFO << "  <no devices>" << std::endl;
    } else {
      for(std::size_t i = 0; i < hw_manager->get_num_devices(); ++i){
        hardware_context* hw = hw_manager->get_device(i);

        HIPSYCL_DEBUG_INFO << "  device " << i << ": " << std::endl;
        HIPSYCL_DEBUG_INFO << "    vendor: " << hw->get_vendor_name() << std::endl;
        HIPSYCL_DEBUG_INFO << "    name: " << hw->get_device_name() << std::endl;
      }
    }
  }

  auto is_cpu = [](const std::shared_ptr<backend> &b) {
    return b->get_hardware_platform() == hardware_platform::cpu;
  };

  if (std::none_of(_backends.cbegin(), _backends.cend(), is_cpu)) {
    HIPSYCL_DEBUG_ERROR << "No CPU backend has been loaded. Terminating."
                        << std::endl;
    std::terminate();
  }
}

backend_manager::~backend_manager()
{
  _kernel_cache->unload();
}

std::shared_ptr<backend> backend_manager::get(backend_id id) const {

  auto has_same_id = [id](const std::shared_ptr<backend> &b) {
    return b->get_backend_descriptor().id == id;
  };

  const auto it =
      std::find_if(_backends.begin(), _backends.end(), has_same_id);

  if (it == _backends.end()) {
    register_error(
        __acpp_here(),
        error_info{"backend_manager: Requested backend is not available.",
                   error_type::runtime_error});

    return nullptr;
  }

  return *it;
}

hw_model &backend_manager::hardware_model()
{
  return *_hw_model;
}

const hw_model &backend_manager::hardware_model() const
{
  return *_hw_model;
}

}
}
