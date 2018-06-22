#include "interpolate.h"
#include <cstring>
#include <thread>
using namespace std;
#ifndef _N_THREADS
#define _N_THREADS 8
#endif

void parallel(int *A, int N) {
  /* Part One: Parallel Sampling  */
  const int _N_SAMP = 2;
  int S_Vm[_N_THREADS][_N_SAMP];
  int S_Vs[_N_THREADS][_N_SAMP];

  int BSIZE = N / _N_THREADS;
  thread *workers = new thread[_N_THREADS];
  for (int tid = 0; tid < _N_THREADS; tid++) {

    workers[tid] = thread([=, &S_Vm, &S_Vs] {
      int Vm[_N_THREADS] = {0, 0};
      int Vs[_N_THREADS] = {0, 1};
      int start_pos = tid * BSIZE;
      int end_pos = min(tid * BSIZE, N);

      for (int i = start_pos; i < end_pos; i++) {
        for (int _i_s = 0; _i_s < _N_SAMP; _i_s++) {
          Vs[_i_s] += A[i - 1];
          if (Vm[_i_s] < Vs[_i_s]) {
            Vm[_i_s] = Vs[_i_s];
          }
        } // end sampling
      }
      memcpy(S_Vm[tid], Vm, sizeof(Vm));
      memcpy(S_Vs[tid], Vs, sizeof(Vs));
    });
  } // end threading

  /* Part Two: Sequential Propagation */
  int s = 0;
  int m = -99999;
  for (int tid = 0; tid < _N_THREADS; tid++) {
    workers[tid].join();
    int ms0 = Rectf_max(S_Vm[tid][0], (s + S_Vm[tid][0] - 1.000000 * MY_LARGE));
    int mm0 = Rectf_max(ms0, (1.000000 * m));
    int ss0 = 1.000000 * s + S_Vs[tid][0];
    m = mm0;
    s = ss0;
  } // end propagation
}
