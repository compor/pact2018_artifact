void serial(int N, float alpha, float beta, float *x, float *y_seq) {
  float s = x[1];
  float b = x[1] - x[0];
  for(int i=2; i<N; i++) {
    float s_next = alpha * x[i] + (1 - alpha) * (s + b);
    b = beta * (s_next - s) + (1 - beta) * b;
    s = s_next;
    y_seq[i] = s + b;
  }
}
