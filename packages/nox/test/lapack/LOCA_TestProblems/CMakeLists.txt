

SET(HEADERS "")
SET(SOURCES "")

TRIBITS_INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

APPEND_SET(HEADERS
  ChanProblemInterface.H
  NormConstraint.H
  LOCALinearConstraint.H
  )

APPEND_SET(SOURCES
  ChanProblemInterface.C
  NormConstraint.C
  LOCALinearConstraint.C
  )

IF(NOX_ENABLE_ABSTRACT_IMPLEMENTATION_LAPACK AND NOX_ENABLE_LOCA)

  TRIBITS_ADD_LIBRARY(
    localapacktestproblems
    HEADERS ${HEADERS}
    SOURCES ${SOURCES}
    TESTONLY
    DEPLIBS loca localapack
    )

ENDIF()
