void parallel(int N, float *A) {
  /* Part One: Parallel Sampling  */
  const int _N_SAMP = 2;
  float S_Vm[_N_THREADS][_N_SAMP];
  float S_Vm2[_N_THREADS][_N_SAMP];
  int BSIZE = N / _N_THREADS;
  thread *workers = new thread[_N_THREADS];
  for (int tid = 0; tid < _N_THREADS; tid++) {
    workers[tid] = thread([=] {
      float Vm[_N_THREADS] = {MY_SMALL, MY_LARGE};
      float Vm2[_N_THREADS] = {MY_LARGE, MY_LARGE};
      int start_pos = tid * BSIZE;
      int end_pos = min(tid * BSIZE, N);

      for (int i = start_pos; i < end_pos; i++) {
        for (int _i_s = 0; _i_s < _N_SAMP; _i_s++) {
          if (Vm[_i_s] > A[i]) {
            Vm2[_i_s] = Vm[_i_s];
            Vm[_i_s] = A[i];
          } else if (A[i] < Vm2[_i_s]) {
            Vm2[_i_s] = A[i];
          }
        } // end sampling
      }
      memcpy(S_Vm[tid], Vm, sizeof(Vm));
      memcpy(S_Vm2[tid], Vm2, sizeof(Vm2));
    });
  } // end threading

  /* Part Two: Sequential Propagation */
  float m = 999999;
  float m2 = 999999;
  for (tid = 0; tid < _N_THREADS; tid++) {
    workers[tid].join();
    float mm0 = Rectf_min(S_Vm[tid][1], (1.000000 * m));
    float m2m0 =
        (m > 0 ? Rectf_min(S_Vm2[tid][1], m) : Rectf_max(S_Vm2[tid][0], m));
    float m2m20 = Rectf_min(m2m0, (1.000000 * m2));
    m = mm0;
    m2 = m2m20;
  } // end propagation
}
