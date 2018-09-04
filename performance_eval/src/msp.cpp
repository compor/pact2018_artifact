// Max Prefix Sum
//
#include <iostream>
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

void serial(float ma, float mi, float m, float *A) {
  gettimeofday(&tv1, &tz1);
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
  gettimeofday(&tv2, &tz2);

  cout << "serial res: " << m << " " << ma << " " << mi << endl;
  serial_time = 1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "serial time: " << serial_time<< endl;
}

void parallel(float ma, float mi, float m, float *A, int nth) {

  int block = N / nth;
  if(N%nth)block+=1;
  thread *workers = new thread[nth];
  vector<float> mama = {FLT_TINY, FLT_SMALL, FLT_BIG, FLT_HUGE}; 
  vector<float> mimi = {FLT_TINY, FLT_SMALL, FLT_BIG, FLT_HUGE}; 
  vector<float> mm = {FLT_TINY}; 
  float *vma = (float *)_mm_malloc(16*sizeof(float), 64);
  float *vmi = (float *)_mm_malloc(16*sizeof(float), 64);
  float *vm = (float *)_mm_malloc(16*sizeof(float), 64);
  MeshGrid(mama, mimi, mm, vma, vmi, vm);
  float *bvma = (float *)_mm_malloc(16*nth*sizeof(float), 64);
  float *bvmi = (float *)_mm_malloc(16*nth*sizeof(float), 64);
  float *bvm = (float *)_mm_malloc(16*nth*sizeof(float), 64);


  gettimeofday(&tv1, &tz1);
  for(int tid=0; tid<nth; tid++) {
    workers[tid] = thread([=]{
                          __m512 vecma;
                          __m512 vecmi;
                          __m512 vecm;
                          vecma = _mm512_load_ps(vma);
                          vecmi = _mm512_load_ps(vmi);
                          vecm = _mm512_load_ps(vm);
                          for(int j=tid*block; j<((tid+1)*block<N?(tid+1)*block:N); j++) {
                          if(A[j]>=0) {
                           vecma = _mm512_max_ps(_mm512_mul_ps(vecma, _mm512_set1_ps(A[j])), _mm512_set1_ps(A[j]));
                           vecmi = _mm512_min_ps(_mm512_mul_ps(vecmi, _mm512_set1_ps(A[j])), _mm512_set1_ps(A[j]));
                          } else {
                          
                           __m512 vectmp = _mm512_max_ps(_mm512_mul_ps(vecmi, _mm512_set1_ps(A[j])), _mm512_set1_ps(A[j]));
                           vecmi = _mm512_min_ps(_mm512_mul_ps(vecma, _mm512_set1_ps(A[j])), _mm512_set1_ps(A[j]));
                           vecma = vectmp;
                          }
                          vecm = _mm512_max_ps(vecm, vecma);
                          }
                          _mm512_store_ps(bvma+tid*16, vecma);
                          _mm512_store_ps(bvmi+tid*16, vecmi);
                          _mm512_store_ps(bvm+tid*16, vecm);
    });
  }

  for(int tid=0; tid<nth; tid++) {
    workers[tid].join();
    float ma1 = Rectf(vmi[0], bvma[tid*16], vmi[4], bvma[tid*16+4], vmi[8], bvma[tid*16+8],  vmi[12], bvma[tid*16+12], mi);
    float ma2 = Rectf(vmi[0], bvma[tid*16+1], vmi[4], bvma[tid*16+5], vmi[8], bvma[tid*16+9],  vmi[12], bvma[tid*16+13], mi);
    float ma3 = Rectf(vmi[0], bvma[tid*16+2], vmi[4], bvma[tid*16+6], vmi[8], bvma[tid*16+10],  vmi[12], bvma[tid*16+14], mi);
    float ma4 = Rectf(vmi[0], bvma[tid*16+3], vmi[4], bvma[tid*16+7], vmi[8], bvma[tid*16+11],  vmi[12], bvma[tid*16+15], mi);
    float ma_next = Rectf(vma[0], ma1, vma[1], ma2, vma[2], ma3, vma[3], ma4, ma);

    float mi1 = Rectf(vma[0], bvmi[tid*16], vma[1], bvmi[tid*16+1], vmi[2], bvmi[tid*16+2],  vma[3], bvmi[tid*16+3], ma);
    float mi2 = Rectf(vma[0], bvmi[tid*16+4], vma[1], bvmi[tid*16+5], vmi[2], bvmi[tid*16+6],  vma[3], bvmi[tid*16+7], ma);
    float mi3 = Rectf(vma[0], bvmi[tid*16+8], vma[1], bvmi[tid*16+9], vmi[2], bvmi[tid*16+10],  vma[3], bvmi[tid*16+11], ma);
    float mi4 = Rectf(vma[0], bvmi[tid*16+12], vma[1], bvmi[tid*16+13], vmi[2], bvmi[tid*16+14],  vma[3], bvmi[tid*16+15], ma);
    float mi_next = Rectf(vmi[0], mi1, vmi[4], mi2, vmi[8], mi3, vmi[12], mi4, mi);

    float mma1 = Rectf(vmi[0], bvm[tid*16], vmi[4], bvm[tid*16+4], vmi[8], bvm[tid*16+8],  vmi[12], bvm[tid*16+12], mi);
    float mma2 = Rectf(vmi[0], bvm[tid*16+1], vmi[4], bvm[tid*16+5], vmi[8], bvm[tid*16+9],  vmi[12], bvm[tid*16+13], mi);
    float mma3 = Rectf(vmi[0], bvm[tid*16+2], vmi[4], bvm[tid*16+6], vmi[8], bvm[tid*16+10],  vmi[12], bvm[tid*16+14], mi);
    float mma4 = Rectf(vmi[0], bvm[tid*16+3], vmi[4], bvma[tid*16+7], vmi[8], bvm[tid*16+11],  vmi[12], bvm[tid*16+15], mi);
    float mma = Rectf(vma[0], mma1, vma[1], mma2, vma[2], mma3, vma[3], mma4, ma);

    m = Rectf_max(m, mma);
    ma = ma_next;
    mi = mi_next;
  }


  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << m << " " << ma << " " << mi << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "speed up: " << 1.0 * serial_time / par_time << endl;
  fout <<  1.0 * serial_time / par_time << "\t";
}

int main(int argc, char *argv[])
{
  srand(time(0));

  float ma = 1, mi = 1;
  float m = FLT_TINY;

  float *A = new float[N];

  for(int i=0; i<N; i++) {
    A[i] = 2.0 * rand() / INT_MAX - 1;
  }
  serial(ma, mi, m, A);
  fout << "msp-our-vec1\t";
  for(int nth=1; nth<=64;nth*=2) {
    parallel(ma, mi, m, A, nth);
  }
  fout << endl;
  return 0;
}

