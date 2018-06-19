#include <vector>
using namespace std;
void parallel(int N, float *A) {
  /* Part One: Parallel Sampling  */
  const int _N_SAMP = 2;
  int S_Vc[_N_THREADS][_N_SAMP];
  int BSIZE = N / _N_THREADS;
  thread *workers = new thread[_N_THREADS];
  for (int tid = 0; tid < _N_THREADS; tid++) {
    workers[tid] = thread([=] {
      int Vc[_N_THREADS] = {MY_SMALL, MY_LARGE};
      int start_pos = tid * BSIZE;
      int end_pos = min(tid * BSIZE, N);

      vector<int> res;
      for (int i = start_pos; i < end_pos; i++) {
        for (int _i_s = 0; _i_s < _N_SAMP; _i_s++) {
          if (A[i] - A[i - 1] < 1) {
            Vc[_i_s]++;
          } else {
            res.push_back(Vc[_i_s]);
            Vc[_i_s] = 0;
          }
        } // end sampling
      }
      memcpy(S_Vc[tid], Vc, sizeof(Vc));
    });
  } // end threading

  /* Part Two: Sequential Propagation */
  int c = 0;
  for (tid = 0; tid < _N_THREADS; tid++) {
    workers[tid].join();
    int cc0 = Linr(S_Vc[tid][0], S_Vc[tid][1], MY_SMALL, MY_LARGE, c);
    c = cc0;
  } // end propagation
}
