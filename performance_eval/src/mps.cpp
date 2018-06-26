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
#include "sse_api.h"

using namespace std;



#define N (1<<29)
int serial_time, par_time;
struct timeval tv1, tv2;
struct timezone tz1, tz2;
ofstream fout("speedups.txt", fstream::out|fstream::app);


void serial(int s, int m, int *A) {
  gettimeofday(&tv1, &tz1); 
  for(int i=0; i<N; i++) {
    s += A[i];
    if(m < s) {
      m = s;
    }
  }
  gettimeofday(&tv2, &tz2);
  cout << "serial res: " << m << " " << s << endl;
  serial_time = 1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "serial time: " << serial_time<< endl;
}


void simd_mimd(int s, int m, int *A, int nth) {
 int block = N / nth;
  if(N%nth)block+=1;
  thread *workers = new thread[nth];
  vector<int> ss = {0};
  vector<int> mm = {INT_TINY}; 
  int *vs = (int *)_mm_malloc(16*sizeof(int), 64);
  int *vm = (int *)_mm_malloc(16*sizeof(int), 64);
  MeshGridFull(ss, mm, vs, vm);
  int *bvs = (int *)_mm_malloc(16*nth*sizeof(int), 64);
  int *bvm = (int *)_mm_malloc(16*nth*sizeof(int), 64);

  int nsmall = 16;
  int smallblock = 512;
  int *idx = (int *)_mm_malloc(16*sizeof(int), 64);
  for(int i=0;i<16;i++) {
    if(i<nsmall)idx[i] = i*smallblock;
    else idx[i] = 0;
  }
  vint vidx;
  vidx.load(idx);

  gettimeofday(&tv1, &tz1);
  for(int tid=0; tid<nth; tid++) {
    workers[tid] = thread([=]{
                          
                          vint vecs, vecss;
                          vint vecm, vecmm;
                          vecss.load(vs);
                          vecmm.load(vm);
                          int cur = 0;
                          while(cur+16*smallblock<=block) {
                          vecs.load(vs);
                          vecm.load(vm);
                          for(int j=tid*block+cur; j<tid*block+cur+smallblock; j++) {
                          vint va;
                          va.load(A, vidx+j, 4);
                          vecs += va;
                          vecm = max(vecm, vecs);
                          }
                          vecs.store(bvs+tid*16);
                          vecm.store(bvm+tid*16);
                          for(int i=0;i<16;i++) {
                          vecmm = max(vecmm, bvm[tid*16+i] + vecss);
                          vecss = bvs[tid*16+i] + vecss;
                          }
                          cur += 16*smallblock;
                          }
                          vecss.store(bvs+tid*16);
                          vecmm.store(bvm+tid*16);
                          });
  }

  for(int tid=0; tid<nth; tid++) {
    workers[tid].join();
    m = Rectf_max(bvm[tid*16] + s, m);
    s = bvs[tid*16] + s;
  }

  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << m << " " << s << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "simd+mimd speed up: " << 1.0 * serial_time / par_time << endl;
  fout <<  1.0 * serial_time / par_time << "\t";


}


void parsynt(int s, int m, int *A, int nth) {
  int block = N / nth;
  if(N%nth)block+=1;
  thread *workers = new thread[nth];
  int *bvs = (int *)_mm_malloc(nth*sizeof(int), 64);
  int *bvm = (int *)_mm_malloc(nth*sizeof(int), 64);

  gettimeofday(&tv1, &tz1);
  for(int tid=0; tid<nth; tid++) {
    workers[tid] = thread([=]{
                            int s = 0;
                            int m = INT_TINY;

                          for(int j=tid*block; j<(tid+1)*block; j++) {
                          m = (s + A[j] > m)? s + A[j] : m;
                          s += A[j];
                          }
                          bvs[tid] = s;
                          bvm[tid] = m;
                          });
  }

  for(int tid=0; tid<nth; tid++) {
    workers[tid].join();
    m = (s + bvm[tid] > m)? s + bvm[tid] : m;
    s = bvs[tid] + s;
  }

  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << m << " " << s << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "parsynt speed up: " << 1.0 * serial_time / par_time << endl;
  fout <<  1.0 * serial_time / par_time << "\t";

}



void parallel(int s, int m, int *A, int nth) {
  int block = N / nth;
  if(N%nth)block+=1;
  thread *workers = new thread[nth];
  int *bvs = (int *)_mm_malloc(nth*sizeof(int), 64);
  int *bvm = (int *)_mm_malloc(nth*sizeof(int), 64);

  gettimeofday(&tv1, &tz1);
  for(int tid=0; tid<nth; tid++) {
    workers[tid] = thread([=]{
                            int s = 0;
                            int m = INT_TINY;

                          for(int j=tid*block; j<(tid+1)*block; j++) {
                          s += A[j];
                          m = m > s?m:s;
                          }
                          bvs[tid] = s;
                          bvm[tid] = m;
                          });
  }

  for(int tid=0; tid<nth; tid++) {
    workers[tid].join();
    m = bvm[tid]+s> m?bvm[tid]+s:m;
    s = bvs[tid] + s;
  }

  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << m << " " << s << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "speed up: " << 1.0 * serial_time / par_time << endl;
  fout <<  1.0 * serial_time / par_time << "\t";

}

int main()
{
  srand(time(0));


  int s = 0;
  int m = INT_TINY;

  int *A = new int[N];

  for(int i=0; i<N; i++) {
    A[i] = rand() % 100 - 50;
  }
  serial(s, m, A);
  fout << "mps-our-nonvec\t";
  for(int nth=1; nth<=64;nth*=2) {
    parallel(s, m, A, nth);
  }
  fout << endl;
  fout << "mps-our-vec2\t";
  for(int nth=1; nth<=64;nth*=2) {
    simd_mimd(s, m, A, nth);
  }
  fout << endl;

  fout << "mps-parsynt\t";
  
  for(int nth=1; nth<=64;nth*=2) {

    parsynt(s, m, A, nth);
  }
  
  fout << endl;

  return 0;
}

