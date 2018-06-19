void parallel(float *A, int N) {
  /* Part One: Parallel Sampling  */
  const int _N_SAMP = 4;
  float S_Vm[_N_THREADS][_N_SAMP];
  float S_Vma[_N_THREADS][_N_SAMP];
  float S_Vmi[_N_THREADS][_N_SAMP];
  int BSIZE = N / _N_THREADS;
  thread *workers = new thread[_N_THREADS];
  for (int tid = 0; tid < _N_THREADS; tid++) {
    workers[tid] = thread([=] {
      float Vm[_N_THREADS] = {MY_SMALL, MY_SMALL, MY_SMALL, MY_SMALL};
      float Vma[_N_THREADS] = {MY_SMALL, MY_SMALL, MY_LARGE, MY_LARGE};
      float Vmi[_N_THREADS] = {MY_SMALL, MY_LARGE, MY_SMALL, MY_LARGE};
      int start_pos = tid * BSIZE;
      int end_pos = min(tid * BSIZE, N);

      for (int i = start_pos; i < end_pos; i++) {
        for (int _i_s = 0; _i_s < _N_SAMP; _i_s++) {
          if (A[i] >= 0) {
            Vma[_i_s] = Vma[_i_s] * A[i] > A[i] ? Vma[_i_s] * A[i] : A[i];
            Vmi[_i_s] = Vmi[_i_s] * A[i] < A[i] ? Vmi[_i_s] * A[i] : A[i];
          } else {
            float tmp = Vmi[_i_s] * A[i] > A[i] ? Vmi[_i_s] * A[i] : A[i];
            Vmi[_i_s] = Vma[_i_s] * A[i] < A[i] ? Vma[_i_s] * A[i] : A[i];
            Vma[_i_s] = tmp;
          }

          if (Vm[_i_s] < Vma[_i_s]) {
            Vm[_i_s] = Vma[_i_s];
          }

        } // end sampling
      }
      memcpy(S_Vm[tid], Vm, sizeof(Vm));
      memcpy(S_Vma[tid], Vma, sizeof(Vma));
      memcpy(S_Vmi[tid], Vmi, sizeof(Vmi));
    });
  } // end threading

  /* Part Two: Sequential Propagation */
  float ma = -99999;
  float mi = -99999;
  float m = -99999;
  for (tid = 0; tid < _N_THREADS; tid++) {
    workers[tid].join();
    mma0 = Rectf_max(S_Vm[tid][0], (ma * S_Vm[tid][2] / MY_LARGE));
    mma1 = Rectf_max(S_Vm[tid][1], (ma * S_Vm[tid][3] / MY_LARGE));
    mmi0 = Rectf_max(mma1, (mi * mma0 / MY_SMALL));
    mm0 = Rectf_max(mmi0, (1.000000 * m));
    mama0 = Rectf_max(S_Vma[tid][0], (ma * S_Vma[tid][2] / MY_LARGE));
    mama1 = Rectf_max(S_Vma[tid][1], (ma * S_Vma[tid][3] / MY_LARGE));
    mami0 = Rectf_max(mama1, (mi * mama0 / MY_SMALL));
    mima0 = Rectf_min(S_Vmi[tid][0], (ma * S_Vmi[tid][2] / MY_LARGE));
    mima1 = Rectf_min(S_Vmi[tid][1], (ma * S_Vmi[tid][3] / MY_LARGE));
    mimi0 = Rectf_min(mima1, (mi * mima0 / MY_SMALL));
    m = mm0;
    ma = mami0;
    mi = mimi0;
  } // end propagation
}
