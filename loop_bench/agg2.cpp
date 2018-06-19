#include <vector>
using namespace std;
void serial(int N, float *A) {
  int c = 0;
  vector<int> res;
  for(int i=1; i<N; i++) {
    if(A[i]-A[i-1] < 1) {
        c++;
    } else {
        res.push_back(c);
        c = 0;
    }
  }
}
