/*
 * STRUMPACK -- STRUctured Matrices PACKage, Copyright (c) 2014, The
 * Regents of the University of California, through Lawrence Berkeley
 * National Laboratory (subject to receipt of any required approvals
 * from the U.S. Dept. of Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE. This software is owned by the U.S. Department of Energy. As
 * such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * Developers: Pieter Ghysels, Francois-Henry Rouet, Xiaoye S. Li.
 *             (Lawrence Berkeley National Lab, Computational Research
 *             Division).
 *
 */
#ifndef STRUMPACK_SPARSE_SOLVER_H
#define STRUMPACK_SPARSE_SOLVER_H

#include <stdint.h>
#include "mpi.h"

typedef enum {
  STRUMPACK_FLOAT, STRUMPACK_DOUBLE,
  STRUMPACK_FLOATCOMPLEX, STRUMPACK_DOUBLECOMPLEX,
  STRUMPACK_FLOAT_64, STRUMPACK_DOUBLE_64,
  STRUMPACK_FLOATCOMPLEX_64, STRUMPACK_DOUBLECOMPLEX_64
} STRUMPACK_PRECISION;

typedef enum {
  STRUMPACK_MT, STRUMPACK_MPI_DIST
} STRUMPACK_INTERFACE;

typedef struct {
  void* solver;
  STRUMPACK_PRECISION precision;
  STRUMPACK_INTERFACE interface;
} STRUMPACK_SparseSolver;

typedef struct { float r, i; } floatcomplex;
typedef struct { double r, i; } doublecomplex;

typedef enum {
  STRUMPACK_NATURAL=0,
  STRUMPACK_METIS=1,
  STRUMPACK_PARMETIS=2,
  STRUMPACK_SCOTCH=3,
  STRUMPACK_PTSCOTCH=4,
  STRUMPACK_RCM=5,
  STRUMPACK_GEOMETRIC=6
} STRUMPACK_REORDERING_STRATEGY;

typedef enum {
  STRUMPACK_CLASSICAL=0,
  STRUMPACK_MODIFIED=1
} STRUMPACK_GRAM_SCHMIDT_TYPE;

typedef enum {
  STRUMPACK_NORMAL=0,
  STRUMPACK_UNIFORM=1
} STRUMPACK_RANDOM_DISTRIBUTION;

typedef enum {
  STRUMPACK_LINEAR=0,
  STRUMPACK_MERSENNE=1
} STRUMPACK_RANDOM_ENGINE;

typedef enum {
  STRUMPACK_AUTO=0,
  STRUMPACK_DIRECT=1,
  STRUMPACK_REFINE=2,
  STRUMPACK_PREC_GMRES=3,
  STRUMPACK_GMRES=4,
  STRUMPACK_PREC_BICGSTAB=5,
  STRUMPACK_BICGSTAB=6
 } STRUMPACK_KRYLOV_SOLVER;

typedef enum {
  STRUMPACK_SUCCESS=0,
  STRUMPACK_MATRIX_NOT_SET=1,
  STRUMPACK_REORDERING_ERROR=2
} STRUMPACK_RETURN_CODE;


#ifdef __cplusplus
extern "C" {
#endif

  void STRUMPACK_init
  (STRUMPACK_SparseSolver* S, MPI_Comm comm, STRUMPACK_PRECISION precision,
   STRUMPACK_INTERFACE interface, int argc, char* argv[], int verbose);
  void STRUMPACK_destroy(STRUMPACK_SparseSolver* S);

  void STRUMPACK_set_csr_matrix
  (STRUMPACK_SparseSolver S, const void* N, const void* row_ptr,
   const void* col_ind, const void* values, int symmetric_pattern);
  void STRUMPACK_set_distributed_csr_matrix
  (STRUMPACK_SparseSolver S, const void* local_rows, const void* row_ptr,
   const void* col_ind, const void* values, const void* dist,
   int symmetric_pattern);
  void STRUMPACK_set_MPIAIJ_matrix
  (STRUMPACK_SparseSolver S, const void* n, const void* d_ptr,
   const void* d_ind, const void* d_val, const void* o_ptr,
   const void* o_ind, const void* o_val, const void* garray);

  STRUMPACK_RETURN_CODE STRUMPACK_solve
  (STRUMPACK_SparseSolver S, const void* b, void* x, int use_initial_guess);

  void STRUMPACK_set_from_options(STRUMPACK_SparseSolver S);

  STRUMPACK_RETURN_CODE STRUMPACK_reorder(STRUMPACK_SparseSolver S);

  STRUMPACK_RETURN_CODE STRUMPACK_reorder_regular
  (STRUMPACK_SparseSolver S, int nx, int ny, int nz);

  STRUMPACK_RETURN_CODE STRUMPACK_factor(STRUMPACK_SparseSolver S);

  /*************************************************************
   ** Set options **********************************************
   ************************************************************/
  void STRUMPACK_set_verbose(STRUMPACK_SparseSolver S, int v);
  void STRUMPACK_set_maxit(STRUMPACK_SparseSolver S, int maxit);
  void STRUMPACK_set_gmres_restart(STRUMPACK_SparseSolver S, int m);
  void STRUMPACK_set_rel_tol(STRUMPACK_SparseSolver S, double tol);
  void STRUMPACK_set_abs_tol(STRUMPACK_SparseSolver S, double tol);
  void STRUMPACK_set_nd_param(STRUMPACK_SparseSolver S, int nd_param);
  void STRUMPACK_set_reordering_method
  (STRUMPACK_SparseSolver S, STRUMPACK_REORDERING_STRATEGY m);
  void STRUMPACK_set_GramSchmidt_type
  (STRUMPACK_SparseSolver S, STRUMPACK_GRAM_SCHMIDT_TYPE t);
  void STRUMPACK_set_matching(STRUMPACK_SparseSolver S, int job);
  void STRUMPACK_set_Krylov_solver
  (STRUMPACK_SparseSolver S, STRUMPACK_KRYLOV_SOLVER solver_type);
  /* set HSS specific options */
  void STRUMPACK_enable_HSS(STRUMPACK_SparseSolver S);
  void STRUMPACK_disable_HSS(STRUMPACK_SparseSolver S);
  void STRUMPACK_set_HSS_min_front_size(STRUMPACK_SparseSolver S, int size);
  void STRUMPACK_set_HSS_min_sep_size(STRUMPACK_SparseSolver S, int size);
  void STRUMPACK_set_HSS_max_rank(STRUMPACK_SparseSolver S, int max_rank);
  void STRUMPACK_set_HSS_leaf_size(STRUMPACK_SparseSolver S, int leaf_size);
  void STRUMPACK_set_HSS_rel_tol(STRUMPACK_SparseSolver S, double rctol);
  void STRUMPACK_set_HSS_abs_tol(STRUMPACK_SparseSolver S, double actol);


  /*************************************************************
   ** Get options **********************************************
   ************************************************************/
  int STRUMPACK_verbose(STRUMPACK_SparseSolver S);
  int STRUMPACK_maxit(STRUMPACK_SparseSolver S);
  int STRUMPACK_get_gmres_restart(STRUMPACK_SparseSolver S);
  double STRUMPACK_rel_tol(STRUMPACK_SparseSolver S);
  double STRUMPACK_abs_tol(STRUMPACK_SparseSolver S);
  int STRUMPACK_nd_param(STRUMPACK_SparseSolver S);
  STRUMPACK_REORDERING_STRATEGY STRUMPACK_reordering_method
  (STRUMPACK_SparseSolver S);
  STRUMPACK_GRAM_SCHMIDT_TYPE STRUMPACK_GramSchmidt_type
  (STRUMPACK_SparseSolver S);
  int STRUMPACK_matching(STRUMPACK_SparseSolver S);
  STRUMPACK_KRYLOV_SOLVER STRUMPACK_Krylov_solver(STRUMPACK_SparseSolver S);

  /* get HSS specific options */
  int use_HSS(STRUMPACK_SparseSolver S);
  int STRUMPACK_HSS_min_front_size(STRUMPACK_SparseSolver S);
  int STRUMPACK_HSS_min_sep_size(STRUMPACK_SparseSolver S);
  int STRUMPACK_HSS_max_rank(STRUMPACK_SparseSolver S);
  int STRUMPACK_HSS_leaf_size(STRUMPACK_SparseSolver S);
  double STRUMPACK_HSS_rel_tol(STRUMPACK_SparseSolver S);
  double STRUMPACK_HSS_abs_tol(STRUMPACK_SparseSolver S);


  /*************************************************************
   ** Get solve statistics *************************************
   ************************************************************/
  int STRUMPACK_its(STRUMPACK_SparseSolver S);
  int STRUMPACK_rank(STRUMPACK_SparseSolver S);
  long long STRUMPACK_factor_nonzeros(STRUMPACK_SparseSolver S);
  long long STRUMPACK_factor_memory(STRUMPACK_SparseSolver S);

#ifdef __cplusplus
}
#endif

#endif
