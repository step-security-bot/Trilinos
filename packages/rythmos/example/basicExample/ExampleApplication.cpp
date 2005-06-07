//
// @HEADER
// ***********************************************************************
// 
//                           Rythmos Package
//                 Copyright (2005) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
// @HEADER


//-----------------------------------------------------------------------------
// Filename      : $RCSfile$
//
// Purpose       : This file contains the functions which define the
//                 example application differential equation
// Special Notes :
// Creator       : Todd Coffey
// Creation Date : 05/05/05
//
// Revision Information:
// ---------------------
//
// Revision Number: $Revision$
//
// Revision Date  : $Date$
//
// Current Owner  : $Author$
//-----------------------------------------------------------------------------

#include "Epetra_Map.h"
#include "Epetra_Vector.h"
#include "Rythmos_ConfigDefs.h"

#include "ExampleApplication.hpp"
//#include "Thyra_EpetraThyraWrappers.hpp"

//-----------------------------------------------------------------------------
// Class         : ExampleApplication
// Purpose       : Example application code with DE: \dot{x}=\lambda x
// Special Notes :
// Creator       : Todd Coffey, SNL
// Creation Date : 05/05/05
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Function      : ExampleApplication::ExampleApplication
// Purpose       : constructor
// Special Notes :
// Scope         : public
// Creator       : Todd Coffey, SNL
// Creation Date : 05/05/05
//-----------------------------------------------------------------------------
ExampleApplication::ExampleApplication(double lam, int numelements)
{
  lambda_ = lam;
  numElements_ = numelements;
  // Serial only implementation here:
  // 05/26/05 tscoffe:  I haven't figured out how to get MPI_Init called with
  // argc and argv in such a way that MPI_COMM_WORLD is passed down here.
  Teuchos::RefCountPtr<Epetra_Comm> epetra_comm_ = Teuchos::rcp( new Epetra_SerialComm );
  
  // Construct a Map with NumElements and index base of 0
  Teuchos::RefCountPtr<Epetra_Map> epetra_map_ = Teuchos::rcp( new Epetra_Map(numElements_, 0, *epetra_comm_) );
  
}

//-----------------------------------------------------------------------------
// Function      : ExampleApplication::~ExampleApplication
// Purpose       : destructor
// Special Notes :
// Scope         : public
// Creator       : Todd Coffey, SNL
// Creation Date : 05/05/05
//-----------------------------------------------------------------------------
ExampleApplication::~ExampleApplication()
{
}

//-----------------------------------------------------------------------------
// Function      : ExampleApplication::evalResidual
// Purpose       : Evaluate problem residual
// Special Notes :
// Scope         : public
// Creator       : Todd Coffey, SNL
// Creation Date : 05/05/05
//-----------------------------------------------------------------------------
int ExampleApplication::evalResidual(Epetra_Vector *y, const Epetra_Vector &x, double t)
{
  y->Scale(lambda_,x); // y = lambda*x
  return 0;
}


//-----------------------------------------------------------------------------
// Function      : ExampleApplication::getCoeff
// Purpose       : Return coefficient for ODE
// Special Notes :
// Scope         : public
// Creator       : Todd Coffey, SNL
// Creation Date : 05/05/05
//-----------------------------------------------------------------------------
double ExampleApplication::getCoeff()
{
  return lambda_;
}

//-----------------------------------------------------------------------------
// Function      : ExampleApplication::get_epetra_map
// Purpose       : Return epetra_map generated at construction time
// Special Notes :
// Scope         : public
// Creator       : Todd Coffey, SNL
// Creation Date : 05/26/05
//-----------------------------------------------------------------------------
Teuchos::RefCountPtr<const Epetra_Map> &ExampleApplication::get_epetra_map()
{
  return(epetra_map_);
}
//
//-----------------------------------------------------------------------------
// Function      : ExampleApplication::get_epetra_comm
// Purpose       : Return epetra_comm generated at construction time
// Special Notes :
// Scope         : public
// Creator       : Todd Coffey, SNL
// Creation Date : 06/02/05
//-----------------------------------------------------------------------------
Teuchos::RefCountPtr<const Epetra_Comm> &ExampleApplication::get_epetra_comm()
{
  return(epetra_comm_);
}


//-----------------------------------------------------------------------------
// Function      : ExampleApplication::get_x0
// Purpose       : Return nominal initial vector
// Special Notes :
// Scope         : public
// Creator       : Todd Coffey, SNL
// Creation Date : 05/26/05
//-----------------------------------------------------------------------------
Teuchos::RefCountPtr<Epetra_Vector> &ExampleApplication::get_x0()
{
  Teuchos::RefCountPtr<Epetra_Vector> x0 = rcp(new Epetra_Vector(epetra_map_));
  (*x0).Random();
  return(x0);
}

