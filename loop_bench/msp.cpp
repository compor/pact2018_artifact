void serial(float *A, int N) {
  float ma = -99999;
  float mi = -99999;
  float m = -99999;
  for(int i=0; i<N; i++) {
    if(A[i] >= 0) {
      ma = ma*A[i]>A[i]?ma*A[i]:A[i];
      mi = mi*A[i]<A[i]?mi*A[i]:A[i];
    } else {
      float tmp = mi*A[i]>A[i]?mi*A[i]:A[i];
      mi = ma*A[i]<A[i]?ma*A[i]:A[i];
      ma = tmp;
    }

    if(m < ma) {
      m = ma;
    }
  
  }
}

