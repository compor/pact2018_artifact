void serial(int N, int *A) {
  int s = 0;
  int m = -99999;
  for(int i=0; i<N; i++) {
    if(s < 0) {
      s = A[i];
    } else {
      s += A[i];
    }
    if(m < s) {
      m = s;
    }
  }
}
