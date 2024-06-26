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

#ifndef STK_STK_UTIL_STK_UTIL_COMMAND_LINE_COMMANDLINEPARSERUTILS_HPP_
#define STK_STK_UTIL_STK_UTIL_COMMAND_LINE_COMMANDLINEPARSERUTILS_HPP_

#include <stk_util/parallel/Parallel.hpp>
#include <stk_util/command_line/CommandLineParser.hpp>
#include <string>

namespace stk { class CommandLineParserParallel; }

namespace stk {

template<typename ValueType>
ValueType get_command_line_option(int argc, char** argv,
                                  const std::string& optionName,
                                  const ValueType& defaultValue)
{
  CommandLineParser parser;
  parser.add_optional<ValueType>(optionName, "", defaultValue);
  CommandLineParser::ParseState parseResult = parser.parse(argc, const_cast<const char**>(argv));
  if (parseResult == CommandLineParser::ParseComplete && parser.is_option_parsed(optionName)) {
    return parser.get_option_value<ValueType>(optionName);
  }
  if (parseResult == CommandLineParser::ParseError) {
    STK_ThrowErrorMsg("Failed to parse for --"<<optionName);
  }
  return defaultValue;
}

std::string get_quick_error(const std::string &execName, const std::string &quickExample);

void parse_command_line(int argc,
                        const char** argv,
                        const std::string& quickExample,
                        const std::string& longExample,
                        stk::CommandLineParserParallel& commandLine,
                        MPI_Comm comm);
namespace parallel {
void print_and_exit(const std::string &msg, MPI_Comm comm);
}

}

#endif /* STK_STK_UTIL_STK_UTIL_COMMAND_LINE_COMMANDLINEPARSERUTILS_HPP_ */
