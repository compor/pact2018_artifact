// Max Segmented Sum
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

void serial(int s, int m, int *A) {
  gettimeofday(&tv1, &tz1);
  for(int i=0; i<N; i++) {
    if(s < 0) {
      s = A[i];
    } else {
      s += A[i];
    }
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
  vector<int> ss = {INT_TINY, INT_BIG}; 
  vector<int> mm = {INT_TINY}; 
  int *vs = (int *)_mm_malloc(16*sizeof(int), 64);
  int *vm = (int *)_mm_malloc(16*sizeof(int), 64);
  MeshGridFull(ss, mm, vs, vm);
  int *bvs = (int *)_mm_malloc(16*nth*sizeof(int), 64);
  int *bvm = (int *)_mm_malloc(16*nth*sizeof(int), 64);

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
                          vint vecm, vecmm;
                          vecss = _mm512_load_epi32(vs);
                          vecmm = _mm512_load_epi32(vm);
                          int cur = 0;
                          while(cur+8*smallblock<=block) {
                          vecs.load(vs);
                          vecm.load(vm);
                          for(int j=tid*block+cur; j<tid*block+cur+smallblock; j++) {
                          vint va;
                          va.load(A, vidx+j, 4);
                          vecs = max(va, vecs+va);
                          vecm = max(vecm, vecs);
                          }
                          vecs.store(bvs+tid*16);
                          vecm.store(bvm+tid*16);

                          for(int i=0;i<8;i++) {
                          vecmm = Rectf_vmax(Rectf_vmax2(bvm[tid*16+2*i], vs[1], bvm[tid*16+2*i+1], vecss), vecmm);
                          vecss = Rectf_vmax2(bvs[tid*16+2*i], vs[1], bvs[tid*16+2*i+1], vecss);

                          }
                          cur += 8*smallblock;
                          }
                          vecss.store(bvs+tid*16);
                          vecmm.store(bvm+tid*16);
                          
    });
  }

  for(int tid=0; tid<nth; tid++) {
    workers[tid].join();
    m = Rectf_max(Rectf_max2(bvm[tid*16+0], vs[1], bvm[tid*16+1], s), m);
    s = Rectf_max2(bvs[tid*16], vs[1], bvs[tid*16+1], s);
  }

  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << m << " " << s << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "simd+mimd speed up: " << 1.0 * serial_time / par_time << endl;
  fout1 <<  1.0 * serial_time / par_time << "\t";
}
void parallel_nonvec(int s, int m, int *A, int nth) {

  int block = N / nth;
  if(N%nth)block+=1;
  thread *workers = new thread[nth];
  vector<int> ss = {INT_TINY, INT_BIG}; 
  vector<int> mm = {INT_TINY}; 
  int *vs = (int *)_mm_malloc(16*sizeof(int), 64);
  int *vm = (int *)_mm_malloc(16*sizeof(int), 64);
  MeshGrid(ss, mm, vs, vm);
  int *bvs = (int *)_mm_malloc(16*nth*sizeof(int), 64);
  int *bvm = (int *)_mm_malloc(16*nth*sizeof(int), 64);


  gettimeofday(&tv1, &tz1);
  for(int tid=0; tid<nth; tid++) {
    workers[tid] = thread([=]{
                          int Vs[2] = {INT_TINY, INT_BIG};
                          int Vm[2] = {INT_TINY, INT_TINY};
                          for(int j=tid*block; j<((tid+1)*block<N?(tid+1)*block:N); j++) {
                          Vs[0] = A[j] > Vs[0]+A[j]? A[j] : Vs[0] + A[j];
                          Vm[0] = Vm[0] > Vs[0]? Vm[0] : Vs[0];
                          Vs[1] = A[j] > Vs[1]+A[j]? A[j] : Vs[1] + A[j];
                          Vm[1] = Vm[1] > Vs[1]? Vm[1] : Vs[1];
                          }
                          bvs[tid*16] = Vs[0];
                          bvs[tid*16+1] = Vs[1];
                          bvm[tid*16] = Vm[0];
                          bvm[tid*16+1] = Vm[1];
                          });
  }

  for(int tid=0; tid<nth; tid++) {
      workers[tid].join();
      m = Rectf_max(Rectf_max2(bvm[tid*16+0], vs[1], bvm[tid*16+1], s), m);
      s = Rectf_max2(bvs[tid*16], vs[1], bvs[tid*16+1], s);
  }

  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << m << " " << s << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "parallel non-vec speed up: " << 1.0 * serial_time / par_time << endl;
  fout <<  1.0 * serial_time / par_time << "\t";
}




void parallel(int s, int m, int *A, int nth) {

    int block = N / nth;
    if(N%nth)block+=1;
    thread *workers = new thread[nth];
    vector<int> ss = {INT_TINY, INT_BIG}; 
    vector<int> mm = {INT_TINY}; 
    int *vs = (int *)_mm_malloc(16*sizeof(int), 64);
    int *vm = (int *)_mm_malloc(16*sizeof(int), 64);
    MeshGrid(ss, mm, vs, vm);
    int *bvs = (int *)_mm_malloc(16*nth*sizeof(int), 64);
    int *bvm = (int *)_mm_malloc(16*nth*sizeof(int), 64);


    gettimeofday(&tv1, &tz1);
    for(int tid=0; tid<nth; tid++) {
        workers[tid] = thread([=]{
                __m512i vecs;
                          __m512i vecm;
                          vecs = _mm512_load_epi32(vs);
                          vecm = _mm512_load_epi32(vm);
                          for(int j=tid*block; j<((tid+1)*block<N?(tid+1)*block:N); j++) {
                          vecs = _mm512_max_epi32(_mm512_add_epi32(vecs, _mm512_set1_epi32(A[j])), _mm512_set1_epi32(A[j]));
                          vecm = _mm512_max_epi32(vecm, vecs);
                          }
                          _mm512_store_epi32(bvs+tid*16, vecs);
                          _mm512_store_epi32(bvm+tid*16, vecm);
    });
  }

  for(int tid=0; tid<nth; tid++) {
    workers[tid].join();
    m = Rectf_max(Rectf_max2(bvm[tid*16+0], vs[1], bvm[tid*16+1], s), m);
    s = Rectf_max2(bvs[tid*16], vs[1], bvs[tid*16+1], s);
  }

  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << m << " " << s << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "speed up: " << 1.0 * serial_time / par_time << endl;
  fout <<  1.0 * serial_time / par_time << "\t";
}

void parsynt(int s, int m, int *A, int nth) {

  int block = N / nth;
  if(N%nth)block+=1;
  thread *workers = new thread[nth];
  int *bvs = (int *)_mm_malloc(nth*sizeof(int), 64);
  int *bvm = (int *)_mm_malloc(nth*sizeof(int), 64);
  int *bv_aux1 = (int *)_mm_malloc(nth*sizeof(int), 64);
  int *bv_aux2 = (int *)_mm_malloc(nth*sizeof(int), 64);
  int aux1 = 0;
  int aux2 = INT_TINY;


  gettimeofday(&tv1, &tz1);
  for(int tid=0; tid<nth; tid++) {
    workers[tid] = thread([=]{
                          int m = INT_TINY;
                          int s = 0;
                          int aux1 = 0;
                          int aux2 = INT_TINY;
                          for(int j=tid*block; j<((tid+1)*block<N?(tid+1)*block:N); j++) {
                          aux1 += A[j];
                          m = ((m > (((s + A[j]) > A[j]) ? (s + A[j]) : A[j])) ? m :
                              (((s + A[j]) > A[j]) ? (s + A[j]) : A[j]));
                          s = (((s + A[j]) > A[j]) ? (s + A[j]) : A[j]);
                          aux2 = ((aux1 > aux2) ? aux1 : aux2);

                          }
                          bvs[tid] = s;
                          bvm[tid] = m;
                          bv_aux1[tid] = aux1;
                          bv_aux2[tid] = aux2;
                          });
  }

  for(int tid=0; tid<nth; tid++) {
      workers[tid].join();
      int aux_1 = (aux1 + bv_aux1[tid]);
      int m_t = (((bv_aux2[tid] + s) > (((m + 0) > ((bv_aux1[tid] > bvm[tid]) ?
                              bv_aux1[tid] : bvm[tid])) ?
                      (m + 0) :
                      ((bv_aux1[tid] > bvm[tid]) ? bv_aux1[tid] :
                       bvm[tid]))) ? (bv_aux2[tid] + s) :
              (((m + 0) > ((bv_aux1[tid] > bvm[tid]) ? bv_aux1[tid] : bvm[tid])) ?
               (m + 0) : ((bv_aux1[tid] > bvm[tid]) ? bv_aux1[tid] : bvm[tid])));
      int s_t = ((((bv_aux1[tid] - 0) + s) > bvs[tid]) ? ((bv_aux1[tid] - 0) + s) :
            bvs[tid]);
      int aux_2 = (((aux1 + bv_aux2[tid]) > aux2) ?
              (aux1 + bv_aux2[tid]) :aux2);
      s = s_t;
      m = m_t;
      aux1 = aux_1;
      aux2 = aux_2;

  }

  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << m << " " << s << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "speed up: " << 1.0 * serial_time / par_time << endl;
  fout <<  1.0 * serial_time / par_time << "\t";
}



int main(int argc, char *argv[])
{
    srand(time(0));

    int s = 0;
    int m = INT_TINY;

    int *A = new int[N];

    for(int i=0; i<N; i++) {
        A[i] = rand() % 100 - 50;
    }

    serial(s, m, A);
   fout << "mss-our-nonvec\t";
    for(int nth=1;nth<=64;nth*=2) {
        parallel_nonvec(s, m, A, nth);
    }
    fout << endl;

    fout << "mss-our-vec1\t";
    for(int nth=1;nth<=64;nth*=2) {
        parallel(s, m, A, nth);
    }
    fout << endl;
    fout << "mss-our-vec2\t";
    for(int nth=1;nth<=64;nth*=2) {
        simd_mimd(s, m, A, nth);
    }
    fout << endl;
    fout << "mss-parsynt\t";
    for(int nth=1;nth<=64;nth*=2) {
        parsynt(s, m, A, nth);
    }
    fout << endl;
 
    return 0;

}

