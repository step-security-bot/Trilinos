#!/usr/bin/env python

# @HEADER
# ************************************************************************
#
#            TriBITS: Tribal Build, Integrate, and Test System
#                    Copyright 2013 Sandia Corporation
#
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the Corporation nor the names of the
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# ************************************************************************
# @HEADER

#
# Imports
#

import os
import sys
import traceback
from optparse import OptionParser

if os.environ.get("TRIBITS_IS_CHECKIN_TESTED_COMMIT", "") == "ON":
  debugDump = True
else:
  debugDump = False

if debugDump:
  print "NOTE: TRIBITS_IS_CHECKIN_TESTED_COMMIT=ON set in env, doing debug dump ..."

thisFilePath = __file__
if debugDump: print "\nthisFilePath =", thisFilePath

thisFileRealAbsBasePath = os.path.dirname(os.path.abspath(os.path.realpath(thisFilePath)))
if debugDump: print "\nthisFileRealAbsBasePath = '"+thisFileRealAbsBasePath+"'"

sys.path = [thisFileRealAbsBasePath+"/../python_utils"] + sys.path

from GeneralScriptSupport import *

#
# Utility classes and functions.
#

usageHelp = r"""is_checkin_tested_commit.py [OPTIONS]

This script determines if a commit has been tested (and pushed) with the
checkin-test.py script.  Currently, this just looks for the string:

   "Build/Test Cases Summary"

in the log message which is written there by the checkin-test.py script when
it amends the top commit message before it pushes.

This script is used with 'git bisect' to help week out commits that might not
build or pass all of the tests and therefore not appropriate to bother testing
when doing a 'git bisect' on to find an issue for a customer.

Basically, this script should be added to the top of a 'test-commit.sh' script
(as described in the 'git bisect --help' documentation) to ignore commits that
are not known to be tested with the checkin-test.py script using a
<test-commit> script of the form:

  #!/bin/bash

  $TRIBITS_DIR/ci_support/is_checkin_tested_commit.py
  IS_CHECKIN_TESTED_COMMIT_RTN=$?
  if [ "$IS_CHECKIN_TESTED_COMMIT_RTN" != "0" ] ; then
    exit 125 # Skip the commit because HEAD is was not known to be tested!
  fi

  ./build_and_test_customer_code.sh  # Rtn 0 "good", or [1, 124] if "bad"

See 'git bisect --help' for more details and TriBITS documentation on using
'git bisect' using is_checkin_tested_commit.py.
"""

#
# A) Get command-line options
#

from optparse import OptionParser

clp = OptionParser(usage=usageHelp)

clp.add_option(
  "--ref", dest="gitRef", type="string", default="",
  help="The git reference of the commit to test (empty '' means HEAD)" )

(options, args) = clp.parse_args()

# If --ref is not specified, then take HEAD
if options.gitRef:
  gitRef = options.gitRef
else:
  gitRef = "HEAD"

#
# B) Deterimine if the commit was tested and pushed with checkin-test.py
# script.
#

cmnd = "git log -1 "+gitRef
rawGitLogTxt = getCmndOutput(cmnd)

if re.search("Build/Test Cases Summary", rawGitLogTxt):
  isCheckinTestedCommit = True
else:
  isCheckinTestedCommit = False
  
if isCheckinTestedCommit:
  print "The commit "+gitRef+" is checkin-test.py tested!"
  sys.exit(0)
else:
  print "The commit "+gitRef+" is *NOT* checkin-test.py tested!"
  sys.exit(1)
