void serial(int N, int *A) {
  int s = 0;
  int p = 0;
  for(int i=0; i<N; i++) {
    if(s+A[i]>0) {
      s += A[i];
    } else {
      s = 0;
      p = i + 1;
    }
  }
}
