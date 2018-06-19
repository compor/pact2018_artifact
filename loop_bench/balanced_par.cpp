void parallel(int N, int *A) {
  /* Part One: Parallel Sampling  */
  const int _N_SAMP = 2;
  int S_Vbal[_N_THREADS][_N_SAMP];
  int S_Vofs[_N_THREADS][_N_SAMP];
  int BSIZE = N / _N_THREADS;
  thread *workers = new thread[_N_THREADS];
  for (int tid = 0; tid < _N_THREADS; tid++) {
    workers[tid] = thread([=] {
      int Vbal[_N_THREADS] = {0, 0};
      int Vofs[_N_THREADS] = {MY_SMALL, MY_LARGE};
      int start_pos = tid * BSIZE;
      int end_pos = min(tid * BSIZE, N);

      for (int i = start_pos; i < end_pos; i++) {
        for (int _i_s = 0; _i_s < _N_SAMP; _i_s++) {
          if (A[i] == 0) {
            Vofs[_i_s] = Vofs[_i_s] + 1;
          } else {
            Vofs[_i_s] = Vofs[_i_s] - 1;
          }
          if (Vofs[_i_s] < 0) {
            Vbal[_i_s] = false;
          }
        } // end sampling
      }
      memcpy(S_Vbal[tid], Vbal, sizeof(Vbal));
      memcpy(S_Vofs[tid], Vofs, sizeof(Vofs));
    });
  } // end threading

  /* Part Two: Sequential Propagation */
  int ofs = 0;
  int bal = true;
  for (tid = 0; tid < _N_THREADS; tid++) {
    workers[tid].join();
    int balofs0 =
        Rectf_max(S_Vbal[tid][0], (ofs + S_Vbal[tid][1] - 0.000000 * MY_LARGE));
    int ofsofs0 = 1.000000 * ofs + S_Vofs[tid][1] - MY_LARGE;
    bal = balofs0;
    ofs = ofsofs0;
  } // end propagation
}
