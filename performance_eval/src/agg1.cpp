// Number of operations between Push and Pull on a repository

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/time.h>
#include <thread>
#include <stdlib.h>
#include "interpolate.h"
using namespace std;


int serial_time, par_time;
struct timeval tv1, tv2;
struct timezone tz1, tz2;
ofstream fout("speedups.txt", fstream::out|fstream::app);



void serial(int nlines, string *A) {
	vector<int> res;
        
	gettimeofday(&tv1, &tz1);

	int pushed = 0;
	int count = 0;

	for(int i=0;i<nlines;i++) {
		string type = A[2*i];
		string id = A[2*i+1];
		if(atoi(id.c_str()) == 1579990) {
			if(type=="PushEvent") {
				pushed = 1;
				count = 0;
			} else if(type=="PullRequestEvent" && pushed == 1) {
				pushed = 0;
				res.push_back(count);
			} else if(pushed) {
				count++;
			}
		}
	}

	gettimeofday(&tv2, &tz2);

	serial_time = 1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
	cout << "serial time: " << serial_time<< endl;
	cout << "res size: " << res.size() << endl;
	cout << pushed << " " << count << endl;
	for(auto & r : res) cout << r << " ";
	cout << endl;
}

void parallel(int nlines, string *A, int nth) {

	vector<int> res;
	int block = nlines / nth;
	if(nlines%nth)block+=1;

	int pushed = 0;
	int count = 0;

	thread *workers = new thread[nth];
	vector<int> pushedpushed = {0,1};
	vector<int> countcount = {0,1};
	int *vpushed = (int *)_mm_malloc(16*sizeof(int), 64);
	int *vcount = (int *)_mm_malloc(16*sizeof(int), 64);
	MeshGrid(pushedpushed, countcount, vpushed, vcount);
	int *bvpushed = (int *)_mm_malloc(16*nth*sizeof(int), 64);
	int *bvcount = (int *)_mm_malloc(16*nth*sizeof(int), 64);
	SymList<int> *bres = new SymList<int>[nth];
	SymList<int> *bres_aux = new SymList<int>[nth];

	gettimeofday(&tv1, &tz1);
	for(int tid=0; tid<nth; tid++) {
		workers[tid] = thread([=]{
				__m512i vecpushed;
				__m512i veccount;
				vecpushed = _mm512_load_epi32(vpushed); 
				veccount = _mm512_load_epi32(vcount);
				for(int j=tid*block; j<((tid+1)*block<nlines?(tid+1)*block:nlines); j++) {
				string type = A[2*j];
				string id = A[2*j+1];
				if(atoi(id.c_str()) == 1579990) {
				if(type=="PushEvent") {
				vecpushed = _mm512_set1_epi32(1);
				veccount = _mm512_set1_epi32(0);
				} else if(type=="PullRequestEvent") {
				__mmask16 mas = _mm512_cmpeq_epi32_mask(vecpushed, _mm512_set1_epi32(1));
				bres_aux[tid].push_back(vecpushed);
				vecpushed = _mm512_mask_mov_epi32(vecpushed, mas, _mm512_set1_epi32(0));
				bres[tid].push_back(veccount);
				} else {
				__mmask16 mas = _mm512_cmpeq_epi32_mask(vecpushed, _mm512_set1_epi32(1));
				veccount = _mm512_mask_add_epi32(veccount, mas, veccount, _mm512_set1_epi32(1));
				}
				}
                          }

                          _mm512_store_epi32(bvpushed+tid*16, vecpushed);
                          _mm512_store_epi32(bvcount+tid*16, veccount);
    });

  }

  for(int tid=0; tid<nth; tid++) {
    workers[tid].join();

    for(int i=0;i<bres[tid].size();i++) {
      int tmppushed = bres_aux[tid][i][pushed];
      if(tmppushed) {
        int tmpcount = Linr_01(bres[tid][i][pushed], bres[tid][i][pushed+2], count);
        res.push_back(tmpcount);
      }
    }

    count = Linr(bvcount[tid*16+pushed], bvcount[tid*16+2+pushed], vcount[0], vcount[2], count); 
    pushed = bvpushed[tid*16+pushed];


  }


  gettimeofday(&tv2, &tz2);

  cout << "nthreads: " << nth << endl;
  par_time =  1000000*(tv2.tv_sec - tv1.tv_sec) + tv2.tv_usec - tv1.tv_usec;
  cout << "parallel time: " << par_time << endl;
  cout << pushed << " " << count << endl;
  cout << "res size: " << res.size() << endl;
  for(auto & r : res) cout << r << " ";
  cout << endl;

  cout << "speed up: " << 1.0 * serial_time / par_time << endl;
  fout <<  1.0 * serial_time / par_time << "\t";
}

#define N (1<<27)
int main(int argc, char *argv[])
{

	string *A = new string[N];
	ifstream fin("../datasets/type_repo.txt");
	string type, id;
  	int nlines = 0;
	while(fin>>type>>id) {
		A[2*nlines] = type;
		A[2*nlines+1] = id;
		nlines++;
	}

	serial(nlines, A);
	fout << "agg1\t";
	for(int nth=1; nth<=64;nth*=2) {
		parallel(nlines, A, nth);
	}
	fout << endl;
	return 0;
}


