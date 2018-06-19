// Double Exponential Smoothing
//
#include <iostream>
#include <limits.h>
#include <stdlib.h>
#include <thread>
#include <sys/time.h>
#include <fstream>
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



void serial(float alpha, float beta, float *x, float *y_seq) {
  gettimeofday(&tv1, &tz1);
  float s = x[1];
  float b = x[1] - x[0];
  for(int i=2; i<N; i++) {
    float s_next = alpha * x[i] + (1 - alpha) * (s + b);
    b = beta * (s_next - s) + (1 - beta) * b;
    s = s_next;
    y_seq[i] = s + b;
  }
  
  gettimeofday(&tv2, &tz2);

  cout << "serial res: " << y_seq[N/5*3] << endl;
  serial_time = 1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "serial time: " << serial_time<< endl;
}

void simd_mimd(float alpha, float beta, float *x, float *y_par, int nth) {

  int block = N / nth;
  if(N%nth)block+=1;
  float s = x[1];
  float b = x[1] - x[0];

  thread *workers = new thread[nth];
  vector<float> ss = {0,1};
  vector<float> bb = {0,1};
  float *vs = (float *)_mm_malloc(16*sizeof(float), 64);
  float *vb = (float *)_mm_malloc(16*sizeof(float), 64);
  MeshGridFull(ss, bb, vs, vb);
  float *bvs = (float *)_mm_malloc(16*nth*sizeof(float), 64);
  float *bvb = (float *)_mm_malloc(16*nth*sizeof(float), 64);

  int nsmall = 4;
  int smallblock = 2048;
  int *idx = (int *)_mm_malloc(16*sizeof(int), 64);
  for(int i=0;i<4;i++) {
    idx[4*i] = idx[4*i+1] = idx[4*i+2] = idx[4*i+3] = i*smallblock;
  }
  vint vidx;
  vidx.load(idx);




  gettimeofday(&tv1, &tz1);
  for(int tid=0; tid<nth; tid++) {
    workers[tid] = thread([=]{
                          vfloat vecs, vecb, vecss, vecbb;
                          vecss.load(vs); 
                          vecbb.load(vb);
                          int cur = 0;
                          while(cur+4*smallblock<=block) {
                          vecs.load(vs);
                          vecb.load(vb);
                          for(int j=tid*block+cur; j<tid*block+cur+smallblock; j++) {
                          vfloat vx;
                          vx.load(x, vidx+j, 4);
                          vfloat vecs_t = alpha * vx + (1-alpha) *(vecs+vecb);
                          vecb = beta * (vecs_t-vecs) + (1-beta) * vecb;
                          vecs = vecs_t;
                          }
                          vecs.store(bvs+tid*16);
                          vecb.store(bvb+tid*16);
                          for(int i=0;i<4;i++) {
                          vfloat vsb0 = Linr_01(bvs[tid*16], bvs[tid*16+1], vecss);
                          vfloat vsb1 = Linr_01(bvs[tid*16+2], bvs[tid*16+3], vecss);
                          vfloat s_next = Linr_01(vsb0, vsb1, vecbb);

                          vfloat vbs0 = Linr_01(bvb[tid*16], bvb[tid*16+2], vecbb);
                          vfloat vbs1 = Linr_01(bvb[tid*16+1], bvb[tid*16+3], vecbb);
                          vecbb = Linr_01(vbs0, vbs1, vecss);

                          vecss = s_next;



                          }
                          cur += 4*smallblock;
                          }
                          vecss.store(bvs+tid*16);
                          vecbb.store(bvb+tid*16);
    });
  }


  for(int tid=0; tid<nth; tid++) {
      workers[tid].join();

      workers[tid] = thread([&](int tid, float s, float b){  
              for(int i=tid*block; i<((tid+1)*block<N?(tid+1)*block:N); i++) {
              float s_next = alpha * x[i] + (1 - alpha) * (s + b);
              b = beta * (s_next - s) + (1 - beta) * b;
              s = s_next;
              y_par[i] = s + b;
              }
              }, tid, s, b);

      float sb0 = Linr_01(bvs[tid*16], bvs[tid*16+1], s);
      float sb1 = Linr_01(bvs[tid*16+2], bvs[tid*16+3], s);
      float s_next = Linr_01(sb0, sb1, b);

      float bs0 = Linr_01(bvb[tid*16], bvb[tid*16+2], b);
      float bs1 = Linr_01(bvb[tid*16+1], bvb[tid*16+3], b);
      b = Linr_01(bs0, bs1, s);

      s = s_next;

  }

  for(int tid=0; tid<nth; tid++) {
      workers[tid].join();
  }


  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << y_par[N/5*3] << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << "speed up: " << 1.0 * serial_time / par_time << endl;
  fout1 <<  1.0 * serial_time / par_time << "\t";
}



void parallel(float alpha, float beta, float *x, float *y_par, int nth) {

    int block = N / nth;
    if(N%nth)block+=1;
    float s = x[1];
    float b = x[1] - x[0];

    thread *workers = new thread[nth];
    vector<float> ss = {0,1};
    vector<float> bb = {0,1};
    float *vs = (float *)_mm_malloc(16*sizeof(float), 64);
    float *vb = (float *)_mm_malloc(16*sizeof(float), 64);
    MeshGrid(ss, bb, vs, vb);
    float *bvs = (float *)_mm_malloc(16*nth*sizeof(float), 64);
    float *bvb = (float *)_mm_malloc(16*nth*sizeof(float), 64);

    gettimeofday(&tv1, &tz1);
    for(int tid=0; tid<nth; tid++) {
        workers[tid] = thread([=]{
                __m512 vecs;
                __m512 vecb;
                vecs = _mm512_load_ps(vs);
                vecb = _mm512_load_ps(vb);
                for(int j=tid*block; j<((tid+1)*block<N?(tid+1)*block:N); j++) {
                __m512 vecs_t = _mm512_add_ps(_mm512_mul_ps(_mm512_set1_ps(alpha), _mm512_set1_ps(x[j])), _mm512_mul_ps(_mm512_set1_ps(1-alpha), _mm512_add_ps(vecs, vecb)));
                vecb = _mm512_add_ps(_mm512_mul_ps(_mm512_set1_ps(beta), _mm512_sub_ps(vecs_t, vecs)), _mm512_mul_ps(_mm512_set1_ps(1-beta), vecb));
                vecs = vecs_t;
                }
                _mm512_store_ps(bvs+tid*16, vecs);
                _mm512_store_ps(bvb+tid*16, vecb);
                });
    }


    for(int tid=0; tid<nth; tid++) {
        workers[tid].join();

        workers[tid] = thread([&](int tid, float s, float b){  
                for(int i=tid*block; i<((tid+1)*block<N?(tid+1)*block:N); i++) {
		    float s_next = alpha * x[i] + (1 - alpha) * (s + b);
		    b = beta * (s_next - s) + (1 - beta) * b;
		    s = s_next;
		    y_par[i] = s + b;
		    }
		    }, tid, s, b);

    float sb0 = Linr_01(bvs[tid*16], bvs[tid*16+1], s);
    float sb1 = Linr_01(bvs[tid*16+2], bvs[tid*16+3], s);
    float s_next = Linr_01(sb0, sb1, b);

    float bs0 = Linr_01(bvb[tid*16], bvb[tid*16+2], b);
    float bs1 = Linr_01(bvb[tid*16+1], bvb[tid*16+3], b);
    b = Linr_01(bs0, bs1, s);

    s = s_next;

  }

  for(int tid=0; tid<nth; tid++) {
	  workers[tid].join();
  }


  gettimeofday(&tv2, &tz2);
  cout << "nthreads: " << nth << endl;
  cout << "parallel res: " << y_par[N/5*3] << endl;
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

	float alpha = 0.6;
	float beta = 0.8;

	float *x = new float[N];
	float *y_seq = new float[N];
	float *y_par = new float[N];

	x[0] = 0;
	for(int i=1; i<N; i++) {
		x[i] = x[i-1] + 4*(rand() / INT_MAX) - 2;
	}

	cout << "real value: " << x[N/5*3] << endl;
	serial(alpha, beta, x, y_seq);
	fout << "des_parallel\t";
	for(int nth=1; nth<=64;nth*=2) {
		parallel(alpha, beta, x, y_par, nth);
	}
	fout << endl;

	fout1 << "des_simd+mimd\t";
	for(int nth=1; nth<=64;nth*=2) {
		simd_mimd(alpha, beta, x, y_par, nth);
	}
	fout1 << endl;

	return 0;
}


