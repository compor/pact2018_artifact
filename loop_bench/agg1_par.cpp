#include <cstdlib>
#include <string>
#include <vector>
using namespace std;

void parallel(int nlines, string *A) {
  /* Part One: Parallel Sampling  */
  const int _N_SAMP = 4;
  int S_Vcount[_N_THREADS][_N_SAMP];
  int S_Vpushed[_N_THREADS][_N_SAMP];

  int BSIZE = N / _N_THREADS;
  thread *workers = new thread[_N_THREADS];
  for (int tid = 0; tid < _N_THREADS; tid++) {

    workers[tid] = thread([=, &S_Vcount, &S_Vpushed] {
      int Vcount[_N_SAMP] = {0, 1, 0, 1};
      int Vpushed[_N_SAMP] = {0, 0, 1, 1};
      int start_pos = max(0, tid * BSIZE);
      int end_pos = min(tid * BSIZE + BSIZE, N);
      vector<int> res;

      for (int i = start_pos; i < end_pos; i++) {
        for (int _i_s = 0; _i_s < _N_SAMP; _i_s++) {
          string type = A[2 * i];
          string id = A[2 * i + 1];
          if (atoi(id.c_str()) == 1579990) {
            if (type == "PushEvent") {
              Vpushed[_i_s] = 1;
              Vcount[_i_s] = 0;
            } else if (type == "PullRequestEvent" && pushed == 1) {
              Vpushed[_i_s] = 0;
              res.push_back(count);
            } else if (Vpushed[_i_s]) {
              Vcount[_i_s]++;
            }
          }
        } // end sampling
      }

      memcpy(S_Vcount[tid], Vcount, sizeof(Vcount));
      memcpy(S_Vpushed[tid], Vpushed, sizeof(Vpushed));
    });
  } // end threading

  /* Part Two: Sequential Propagation */
  int pushed = 0;
  int count = 0;
  for (int tid = 0; tid < _N_THREADS; tid++) {
    workers[tid].join();
    int countpushed0;
    if (pushed == 0) {
      countpushed0 = S_Vcount[tid][0];
    } else {
      countpushed0 = S_Vcount[tid][2];
    }
    int countpushed1;
    if (pushed == 0) {
      countpushed1 = S_Vcount[tid][1];
    } else {
      countpushed1 = S_Vcount[tid][3];
    }
    int countcount0 = Linr_01(countpushed0, countpushed1, count);
    int pushedpushed0;
    if (pushed == 0) {
      pushedpushed0 = S_Vpushed[tid][0];
    } else {
      pushedpushed0 = S_Vpushed[tid][2];
    }
    count = countcount0;
    pushed = pushedpushed0;
  } // end propagation
}
