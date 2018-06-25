// template to test the correctness of parallelized codes
// this file test the parallelization of 'mss.cpp'
// compile with: g++ -std=c++11 test_mts.cpp -lpthread -o test_mts


#include <iostream>
#include <cstring>
#include <thread>
#include <time.h>
#include <stdlib.h>
#include "interpolate.h"
using namespace std;

const int N = 512;
const int _N_THREADS = 4;


void serial(int N, int *A) {
  int s = 0;
  int p = 0;
  for(int i=0; i<N; i++) {
    if(s+A[i]>0) {
      s += A[i];
    } else {
      s = 0;
	  p = i + 1;
    }
  }
  cout << s << " " << p << endl;
}



void parallel(int N, int *A) {
  /* Part One: Parallel Sampling  */
  const int _N_SAMP = 2;
  int S_Vp[_N_THREADS][_N_SAMP];
  int S_Vs[_N_THREADS][_N_SAMP];

  int BSIZE = N / _N_THREADS;
  thread *workers = new thread[_N_THREADS];
  for (int tid = 0; tid < _N_THREADS; tid++) {

    workers[tid] = thread([=, &S_Vp, &S_Vs] {
      int Vp[_N_SAMP] = {0, 0};
      int Vs[_N_SAMP] = {MY_SMALL, MY_LARGE};
      int start_pos = max(0, tid * BSIZE);
      int end_pos = min(tid * BSIZE + BSIZE, N);

      for (int i = start_pos; i < end_pos; i++) {
        for (int _i_s = 0; _i_s < _N_SAMP; _i_s++) {
          if (Vs[_i_s] + A[i] > 0) {
            Vs[_i_s] += A[i];
          } else {
            Vs[_i_s] = 0;
            Vp[_i_s] = i + 1;
          }
        } // end sampling
      }
      memcpy(S_Vp[tid], Vp, sizeof(Vp));
      memcpy(S_Vs[tid], Vs, sizeof(Vs));
    });
  } // end threading

  /* Part Two: Sequential Propagation */
  int s = 0;
  int p = 0;
  for (int tid = 0; tid < _N_THREADS; tid++) {
    workers[tid].join();
    int ps0 =
        Thrld_max(S_Vs[tid][0], (s + S_Vs[tid][1] - MY_LARGE), S_Vp[tid][0], p);
    int ss0 = Rectf_max(S_Vs[tid][0], (s + S_Vs[tid][1] - 1.000000 * MY_LARGE));
    p = ps0;
    s = ss0;
  } // end propagation
  cout << s << " " << p << endl;
}

int main() {
	int *A = new int[N];

	srand(time(NULL));
	for (int i=0; i<N; i++)
		A[i] = rand() % 100 - 50;

	serial(N, A);
	parallel(N, A);
	return 0;
}
