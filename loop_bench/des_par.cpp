void parallel(int N, float alpha, float beta, float *x, float *y_seq) {
  /* Part One: Parallel Sampling  */
  const int _N_SAMP = 4;
  float S_Vb[_N_THREADS][_N_SAMP];
  float S_Vs[_N_THREADS][_N_SAMP];

  int BSIZE = N / _N_THREADS;
  thread *workers = new thread[_N_THREADS];
  for (int tid = 0; tid < _N_THREADS; tid++) {

    workers[tid] = thread([=, &S_Vb, &S_Vs] {
      float Vb[_N_SAMP] = {0, 1, 0, 1};
      float Vs[_N_SAMP] = {0, 0, 1, 1};
      int start_pos = max(2, tid * BSIZE);
      int end_pos = min(tid * BSIZE + BSIZE, N);

      for (int i = start_pos; i < end_pos; i++) {
        for (int _i_s = 0; _i_s < _N_SAMP; _i_s++) {
          float s_next = alpha * x[i] + (1 - alpha) * (Vs[_i_s] + Vb[_i_s]);
          Vb[_i_s] = beta * (s_next - Vs[_i_s]) + (1 - beta) * Vb[_i_s];
          Vs[_i_s] = s_next;
          y_seq[i] = Vs[_i_s] + Vb[_i_s];
        } // end sampling
      }
      memcpy(S_Vb[tid], Vb, sizeof(Vb));
      memcpy(S_Vs[tid], Vs, sizeof(Vs));
    });
  } // end threading

  /* Part Two: Sequential Propagation */
  float s = x[1];
  float b = x[1] - x[0];
  for (int tid = 0; tid < _N_THREADS; tid++) {
    workers[tid].join();
    float bs0 = Linr_01(S_Vb[tid][0], S_Vb[tid][2], s);
    float bs1 = Linr_01(S_Vb[tid][1], S_Vb[tid][3], s);
    float bb0 = Linr_01(bs0, bs1, b);
    float ss0 = Linr_01(S_Vs[tid][0], S_Vs[tid][2], s);
    float ss1 = Linr_01(S_Vs[tid][1], S_Vs[tid][3], s);
    float sb0 = Linr_01(ss0, ss1, b);
    b = bb0;
    s = sb0;
  } // end propagation
}
