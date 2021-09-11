/*
  Joshua Dennis Booth
  Test the Amesos2 Interface calls with refactoring
*/

#include "test_util.hpp"
#include "shylubasker_decl.hpp"
#include "shylubasker_def.hpp"
#include <Kokkos_Core.hpp>

int main(int argc, char* argv[])
{

  typedef long           Int;
  typedef double         Entry;
  typedef Kokkos::OpenMP Exe_Space;

  const bool nontranspose = true;
  const bool transpose = true;
  const bool copytranspose = false;
  const bool ones_setup = false;

  if(argc < 2)
  {
    std::cout << "Inputs nthreads matrix.mtx (optional rhs.mtx)" << std::endl;
    return -1;
  }
  
  Int nthreads = atoi(argv[1]);
  std::string mname = std::string(argv[2]);
  std::string vname;

  //Load inital information
  //Matrix
  Int m, n, nnz; 
  m = n = nnz = 0;
  Int *col_ptr = nullptr;
  Int *row_idx = nullptr;
  Entry *val = nullptr;
  
  std::cout << "Matrix read" << std::endl;
  double rmatrix = myTime();
  readMatrix<Int,Entry>(mname, m, n, nnz, 
			&col_ptr, &row_idx, &val);
  std::cout << "Read Matrix, Time: " 
	    << totalTime(rmatrix,myTime()) << std::endl;
  
  //RHS
  Int vn, vm;
  vn = vm = 0;
  Entry *x, *xhat, *y;
  x = xhat = y = NULL;
  vn = n;
  x = new Entry[vn]();
  xhat = new Entry[vn]();

  if(argc == 4)
  {
    vname = std::string(argv[3]);
    //std::cout << "reading vector " << vname << std::endl;
    readVector<Int,Entry>(vname, vm, &y);
  }
  else
  {
    vm = m;
    y = new Entry[m]();
    for(Int i = 0; i < vm; i++)
    {
      if (ones_setup)
        xhat[i] = (Entry) 1;
      else
        xhat[i] = (Entry) i;
    }
    multiply<Int,Entry>(m,n,col_ptr,row_idx,val,xhat,y);
    for(Int i = 0; i < vm; i++)
    {
      //std::cout  << "y " << y[i] << std::endl;
      xhat[i] = (Entry) 0.0;
    }
  }
  
  //Starting up Kokkos
  Kokkos::initialize(Kokkos::InitializationSettings().set_num_threads(nthreads));
 
  //Start Basker
  {
//    int result = 0; // NDE warning unused
    BaskerNS::Basker<Int, Entry, Exe_Space> mybasker;
    //---Options
    mybasker.Options.same_pattern       = BASKER_FALSE;
    mybasker.Options.verbose            = BASKER_TRUE;
    mybasker.Options.verbose_matrix_out = BASKER_FALSE;
    mybasker.Options.realloc            = BASKER_TRUE;
    mybasker.Options.transpose          = BASKER_FALSE;
    mybasker.Options.symmetric          = BASKER_FALSE;
    mybasker.Options.AtA                = BASKER_TRUE;
    mybasker.Options.A_plus_At          = BASKER_TRUE;
    mybasker.Options.btf_max_percent    = BASKER_BTF_MAX_PERCENT;
    mybasker.Options.btf_large          = BASKER_BTF_LARGE;

    mybasker.Options.btf                = BASKER_TRUE;
    mybasker.Options.btf_matching       = 2; // Trilinos
    //mybasker.Options.btf_matching       = 1; // Basker's
    mybasker.Options.matching           = BASKER_TRUE;
    mybasker.Options.matching_type      = BASKER_MATCHING_BN;
    mybasker.Options.no_pivot           = BASKER_TRUE; // Disable pivoting for matlab comparison

    // Modified for transpose testing
    mybasker.Options.blk_matching       = BASKER_FALSE;
    mybasker.Options.replace_tiny_pivot = BASKER_FALSE;

//    mybasker.Options.amd_dom              = BASKER_FALSE; // Setting this false breaks the transpose solve with atond
    mybasker.Options.static_delayed_pivot = BASKER_FALSE;
   
    mybasker.SetThreads(nthreads);
    std::cout << "Setting Threads:" << nthreads << std::endl;
    double stime = myTime();
    mybasker.Symbolic(m,n,nnz,col_ptr,row_idx,val);
    std::cout << "Done with Symbolic, Time: " 
	      << totalTime(stime, myTime()) << std::endl;
    double ftime = myTime();
    mybasker.Factor(m,n,nnz,col_ptr,row_idx,val);
    std::cout << "Done with Factor, Time: "
	      << totalTime(ftime, myTime()) << std::endl;
    //mybasker.DEBUG_PRINT();
    double ttime = myTime();
    
    if (nontranspose) {
    std::cout << "\n\n** Begin Solve **\n" << std::endl;
    mybasker.Solve(y,x);
    std::cout << "Done with Solve, Time: "
	      << totalTime(ttime, myTime()) << std::endl;

    multiply<Int,Entry>(m,n,col_ptr,row_idx,val, x, xhat);
    for(Int i = 0; i < m; i++)
    {
      xhat[i] = y[i] - xhat[i];
    }

    std::cout << "||X||: " << norm2<Int,Entry>(n,x)
	      << " ||Y-AX||: " << norm2<Int,Entry>(m,xhat)
        << "   Matrix: " << mname
	      << std::endl;

    }


    if (transpose) {
    // Transpose solve:
    // This solve only works with square matrices
    Entry* yt = new Entry[m]();
    // Re-init xhat to 1's
    for(Int i = 0; i < vm; i++)
    {
      xhat[i] = (Entry) 1;
    }
    multiply_tr<Int,Entry>(m,n,col_ptr,row_idx,val,xhat,yt);
    for(Int i = 0; i < vm; i++)
    {
      //std::cout  << "y " << y[i] << std::endl;
      xhat[i] = (Entry) 0.0;
      x[i] = (Entry) 0.0;
    }

    ttime = myTime();
    std::cout << "\n\n** Begin Transpose Solve **\n" << std::endl;
    // transpose
    mybasker.Solve(yt,x,true);
    std::cout << "Done with Transpose Solve, Time: "
	      << totalTime(ttime, myTime()) << std::endl;

    multiply_tr<Int,Entry>(m,n,col_ptr,row_idx,val,x,xhat);
    for(Int i = 0; i < m; i++)
    {
      xhat[i] = yt[i] - xhat[i];
    }

    std::cout << "||X||: " << norm2<Int,Entry>(n,x)
	      << " ||Y-AX||: " << norm2<Int,Entry>(m,xhat)
        << "   Matrix: " << mname
	      << std::endl;

    }
    mybasker.Finalize();

    if (copytranspose) {

    BaskerNS::Basker<Int, Entry, Exe_Space> mybaskertr;
    mybaskertr.Options.transpose          = BASKER_TRUE; // CHANGE HERE TO TEST WITH TRANSPOSE via COPY
    //---Options
    mybaskertr.Options.same_pattern       = BASKER_FALSE;
    mybaskertr.Options.verbose            = BASKER_TRUE;
    mybaskertr.Options.verbose_matrix_out = BASKER_FALSE;
    mybaskertr.Options.realloc            = BASKER_TRUE;
    mybaskertr.Options.symmetric          = BASKER_FALSE;
    mybaskertr.Options.AtA                = BASKER_TRUE;
    mybaskertr.Options.A_plus_At          = BASKER_TRUE;
    mybaskertr.Options.btf_max_percent    = BASKER_BTF_MAX_PERCENT;
    mybaskertr.Options.btf_large          = BASKER_BTF_LARGE;

    mybaskertr.Options.btf                = BASKER_TRUE;
    mybaskertr.Options.btf_matching       = 2; // Trilinos
    //mybaskertr.Options.btf_matching       = 1; // Basker's
    mybaskertr.Options.matching           = BASKER_TRUE;
    mybaskertr.Options.matching_type      = BASKER_MATCHING_BN;
    //mybaskertr.Options.no_pivot           = BASKER_FALSE;
    mybaskertr.Options.no_pivot           = BASKER_TRUE;

    // Modified for transpose testing
    mybaskertr.Options.blk_matching       = BASKER_FALSE;
    mybaskertr.Options.replace_tiny_pivot = BASKER_FALSE;

// Change: tested 09/14/2022
    mybaskertr.Options.amd_dom              = BASKER_FALSE;
    mybaskertr.Options.static_delayed_pivot = BASKER_FALSE;
   
    mybaskertr.SetThreads(nthreads);
    std::cout << "Setting Threads:" << nthreads << std::endl;
    double stime = myTime();
    mybaskertr.Symbolic(m,n,nnz,col_ptr,row_idx,val);
    std::cout << "Done with Symbolic, Time: " 
	      << totalTime(stime, myTime()) << std::endl;
    double ftime = myTime();
    mybaskertr.Factor(m,n,nnz,col_ptr,row_idx,val);
    std::cout << "Done with Factor, Time: "
	      << totalTime(ftime, myTime()) << std::endl;
    //mybaskertr.DEBUG_PRINT();
    double ttime = myTime();


    // Transpose solve:
    // This solve only works with square matrices
    Entry* yt = new Entry[m]();
    // Re-init xhat to 1's
    for(Int i = 0; i < vm; i++)
    {
      if (ones_setup)
        xhat[i] = (Entry) 1;
      else
        xhat[i] = (Entry) i;
    }
    multiply_tr<Int,Entry>(m,n,col_ptr,row_idx,val,xhat,yt);
    for(Int i = 0; i < vm; i++)
    {
      //std::cout  << "y " << y[i] << std::endl;
      xhat[i] = (Entry) 0.0;
      x[i] = (Entry) 0.0;
    }

    ttime = myTime();
    std::cout << "\n\n** Begin Transpose Copy Solve **\n" << std::endl;
    // transpose
    mybaskertr.Solve(yt,x);
    std::cout << "Done with Transpose Copy Solve, Time: "
	      << totalTime(ttime, myTime()) << std::endl;

    multiply_tr<Int,Entry>(m,n,col_ptr,row_idx,val,x,xhat);
    for(Int i = 0; i < m; i++)
    {
      xhat[i] = yt[i] - xhat[i];
    }

    std::cout << "||X||: " << norm2<Int,Entry>(n,x)
	      << " ||Y-AX||: " << norm2<Int,Entry>(m,xhat)
        << "   Matrix: " << mname
	      << std::endl;

    delete [] yt;
    mybaskertr.Finalize();
    }
  }
  
  Kokkos::finalize();

}//end main
