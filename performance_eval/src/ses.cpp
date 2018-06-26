// Single Exponential Smoothing
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


void serial(float alpha, float *x, float *s_seq) {
	gettimeofday(&tv1, &tz1);
	s_seq[0] = x[0];
	for(int i=1; i<N; i++) {
		s_seq[i] = alpha * x[i] + (1 - alpha) * s_seq[i-1];
	}

	gettimeofday(&tv2, &tz2);

	cout << "serial res: " << s_seq[N/10*9]  << endl;
	serial_time = 1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
	cout << "serial time: " << serial_time<< endl;
}

void simd_mimd(float alpha, float *x, float *s_par, int nth) {

    int block = N / nth;
    if(N%nth)block+=1;
    thread *workers = new thread[nth];
    float *vs = (float *)_mm_malloc(16*sizeof(float), 64);
    for(int i=0;i<16;i++) vs[i] = i%2;
    float *bvs = (float *)_mm_malloc(16*nth*sizeof(float), 64);

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
                vfloat vecs, vecss;
                vecss.load(vs);
                int cur = 0;
                while(cur+8*smallblock<=block) {
                vecs.load(vs);
                for(int j=tid*block+cur; j<tid*block+cur+smallblock; j++) {
                vfloat vx;
                vx.load(x, vidx+j, 4);
                vecs = alpha * vx + (1-alpha) * vecs;
                }
                vecs.store(bvs+tid*16);
                for(int i=0; i<8; i++) {
                vecss = vecss * (bvs[tid*16+2*i+1]-bvs[tid*16+2*i]) + bvs[tid*16+2*i];

                }
                cur += 8*smallblock;
                }
                vecss.store(bvs+tid*16);
                });
    }


    for(int tid=0; tid<nth; tid++) {
        workers[tid].join();

        workers[tid] = thread([&](int tid){  
                for(int i=tid*block; i<((tid+1)*block<N?(tid+1)*block:N); i+=4) {
                s_par[i] = alpha * x[i] + (1 - alpha) * s_par[i-1];
                s_par[i+1] = alpha * x[i+1] + (1 - alpha) * s_par[i];
                s_par[i+2] = alpha * x[i+2] + (1 - alpha) * s_par[i+1];
                s_par[i+3] = alpha * x[i+3] + (1 - alpha) * s_par[i+2];
                }
                }, tid);

        s_par[(tid+1)*block-1] = s_par[tid*block-1<0?0:tid*block-1] * (bvs[tid*16+1]-bvs[tid*16]) + bvs[tid*16];

    }

    for(int tid=0; tid<nth; tid++) {
        workers[tid].join();
    }


    gettimeofday(&tv2, &tz2);
    cout << "nthreads: " << nth << endl;
    cout << "parallel res: " << s_par[N/10*9] << endl;
    par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
    cout << "parallel time: " << par_time << endl;
    cout << "simd+mimd speed up: " << 1.0 * serial_time / par_time << endl;
    fout1 <<  1.0 * serial_time / par_time << "\t";
}



void parallel(float alpha, float *x, float *s_par, int nth) {

    int block = N / nth;
    if(N%nth)block+=1;
    thread *workers = new thread[nth];
    float *vs = (float *)_mm_malloc(16*sizeof(float), 64);
    vs[0] = 0; vs[1] = 1;
    float *bvs = (float *)_mm_malloc(16*nth*sizeof(float), 64);

    gettimeofday(&tv1, &tz1);
    for(int tid=0; tid<nth; tid++) {
        workers[tid] = thread([=]{
                __m512 vecs;
                vecs = _mm512_load_ps(vs);
                for(int j=tid*block; j<((tid+1)*block<N?(tid+1)*block:N); j++) {
                vecs = _mm512_add_ps(_mm512_set1_ps(alpha * x[j]), _mm512_mul_ps(_mm512_set1_ps(1-alpha), vecs));
                }
                _mm512_store_ps(bvs+tid*16, vecs);
                });
    }


    for(int tid=0; tid<nth; tid++) {
        workers[tid].join();

        workers[tid] = thread([&](int tid){  
                for(int i=tid*block; i<((tid+1)*block<N?(tid+1)*block:N); i+=4) {
                s_par[i] = alpha * x[i] + (1 - alpha) * s_par[i-1];
                s_par[i+1] = alpha * x[i+1] + (1 - alpha) * s_par[i];
                s_par[i+2] = alpha * x[i+2] + (1 - alpha) * s_par[i+1];
                s_par[i+3] = alpha * x[i+3] + (1 - alpha) * s_par[i+2];
                }
                }, tid);

        s_par[(tid+1)*block-1] = s_par[tid*block-1<0?0:tid*block-1] * (bvs[tid*16+1]-bvs[tid*16]) + bvs[tid*16];

    }

    for(int tid=0; tid<nth; tid++) {
        workers[tid].join();
    }


    gettimeofday(&tv2, &tz2);
    cout << "nthreads: " << nth << endl;
    cout << "parallel res: " << s_par[N/10*9] << endl;
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

    float *x = new float[N];
    float *s_seq = new float[N];
    float *s_par = new float[N];

    x[0] = 0;
    for(int i=1; i<N; i++) {
        x[i] = x[i-1] + 4*(rand() / INT_MAX) - 2;
    }

    cout << "real value: " << x[N/10*9] << endl;
    serial(alpha, x, s_seq);
    fout << "ses-our-vec1\t";
    for(int nth=1; nth<=64;nth*=2) {
        parallel(alpha, x, s_par, nth);
    }
    fout << endl;

    fout << "ses-our-vec2\t";
    for(int nth=1; nth<=64;nth*=2) {
        simd_mimd(alpha, x, s_par, nth);
    }
    fout << endl;

    return 0;
}



