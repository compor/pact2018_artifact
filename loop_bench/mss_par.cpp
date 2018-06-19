void parallel(int N, int *A) {
  /* Part One: Parallel Sampling  */
  const int _N_SAMP = 2;
  int S_Vm[_N_THREADS][_N_SAMP];
  int S_Vs[_N_THREADS][_N_SAMP];
  int BSIZE = N / _N_THREADS;
  thread *workers = new thread[_N_THREADS];
  for (int tid = 0; tid < _N_THREADS; tid++) {
    workers[tid] = thread([=] {
      int Vm[_N_THREADS] = {MY_SMALL, MY_SMALL};
      int Vs[_N_THREADS] = {MY_SMALL, MY_LARGE};
      int start_pos = tid * BSIZE;
      int end_pos = min(tid * BSIZE, N);

      for (int i = start_pos; i < end_pos; i++) {
        for (int _i_s = 0; _i_s < _N_SAMP; _i_s++) {
          if (Vs[_i_s] < 0) {
            Vs[_i_s] = A[i];
          } else {
            Vs[_i_s] += A[i];
          }
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
  for (tid = 0; tid < _N_THREADS; tid++) {
    workers[tid].join();
    ms0 = Rectf_max(S_Vm[tid][0], (s + S_Vm[tid][1] - 1.000000 * MY_LARGE));
    mm0 = Rectf_max(ms0, (1.000000 * m));
    ss0 = Rectf_max(S_Vs[tid][0], (s + S_Vs[tid][1] - 1.000000 * MY_LARGE));
    m = mm0;
    s = ss0;
  } // end propagation
}
