void serial(int N, float *A) {
  float m = 999999;
  float m2 = 999999;
  for(int i=0; i<N; i++) {
    if(m > A[i]) {
      m2 = m;
      m = A[i];
    } else if(A[i] < m2) {
      m2 = A[i];
    }
  }
}
