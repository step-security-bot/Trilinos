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
#ifndef MUELU_IPCFACTORY_DECL_HPP
#define MUELU_IPCFACTORY_DECL_HPP

#include <string>
#include <vector>

#include "MueLu_ConfigDefs.hpp"
#include "MueLu_IntrepidPCoarsenFactory_fwd.hpp"

#include "MueLu_Level_fwd.hpp"
#include "MueLu_ParameterListAcceptor.hpp"
#include "MueLu_PerfUtils_fwd.hpp"
#include "MueLu_PFactory.hpp"
#include "MueLu_Utilities_fwd.hpp"

#include "Intrepid2_Basis.hpp"

#include "Kokkos_DynRankView.hpp"

#include <Xpetra_Import_fwd.hpp>

namespace MueLu {

  /*!
    @class IntrepidPCoarsenFactory class.
    @ingroup MueLuTransferClasses
    @brief Factory for building transfer operators based on coarsening in polynomial degree, following the Intrepid basis functions

    ## Input/output of IntrepidPCoarsenFactory ##

    ### User parameters of IntrepidPCoarsenFactory ###
    Parameter | type | default | master.xml | validated | requested | description
    ----------|------|---------|:----------:|:---------:|:---------:|------------
    | pcoarsen: schedule                          | string  | ""    | * | * |   | String describing the higher order coarsening scheme to use |
    | pcoarsen: element                          | string  | ""    | * | * |   | String describing the class of element to use for higher order coarsening |
    | pcoarsen: hi basis                          | string  | ""    | * | * |   | String describing higher-order basis function used |
    | pcoarsen: lo basis                          | string  | ""    | * | * |   | String describing lower-order basis function to be used for coarse grid |
    | pcoarsen: element to node map               | RCP<Intrepid::FieldContainer<LocalOrdinal> > | null | * | * |   | A FieldContainer with the element-to-node map in local ids compatible with the matrix column map |
    | A                                      | Factory | null      | * | * |   | Generating factory of the matrix A used during the prolongator smoothing process |


    The * in the @c master.xml column denotes that the parameter is defined in the @c master.xml file.<br>
    The * in the @c validated column means that the parameter is declared in the list of valid input parameters (see IntrepidPCoarsen::GetValidParameters).<br>
    The * in the @c requested column states that the data is requested as input with all dependencies (see IntrepidPCoarsen::DeclareInput).

    ### Variables provided by IntrepidPCoarsen ###

    After IntrepidPCoarsen::Build the following data is available (if requested)

    Parameter | generated by | description
    ----------|--------------|------------
    | P       | IntrepidPCoarsenFactory   | Smoothed prolongator


    NOTE:  The prolongator generated by this routine strips Dirichlet unknowns out of the coarse grid.  This means that the prolongator cannot
    transfer *solutions* between grids, only updates (because the update at a Dirichlet node is zero).  This is a subtle distinction, but an
    important one for testing purposes.

    FIXME: There may be implications for the correctness of the nullspace transfer from fine to coarse that I need to understand.


  */

template <class Scalar = DefaultScalar,
          class LocalOrdinal = DefaultLocalOrdinal,
          class GlobalOrdinal = DefaultGlobalOrdinal,
          class Node = DefaultNode>
  class IntrepidPCoarsenFactory : public PFactory {
#undef MUELU_INTREPIDPCOARSENFACTORY_SHORT
#include "MueLu_UseShortNames.hpp"

  public:
    typedef Kokkos::DynRankView<LocalOrdinal,typename Node::device_type> LOFieldContainer;
    typedef Kokkos::DynRankView<double,typename Node::device_type> SCFieldContainer;
    typedef Intrepid2::Basis<typename Node::device_type::execution_space,double,double> Basis; // Hardwired on purpose

    //! @name Constructors/Destructors.
    //@{

    /*! @brief Constructor.
      User can supply a factory for generating the tentative prolongator.
    */
    IntrepidPCoarsenFactory() { }

    //! Destructor.
    virtual ~IntrepidPCoarsenFactory() { }

    RCP<const ParameterList> GetValidParameterList() const;

    //@}

    //! Input
    //@{

    void DeclareInput(Level &fineLevel, Level &coarseLevel) const;

    //@}

    //! @name Build methods.
    //@{

    /*!
      @brief Build method.

      Builds IPC prolongator and returns it in <tt>coarseLevel</tt>.
      */
    void Build(Level& fineLevel, Level &coarseLevel) const;

    void BuildP(Level &fineLevel, Level &coarseLevel) const; //Build()

    //@}
  private:
    //! @name Internal Utilities
    //@{
    // NOTE: This is hardwired to double on purpose.
    void GenerateLinearCoarsening_pn_kirby_to_p1(const LOFieldContainer & hi_elemToNode,
                                                 const std::vector<bool> & hi_nodeIsOwned,
                                                 const SCFieldContainer & hi_DofCoords,
                                                 const std::vector<size_t> &lo_node_in_hi,
                                                 const Basis &lo_Basis,
                                                 const std::vector<LocalOrdinal> & hi_to_lo_map,
                                                 const Teuchos::RCP<const Map> & lo_colMap,
                                                 const Teuchos::RCP<const Map> & lo_domainMap,
                                                 const Teuchos::RCP<const Map> & hi_map,
                                                 Teuchos::RCP<Matrix>& P) const;
    //@}

    // NOTE: This is hardwired to double on purpose.
    void GenerateLinearCoarsening_pn_kirby_to_pm(const LOFieldContainer & hi_elemToNode,
                                                 const std::vector<bool> & hi_nodeIsOwned,
                                                 const SCFieldContainer & hi_DofCoords,
                                                 const LOFieldContainer & lo_elemToHiRepresentativeNode,
                                                 const Basis &lo_basis,
                                                 const std::vector<LocalOrdinal> & hi_to_lo_map,
                                                 const Teuchos::RCP<const Map> & lo_colMap,
                                                 const Teuchos::RCP<const Map> & lo_domainMap,
                                                 const Teuchos::RCP<const Map> & hi_map,
                                                 Teuchos::RCP<Matrix>& P) const;


  }; //class IntrepidPCoarsenFactory


  /* Utility functions for use with Intrepid */
  namespace MueLuIntrepid {

    template<class Scalar,class KokkosExecutionSpace>
    Teuchos::RCP<Intrepid2::Basis<KokkosExecutionSpace,Scalar,Scalar> >  BasisFactory(const std::string & name, int & degree);

    template<class Scalar,class KokkosDeviceType>
    void IntrepidGetLoNodeInHi(const Teuchos::RCP<Intrepid2::Basis<typename KokkosDeviceType::execution_space,Scalar,Scalar> > &hi_basis,
                               const Teuchos::RCP<Intrepid2::Basis<typename KokkosDeviceType::execution_space,Scalar,Scalar> > &lo_basis,
                               std::vector<size_t> & lo_node_in_hi,
                               Kokkos::DynRankView<Scalar,KokkosDeviceType> & hi_DofCoords);

    template<class LocalOrdinal, class GlobalOrdinal, class Node, class LOFieldContainer>
    void GenerateLoNodeInHiViaGIDs(const std::vector<std::vector<size_t> > & candidates,const LOFieldContainer & hi_elemToNode,
                                   RCP<const Xpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > & hi_columnMap,
                                   LOFieldContainer & lo_elemToHiRepresentativeNode);

    template <class LocalOrdinal, class LOFieldContainer>
    void BuildLoElemToNode(const LOFieldContainer & hi_elemToNode,
                           const std::vector<bool> & hi_nodeIsOwned,
                           const std::vector<size_t> & lo_node_in_hi,
                           const Teuchos::ArrayRCP<const int> & hi_isDirichlet,
                           LOFieldContainer & lo_elemToNode,
                           std::vector<bool> & lo_nodeIsOwned,
                           std::vector<LocalOrdinal> & hi_to_lo_map,
                           int & lo_numOwnedNodes);

    template <class LocalOrdinal, class LOFieldContainer>
    void BuildLoElemToNodeViaRepresentatives(const LOFieldContainer & hi_elemToNode,
                                             const std::vector<bool> & hi_nodeIsOwned,
                                             const LOFieldContainer & lo_elemToHiRepresentativeNode,
                                             LOFieldContainer & lo_elemToNode,
                                             std::vector<bool> & lo_nodeIsOwned,
                                             std::vector<LocalOrdinal> & hi_to_lo_map,
                                             int & lo_numOwnedNodes);


    template <class LocalOrdinal, class GlobalOrdinal, class Node>
    void GenerateColMapFromImport(const Xpetra::Import<LocalOrdinal,GlobalOrdinal,Node> & hi_importer,const std::vector<LocalOrdinal> &hi_to_lo_map,const Xpetra::Map<LocalOrdinal,GlobalOrdinal,Node> & lo_domainMap, const size_t & lo_columnMapLength, RCP<const Xpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > & lo_columnMap);


    template<class Basis, class SCFieldContainer>
    void GenerateRepresentativeBasisNodes(const Basis & basis, const SCFieldContainer & ReferenceNodeLocations, const double threshold, std::vector<std::vector<size_t> > & representative_node_candidates);

    // ! Given an element to (global) node map and a basis, determine one global ordinal per geometric entity (vertex, edge, face,
    // ! interior).  On exit, seeds container is of dimension (spaceDim+1), and contains a sorted vector of local ordinals
    // ! belonging to entities of that dimension.  Only locally-owned degrees of freedom (as determined by rowMap and columnMap)
    // ! will be stored in seeds.
    template<class Basis, class LOFieldContainer, class LocalOrdinal, class GlobalOrdinal, class Node>
    void FindGeometricSeedOrdinals(Teuchos::RCP<Basis> basis, const LOFieldContainer &elementToNodeMap,
                                   std::vector<std::vector<LocalOrdinal> > &seeds,
                                   const Xpetra::Map<LocalOrdinal,GlobalOrdinal,Node> &rowMap,
                                   const Xpetra::Map<LocalOrdinal,GlobalOrdinal,Node> &columnMap);

  }//namespace MueLuIntrepid
} //namespace MueLu

#define  MUELU_INTREPIDPCOARSENFACTORY_SHORT
#endif // MUELU_INTREPIDPCOARSENFACTORY_DECL_HPP
