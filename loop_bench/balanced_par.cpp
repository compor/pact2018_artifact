void parallel(int N, int *A) {
  /* Part One: Parallel Sampling  */
  const int _N_SAMP = 1;
  int S_Vofs[_N_THREADS][_N_SAMP];

  int BSIZE = N / _N_THREADS;
  thread *workers = new thread[_N_THREADS];
  for (int tid = 0; tid < _N_THREADS; tid++) {

    workers[tid] = thread([=, &S_Vofs] {
      int Vofs[_N_SAMP] = {0};
      int start_pos = max(0, tid * BSIZE);
      int end_pos = min(tid * BSIZE + BSIZE, N);

      for (int i = start_pos; i < end_pos; i++) {
        for (int _i_s = 0; _i_s < _N_SAMP; _i_s++) {
          if (A[i] == 0) {
            Vofs[_i_s] = Vofs[_i_s] + 1;
          } else {
            Vofs[_i_s] = Vofs[_i_s] - 1;
          }
        } // end sampling
      }
      memcpy(S_Vofs[tid], Vofs, sizeof(Vofs));
    });
  } // end threading

  /* Part Two: Sequential Propagation */
  int ofs = 0;
  for (int tid = 0; tid < _N_THREADS; tid++) {
    workers[tid].join();
    int ofsofs0 = 1.000000 * ofs + S_Vofs[tid][0];
    ofs = ofsofs0;
  } // end propagation
}
