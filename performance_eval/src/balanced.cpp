// Balanced
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
ofstream fout1("fullspeedups.txt", fstream::out|fstream::app);

void serial(int bal, int ofs, int *A) {
  gettimeofday(&tv1, &tz1); 
  for(int i=0; i<N; i++) {
    if(A[i] == 0) {
        ofs = ofs + 1;
    } else {
        ofs = ofs - 1;
    }
    bal = Rectf_min(bal, (ofs>=0?(ofs+1):0));
  }
  gettimeofday(&tv2, &tz2);
  cout << "serial res: " << bal << " " << ofs << endl;
  serial_time = 1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "serial time: " << serial_time<< endl;
}

void simd_mimd(int bal, int ofs, int *A, int nth) {
  int block = N / nth;
  if(N%nth)block+=1;
  thread *workers = new thread[nth];
  vector<int> balbal = {INT_BIG+1};
  vector<int> ofsofs = {INT_BIG};
  int *vbal = (int *)_mm_malloc(16*nth*sizeof(int), 64);
  int *vofs = (int *)_mm_malloc(16*nth*sizeof(int), 64);
  MeshGridFull(balbal, ofsofs, vbal, vofs);
  int *bvbal = (int *)_mm_malloc(16*nth*sizeof(int), 64);
  int *bvofs = (int *)_mm_malloc(16*nth*sizeof(int), 64);

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
                          vint vecbal, vecofs, vecbalbal, vecofsofs;
                          vecofsofs.load(vofs);
                          vecbalbal.load(vbal);
                          int cur = 0;
                          while(cur+16*smallblock<=block) {
                          vecbal.load(vbal);
                          vecofs.load(vofs);
                          for(int j=tid*block+cur; j<tid*block+cur+smallblock; j++) {
                            vint va;
                            va.load(A, vidx+j, 4);
                            vint tmp;
                            tmp = mask_mov(va==0, 1, -1);
                            vecofs += tmp;

                            vecbal = Rectf_vmin(vecbal, mask_mov(vecofs>=0, vecofs+1, 0));
                          }
                          vecbal.store(bvbal+tid*16);
                          vecofs.store(bvofs+tid*16);
                          for(int i=0;i<16;i++) {
                          vecbalbal = Rectf_vmax(bvbal[tid*16+i]-INT_BIG+vecofsofs, 0);
                          vecofsofs = bvofs[tid*16+i] - INT_BIG + vecofsofs;
                          
                          }
                          cur += 16*smallblock;
                          }
                          vecbalbal.store(bvbal+tid*16);
                          vecofsofs.store(bvofs+tid*16);
    });
  }

  for(int tid=0; tid<nth; tid++) {
    workers[tid].join();
    bal = Rectf_max(bvbal[tid*16]-INT_BIG+ofs, 0);
    ofs = bvofs[tid*16] - INT_BIG + ofs;
  }

  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << bal << " " << ofs << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "speed up: " << 1.0 * serial_time / par_time << endl;
  fout1 <<  1.0 * serial_time / par_time << "\t";

}



void parallel(int bal, int ofs, int *A, int nth) {
  int block = N / nth;
  if(N%nth)block+=1;
  thread *workers = new thread[nth];
  int *bvbal = (int *)_mm_malloc(16*nth*sizeof(int), 64);
  int *bvofs = (int *)_mm_malloc(16*nth*sizeof(int), 64);

  gettimeofday(&tv1, &tz1);
  for(int tid=0; tid<nth; tid++) {
    workers[tid] = thread([=]{
                          int bal = INT_BIG+1;
                          int ofs = INT_BIG;
                          for(int j=tid*block; j<((tid+1)*block<N?(tid+1)*block:N); j++) {
                            if(A[j] == 0) {
                                ofs += 1;
                            } else {
                                ofs -= 1;
                            }
                            bal = Rectf_min(bal, (ofs>=0?(ofs+1):0));
                          }
                          bvbal[tid*16] = bal;
                          bvofs[tid*16] = ofs;
    });
  }

  for(int tid=0; tid<nth; tid++) {
    workers[tid].join();
    bal = Rectf_max(bvbal[tid*16]-INT_BIG+ofs, 0);
    ofs = bvofs[tid*16] - INT_BIG + ofs;
  }

  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << bal << " " << ofs << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "speed up: " << 1.0 * serial_time / par_time << endl;
  fout <<  1.0 * serial_time / par_time << "\t";

}


void parsynt(int bal, int ofs, int *A, int nth) {
  int block = N / nth;
  if(N%nth)block+=1;
  thread *workers = new thread[nth];
  int *bvbal = (int *)_mm_malloc(nth*sizeof(int), 64);
  int *bvofs = (int *)_mm_malloc(nth*sizeof(int), 64);
  int *bvaux = (int *)_mm_malloc(nth*sizeof(int), 64);
  int aux = INT_BIG;

  gettimeofday(&tv1, &tz1);
  for(int tid=0; tid<nth; tid++) {
    workers[tid] = thread([=]{
                          int bal = 1;
                          int ofs = 0;
                          int aux = INT_BIG;
                          for(int j=tid*block; j<((tid+1)*block<N?(tid+1)*block:N); j++) {
                          ofs = (ofs + (A[j] ? 1 : (-1)));
                          bal = (bal * ((ofs + (A[j] ? 1 : (-1))) >= 0));
                          aux = ((ofs < aux) ? ofs : aux);

                          }
                          bvbal[tid] = bal;
                          bvofs[tid] = ofs;
                          bvaux[tid] = aux;
                          });
  }

  for(int tid=0; tid<nth; tid++) {
    workers[tid].join();
    int ofs_t = (bvofs[tid] + ofs);
    int bal_t = ((((bvaux[tid] - 2) + ofs) > (-3)) * bal);
    int aux_t = (((bvaux[tid] + ofs) < aux) ? (bvaux[tid] + ofs) :
            aux);
    bal = bal_t;
    ofs = ofs_t;
    aux = aux_t;
  }

  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << bal << " " << ofs << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "parsynt speed up: " << 1.0 * serial_time / par_time << endl;
  fout <<  1.0 * serial_time / par_time << "\t";

}




int main()
{
  srand(time(0));


  int bal = 1;
  int ofs = 0;

  int *A = new int[N];

  for(int i=0; i<N; i++) {
    A[i] = rand() % 2;
  }
  serial(bal, ofs, A);
  fout << "balanced-our-nonvec\t";
  for(int nth=1; nth<=64;nth*=2) {
    parallel(bal, ofs, A, nth);
  }
  fout << endl;

  fout << "balanced-our-vec2\t";
  for(int nth=1; nth<=64;nth*=2) {
    simd_mimd(bal, ofs, A, nth);
  }
  fout << endl;

  fout << "balanced-parsynt\t";
  for(int nth=1; nth<=64;nth*=2) {
    parsynt(bal, ofs, A, nth);
  }
  fout << endl;
 
  return 0;
}

