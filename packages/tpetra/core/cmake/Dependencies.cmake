TRIBITS_PACKAGE_DEFINE_DEPENDENCIES(
  LIB_REQUIRED_PACKAGES Teuchos TpetraClassic KokkosCore KokkosContainers KokkosAlgorithms TeuchosKokkosCompat TeuchosKokkosComm KokkosKernels
  LIB_OPTIONAL_PACKAGES Epetra TpetraTSQR TeuchosNumerics
  TEST_OPTIONAL_PACKAGES EpetraExt
  LIB_OPTIONAL_TPLS MPI CUDA QD quadmath
)
