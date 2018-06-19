void parallel(int N, float alpha, float *x, float *s_seq) {
  /* Part One: Parallel Sampling  */
  const int _N_SAMP = 2;
  float S_Vs[_N_THREADS][_N_SAMP];
  int BSIZE = N / _N_THREADS;
  thread *workers = new thread[_N_THREADS];
  for (int tid = 0; tid < _N_THREADS; tid++) {
    workers[tid] = thread([=] {
      float Vs[_N_THREADS] = {MY_SMALL, MY_LARGE};
      int start_pos = tid * BSIZE;
      int end_pos = min(tid * BSIZE, N);

      for (int i = start_pos; i < end_pos; i++) {
        for (int _i_s = 0; _i_s < _N_SAMP; _i_s++) {
          Vs[_i_s] = alpha * x[i] + (1 - alpha) * Vs[_i_s];
          s_seq[i] = Vs[_i_s];
        } // end sampling
      }
      memcpy(S_Vs[tid], Vs, sizeof(Vs));
    });
  } // end threading

  /* Part Two: Sequential Propagation */
  float s = s_seq[0] = x[0];
  for (tid = 0; tid < _N_THREADS; tid++) {
    workers[tid].join();
    float ss0 = Linr(S_Vs[tid][0], S_Vs[tid][1], MY_SMALL, MY_LARGE, s);
    s = ss0;
  } // end propagation
}
