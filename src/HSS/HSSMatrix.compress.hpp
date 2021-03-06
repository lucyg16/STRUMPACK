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
 */
#ifndef HSS_MATRIX_COMPRESS_HPP
#define HSS_MATRIX_COMPRESS_HPP

#include "misc/RandomWrapper.hpp"

namespace strumpack {
  namespace HSS {

    template<typename scalar_t> void HSSMatrix<scalar_t>::compress_original
    (const DenseM_t& A, const opts_t& opts) {
      AFunctor<scalar_t> afunc(A);
      compress_original(afunc, afunc, opts);
    }

    template<typename scalar_t> void HSSMatrix<scalar_t>::compress_original
    (const mult_t& Amult, const elem_t& Aelem, const opts_t& opts) {
      int d_old = 0, d = opts.d0() + opts.p();
      auto n = this->cols();
      DenseM_t Rr, Rc, Sr, Sc;
      std::unique_ptr<random::RandomGeneratorBase<real_t>> rgen;
      if (!opts.user_defined_random())
        rgen = random::make_random_generator<real_t>
          (opts.random_engine(), opts.random_distribution());
      WorkCompress<scalar_t> w;
      while (!this->is_compressed()) {
        Rr.resize(n, d);
        Rc.resize(n, d);
        Sr.resize(n, d);
        Sc.resize(n, d);
        DenseMW_t Rr_new(n, d-d_old, Rr, 0, d_old);
        DenseMW_t Rc_new(n, d-d_old, Rc, 0, d_old);
        if (!opts.user_defined_random()) {
          Rr_new.random(*rgen);
          STRUMPACK_RANDOM_FLOPS
            (rgen->flops_per_prng() * Rr_new.rows() * Rr_new.cols());
          Rc_new.copy(Rr_new);
        }
        DenseMW_t Sr_new(n, d-d_old, Sr, 0, d_old);
        DenseMW_t Sc_new(n, d-d_old, Sc, 0, d_old);
        Amult(Rr_new, Rc_new, Sr_new, Sc_new);
        if (opts.verbose())
          std::cout << "# compressing with d = " << d-opts.p()
                    << " + " << opts.p() << " (original)" << std::endl;
#pragma omp parallel if(!omp_in_parallel())
#pragma omp single nowait
        compress_recursive_original
          (Rr, Rc, Sr, Sc, Aelem, opts, w, d-d_old,
           this->_openmp_task_depth);
        if (!this->is_compressed()) {
          d_old = d;
          d = 2 * (d_old - opts.p()) + opts.p();
        }
      }
    }

    template<typename scalar_t> void
    HSSMatrix<scalar_t>::compress_hard_restart
    (const DenseM_t& A, const opts_t& opts) {
      AFunctor<scalar_t> afunc(A);
      compress_hard_restart(afunc, afunc, opts);
    }

    template<typename scalar_t> void
    HSSMatrix<scalar_t>::compress_hard_restart
    (const mult_t& Amult, const elem_t& Aelem, const opts_t& opts) {
      int d_old = 0, d = opts.d0() + opts.p();
      auto n = this->cols();
      DenseM_t Rr, Rc, Sr, Sc, R2, Sr2, Sc2;
      std::unique_ptr<random::RandomGeneratorBase<real_t>> rgen;
      if (!opts.user_defined_random())
        rgen = random::make_random_generator<real_t>
          (opts.random_engine(), opts.random_distribution());
      while (!this->is_compressed()) {
        WorkCompress<scalar_t> w;
        Rr = DenseM_t(n, d);
        Rc = DenseM_t(n, d);
        Sr = DenseM_t(n, d);
        Sc = DenseM_t(n, d);
        strumpack::copy(R2,  Rr, 0, 0);
        strumpack::copy(R2,  Rc, 0, 0);
        strumpack::copy(Sr2, Sr, 0, 0);
        strumpack::copy(Sc2, Sc, 0, 0);
        DenseMW_t Rr_new(n, d-d_old, Rr, 0, d_old);
        DenseMW_t Rc_new(n, d-d_old, Rc, 0, d_old);
        if (!opts.user_defined_random()) {
          Rr_new.random(*rgen);
          STRUMPACK_RANDOM_FLOPS
            (rgen->flops_per_prng() * Rr_new.rows() * Rr_new.cols());
          Rc_new.copy(Rr_new);
        }
        DenseMW_t Sr_new(n, d-d_old, Sr, 0, d_old);
        DenseMW_t Sc_new(n, d-d_old, Sc, 0, d_old);
        Amult(Rr_new, Rc_new, Sr_new, Sc_new);
        R2 = Rr; Sr2 = Sr; Sc2 = Sc;
        if (opts.verbose())
          std::cout << "# compressing with d = " << d-opts.p()
                    << " + " << opts.p() << " (original, hard restart)"
                    << std::endl;
#pragma omp parallel if(!omp_in_parallel())
#pragma omp single nowait
        compress_recursive_original
          (Rr, Rc, Sr, Sc, Aelem, opts, w, d,
           this->_openmp_task_depth);
        if (!this->is_compressed()) {
          d_old = d;
          d = 2 * (d_old - opts.p()) + opts.p();
          reset();
        }
      }
    }

    template<typename scalar_t> void
    HSSMatrix<scalar_t>::compress_recursive_original
    (DenseM_t& Rr, DenseM_t& Rc, DenseM_t& Sr, DenseM_t& Sc,
     const elem_t& Aelem, const opts_t& opts,
     WorkCompress<scalar_t>& w, int dd, int depth) {
      if (this->leaf()) {
        if (this->is_untouched()) {
          std::vector<std::size_t> I, J;
          I.reserve(this->rows());
          J.reserve(this->cols());
          for (std::size_t i=0; i<this->rows(); i++)
            I.push_back(i+w.offset.first);
          for (std::size_t j=0; j<this->cols(); j++)
            J.push_back(j+w.offset.second);
          _D = DenseM_t(this->rows(), this->cols());
          Aelem(I, J, _D);
        }
      } else {
        w.split(this->_ch[0]->dims());
        bool tasked = depth < params::task_recursion_cutoff_level;
        if (tasked) {
#pragma omp task default(shared)                                        \
  final(depth >= params::task_recursion_cutoff_level-1) mergeable
          this->_ch[0]->compress_recursive_original
            (Rr, Rc, Sr, Sc, Aelem, opts, w.c[0], dd, depth+1);
#pragma omp task default(shared)                                        \
  final(depth >= params::task_recursion_cutoff_level-1) mergeable
          this->_ch[1]->compress_recursive_original
            (Rr, Rc, Sr, Sc, Aelem, opts, w.c[1], dd, depth+1);
#pragma omp taskwait
        } else {
          this->_ch[0]->compress_recursive_original
            (Rr, Rc, Sr, Sc, Aelem, opts, w.c[0], dd, depth+1);
          this->_ch[1]->compress_recursive_original
            (Rr, Rc, Sr, Sc, Aelem, opts, w.c[1], dd, depth+1);
        }
        if (!this->_ch[0]->is_compressed() ||
            !this->_ch[1]->is_compressed()) return;
        if (this->is_untouched()) {
#pragma omp task default(shared) if(tasked)                             \
  final(depth >= params::task_recursion_cutoff_level-1) mergeable
          {
            _B01 = DenseM_t(this->_ch[0]->U_rank(), this->_ch[1]->V_rank());
            Aelem(w.c[0].Ir, w.c[1].Ic, _B01);
          }
#pragma omp task default(shared) if(tasked)                             \
  final(depth >= params::task_recursion_cutoff_level-1) mergeable
          {
            _B10 = DenseM_t(this->_ch[1]->U_rank(), this->_ch[0]->V_rank());
            Aelem(w.c[1].Ir, w.c[0].Ic, _B10);
          }
#pragma omp taskwait
        }
      }
      if (w.lvl == 0) this->_U_state = this->_V_state = State::COMPRESSED;
      else {
        auto d = Rr.cols();
        if (this->is_untouched())
          compute_local_samples(Rr, Rc, Sr, Sc, w, 0, d, depth);
        else compute_local_samples(Rr, Rc, Sr, Sc, w, d-dd, dd, depth);
        if (!this->is_compressed()) {
          if (compute_U_V_bases(Sr, Sc, opts, w, d, depth)) {
            reduce_local_samples(Rr, Rc, w, 0, d, depth);
            this->_U_state = this->_V_state = State::COMPRESSED;
          } else
            this->_U_state = this->_V_state = State::PARTIALLY_COMPRESSED;
        } else reduce_local_samples(Rr, Rc, w, d-dd, dd, depth);
      }
    }

    template<typename scalar_t> void
    HSSMatrix<scalar_t>::compress_level_original
    (DenseM_t& Rr, DenseM_t& Rc, DenseM_t& Sr, DenseM_t& Sc,
     const opts_t& opts, WorkCompress<scalar_t>& w,
     int dd, int lvl, int depth) {
      if (this->leaf()) {
        if (w.lvl < lvl) return;
      } else {
        if (w.lvl < lvl) {
          bool tasked = depth < params::task_recursion_cutoff_level;
          if (tasked) {
#pragma omp task default(shared)                                        \
  final(depth >= params::task_recursion_cutoff_level-1) mergeable
            this->_ch[0]->compress_level_original
              (Rr, Rc, Sr, Sc, opts, w.c[0], dd, lvl, depth+1);
#pragma omp task default(shared)                                        \
  final(depth >= params::task_recursion_cutoff_level-1) mergeable
            this->_ch[1]->compress_level_original
              (Rr, Rc, Sr, Sc, opts, w.c[1], dd, lvl, depth+1);
#pragma omp taskwait
          } else {
            this->_ch[0]->compress_level_original
              (Rr, Rc, Sr, Sc, opts, w.c[0], dd, lvl, depth+1);
            this->_ch[1]->compress_level_original
              (Rr, Rc, Sr, Sc, opts, w.c[1], dd, lvl, depth+1);
          }
          return;
        }
        if (!this->_ch[0]->is_compressed() ||
            !this->_ch[1]->is_compressed()) return;
      }
      if (w.lvl==0) this->_U_state = this->_V_state = State::COMPRESSED;
      else {
        auto d = Rr.cols();
        if (this->is_untouched())
          compute_local_samples(Rr, Rc, Sr, Sc, w, 0, d, depth);
        else compute_local_samples(Rr, Rc, Sr, Sc, w, d-dd, dd, depth);
        if (!this->is_compressed()) {
          if (compute_U_V_bases(Sr, Sc, opts, w, d, depth)) {
            reduce_local_samples(Rr, Rc, w, 0, d, depth);
            this->_U_state = this->_V_state = State::COMPRESSED;
          } else
            this->_U_state = this->_V_state = State::PARTIALLY_COMPRESSED;
        } else reduce_local_samples(Rr, Rc, w, d-dd, dd, depth);
      }
    }

    template<typename scalar_t> void
    HSSMatrix<scalar_t>::get_extraction_indices
    (std::vector<std::vector<std::size_t>>& I,
     std::vector<std::vector<std::size_t>>& J,
     const std::pair<std::size_t,std::size_t>& off,
     WorkCompress<scalar_t>& w, int& self, int lvl) {
      if (this->leaf()) {
        if (w.lvl == lvl && this->is_untouched()) {
          self++;
          I.emplace_back();  J.emplace_back();
          I.back().reserve(this->rows());  J.back().reserve(this->cols());
          for (std::size_t i=0; i<this->rows(); i++)
            I.back().push_back(i+w.offset.first+off.first);
          for (std::size_t j=0; j<this->cols(); j++)
            J.back().push_back(j+w.offset.second+off.second);
        }
      } else {
        w.split(this->_ch[0]->dims());
        if (w.lvl < lvl) {
          this->_ch[0]->get_extraction_indices(I, J, off, w.c[0], self, lvl);
          this->_ch[1]->get_extraction_indices(I, J, off, w.c[1], self, lvl);
          return;
        }
        if (this->is_untouched()) {
          self += 2;
          I.push_back(w.c[0].Ir);  J.push_back(w.c[1].Ic);
          for (auto& i : I.back()) i += off.first;
          for (auto& j : J.back()) j += off.second;
          I.push_back(w.c[1].Ir);  J.push_back(w.c[0].Ic);
          for (auto& i : I.back()) i += off.first;
          for (auto& j : J.back()) j += off.second;
        }
      }
    }

    template<typename scalar_t> void
    HSSMatrix<scalar_t>::get_extraction_indices
    (std::vector<std::vector<std::size_t>>& I,
     std::vector<std::vector<std::size_t>>& J, std::vector<DenseM_t*>& B,
     const std::pair<std::size_t,std::size_t>& off,
     WorkCompress<scalar_t>& w, int& self, int lvl) {
      if (this->leaf()) {
        if (w.lvl == lvl && this->is_untouched()) {
          self++;
          I.emplace_back();  J.emplace_back();
          I.back().reserve(this->rows());  J.back().reserve(this->cols());
          for (std::size_t i=0; i<this->rows(); i++)
            I.back().push_back(i+w.offset.first+off.first);
          for (std::size_t j=0; j<this->cols(); j++)
            J.back().push_back(j+w.offset.second+off.second);
          _D = DenseM_t(this->rows(), this->cols());
          B.push_back(&_D);
        }
      } else {
        w.split(this->_ch[0]->dims());
        if (w.lvl < lvl) {
          this->_ch[0]->get_extraction_indices(I, J, B, off, w.c[0], self, lvl);
          this->_ch[1]->get_extraction_indices(I, J, B, off, w.c[1], self, lvl);
          return;
        }
        if (this->is_untouched()) {
          self += 2;
          I.push_back(w.c[0].Ir);  J.push_back(w.c[1].Ic);
          for (auto& i : I.back()) i += off.first;
          for (auto& j : J.back()) j += off.second;
          I.push_back(w.c[1].Ir);  J.push_back(w.c[0].Ic);
          for (auto& i : I.back()) i += off.first;
          for (auto& j : J.back()) j += off.second;
          _B01 = DenseM_t(this->_ch[0]->U_rank(), this->_ch[1]->V_rank());
          _B10 = DenseM_t(this->_ch[1]->U_rank(), this->_ch[0]->V_rank());
          B.push_back(&_B01);
          B.push_back(&_B10);
        }
      }
    }

    template<typename scalar_t> void HSSMatrix<scalar_t>::extract_D_B
    (const elem_t& Aelem, const opts_t& opts,
     WorkCompress<scalar_t>& w, int lvl) {
      if (this->leaf()) {
        if (w.lvl < lvl) return;
        if (this->is_untouched()) {
          std::vector<std::size_t> I, J;
          I.reserve(this->rows());
          J.reserve(this->cols());
          for (std::size_t i=0; i<this->rows(); i++)
            I.push_back(i+w.offset.first);
          for (std::size_t j=0; j<this->cols(); j++)
            J.push_back(j+w.offset.second);
          _D = DenseM_t(this->rows(), this->cols());
          Aelem(I, J, _D);
        }
      } else {
        if (w.lvl < lvl) {
          this->_ch[0]->extract_D_B(Aelem, opts, w.c[0], lvl);
          this->_ch[1]->extract_D_B(Aelem, opts, w.c[1], lvl);
          return;
        }
        if (this->is_untouched()) {
          _B01 = DenseM_t(this->_ch[0]->U_rank(), this->_ch[1]->V_rank());
          _B10 = DenseM_t(this->_ch[1]->U_rank(), this->_ch[0]->V_rank());
          Aelem(w.c[0].Ir, w.c[1].Ic, _B01);
          Aelem(w.c[1].Ir, w.c[0].Ic, _B10);
        }
      }
    }

    template<typename scalar_t> void
    HSSMatrix<scalar_t>::compute_local_samples
    (DenseM_t& Rr, DenseM_t& Rc, DenseM_t& Sr, DenseM_t& Sc,
     WorkCompress<scalar_t>& w, int d0, int d, int depth) {
      TIMER_TIME(TaskType::COMPUTE_SAMPLES, 1, t_compute);
#pragma omp task default(shared)                                        \
  if(depth < params::task_recursion_cutoff_level)                       \
  final(depth >= params::task_recursion_cutoff_level-1) mergeable
      {
        if (this->leaf()) {
          DenseMW_t wSr(this->rows(), d, Sr, w.offset.second, d0);
          DenseMW_t wRr(this->rows(), d, Rr, w.offset.second, d0);
          gemm(Trans::N, Trans::N, scalar_t(-1), _D, wRr,
               scalar_t(1.), wSr, depth);
          STRUMPACK_UPDATE_SAMPLE_FLOPS
            (gemm_flops
             (Trans::N, Trans::N, scalar_t(-1), _D, wRr, scalar_t(1.)));
        } else {
          DenseMW_t wSr0(this->_ch[0]->U_rank(), d, Sr,
                         w.offset.second, d0);
          DenseMW_t wSr1(this->_ch[1]->U_rank(), d, Sr,
                         w.offset.second+this->_ch[0]->U_rank(), d0);
          DenseMW_t wSr_ch0(this->_ch[0]->U_rows(), d, Sr,
                            w.c[0].offset.second, d0);
          DenseMW_t wSr_ch1(this->_ch[1]->U_rows(), d, Sr,
                            w.c[1].offset.second, d0);
          auto tmp0 = wSr_ch0.extract_rows(w.c[0].Jr);
          auto tmp1 = wSr_ch1.extract_rows(w.c[1].Jr);
          wSr0.copy(tmp0);
          wSr1.copy(tmp1);
          DenseMW_t wRr1(this->_ch[1]->V_rank(), d, Rr,
                         w.c[1].offset.second, d0);
          DenseMW_t wRr0(this->_ch[0]->V_rank(), d, Rr,
                         w.c[0].offset.second, d0);
          gemm(Trans::N, Trans::N, scalar_t(-1.), _B01, wRr1,
               scalar_t(1.), wSr0, depth);
          gemm(Trans::N, Trans::N, scalar_t(-1.), _B10, wRr0,
               scalar_t(1.), wSr1, depth);
          STRUMPACK_UPDATE_SAMPLE_FLOPS
            (gemm_flops
             (Trans::N, Trans::N, scalar_t(-1.), _B01, wRr1, scalar_t(1.)) +
             gemm_flops
             (Trans::N, Trans::N, scalar_t(-1.), _B10, wRr0, scalar_t(1.)));
        }
      }
#pragma omp task default(shared)                                        \
  if(depth < params::task_recursion_cutoff_level)                       \
  final(depth >= params::task_recursion_cutoff_level-1) mergeable
      {
        if (this->leaf()) {
          DenseMW_t wSc(this->rows(), d, Sc, w.offset.second, d0);
          DenseMW_t wRc(this->rows(), d, Rc, w.offset.second, d0);
          gemm(Trans::C, Trans::N, scalar_t(-1), _D, wRc,
               scalar_t(1.), wSc, depth);
          STRUMPACK_UPDATE_SAMPLE_FLOPS
            (gemm_flops(Trans::C, Trans::N, scalar_t(-1), _D, wRc, scalar_t(1.)));
        } else {
          DenseMW_t wSc0(this->_ch[0]->V_rank(), d, Sc, w.offset.second, d0);
          DenseMW_t wSc1(this->_ch[1]->V_rank(), d, Sc,
                         w.offset.second+this->_ch[0]->V_rank(), d0);
          DenseMW_t wSc_ch0(this->_ch[0]->V_rows(), d, Sc,
                            w.c[0].offset.second, d0);
          DenseMW_t wSc_ch1(this->_ch[1]->V_rows(), d, Sc,
                            w.c[1].offset.second, d0);
          auto tmp1 = wSc_ch1.extract_rows(w.c[1].Jc);
          auto tmp0 = wSc_ch0.extract_rows(w.c[0].Jc);
          wSc0.copy(tmp0);
          wSc1.copy(tmp1);
          DenseMW_t wRc1(this->_ch[1]->U_rank(), d, Rc,
                         w.c[1].offset.second, d0);
          DenseMW_t wRc0(this->_ch[0]->U_rank(), d, Rc,
                         w.c[0].offset.second, d0);
          gemm(Trans::C, Trans::N, scalar_t(-1.), _B10, wRc1,
               scalar_t(1.), wSc0, depth);
          gemm(Trans::C, Trans::N, scalar_t(-1.), _B01, wRc0,
               scalar_t(1.), wSc1, depth);
          STRUMPACK_UPDATE_SAMPLE_FLOPS
            (gemm_flops
             (Trans::C, Trans::N, scalar_t(-1.), _B10, wRc1, scalar_t(1.)) +
             gemm_flops
             (Trans::C, Trans::N, scalar_t(-1.), _B01, wRc0, scalar_t(1.)));
        }
      }
#pragma omp taskwait
    }

    // TODO split in U and V compression
    template<typename scalar_t> bool HSSMatrix<scalar_t>::compute_U_V_bases
    (DenseM_t& Sr, DenseM_t& Sc, const opts_t& opts,
     WorkCompress<scalar_t>& w, int d, int depth) {
      auto rtol = opts.rel_tol() / w.lvl;
      auto atol = opts.abs_tol() / w.lvl;
#pragma omp task default(shared)                                        \
  if(depth < params::task_recursion_cutoff_level)                       \
  final(depth >= params::task_recursion_cutoff_level-1) mergeable
      {
        auto u_rows = this->leaf() ? this->rows() :
          this->_ch[0]->U_rank()+this->_ch[1]->U_rank();
        DenseM_t wSr(u_rows, d, Sr, w.offset.second, 0);
        wSr.ID_row(_U.E(), _U.P(), w.Jr, rtol, atol,
                   opts.max_rank(), depth);
        STRUMPACK_ID_FLOPS(ID_row_flops(wSr, _U.cols()));
      }
#pragma omp task default(shared)                                        \
  if(depth < params::task_recursion_cutoff_level)                       \
  final(depth >= params::task_recursion_cutoff_level-1) mergeable
      {
        auto v_rows = this->leaf() ? this->rows() :
          this->_ch[0]->V_rank()+this->_ch[1]->V_rank();
        DenseM_t wSc(v_rows, d, Sc, w.offset.second, 0);
        wSc.ID_row(_V.E(), _V.P(), w.Jc, rtol, atol,
                   opts.max_rank(), depth);
        STRUMPACK_ID_FLOPS(ID_row_flops(wSc, _V.cols()));
      }
#pragma omp taskwait

      _U.check();  assert(_U.cols() == w.Jr.size());
      _V.check();  assert(_V.cols() == w.Jc.size());
      if (d-opts.p() >= opts.max_rank() ||
          (int(_U.cols()) < d - opts.p() &&
           int(_V.cols()) < d - opts.p())) {
        this->_U_rank = _U.cols();  this->_U_rows = _U.rows();
        this->_V_rank = _V.cols();  this->_V_rows = _V.rows();
        w.Ir.reserve(_U.cols());
        w.Ic.reserve(_V.cols());
        if (this->leaf()) {
          for (auto i : w.Jr) w.Ir.push_back(w.offset.first + i);
          for (auto j : w.Jc) w.Ic.push_back(w.offset.second + j);
        } else {
          auto r0 = w.c[0].Ir.size();
          for (auto i : w.Jr)
            w.Ir.push_back((i < r0) ? w.c[0].Ir[i] : w.c[1].Ir[i-r0]);
          r0 = w.c[0].Ic.size();
          for (auto j : w.Jc)
            w.Ic.push_back((j < r0) ? w.c[0].Ic[j] : w.c[1].Ic[j-r0]);
        }
        return true;
      } else {
        w.Jr.clear();
        w.Jc.clear();
        return false;
      }
    }

    template<typename scalar_t> void HSSMatrix<scalar_t>::reduce_local_samples
    (DenseM_t& Rr, DenseM_t& Rc, WorkCompress<scalar_t>& w,
     int d0, int d, int depth) {
      TIMER_TIME(TaskType::REDUCE_SAMPLES, 1, t_reduce);
#pragma omp task default(shared)                                        \
  if(depth < params::task_recursion_cutoff_level)                       \
  final(depth >= params::task_recursion_cutoff_level-1) mergeable
      {
        DenseMW_t wRr(_V.rows(), d, Rr, w.offset.second, d0);
        if (this->leaf()) copy(_V.applyC(wRr, depth), wRr, 0, 0);
        else {
          DenseMW_t wRr0(this->_ch[0]->V_rank(), d, Rr,
                         w.c[0].offset.second, d0);
          DenseMW_t wRr1(this->_ch[1]->V_rank(), d, Rr,
                         w.c[1].offset.second, d0);
          copy(_V.applyC(vconcat(wRr0, wRr1), depth), wRr, 0, 0);
        }
      }
#pragma omp task default(shared)                                        \
  if(depth < params::task_recursion_cutoff_level)                       \
  final(depth >= params::task_recursion_cutoff_level-1) mergeable
      {
        DenseMW_t wRc(_U.rows(), d, Rc, w.offset.second, d0);
        if (this->leaf()) copy(_U.applyC(wRc, depth), wRc, 0, 0);
        else {
          DenseMW_t wRc0(this->_ch[0]->U_rank(), d, Rc,
                         w.c[0].offset.second, d0);
          DenseMW_t wRc1(this->_ch[1]->U_rank(), d, Rc,
                         w.c[1].offset.second, d0);
          copy(_U.applyC(vconcat(wRc0, wRc1), depth), wRc, 0, 0);
        }
      }
#pragma omp taskwait
      STRUMPACK_REDUCE_SAMPLE_FLOPS
        (_V.applyC_flops(d) + _U.applyC_flops(d));
    }

  } // end namespace HSS
} // end namespace strumpack

#endif // HSS_MATRIX_COMPRESS_HPP
