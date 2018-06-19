void serial(int *A, int N) {
  int s = 0;
  int m = -99999;
  for(int i=1; i<N; i++) {
    s += A[i-1];
    if(m < s) {
      m = s;
    }
  }
}
