// Copyright 2002 - 2008, 2010, 2011 National Technology Engineering
// Solutions of Sandia, LLC (NTESS). Under the terms of Contract
// DE-NA0003525 with NTESS, the U.S. Government retains certain rights
// in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
// 
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
// 
//     * Neither the name of NTESS nor the names of its contributors
//       may be used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 

#ifndef STK_UTIL_PARALLEL_BROADCASTARG_HPP
#define STK_UTIL_PARALLEL_BROADCASTARG_HPP

#include <string>
#include <vector>
#include "stk_util/parallel/Parallel.hpp"  // for ParallelMachine

namespace stk {

/**
 * @brief Class <b>BroadcastArg</b> creates a copy of argc and argv after broadcasting
 * them from processor 0.
 *
 */
struct BroadcastArg 
{
  /**
   * Creates a new <b>BroadcastArg</b> instance.
   *
   * @param parallel_machine	        a <b>ParallelMachine</b> value that species the
   *                                    parallel machine from performing the broadcast.
   *
   * @param argc			an <b>int</b> value of argc from main
   *
   * @param argv			a <b>char</b> pointer pointer of argv from main
   *
   */
  BroadcastArg(ParallelMachine parallel_machine, int argc, char **argv);

  int           m_argc;                         ///< The broadcasted argc
  char **       m_argv;                         ///< The broadcasted argv

private:
  BroadcastArg(const BroadcastArg &argv);
  BroadcastArg &operator=(const BroadcastArg &argv);

  void pack_strings_on_root(int argc, char** argv, ParallelMachine parallel_machine, int root_rank);

  void broadcast_strings(int argc, ParallelMachine parallel_machine, int root_rank);

  void split_strings();


  std::vector<char*> m_argv_storage;
  std::string m_string_storage;
};

} // namespace stk

#endif // STK_UTIL_PARALLEL_BROADCASTARG_HPP
