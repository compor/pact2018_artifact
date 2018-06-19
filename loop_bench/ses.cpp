void serial(int N, float alpha, float *x, float *s_seq) {
	float s = s_seq[0] = x[0];
	for(int i=1; i<N; i++) {
		s = alpha * x[i] + (1 - alpha) * s;
		s_seq[i] = s;
	}
}
