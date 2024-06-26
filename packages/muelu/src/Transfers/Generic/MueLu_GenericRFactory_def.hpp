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
#ifndef MUELU_GENERICRFACTORY_DEF_HPP
#define MUELU_GENERICRFACTORY_DEF_HPP

#include <Xpetra_Matrix.hpp>

#include "MueLu_GenericRFactory_decl.hpp"

#include "MueLu_FactoryBase.hpp"
#include "MueLu_PFactory.hpp"
#include "MueLu_FactoryManagerBase.hpp"
#include "MueLu_DisableMultipleCallCheck.hpp"
#include "MueLu_Monitor.hpp"

namespace MueLu {

template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
RCP<const ParameterList> GenericRFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node>::GetValidParameterList() const {
  RCP<ParameterList> validParamList = rcp(new ParameterList());
  validParamList->set<RCP<const FactoryBase> >("P", Teuchos::null, "Generating factory of the matrix P");
  return validParamList;
}

template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
void GenericRFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node>::DeclareInput(Level & /* fineLevel */, Level &coarseLevel) const {
  RCP<const FactoryBase> PFact1 = GetFactory("P");
  if (PFact1 == Teuchos::null) {
    PFact1 = coarseLevel.GetFactoryManager()->GetFactory("P");
  }
  RCP<PFactory> PFact = Teuchos::rcp_const_cast<PFactory>(rcp_dynamic_cast<const PFactory>(PFact1));
  ;

  bool rmode = PFact->isRestrictionModeSet();
  PFact->setRestrictionMode(true);  // set restriction mode

  // Force request call for PFact
  // In general, Request is only called once for each factory, since we
  // can reuse data generated by the factory. However, here we have to
  // run the code in PFact.Build again, so we have to request the
  // dependencies of PFact first!  Thedependencies are (automatically)
  // cleaned up after the second run of PFact.Build in
  // coarseLevel.Get<RCP<Matrix> >("R",PFact.get())!
  coarseLevel.DeclareDependencies(PFact.get());

  coarseLevel.DeclareInput("R", PFact.get(), this);  // we expect the prolongation operator factory to produce "R" as output
  // call declareInput is called within DeclareInput call
  PFact->setRestrictionMode(rmode);  // reset restriciton mode flag
}

template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
void GenericRFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node>::Build(Level & /* fineLevel */, Level &coarseLevel) const {
  FactoryMonitor m(*this, "Call prolongator factory for calculating restrictor", coarseLevel);

  RCP<const FactoryBase> PFact1 = GetFactory("P");
  if (PFact1 == Teuchos::null) {
    PFact1 = coarseLevel.GetFactoryManager()->GetFactory("P");
  }
  RCP<PFactory> PFact = Teuchos::rcp_const_cast<PFactory>(rcp_dynamic_cast<const PFactory>(PFact1));
  ;
  MueLu::DisableMultipleCallCheck check(PFact);

  // BuildR
  bool rmode = PFact->isRestrictionModeSet();
  PFact->setRestrictionMode(true);  // switch prolongator factory to restriction mode

  // PFact->Build(fineLevel, coarseLevel);  // call PFactory::Build explicitely
  RCP<Matrix> R = coarseLevel.Get<RCP<Matrix> >("R", PFact.get());

  PFact->setRestrictionMode(rmode);  // reset restriction mode flag

  Set(coarseLevel, "R", R);

}  // BuildR

}  // namespace MueLu

#define MUELU_GENERICRFACTORY_SHORT
#endif  // MUELU_GENERICRFACTORY_DEF_HPP
