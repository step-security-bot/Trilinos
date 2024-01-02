// @HEADER
//
// ***********************************************************************
//
//        MueLu: A package for multigrid based preconditioning
//                  Copyright 2012 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Andrey Prokopenko (aprokop@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER

#ifndef MUELU_INVERSEAPPROXIMATIONFACTORY_DECL_HPP_
#define MUELU_INVERSEAPPROXIMATIONFACTORY_DECL_HPP_

#include "MueLu_ConfigDefs.hpp"

#include <Teuchos_ParameterList.hpp>

#include <Xpetra_CrsMatrix_fwd.hpp>
#include <Xpetra_Map_fwd.hpp>

#include "MueLu_FactoryBase_fwd.hpp"
#include "MueLu_SingleLevelFactoryBase.hpp"
#include "MueLu_Utilities_fwd.hpp"

namespace MueLu {

/*!
  @class InverseApproximationFactory class.
  @brief Factory for building the approximate inverse of a matrix.

  ## Context, assumptions, and use cases ##

  This factory is intended to be used for building an approximate inverse of a given matrix \A. This is for now only
  used in the SchurComplementFactory to generate a respective \Ainv matrix.

  For blocked matrices, the InverseApproximationFactory per default generates an approximate inverse of the A_00 term.

  ## Input/output of this factory ##

  ### User parameters of InterfaceAggregationFactory ###
  Parameter | type | default | master.xml | validated | requested | description
  ----------|------|---------|:----------:|:---------:|:---------:|------------
  A                           | Factory | null     |  | * | * | Generating factory of the matrix A
  inverse: approximation type | string  | diagonal |  | * | * | Method used to approximate the inverse
  inverse: fixing             | bool    | false    |  | * | * | Fix diagonal by replacing small entries with 1.0

  The * in the master.xml column denotes that the parameter is defined in the master.xml file.
  The * in the validated column means that the parameter is declared in the list of valid input parameters (see GetValidParameters() ).
  The * in the requested column states that the data is requested as input with all dependencies (see DeclareInput() ).

  ### Variables provided by this factory ###

  After InverseApproximationFactory::Build the following data is available (if requested)

  Parameter | generated by | description
  ----------|--------------|------------
  | Ainv | InverseApproximationFactory | The approximate inverse of a given matrix.
*/

template <class Scalar        = DefaultScalar,
          class LocalOrdinal  = DefaultLocalOrdinal,
          class GlobalOrdinal = DefaultGlobalOrdinal,
          class Node          = DefaultNode>
class InverseApproximationFactory : public SingleLevelFactoryBase {
#undef MUELU_INVERSEAPPROXIMATIONFACTORY_SHORT
#include "MueLu_UseShortNames.hpp"

 public:
  //! @name Constructors/Destructors.
  //@{

  //! Constructor.
  InverseApproximationFactory() = default;

  //! Input
  //@{

  void DeclareInput(Level& currentLevel) const;

  RCP<const ParameterList> GetValidParameterList() const;

  //@}

  //@{
  //! @name Build methods.

  //! Build an object with this factory.
  void Build(Level& currentLevel) const;

  //@}

 private:
  //! Sparse inverse calculation method.
  RCP<Matrix> GetSparseInverse(const RCP<Matrix>& A, const RCP<const CrsGraph>& sparsityPattern) const;

};  // class InverseApproximationFactory

}  // namespace MueLu

#define MUELU_INVERSEAPPROXIMATIONFACTORY_SHORT
#endif /* MUELU_INVERSEAPPROXIMATIONFACTORY_DECL_HPP_ */
