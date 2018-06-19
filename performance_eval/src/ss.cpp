// Second Smallest
//
#include <iostream>
#include <limits.h>
#include <stdlib.h>
#include <thread>
#include <sys/time.h>
#include <vector>
#include <fstream>
#include "immintrin.h"
#include "interpolate.h"
using namespace std;


#define N (1<<29)
int serial_time, par_time;
struct timeval tv1, tv2;
struct timezone tz1, tz2;
ofstream fout("speedups.txt", fstream::out|fstream::app);
ofstream fout1("fullspeedups.txt", fstream::out|fstream::app);


void serial(float m, float m2, float *A) {
  gettimeofday(&tv1, &tz1);
  for(int i=0; i<N; i++) {

      m2 = ((m2 < ((m > A[i]) ? m : A[i])) ? m2 : ((m > A[i]) ? m : A[i]));
      m = ((m < A[i]) ? m : A[i]);
  }
  gettimeofday(&tv2, &tz2);

  cout << "serial res: " << m2 << endl;
  serial_time = 1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "serial time: " << serial_time<< endl;

}

void simd_mimd(float m, float m2, float *A, int nth) {

  int block = N / nth;
  if(N%nth)block+=1;
  thread *workers = new thread[nth];
  vector<float> mm = {FLT_HUGE}; 
  vector<float> m2m2 = {FLT_HUGE}; 
  float *vm = (float *)_mm_malloc(16*sizeof(float), 64);
  float *vm2 = (float *)_mm_malloc(16*sizeof(float), 64);
  MeshGridFull(mm, m2m2, vm, vm2);
  float *bvm = (float *)_mm_malloc(16*nth*sizeof(float), 64);
  float *bvm2 = (float *)_mm_malloc(16*nth*sizeof(float), 64);

  int nsmall = 16;
  int smallblock = 512;
  int *idx = (int *)_mm_malloc(16*sizeof(int), 64);
  for(int i=0;i<16;i++) {
    idx[i] = i*smallblock;
  }
  vint vidx;
  vidx.load(idx);





  gettimeofday(&tv1, &tz1);
  for(int tid=0; tid<nth; tid++) {
    workers[tid] = thread([=]{
                          vfloat vecm, vecmm;
                          vfloat vecm2, vecm2m2;
                          vecmm.load(vm);
                          vecm2m2.load(vm2);
                          int cur = 0;
                          while(cur+16*smallblock<=block) {
                          vecm.load(vm);
                          vecm2.load(vm2);
                          for(int j=tid*block+cur; j<tid*block+cur+smallblock; j++) {
                          vfloat va;
                          va.load(A, vidx+j, 4);
                          vecm2 = min(vecm2, max(vecm, va));
                          vecm = min(vecm, va);
                          }
                          vecm.store(bvm+tid*16);
                          vecm2.store(bvm2+tid*16);
                          for(int i=0;i<16;i++) {
                          vfloat vecmm_next = Rectf_vmin(bvm[tid*16+i], vecmm);
                          mask mas = vecmm > bvm[tid*16+i];
                          vfloat tmp2 = Rectf_vmin(bvm[tid*16+i], vecm2m2);
                          Mask::set_mask(mas, tmp2);
                          vfloat tmp1 = Rectf_vmin(bvm2[tid*16+i], vecmm);
                          vecm2m2.mask() = tmp1.mask();
                          vecmm = vecmm_next;
                          }
                          cur += 16*smallblock;
                          }
                          vecmm.store(bvm+tid*16);
                          vecm2m2.store(bvm2+tid*16);
                          });
  }
 for(int tid=0; tid<nth; tid++) {
    workers[tid].join();
    float next_m = Rectf_min(bvm[tid*16], m);
    if(m>bvm[tid*16]) {
      m2 = Rectf_min(bvm2[tid*16], m);
    } else {
      float tm1 = bvm[tid*16];
      m2 = Rectf_min(tm1, m2);
    }
    m = next_m;
  }
  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << m2 << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "simd+mimd speed up: " << 1.0 * serial_time / par_time << endl;
  fout1 <<  1.0 * serial_time / par_time << "\t";
}



void parallel(float m, float m2, float *A, int nth) {

  int block = N / nth;
  if(N%nth)block+=1;
  thread *workers = new thread[nth];
  vector<float> mm = {FLT_HUGE}; 
  vector<float> m2m2 = {FLT_HUGE}; 
  float *vm = (float *)_mm_malloc(16*sizeof(float), 64);
  float *vm2 = (float *)_mm_malloc(16*sizeof(float), 64);
  MeshGrid(mm, m2m2, vm, vm2);
  float *bvm = (float *)_mm_malloc(16*nth*sizeof(float), 64);
  float *bvm2 = (float *)_mm_malloc(16*nth*sizeof(float), 64);


  gettimeofday(&tv1, &tz1);
  for(int tid=0; tid<nth; tid++) {
    workers[tid] = thread([=]{
                          float m = FLT_HUGE;
                          float m2 = FLT_HUGE;
                          for(int j=tid*block; j<((tid+1)*block<N?(tid+1)*block:N); j++) {

                          m2 = ((m2 < ((m > A[j]) ? m : A[j])) ? m2 : ((m > A[j]) ? m : A[j]));
                          m = ((m < A[j]) ? m : A[j]);



                          }
                          bvm[tid*16] = m;
                          bvm2[tid*16] = m2;
                          });
  }

  for(int tid=0; tid<nth; tid++) {
    workers[tid].join();
    float next_m = min(bvm[tid*16], m);
    if(m>bvm[tid*16]) {
      m2 = min(bvm2[tid*16], m);
    } else {
      float tm1 = bvm[tid*16];
      m2 = min(tm1, m2);
    }
    m = next_m;
  }

  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << m2 << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "speed up: " << 1.0 * serial_time / par_time << endl;
  fout <<  1.0 * serial_time / par_time << "\t";
}

void parsynt(float m, float m2, float *A, int nth) {

  int block = N / nth;
  if(N%nth)block+=1;
  thread *workers = new thread[nth];
  float *vm = (float *)_mm_malloc(nth*sizeof(float), 64);
  float *vm2 = (float *)_mm_malloc(nth*sizeof(float), 64);


  gettimeofday(&tv1, &tz1);
  for(int tid=0; tid<nth; tid++) {
    workers[tid] = thread([=]{
                          float m = FLT_HUGE;
                          float m2 = FLT_HUGE;
                          for(int j=tid*block; j<((tid+1)*block<N?(tid+1)*block:N); j++) {
                          m2 = ((m2 < ((m > A[j]) ? m : A[j])) ? m2 : ((m > A[j]) ? m : A[j]));
                          m = ((m < A[j]) ? m : A[j]);
                          }
                          vm[tid] = m;
                          vm2[tid] = m2;
                          });
  }

  for(int tid=0; tid<nth; tid++) {
    workers[tid].join();
    int m2_t = (((((vm[tid] + ((0 < 0) ? 0 : 0)) > m) ?
                    (vm[tid] + ((0 < 0) ? 0 : 0)) : m) < ((vm2[tid] < m2) ?
                        vm2[tid] : m2)) ?
            (((vm[tid] + ((0 < 0) ? 0 : 0)) > m) ?
             (vm[tid] + ((0 < 0) ? 0 : 0)) : m) :
            ((vm2[tid] < m2) ? vm2[tid] : m2));
    int m_t = ((vm[tid] < m) ? vm[tid] : m);
    m = m_t;
    m2 = m2_t;



  }

  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << m2 << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parsynt parallel time: " << par_time << endl;
  cout << "speed up: " << 1.0 * serial_time / par_time << endl;
  fout <<  1.0 * serial_time / par_time << "\t";
}



int main(int argc, char *argv[])
{
    srand(time(0));
    struct timeval tv1, tv2;
    struct timezone tz1, tz2;

    float m = FLT_HUGE;
    float m2 = FLT_HUGE;

    float *A = new float[N];

    for(int i=0; i<N; i++) {
        A[i] = rand();
    }
    serial(m, m2, A);
    fout << "ss_parallel\t";
    for(int nth=1; nth<=64;nth*=2) {
        parallel(m, m2, A, nth);
    }
    fout << endl;

    fout1 << "ss_simd+mimd\t";
    for(int nth=1; nth<=64;nth*=2) {
        simd_mimd(m, m2, A, nth);
    }
    fout1 << endl;

    fout << "ss_parsynt\t";
    for(int nth=1; nth<=64;nth*=2) {
        parsynt(m, m2, A, nth);
    }
    fout << endl;


    return 0;
}

