//@HEADER
// ************************************************************************
//
//                        Kokkos v. 4.0
//       Copyright (2022) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Part of Kokkos, under the Apache License v2.0 with LLVM Exceptions.
// See https://kokkos.org/LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//@HEADER

#ifndef KOKKOS_HIP_BLOCKSIZE_DEDUCTION_HPP
#define KOKKOS_HIP_BLOCKSIZE_DEDUCTION_HPP

#include <functional>
#include <Kokkos_Macros.hpp>

#if defined(__HIPCC__)

#include <HIP/Kokkos_HIP_Instance.hpp>
#include <HIP/Kokkos_HIP_KernelLaunch.hpp>

namespace Kokkos {
namespace Impl {

enum class BlockType { Max, Preferred };

template <typename DriverType, typename LaunchBounds = Kokkos::LaunchBounds<>,
          HIPLaunchMechanism LaunchMechanism =
              DeduceHIPLaunchMechanism<DriverType>::launch_mechanism>
unsigned get_preferred_blocksize_impl() {
  if constexpr (!HIPParallelLaunch<DriverType, LaunchBounds,
                                   LaunchMechanism>::default_launchbounds()) {
    // use the user specified value
    return LaunchBounds::maxTperB;
  } else {
    if (HIPParallelLaunch<DriverType, LaunchBounds,
                          LaunchMechanism>::get_scratch_size() > 0) {
      return HIPTraits::ConservativeThreadsPerBlock;
    }
    return HIPTraits::MaxThreadsPerBlock;
  }
}

template <typename DriverType, typename LaunchBounds = Kokkos::LaunchBounds<>,
          HIPLaunchMechanism LaunchMechanism =
              DeduceHIPLaunchMechanism<DriverType>::launch_mechanism>
constexpr unsigned get_max_blocksize_impl() {
  if constexpr (!HIPParallelLaunch<DriverType, LaunchBounds,
                                   LaunchMechanism>::default_launchbounds()) {
    // use the user specified value
    return LaunchBounds::maxTperB;
  } else {
    // we can always fit 1024 threads blocks if we only care about registers
    // ... and don't mind spilling
    return HIPTraits::MaxThreadsPerBlock;
  }
}

// convenience method to select and return the proper function attributes
// for a kernel, given the launch bounds et al.
template <typename DriverType, typename LaunchBounds = Kokkos::LaunchBounds<>,
          BlockType BlockSize = BlockType::Max,
          HIPLaunchMechanism LaunchMechanism =
              DeduceHIPLaunchMechanism<DriverType>::launch_mechanism>
hipFuncAttributes get_hip_func_attributes_impl() {
#ifndef KOKKOS_ENABLE_HIP_MULTIPLE_KERNEL_INSTANTIATIONS
  return HIPParallelLaunch<DriverType, LaunchBounds,
                           LaunchMechanism>::get_hip_func_attributes();
#else
  if constexpr (!HIPParallelLaunch<DriverType, LaunchBounds,
                                   LaunchMechanism>::default_launchbounds()) {
    // for user defined, we *always* honor the request
    return HIPParallelLaunch<DriverType, LaunchBounds,
                             LaunchMechanism>::get_hip_func_attributes();
  } else {
    if constexpr (BlockSize == BlockType::Max) {
      return HIPParallelLaunch<
          DriverType, Kokkos::LaunchBounds<HIPTraits::MaxThreadsPerBlock, 1>,
          LaunchMechanism>::get_hip_func_attributes();
    } else {
      const int blocksize =
          get_preferred_blocksize_impl<DriverType, LaunchBounds,
                                       LaunchMechanism>();
      if (blocksize == HIPTraits::MaxThreadsPerBlock) {
        return HIPParallelLaunch<
            DriverType, Kokkos::LaunchBounds<HIPTraits::MaxThreadsPerBlock, 1>,
            LaunchMechanism>::get_hip_func_attributes();
      } else {
        return HIPParallelLaunch<
            DriverType,
            Kokkos::LaunchBounds<HIPTraits::ConservativeThreadsPerBlock, 1>,
            LaunchMechanism>::get_hip_func_attributes();
      }
    }
  }
#endif
}

// Given an initial block-size limitation based on register usage
// determine the block size to select based on LDS limitation
template <BlockType BlockSize, class DriverType, class LaunchBounds,
          typename ShmemFunctor>
unsigned hip_internal_get_block_size(const HIPInternal *hip_instance,
                                     const ShmemFunctor &f,
                                     const unsigned tperb_reg) {
  // translate LB from CUDA to HIP
  const unsigned min_waves_per_eu =
      LaunchBounds::minBperSM ? LaunchBounds::minBperSM : 1;
  const unsigned min_threads_per_sm = min_waves_per_eu * HIPTraits::WarpSize;
  const unsigned shmem_per_sm       = hip_instance->m_shmemPerSM;
  unsigned block_size               = tperb_reg;
  do {
    unsigned total_shmem = f(block_size);
    // find how many threads we can fit with this blocksize based on LDS usage
    unsigned tperb_shmem = total_shmem > shmem_per_sm ? 0 : block_size;

    if constexpr (BlockSize == BlockType::Max) {
      // we want the maximum blocksize possible
      // just wait until we get a case where we can fit the LDS per SM
      if (tperb_shmem) return block_size;
    } else {
      if (block_size == tperb_reg && tperb_shmem >= tperb_reg) {
        // fast path for exit on first iteration if registers are more limiting
        // than LDS usage, just use the register limited size
        return tperb_reg;
      }
      // otherwise we need to apply a heuristic to choose the blocksize
      // the current launchbound selection scheme is:
      //      1. If no spills, choose 1024 [MaxThreadsPerBlock]
      //      2. Otherwise, choose 256 [ConservativeThreadsPerBlock]
      //
      // For blocksizes between 256 and 1024, we'll be forced to use the 1024 LB
      // and we'll already have pretty decent occupancy, thus dropping to 256
      // *probably* isn't a concern
      const unsigned blocks_per_cu_shmem = shmem_per_sm / total_shmem;
      const unsigned tperb = tperb_shmem < tperb_reg ? tperb_shmem : tperb_reg;

      // for anything with > 4 WF's & can fit multiple blocks
      // we're probably not occupancy limited so just return that
      if (blocks_per_cu_shmem > 1 &&
          tperb > HIPTraits::ConservativeThreadsPerBlock) {
        return block_size;
      }

      // otherwise, it's probably better to drop to the first valid size that
      // fits in the ConservativeThreadsPerBlock
      if (tperb >= min_threads_per_sm) return block_size;
    }
    block_size >>= 1;
  } while (block_size >= HIPTraits::WarpSize);
  // TODO: return a negative, add an error to kernel launch
  return 0;
}

// Standardized blocksize deduction for parallel constructs with no LDS usage
// Returns the preferred blocksize as dictated by register usage
//
// Note: a returned block_size of zero indicates that the algorithm could not
//       find a valid block size.  The caller is responsible for error handling.
template <typename DriverType, typename LaunchBounds>
unsigned hip_get_preferred_blocksize() {
  return get_preferred_blocksize_impl<DriverType, LaunchBounds>();
}

// Standardized blocksize deduction for parallel constructs with no LDS usage
// Returns the max blocksize as dictated by register usage
//
// Note: a returned block_size of zero indicates that the algorithm could not
//       find a valid block size.  The caller is responsible for error handling.
template <typename DriverType, typename LaunchBounds>
unsigned hip_get_max_blocksize() {
  return get_max_blocksize_impl<DriverType, LaunchBounds>();
}

// Standardized blocksize deduction for non-teams parallel constructs with LDS
// usage Returns the 'preferred' blocksize, as determined by the heuristics in
// hip_internal_get_block_size
//
// The ShmemFunctor takes a single argument of the current blocksize under
// consideration, and returns the LDS usage
//
// Note: a returned block_size of zero indicates that the algorithm could not
//       find a valid block size.  The caller is responsible for error handling.
template <typename DriverType, typename LaunchBounds, typename ShmemFunctor>
unsigned hip_get_preferred_blocksize(HIPInternal const *hip_instance,
                                     ShmemFunctor const &f) {
  // get preferred blocksize limited by register usage
  const unsigned tperb_reg =
      hip_get_preferred_blocksize<DriverType, LaunchBounds>();
  return hip_internal_get_block_size<BlockType::Preferred, DriverType,
                                     LaunchBounds>(hip_instance, f, tperb_reg);
}

// Standardized blocksize deduction for teams-based parallel constructs with LDS
// usage Returns the 'preferred' blocksize, as determined by the heuristics in
// hip_internal_get_block_size
//
// The ShmemTeamsFunctor takes two arguments: the hipFunctionAttributes and
//  the current blocksize under consideration, and returns the LDS usage
//
// Note: a returned block_size of zero indicates that the algorithm could not
//       find a valid block size.  The caller is responsible for error handling.
template <typename DriverType, typename LaunchBounds,
          typename ShmemTeamsFunctor>
unsigned hip_get_preferred_team_blocksize(HIPInternal const *hip_instance,
                                          ShmemTeamsFunctor const &f) {
  hipFuncAttributes attr =
      get_hip_func_attributes_impl<DriverType, LaunchBounds,
                                   BlockType::Preferred>();
  // get preferred blocksize limited by register usage
  const unsigned tperb_reg =
      hip_get_preferred_blocksize<DriverType, LaunchBounds>();
  return hip_internal_get_block_size<BlockType::Preferred, DriverType,
                                     LaunchBounds>(
      hip_instance, std::bind(f, attr, std::placeholders::_1), tperb_reg);
}

// Standardized blocksize deduction for non-teams parallel constructs with LDS
// usage Returns the maximum possible blocksize, as determined by the heuristics
// in hip_internal_get_block_size
//
// The ShmemFunctor takes a single argument of the current blocksize under
// consideration, and returns the LDS usage
//
// Note: a returned block_size of zero indicates that the algorithm could not
//       find a valid block size.  The caller is responsible for error handling.
template <typename DriverType, typename LaunchBounds, typename ShmemFunctor>
unsigned hip_get_max_blocksize(HIPInternal const *hip_instance,
                               ShmemFunctor const &f) {
  // get max blocksize limited by register usage
  const unsigned tperb_reg = hip_get_max_blocksize<DriverType, LaunchBounds>();
  return hip_internal_get_block_size<BlockType::Max, DriverType, LaunchBounds>(
      hip_instance, f, tperb_reg);
}

// Standardized blocksize deduction for teams-based parallel constructs with LDS
// usage Returns the maximum possible blocksize, as determined by the heuristics
// in hip_internal_get_block_size
//
// The ShmemTeamsFunctor takes two arguments: the hipFunctionAttributes and
//  the current blocksize under consideration, and returns the LDS usage
//
// Note: a returned block_size of zero indicates that the algorithm could not
//       find a valid block size.  The caller is responsible for error handling.
template <typename DriverType, typename LaunchBounds,
          typename ShmemTeamsFunctor>
unsigned hip_get_max_team_blocksize(HIPInternal const *hip_instance,
                                    ShmemTeamsFunctor const &f) {
  hipFuncAttributes attr =
      get_hip_func_attributes_impl<DriverType, LaunchBounds, BlockType::Max>();
  // get max blocksize
  const unsigned tperb_reg = hip_get_max_blocksize<DriverType, LaunchBounds>();
  return hip_internal_get_block_size<BlockType::Max, DriverType, LaunchBounds>(
      hip_instance, std::bind(f, attr, std::placeholders::_1), tperb_reg);
}

}  // namespace Impl
}  // namespace Kokkos

#endif

#endif
