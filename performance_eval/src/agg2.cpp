// Max Prefix Sum
//
#include <iostream>
#include <limits.h>
#include <stdlib.h>
#include <thread>
#include <fstream>
#include <sys/time.h>
#include <vector>
#include "immintrin.h"
#include "interpolate.h"
using namespace std;


#define N (1<<29)
int serial_time, par_time;
struct timeval tv1, tv2;
struct timezone tz1, tz2;
ofstream fout("speedups.txt", fstream::out|fstream::app);

void serial(float *A) {
  int c = 0;
  vector<int> res;
  gettimeofday(&tv1, &tz1); 
  for(int i=1; i<N; i++) {
    if(A[i]-A[i-1] < 1) {
        c++;
    } else {
        res.push_back(c);
        c = 0;
    }
  }
  gettimeofday(&tv2, &tz2);
  cout << "serial res: " << res.size() << endl;
  serial_time = 1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "serial time: " << serial_time<< endl;
}


void parallel(float *A, int nth) {
  int block = N / nth;
  if(N%nth)block+=1;
  thread *workers = new thread[nth];
  vector<int> cc = {0, 1};
  int *vc = (int *)_mm_malloc(16*sizeof(int), 64);
  vc[0] = 0; vc[1] = 1;
  int *bvc = (int *)_mm_malloc(16*nth*sizeof(int), 64);
  SymList<int> *bres = new SymList<int>[nth];
  vector<int> res;
  int c = 0;

  gettimeofday(&tv1, &tz1);
  for(int tid=0; tid<nth; tid++) {
    workers[tid] = thread([=]{
                          __m512i vecc;
                          vecc = _mm512_load_epi32(vc);
                          for(int j=(tid*block==0?1:tid*block); j<((tid+1)*block<N?(tid+1)*block:N); j++) {
                            if(A[j] - A[j-1] < 1) {
                                vecc = _mm512_add_epi32(vecc, _mm512_set1_epi32(1));
                            } else {
                                bres[tid].push_back(vecc);
                                vecc = _mm512_set1_epi32(0);
                            }
                          }
                          _mm512_store_epi32(bvc+tid*16, vecc);
    });
  }

  for(int tid=0; tid<nth; tid++) {
    workers[tid].join();

    for(int i=0;i<bres[tid].size();i++) {
        res.push_back(Linr_01(bres[tid][i][0], bres[tid][i][1], c));
    }
    c = Linr_01(bvc[tid*16], bvc[tid*16+1], c);
  }

  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << res.size() << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "speed up: " << 1.0 * serial_time / par_time << endl;
  fout <<  1.0 * serial_time / par_time << "\t";

}

int main()
{
  srand(time(0));


  float *A = new float[N];
  A[0] = 0;

  for(int i=1; i<N; i++) {
    A[i] = A[i-1];
    if(rand() % 10000 == 0) A[i] += 1;
  }
  serial(A);
  fout << "agg2\t";
  for(int nth=1; nth<=64;nth*=2) {
    parallel(A, nth);
  }
  fout << endl;

  return 0;
}

