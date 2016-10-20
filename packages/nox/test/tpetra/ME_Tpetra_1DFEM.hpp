#ifndef NOX_TPETRA_ME_1DFEM_DECL_HPP
#define NOX_TPETRA_ME_1DFEM_DECL_HPP

#include "Thyra_StateFuncModelEvaluatorBase.hpp"

#include "Tpetra_Map.hpp"
#include "Tpetra_Vector.hpp"
#include "Tpetra_Import.hpp"
#include "Tpetra_CrsGraph.hpp"
#include "Tpetra_CrsMatrix.hpp"

template<class Scalar, class LO, class GO, class Node>
class EvaluatorTpetra1DFEM;

/** \brief Nonmember constuctor.
 *
 * \relates EvaluatorTpetra1DFEM
 */
template<class Scalar, class LO, class GO, class Node>
Teuchos::RCP<EvaluatorTpetra1DFEM<Scalar, LO, GO, Node> >
evaluatorTpetra1DFEM(const Teuchos::RCP<const Teuchos::Comm<int> >& comm,
                     const Tpetra::global_size_t numGlobalElements,
                     const Scalar zMin,
                     const Scalar zMax);


/** \brief 1D Finite Element model for nonlinear heat conduction
 *
 * The equation modeled is:

 \verbatim

   d2T
   --- - T**2 = 0
   dz2

   subject to:
      T  = 1.0 @ z = zMin
      T' = 0.0 @ z = zMax

 \endverbatim

 * The Matrix <tt>W = d(f)/d(x)</tt> is implemented as a
 * <tt>Thyra::LinearOpBase</tt> object and the class
 * <tt>Thyra::DefaultSerialDenseLinearOpWithSolveFactory</tt> is used to
 * create the linear solver.
 */
template<class Scalar, class LO, class GO, class Node>
class EvaluatorTpetra1DFEM
  : public ::Thyra::StateFuncModelEvaluatorBase<Scalar>
{
public:

  // Public typedefs
  typedef Scalar scalar_type;
  typedef LO local_ordinal_type;
  typedef GO global_ordinal_type;
  typedef Node node_type;
  typedef Tpetra::Map<LO, GO, Node> tpetra_map;
  typedef Tpetra::CrsGraph<LO, GO, Node> tpetra_graph;
  typedef Tpetra::CrsMatrix<Scalar, LO, GO, Node> tpetra_matrix;
  typedef Tpetra::Vector<Scalar, LO, GO, Node> tpetra_vec;
  typedef ::Thyra::VectorBase<Scalar> thyra_vec;
  typedef ::Thyra::VectorSpaceBase<Scalar> thyra_vec_space;
  typedef ::Thyra::LinearOpBase<Scalar> thyra_op;
  typedef ::Thyra::PreconditionerBase<Scalar> thyra_prec;

  // Constructor
  EvaluatorTpetra1DFEM(const Teuchos::RCP<const Teuchos::Comm<int> >& comm,
                       const Tpetra::global_size_t numGlobalElements,
                       const Scalar zMin,
                       const Scalar zMax);

  /** \name Initializers/Accessors */
  //@{

  /** \brief . */
  void set_x0(const Teuchos::ArrayView<const Scalar> &x0);

  /** \brief . */
  void setShowGetInvalidArgs(bool showGetInvalidArg);

  void set_W_factory(const Teuchos::RCP<const ::Thyra::LinearOpWithSolveFactoryBase<Scalar> >& W_factory);

  //@}

  /** \name Public functions overridden from ModelEvaulator. */
  //@{

  /** \brief . */
  Teuchos::RCP<const thyra_vec_space> get_x_space() const;
  /** \brief . */
  Teuchos::RCP<const thyra_vec_space> get_f_space() const;
  /** \brief . */
  ::Thyra::ModelEvaluatorBase::InArgs<Scalar> getNominalValues() const;
  /** \brief . */
  Teuchos::RCP<thyra_op> create_W_op() const;
  /** \brief . */
  Teuchos::RCP<const ::Thyra::LinearOpWithSolveFactoryBase<Scalar> > get_W_factory() const;
  /** \brief . */
  ::Thyra::ModelEvaluatorBase::InArgs<Scalar> createInArgs() const;
  /** \brief . */
  Teuchos::RCP<thyra_prec> create_W_prec() const;
  //@}

private:

  /** Allocates and returns the Jacobian matrix graph */
  virtual Teuchos::RCP<const tpetra_graph> createGraph();

  /** \name Private functions overridden from ModelEvaulatorDefaultBase. */
  //@{

  /** \brief . */
  ::Thyra::ModelEvaluatorBase::OutArgs<Scalar> createOutArgsImpl() const;
  /** \brief . */
  void evalModelImpl(
    const ::Thyra::ModelEvaluatorBase::InArgs<Scalar> &inArgs,
    const ::Thyra::ModelEvaluatorBase::OutArgs<Scalar> &outArgs
    ) const;

  //@}

private: // data members

  const Teuchos::RCP<const Teuchos::Comm<int> >  comm_;
  const Tpetra::global_size_t numGlobalElements_;
  const Scalar zMin_;
  const Scalar zMax_;

  Teuchos::RCP<const thyra_vec_space> xSpace_;
  Teuchos::RCP<const tpetra_map>   xOwnedMap_;
  Teuchos::RCP<const tpetra_map>   xGhostedMap_;
  Teuchos::RCP<const Tpetra::Import<LO, GO, Node> > importer_;

  Teuchos::RCP<const thyra_vec_space> fSpace_;
  Teuchos::RCP<const tpetra_map>   fOwnedMap_;

  Teuchos::RCP<const tpetra_graph>  W_graph_;

  Teuchos::RCP<const ::Thyra::LinearOpWithSolveFactoryBase<Scalar> > W_factory_;

  Teuchos::RCP<tpetra_vec> nodeCoordinates_;

  mutable Teuchos::RCP<tpetra_vec> uPtr_;
  mutable Teuchos::RCP<tpetra_vec> xPtr_;

  mutable Teuchos::RCP<tpetra_vec> J_diagonal_;

  ::Thyra::ModelEvaluatorBase::InArgs<Scalar> nominalValues_;
  Teuchos::RCP<thyra_vec> x0_;
  bool showGetInvalidArg_;
  ::Thyra::ModelEvaluatorBase::InArgs<Scalar> prototypeInArgs_;
  ::Thyra::ModelEvaluatorBase::OutArgs<Scalar> prototypeOutArgs_;

};

//==================================================================
// Finite Element Basis Object
/*template <class Scalar>
class Basis {

 public:
  // Constructor
  KOKKOS_INLINE_FUNCTION
  Basis():
    uu(0.0),
    zz(0.0),
    duu(0.0),
    eta(0.0),
    wt(0.0),
    dz(0.0),
    uuold(0.0),
    duuold(0.0)
  {
    phi = new Scalar[2];
    dphide = new Scalar[2];
  }

  // Destructor
  KOKKOS_INLINE_FUNCTION
  ~Basis() {
    delete [] phi;
    delete [] dphide;
  }

  // Calculates the values of u and x at the specified gauss point
  KOKKOS_INLINE_FUNCTION
  void computeBasis(int gp, double* z, double* u, double* uold = 0) {
    int N = 2;
    if (gp==0) {eta=-1.0/sqrt(3.0); wt=1.0;}
    if (gp==1) {eta=1.0/sqrt(3.0); wt=1.0;}

    // Calculate basis function and derivatives at nodal pts
    phi[0]=(1.0-eta)/2.0;
    phi[1]=(1.0+eta)/2.0;
    dphide[0]=-0.5;
    dphide[1]=0.5;

    // Caculate basis function and derivative at GP.
    dz=0.5*(z[1]-z[0]);
    zz=0.0;
    uu=0.0;
    duu=0.0;
    uuold=0.0;
    duuold=0.0;
    for (int i=0; i < N; i++) {
      zz += z[i] * phi[i];
      uu += u[i] * phi[i];
      duu += u[i] * dphide[i];
      if (uold) {
        uuold += uold[i] * phi[i];
        duuold += uold[i] * dphide[i];
      }
    }
  }

 public:
  // Variables that are calculated at the gauss point
  Scalar* phi;
  Scalar* dphide;
  Scalar uu;
  Scalar zz;
  Scalar duu;
  Scalar eta;
  Scalar wt;
  Scalar dz;
  // These are only needed for transient
  Scalar uuold;
  Scalar duuold;
};*/

//==================================================================
#include "ME_Tpetra_1DFEM_def.hpp"
//==================================================================

#endif
