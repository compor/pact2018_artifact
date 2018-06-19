void serial(int N, int *A) {
    int ofs = 0;
    int bal = true;
    for(int i=0; i<N; i++) {
        if(A[i] == 0) {
            ofs = ofs + 1;
        } else {
            ofs = ofs - 1;
        }
        if (ofs<0) {
            bal = false;
        }
    }
}
