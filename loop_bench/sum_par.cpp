void parallel(int *A, int N) {
  /* Part One: Parallel Sampling  */
  const int _N_SAMP = 1;
  int S_Vs[_N_THREADS][_N_SAMP];
  int BSIZE = N / _N_THREADS;
  thread *workers = new thread[_N_THREADS];
  for (int tid = 0; tid < _N_THREADS; tid++) {
    workers[tid] = thread([=] {
      int Vs[_N_THREADS] = {MY_LARGE};
      int start_pos = tid * BSIZE;
      int end_pos = min(tid * BSIZE, N);

      for (int i = start_pos; i < end_pos; i++) {
        for (int _i_s = 0; _i_s < _N_SAMP; _i_s++) {
          Vs[_i_s] += A[i - 1];
        } // end sampling
      }
      memcpy(S_Vs[tid], Vs, sizeof(Vs));
    });
  } // end threading

  /* Part Two: Sequential Propagation */
  int s = 0;
  for (tid = 0; tid < _N_THREADS; tid++) {
    workers[tid].join();
    int ss0 = 1.000000 * s + S_Vs[tid][0] - MY_LARGE;
    s = ss0;
  } // end propagation
}
