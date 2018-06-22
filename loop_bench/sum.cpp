void serial(int *A, int N) {
  int s = 0;
  for(int i=1; i<N; i++) {
    s += A[i-1];
  }
}
