// Max Tail Sum
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
ofstream fout1("fullspeedups.txt", fstream::out|fstream::app);

void serial(int s, int p, int *A) {
  gettimeofday(&tv1, &tz1);
  for(int i=0; i<N; i++) {
    if(s+A[i]>0) {
      s += A[i];
    } else {
      s = 0;
      p = i + 1;
    }
  }
  gettimeofday(&tv2, &tz2);

  cout << "serial res: " << s << " " << p << endl;
  serial_time = 1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "serial time: " << serial_time<< endl;
}

void simd_mimd(int s, int p, int *A, int nth) {

  int block = N / nth;
  if(N%nth)block+=1;
  thread *workers = new thread[nth];
  vector<int> ss = {INT_TINY, INT_HUGE};
  vector<int> pp = {0}; 
  int *vs = (int *)_mm_malloc(16*sizeof(int), 64);
  int *vp = (int *)_mm_malloc(16*sizeof(int), 64);
  MeshGridFull(ss, pp, vs, vp);
  int *bvs = (int *)_mm_malloc(16*nth*sizeof(int), 64);
  int *bvp = (int *)_mm_malloc(16*nth*sizeof(int), 64);

  int nsmall = 8;
  int smallblock = 1024;
  int *idx = (int *)_mm_malloc(16*sizeof(int), 64);
  for(int i=0;i<8;i++) {
    idx[2*i] = idx[2*i+1] = i*smallblock;
  }
  vint vidx;
  vidx.load(idx);



  gettimeofday(&tv1, &tz1);
  for(int tid=0; tid<nth; tid++) {
    workers[tid] = thread([=]{
                          vint vecs, vecss;
                          vint vecp, vecpp;
                          vecss = _mm512_load_epi32(vs);
                          vecpp = _mm512_load_epi32(vp);
                          int cur = 0;
                          while(cur+8*smallblock<=block) {
                          vecs.load(vs);
                          vecp.load(vp);

                          for(int j=tid*block+cur; j<tid*block+cur+smallblock; j+=2) {
                          vint va;
                          vint viidx = vidx + j;
                          va.load(A, viidx, 4);
                          vecs += va;
                          mask mas = vecs <= 0;
                          Mask::set_mask(mas, vecs);
                          vecs.mask() = 0;
                          Mask::set_mask(mas, vecp);
                          viidx+=1;
                          vecp.mask() = viidx.mask();
                          vint va1;
                          va1.load(A, viidx, 4);
                          vecs += va1;
                          mas = vecs <= 0;
                          Mask::set_mask(mas, vecs);
                          vecs.mask() = 0;
                          Mask::set_mask(mas, vecp);
                          viidx+=1;
                          vecp.mask() = viidx.mask();


			  }
                          vecs.store(bvs+tid*16);
                          vecp.store(bvp+tid*16);

                          for(int i=0; i<8; i++) {
                          mask mas1 = (vecss+(bvs[tid*16+1+2*i]-vs[1]) < bvs[tid*16+2*i]);
                          Mask::set_mask(mas1, vecpp);
                          vecpp.mask() =  bvp[tid*16+2*i];
                          vecss = Rectf_vmax2(bvs[tid*16+2*i], vs[1], bvs[tid*16+2*i+1], vecss);
                          }


                          cur += 8*smallblock;

                          } 

                          vecss.store(bvs+tid*16);
                          vecpp.store(bvp+tid*16);
    });
  }

  for(int tid=0; tid<nth; tid++) {
    workers[tid].join();
    if(s+bvs[tid*16+1]-vs[1] <  bvs[tid*16]) p = bvp[tid*16];
    s = Rectf_max2(bvs[tid*16], vs[1], bvs[tid*16+1], s);
  }

  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << s << " " << p << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "simd+mimd speed up: " << 1.0 * serial_time / par_time << endl;
  fout1 <<  1.0 * serial_time / par_time << "\t";
}

void parsynt(int s, int p, int *A, int nth) {

  int block = N / nth;
  if(N%nth)block+=1;
  thread *workers = new thread[nth];
  int *vs = (int *)_mm_malloc(nth*sizeof(int), 64);
  int *vp = (int *)_mm_malloc(nth*sizeof(int), 64);
  int *vaux = (int *)_mm_malloc(nth*sizeof(int), 64);
  int aux = 0;

  gettimeofday(&tv1, &tz1);
  for(int tid=0; tid<nth; tid++) {
    workers[tid] = thread([=]{
                          int s = 0;
                          int p = 0;
                          int aux = 0;
                          for(int j=tid*block; j<((tid+1)*block<N?(tid+1)*block:N); j++) {
                            aux += A[j];
                            s = 0 > s + A[j] ? 0 : s + A[j];
                            p = s + A[j] < 0 ? j : p;
                           } 
                           vs[tid] = s;
                           vp[tid] = p;
                           vaux[tid] = aux;
    });
  }

  for(int tid=0; tid<nth; tid++) {
    workers[tid].join();
    int aux_t = aux + vaux[tid];
   int mts = ((((s - 1) + (vaux[tid] + 1)) > vs[tid]) ?
            ((s - 1) + (vaux[tid] + 1)) : vs[tid]);
   int  pos = ((((vs[tid] - s) + vp[tid]) > (vaux[tid] + vp[tid])) ?
            vp[tid] : p);

    p = pos;
    s = mts;
    aux = aux_t;

  }

  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << s << " " << p << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "speed up: " << 1.0 * serial_time / par_time << endl;
  fout <<  1.0 * serial_time / par_time << "\t";
}


void parallel_nonvec(int s, int p, int *A, int nth) {

  int block = N / nth;
  if(N%nth)block+=1;
  thread *workers = new thread[nth];
  vector<int> ss = {INT_TINY, INT_HUGE};
  vector<int> pp = {0}; 
  int *vs = (int *)_mm_malloc(16*sizeof(int), 64);
  int *vp = (int *)_mm_malloc(16*sizeof(int), 64);
  MeshGrid(ss, pp, vs, vp);
  int *bvs = (int *)_mm_malloc(16*nth*sizeof(int), 64);
  int *bvp = (int *)_mm_malloc(16*nth*sizeof(int), 64);

  gettimeofday(&tv1, &tz1);
  for(int tid=0; tid<nth; tid++) {
    workers[tid] = thread([=]{
    int Vs[2] = {INT_TINY, INT_HUGE};
      int Vp[2] = {0,0};
                          for(int j=tid*block; j<((tid+1)*block<N?(tid+1)*block:N); j++) {
                          for (int _sampl_i = 0; _sampl_i < 2; _sampl_i++) {
                          if (Vs[_sampl_i] + A[j] > 0) {
                          Vs[_sampl_i] += A[j];
                          } else {
                          Vs[_sampl_i] = 0;
                          Vp[_sampl_i] = j + 1;
                          }
                          } // sampling
                          } 
                          bvs[tid*16] = Vs[0];
                          bvs[tid*16+1] = Vs[1];
                          bvp[tid*16] = Vp[0];
                          bvp[tid*16+1] = Vp[1];
                          });
  }

  for(int tid=0; tid<nth; tid++) {
      workers[tid].join();
      if(s+bvs[tid*16+1]-vs[1] < bvs[tid*16]) p = bvp[tid*16];
      s = Rectf_max2(bvs[tid*16], vs[1], bvs[tid*16+1], s);
  }

  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << s << " " << p << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "speed up: " << 1.0 * serial_time / par_time << endl;
  fout <<  1.0 * serial_time / par_time << "\t";
}



void parallel(int s, int p, int *A, int nth) {

  int block = N / nth;
  if(N%nth)block+=1;
  thread *workers = new thread[nth];
  vector<int> ss = {INT_TINY, INT_HUGE};
  vector<int> pp = {0}; 
  int *vs = (int *)_mm_malloc(16*sizeof(int), 64);
  int *vp = (int *)_mm_malloc(16*sizeof(int), 64);
  MeshGrid(ss, pp, vs, vp);
  int *bvs = (int *)_mm_malloc(16*nth*sizeof(int), 64);
  int *bvp = (int *)_mm_malloc(16*nth*sizeof(int), 64);

  gettimeofday(&tv1, &tz1);
  for(int tid=0; tid<nth; tid++) {
    workers[tid] = thread([=]{
                          __m512i vecs;
                          __m512i vecp;
                          vecs = _mm512_load_epi32(vs);
                          vecp = _mm512_load_epi32(vp);
                          for(int j=tid*block; j<((tid+1)*block<N?(tid+1)*block:N); j+=2) {
                          vecs = _mm512_add_epi32(vecs, _mm512_set1_epi32(A[j]));
                          __mmask16 mas = _mm512_cmpgt_epi32_mask(vecs, _mm512_set1_epi32(0));
                          if(mas!=0xFFFF) {
                          vecs = _mm512_mask_mov_epi32(_mm512_set1_epi32(0), mas, vecs);
			  vecp = _mm512_mask_mov_epi32(_mm512_set1_epi32(j+1), mas, vecp);
			  }
			  vecs = _mm512_add_epi32(vecs, _mm512_set1_epi32(A[j+1]));
                          __mmask16 mas1 = _mm512_cmpgt_epi32_mask(vecs, _mm512_set1_epi32(0));
                          if(mas1!=0xFFFF) {
                          vecs = _mm512_mask_mov_epi32(_mm512_set1_epi32(0), mas1, vecs);
			  vecp = _mm512_mask_mov_epi32(_mm512_set1_epi32(j+2), mas1, vecp);
			  }

                          } 
                          _mm512_store_epi32(bvs+tid*16, vecs);
                          _mm512_store_epi32(bvp+tid*16, vecp);
    });
  }

  for(int tid=0; tid<nth; tid++) {
    workers[tid].join();
    if(s+bvs[tid*16+1]-vs[1] < bvs[tid*16]) p = bvp[tid*16];
    s = Rectf_max2(bvs[tid*16], vs[1], bvs[tid*16+1], s);
  }

  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << s << " " << p << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "speed up: " << 1.0 * serial_time / par_time << endl;
  fout <<  1.0 * serial_time / par_time << "\t";
}

int main(int argc, char *argv[])
{
  srand(time(0));
  struct timeval tv1, tv2;
  struct timezone tz1, tz2;

  int s = 0;
  int p = 0;

  int *A = new int[N];

  for(int i=0; i<N; i++) {
    A[i] = rand() % 100 - 50;
  }
  fout << "mts_parallel_non-vec\t";
  serial(s, p, A);
  for(int nth=1;nth<=64;nth*=2) {
    parallel_nonvec(s, p, A, nth);
  }
  fout << endl;


  fout << "mts_parallel\t";
  serial(s, p, A);
  for(int nth=1;nth<=64;nth*=2) {
    parallel(s, p, A, nth);
  }
  fout << endl;

    fout1 << "mts_simd+mimd\t";
  for(int nth=1;nth<=64;nth*=2) {
    simd_mimd(s, p, A, nth);
  }
  fout1 << endl;
  fout << "mts_parsynt\t";
  
  for(int nth=1; nth<=64;nth*=2) {

    parsynt(s, p, A, nth);
  }
  
  fout << endl;


  return 0;
}
  

